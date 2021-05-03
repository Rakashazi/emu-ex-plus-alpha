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

#define LOGTAG "MapIO"
#include <cstring>
#if defined __linux__ || defined __APPLE__
#include <sys/mman.h>
#include <imagine/util/system/pagesize.h>
#endif
#include <imagine/io/MapIO.hh>
#include <imagine/logger/logger.h>
#include "utils.hh"

ssize_t MapIO::read(void *buff, size_t bytes, std::error_code *ecOut)
{
	assert(currPos >= data);
	auto bytesRead = readAtAddr(buff, bytes, currPos, ecOut);
	if(bytesRead > 0)
	{
		currPos += bytesRead;
	}
	return bytesRead;
}

ssize_t MapIO::readAtPos(void *buff, size_t bytes, off_t offset, std::error_code *ecOut)
{
	return readAtAddr(buff, bytes, data + offset, ecOut);
}

const uint8_t *MapIO::mmapConst()
{
	return data;
}

ssize_t MapIO::write(const void *buff, size_t bytes, std::error_code *ecOut)
{
	// TODO
	if(ecOut)
		*ecOut = {ENOSYS, std::system_category()};
	return -1;
}

off_t MapIO::seek(off_t offset, IO::SeekMode mode, std::error_code *ecOut)
{
	if(!isSeekModeValid(mode))
	{
		logErr("invalid seek parameter: %d", (int)mode);
		if(ecOut)
			*ecOut = {EINVAL, std::system_category()};
		return -1;
	}
	auto newPos = (const uint8_t*)transformOffsetToAbsolute(mode, offset, (off_t)data, off_t(dataEnd()), (off_t)currPos);
	if(newPos < data || newPos > dataEnd())
	{
		logErr("illegal seek position");
		if(ecOut)
			*ecOut = {EINVAL, std::system_category()};
		return -1;
	}
	currPos = newPos;
	return currPos - data;
}

size_t MapIO::size()
{
	return dataSize;
}

bool MapIO::eof()
{
	return currPos >= dataEnd();
}

MapIO::operator bool() const
{
	return data;
}

static int adviceToMAdv(IO::Advice advice)
{
	switch(advice)
	{
		default: return MADV_NORMAL;
		case IO::Advice::SEQUENTIAL: return MADV_SEQUENTIAL;
		case IO::Advice::RANDOM: return MADV_RANDOM;
		case IO::Advice::WILLNEED: return MADV_WILLNEED;
	}
}

#if defined __linux__ || defined __APPLE__
void MapIO::advise(off_t offset, size_t bytes, Advice advice)
{
	assert(offset >= 0);
	if(!bytes)
		bytes = dataSize;
	if(bytes > dataSize - offset) // clip to end of data
	{
		bytes = dataSize - offset;
	}
	void *srcAddr = (void*)((uintptr_t)data + offset);
	void *pageSrcAddr = (void*)roundDownToPageSize((uintptr_t)srcAddr);
	bytes += (uintptr_t)srcAddr - (uintptr_t)pageSrcAddr; // add extra bytes from rounding down to page size
	int mAdv = adviceToMAdv(advice);
	if(madvise(pageSrcAddr, bytes, mAdv) != 0 && Config::DEBUG_BUILD)
	{
		logWarn("madvise address:%p size:%zu failed:%s", pageSrcAddr, bytes, strerror(errno));
	}
}
#endif

void MapIO::setData(const void *dataPtr, size_t size)
{
	logMsg("setting data @ %p with size %llu", dataPtr, (unsigned long long)size);
	data = currPos = (const uint8_t*)dataPtr;
	dataSize = size;
}

void MapIO::resetData()
{
	data = currPos = nullptr;
	dataSize = 0;
}

const uint8_t *MapIO::dataEnd()
{
	return data + dataSize;
}

ssize_t MapIO::readAtAddr(void* buff, size_t bytes, const uint8_t *addr, std::error_code *ecOut)
{
	if(addr >= dataEnd())
	{
		if(!data)
		{
			if(ecOut)
				*ecOut = {EBADF, std::system_category()};
			return -1;
		}
		else
			return 0;
	}

	size_t bytesToRead;
	const uint8_t *posToReadTo = addr + bytes;
	if(posToReadTo > dataEnd())
	{
		bytesToRead = dataEnd() - addr;
	}
	else bytesToRead = bytes;

	//logDMsg("reading %llu bytes at offset %llu", (unsigned long long)bytesToRead, (unsigned long long)(addr - data));
	memcpy(buff, addr, bytesToRead);

	return bytesToRead;
}
