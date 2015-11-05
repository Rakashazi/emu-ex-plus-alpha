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

ArchiveIO::~ArchiveIO()
{
	close();
}

ArchiveIO::ArchiveIO(ArchiveIO &&o)
{
	arch = std::move(o.arch);
	entry = o.entry;
}

ArchiveIO &ArchiveIO::operator=(ArchiveIO &&o)
{
	close();
	arch = std::move(o.arch);
	entry = o.entry;
	return *this;
}

ArchiveIO::operator GenericIO()
{
	return GenericIO{*this};
}

std::shared_ptr<struct archive> ArchiveIO::releaseArchive()
{
	return std::move(arch);
}

const char *ArchiveIO::name()
{
	return archive_entry_pathname(entry);
}

BufferMapIO ArchiveIO::moveToMapIO()
{
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

ssize_t ArchiveIO::read(void *buff, size_t bytes, CallResult *resultOut)
{
	int bytesRead = archive_read_data(arch.get(), buff, bytes);
	if(bytesRead < 0)
	{
		bytesRead = -1;
		if(resultOut)
			*resultOut = READ_ERROR;
	}
	return bytesRead;
}

ssize_t ArchiveIO::write(const void* buff, size_t bytes, CallResult *resultOut)
{
	if(resultOut)
		*resultOut = UNSUPPORTED_OPERATION;
	return -1;
}

off_t ArchiveIO::seek(off_t offset, IO::SeekMode mode, CallResult *resultOut)
{
	if(!isSeekModeValid(mode))
	{
		bug_exit("invalid seek mode: %d", (int)mode);
		if(resultOut)
			*resultOut = INVALID_PARAMETER;
		return -1;
	}
	long newPos = archive_seek_data(arch.get(), offset, mode);
	if(newPos < 0)
	{
		logErr("seek to offset %lld failed", (long long)offset);
		if(resultOut)
			*resultOut = IO_ERROR;
		return -1;
	}
	return newPos;
}

void ArchiveIO::close()
{
	arch = {};
}

size_t ArchiveIO::size()
{
	return archive_entry_size(entry);
}

bool ArchiveIO::eof()
{
	return tell() == archive_entry_size(entry);
}

ArchiveIO::operator bool()
{
	return (bool)arch;
}
