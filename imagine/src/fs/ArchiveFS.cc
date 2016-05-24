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
#include <imagine/util/string.h>
#include <archive.h>
#include <archive_entry.h>

struct ArchiveGenericIO
{
	GenericIO io;
	bool rewindOnClose = false;

	ArchiveGenericIO(GenericIO io): io{std::move(io)} {}
};

struct ArchiveGenericBufferedIO : public ArchiveGenericIO
{
	alignas(__BIGGEST_ALIGNMENT__) std::array<char, 32 * 1024> buff{};

	ArchiveGenericBufferedIO(GenericIO io): ArchiveGenericIO{std::move(io)} {}
};

namespace FS
{

static void setReadSupport(struct archive *arch)
{
	archive_read_support_format_7zip(arch);
	archive_read_support_format_rar(arch);
	archive_read_support_format_zip(arch);
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
			logMsg("freeing archive data");
			archive_read_free(arch);
		}
	};
	setReadSupport(archEntry.arch.get());
	auto seekFunc =
		[](struct archive *, void *data, int64_t offset, int whence) -> int64_t
		{
			//logMsg("seek %lld %d", (long long)offset, whence);
			auto &gIO = *((ArchiveGenericIO*)data);
			auto newPos = gIO.io.seek(offset, whence);
			if(newPos == -1)
			{
				logErr("error seeking to %llu", (long long)newPos);
				return ARCHIVE_FATAL;
			}
			return newPos;
		};
	auto skipFunc =
		[](struct archive *, void *data, int64_t request) -> int64_t
		{
			//logMsg("skip %lld", (long long)request);
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
			if(gIO->rewindOnClose)
			{
				//logMsg("rewinding IO");
				gIO->io.seekS(0);
				gIO->rewindOnClose = false;
			}
			else
			{
				gIO->io.close();
				delete gIO;
			}
			return ARCHIVE_OK;
		};
	archive_read_set_seek_callback(archEntry.arch.get(), seekFunc);
	int openRes = ARCHIVE_FATAL;
	auto fileMap = io.mmapConst();
	if(fileMap)
	{
		//logMsg("source IO supports mapping");
		auto readFunc =
			[](struct archive *, void *data, const void **buffOut) -> ssize_t
			{
				auto &gIO = *((ArchiveGenericIO*)data);
				// return the entire buffer after the file position
				auto pos = gIO.io.tell();
				ssize_t bytesRead = gIO.io.size() - pos;
				if(bytesRead)
				{
					*buffOut = gIO.io.mmapConst() + pos;
					gIO.io.seekC(bytesRead);
				}
				//logMsg("read %lld bytes @ offset:%llu", (long long)bytesRead, (long long)pos);
				return bytesRead;
			};
		auto archGenIO = new ArchiveGenericIO{std::move(io)};
		archEntry.genericIO = archGenIO;
		openRes = archive_read_open2(archEntry.arch.get(),
			archGenIO, nullptr, readFunc, skipFunc, closeFunc);
	}
	else
	{
		auto readFunc =
			[](struct archive *, void *data, const void **buffOut) -> ssize_t
			{
				auto &gIO = *((ArchiveGenericBufferedIO*)data);
				auto bytesRead = gIO.io.read(gIO.buff.data(), gIO.buff.size());
				*buffOut = gIO.buff.data();
				return bytesRead;
			};
		auto archGenIO = new ArchiveGenericBufferedIO{std::move(io)};
		archEntry.genericIO = archGenIO;
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
	auto ret = archive_read_next_header(archEntry.arch.get(), &archEntry.ptr);
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

void ArchiveIterator::rewind()
{
	archEntry.genericIO->rewindOnClose = true;
	assumeExpr(archEntry.arch.unique());
	archEntry.arch.reset();
	auto io = std::move(archEntry.genericIO->io);
	delete archEntry.genericIO;
	archEntry.genericIO = nullptr;
	CallResult dummy;
	logMsg("re-opening archive");
	init(std::move(io), dummy);
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
