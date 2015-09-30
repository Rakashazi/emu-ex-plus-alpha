/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "ArchFS"
#include <cstdlib>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/assume.h>
#include <archive.h>
#include <archive_entry.h>

namespace FS
{

struct ArchiveGenericIO
{
	GenericIO io;
	std::array<char, 8192> buff{};

	ArchiveGenericIO(GenericIO io): io{std::move(io)} {}
};

static file_type makeEntryType(int type)
{
	switch(type)
	{
		case AE_IFREG: return file_type::regular;
		case AE_IFDIR: return file_type::directory;
		case AE_IFLNK: return file_type::symlink;
		case AE_IFBLK: return file_type::block;
		case AE_IFCHR: return file_type::character;
		case AE_IFIFO: return file_type::fifo;
		case AE_IFSOCK: return file_type::socket;
	}
	return file_type::unknown;
}

const char *ArchiveEntry::name() const
{
	assumeExpr(entry);
	return archive_entry_pathname(entry);
}

file_type ArchiveEntry::type() const
{
	assumeExpr(entry);
	return makeEntryType(archive_entry_filetype(entry));
}

ArchiveIO ArchiveEntry::moveIO()
{
	return ArchiveIO{std::move(arch), entry};
}

void ArchiveEntry::moveIO(ArchiveIO io)
{
	arch = std::move(io.releaseArchive());
}

static void setReadSupport(struct archive *arch)
{
	if(Config::envIsLinux)
	{
		archive_read_support_filter_all(arch);
		archive_read_support_format_all(arch);
	}
	else
	{
		archive_read_support_format_7zip(arch);
		archive_read_support_format_rar(arch);
		archive_read_support_format_zip(arch);
	}
}

void ArchiveIterator::init(const char *path, CallResult &result)
{
	FileIO file;
	auto fileRes = file.open(path);
	if(fileRes != OK)
	{
		result = fileRes;
		return;
	}
	init(GenericIO{std::move(file)}, result);
}

void ArchiveIterator::init(GenericIO io, CallResult &result)
{
	archEntry.arch = {archive_read_new(),
		[](struct archive *arch)
		{
			logMsg("closing archive");
			archive_read_close(arch);
			archive_read_free(arch);
		}
	};
	setReadSupport(archEntry.arch.get());
	int openRes = ARCHIVE_FATAL;
	auto fileMap = io.mmapConst();
	if(fileMap)
	{
		// Take ownership of generic IO and use directly
		logMsg("source IO supports mapping");
		auto seekFunc =
			[](struct archive *, void *data, int64_t offset, int whence) -> int64_t
			{
				//logMsg("seek %lld %d", (long long)offset, whence);
				auto &gIO = *((IO*)data);
				auto newPos = gIO.seek(offset, whence);
				if(newPos == -1)
					return ARCHIVE_FATAL;
				return newPos;
			};
		auto readFunc =
			[](struct archive *, void *data, const void **buffOut) -> ssize_t
			{
				auto &gIO = *((IO*)data);
				// return the entire buffer after the file position
				auto pos = gIO.tell();
				ssize_t bytesRead = gIO.size() - pos;
				if(bytesRead)
				{
					*buffOut = gIO.mmapConst() + pos;
					gIO.seekC(bytesRead);
				}
				//logMsg("read %lld bytes", (long long)bytesRead);
				return bytesRead;
			};
		auto skipFunc =
			[](struct archive *, void *data, int64_t request) -> int64_t
			{
				//logMsg("skip %lld", (long long)request);
				auto &gIO = *((IO*)data);
				if(gIO.seekC(request) == OK)
					return request;
				else
					return 0;
			};
		auto closeFunc =
			[](struct archive *, void *data)
			{
				auto gIO = (IO*)data;
				gIO->close();
				delete gIO;
				return ARCHIVE_OK;
			};

		archive_read_set_seek_callback(archEntry.arch.get(), seekFunc);
		openRes = archive_read_open2(archEntry.arch.get(),
			io.release(), nullptr, readFunc, skipFunc, closeFunc);
	}
	else
	{
		// Use wrapper object with read buffer
		auto seekFunc =
			[](struct archive *, void *data, int64_t offset, int whence) -> int64_t
			{
				auto &gIO = *((ArchiveGenericIO*)data);
				auto newPos = gIO.io.seek(offset, whence);
				if(newPos == -1)
					return ARCHIVE_FATAL;
				return newPos;
			};
		auto readFunc =
			[](struct archive *, void *data, const void **buffOut) -> ssize_t
			{
				auto &gIO = *((ArchiveGenericIO*)data);
				auto bytesRead = gIO.io.read(gIO.buff.data(), gIO.buff.size());
				*buffOut = gIO.buff.data();
				return bytesRead;
			};
		auto skipFunc =
			[](struct archive *, void *data, int64_t request) -> int64_t
			{
				auto &gIO = *((ArchiveGenericIO*)data);
				if(gIO.io.seekC(request) == OK)
					return request;
				else
					return 0;
			};
		auto closeFunc =
			[](struct archive *, void *data)
			{
				auto gIO = (ArchiveGenericIO*)data;
				gIO->io.close();
				delete gIO;
				return ARCHIVE_OK;
			};

		auto archGenIO = new ArchiveGenericIO{std::move(io)};
		archive_read_set_seek_callback(archEntry.arch.get(), seekFunc);
		openRes = archive_read_open2(archEntry.arch.get(),
			archGenIO, nullptr, readFunc, skipFunc, closeFunc);
	}
	if(openRes != ARCHIVE_OK)
	{
		if(Config::DEBUG_BUILD)
			logErr("error opening archive:%s", archive_error_string(archEntry.arch.get()));
		archEntry.arch = {};
		result = IO_ERROR;
		return;
	}
	result = OK;
	++(*static_cast<ArchiveIterator*>(this)); // go to first entry
}

ArchiveIterator::ArchiveIterator(const char *path)
{
	CallResult dummy;
	init(path, dummy);
}

ArchiveIterator::ArchiveIterator(const char *path, CallResult &result)
{
	init(path, result);
}

ArchiveIterator::ArchiveIterator(GenericIO io)
{
	CallResult dummy;
	init(std::move(io), dummy);
}

ArchiveIterator::ArchiveIterator(GenericIO io, CallResult &result)
{
	init(std::move(io), result);
}

ArchiveIterator::~ArchiveIterator() {}

ArchiveEntry& ArchiveIterator::operator*()
{
	return archEntry;
}

ArchiveEntry* ArchiveIterator::operator->()
{
	return &archEntry;
}

void ArchiveIterator::operator++()
{
	if(!archEntry.arch) // check in case archive object was moved out
		return;
	auto ret = archive_read_next_header(archEntry.arch.get(), &archEntry.entry);
	if(ret != ARCHIVE_OK)
	{
		if(ret == ARCHIVE_EOF)
		{
			logErr("reached archive end");
		}
		else
		{
			if(Config::DEBUG_BUILD)
				logErr("error reading archive entry:%s", archive_error_string(archEntry.arch.get()));
		}
		archEntry.arch = {};
		return;
	}
}

bool ArchiveIterator::operator==(ArchiveIterator const &rhs) const
{
	return archEntry.arch == rhs.archEntry.arch;
}

ArchiveIO fileFromArchive(const char *archivePath, const char *filePath)
{
	for(auto &entry : FS::ArchiveIterator{archivePath})
	{
		if(entry.type() == FS::file_type::directory)
		{
			continue;
		}
		if(string_equal(entry.name(), filePath))
		{
			return entry.moveIO();
		}
	}
	return {};
}

}
