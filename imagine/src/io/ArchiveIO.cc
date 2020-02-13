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

#define LOGTAG "ArchIO"
#include <imagine/io/ArchiveIO.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/logger/logger.h>
#include "utils.hh"
#include <archive.h>
#include <archive_entry.h>

static FS::file_type makeEntryType(int type)
{
	using namespace FS;
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
	assumeExpr(ptr);
	auto name = archive_entry_pathname(ptr);
	return name ? name : "";
}

FS::file_type ArchiveEntry::type() const
{
	assumeExpr(ptr);
	return makeEntryType(archive_entry_filetype(ptr));
}

size_t ArchiveEntry::size() const
{
	assumeExpr(ptr);
	return archive_entry_size(ptr);
}

uint32_t ArchiveEntry::crc32() const
{
	assumeExpr(ptr);
	return archive_entry_crc32(ptr);
}

ArchiveIO ArchiveEntry::moveIO()
{
	return ArchiveIO{std::move(*this)};
}

void ArchiveEntry::moveIO(ArchiveIO io)
{
	*this = io.releaseArchive();
}

ArchiveIO::ArchiveIO(ArchiveEntry entry):
	entry{entry}
{}

ArchiveIO::~ArchiveIO()
{
	close();
}

ArchiveIO::ArchiveIO(ArchiveIO &&o)
{
	*this = std::move(o);
}

ArchiveIO &ArchiveIO::operator=(ArchiveIO &&o)
{
	close();
	entry = std::exchange(o.entry, {});
	return *this;
}

GenericIO ArchiveIO::makeGeneric()
{
	return GenericIO{*this};
}

ArchiveEntry ArchiveIO::releaseArchive()
{
	return std::move(entry);
}

const char *ArchiveIO::name()
{
	return entry.name();
}

BufferMapIO ArchiveIO::moveToMapIO()
{
	if(!*this)
	{
		return {};
	}
	auto s = size();
	auto data = new char[s];
	if(read(data, s) != (ssize_t)s)
	{
		logErr("error reading data for MapIO");
		return {};
	}
	BufferMapIO mapIO{};
	mapIO.open(data, s, [data](BufferMapIO &){ delete[] data; });
	return mapIO;
}

ssize_t ArchiveIO::read(void *buff, size_t bytes, std::error_code *ecOut)
{
	if(!*this)
	{
		if(ecOut)
			*ecOut = {EBADF, std::system_category()};
		return -1;
	}
	int bytesRead = archive_read_data(entry.arch.get(), buff, bytes);
	if(bytesRead < 0)
	{
		bytesRead = -1;
		if(ecOut)
			*ecOut = {EIO, std::system_category()};
	}
	return bytesRead;
}

ssize_t ArchiveIO::write(const void* buff, size_t bytes, std::error_code *ecOut)
{
	if(ecOut)
		*ecOut = {ENOSYS, std::system_category()};
	return -1;
}

off_t ArchiveIO::seek(off_t offset, IO::SeekMode mode, std::error_code *ecOut)
{
	if(!*this)
	{
		if(ecOut)
			*ecOut = {EBADF, std::system_category()};
		return -1;
	}
	if(!isSeekModeValid(mode))
	{
		logErr("invalid seek mode: %d", (int)mode);
		if(ecOut)
			*ecOut = {EINVAL, std::system_category()};
		return -1;
	}
	long newPos = archive_seek_data(entry.arch.get(), offset, mode);
	if(newPos < 0)
	{
		logErr("seek to offset %lld failed", (long long)offset);
		if(ecOut)
			*ecOut = {EINVAL, std::system_category()};
		return -1;
	}
	return newPos;
}

void ArchiveIO::close()
{
	entry.arch = {};
}

size_t ArchiveIO::size()
{
	return entry.size();
}

bool ArchiveIO::eof()
{
	return tell() == (off_t)entry.size();
}

ArchiveIO::operator bool() const
{
	return (bool)entry.arch;
}
