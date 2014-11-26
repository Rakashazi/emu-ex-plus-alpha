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

ssize_t MapIO::read(void *buff, size_t bytes, CallResult *resultOut)
{
	assert(currPos >= data);
	auto bytesRead = readAtAddr(buff, bytes, currPos, resultOut);
	if(bytesRead > 0)
	{
		currPos += bytesRead;
	}
	return bytesRead;
}

ssize_t MapIO::readAtPos(void *buff, size_t bytes, off_t offset, CallResult *resultOut)
{
	return readAtAddr(buff, bytes, data + offset, resultOut);
}

const char *MapIO::mmapConst()
{
	return data;
}

ssize_t MapIO::write(const void *buff, size_t bytes, CallResult *resultOut)
{
	// TODO
	if(resultOut)
		*resultOut = UNSUPPORTED_OPERATION;
	return -1;
}

off_t MapIO::tell(CallResult *resultOut)
{
	assert(currPos >= data);
	return currPos - data;
}

CallResult MapIO::seek(off_t offset, SeekMode mode)
{
	if(!isSeekModeValid(mode))
	{
		logErr("invalid seek parameter: %d", (int)mode);
		return INVALID_PARAMETER;
	}
	auto newPos = (const char*)transformOffsetToAbsolute(mode, offset, (off_t)data, off_t(dataEnd()), (off_t)currPos);
	if(newPos < data || newPos > dataEnd())
	{
		logErr("illegal seek position");
		return OUT_OF_BOUNDS;
	}
	currPos = newPos;
	return OK;
}

size_t MapIO::size()
{
	return dataSize;
}

bool MapIO::eof()
{
	return currPos >= dataEnd();
}

MapIO::operator bool()
{
	return data;
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
	void *srcAddr = (void*)((ptrsize)data + offset);
	void *pageSrcAddr = (void*)roundDownToPageSize((ptrsize)srcAddr);
	bytes += (ptrsize)srcAddr - (ptrsize)pageSrcAddr; // add extra bytes from rounding down to page size

	if(advice == ADVICE_SEQUENTIAL)
	{
		if(madvise(pageSrcAddr, bytes, MADV_SEQUENTIAL) != 0)
		{
			logWarn("advise sequential for offset 0x%llX with size %zu failed", (unsigned long long)offset, bytes);
		}
	}
	else if(advice == ADVICE_WILLNEED)
	{
		//logMsg("advising will need offset 0x%X @ page %p with size %u", (uint)offset, pageSrcAddr, (uint)len);
		if(madvise(pageSrcAddr, bytes, MADV_WILLNEED) != 0)
		{
			logWarn("advise will need for offset 0x%llX with size %zu failed", (unsigned long long)offset, bytes);
		}
	}
}
#endif

void MapIO::setData(const void *dataPtr, size_t size)
{
	logMsg("setting data @ %p with size %llu", dataPtr, (unsigned long long)size);
	data = currPos = (char*)dataPtr;
	dataSize = size;
}

void MapIO::resetData()
{
	data = currPos = nullptr;
	dataSize = 0;
}

const char *MapIO::dataEnd()
{
	return data + dataSize;
}

ssize_t MapIO::readAtAddr(void* buff, size_t bytes, const char *addr, CallResult *resultOut)
{
	if(addr >= dataEnd())
	{
		if(!data)
		{
			if(resultOut)
				*resultOut = BAD_STATE;
			return -1;
		}
		else
			return 0;
	}

	size_t bytesToRead;
	const char *posToReadTo = addr + bytes;
	if(posToReadTo > dataEnd())
	{
		bytesToRead = dataEnd() - addr;
	}
	else bytesToRead = bytes;

	//logDMsg("reading %llu bytes at offset %llu", (unsigned long long)bytesToRead, unsigned long long(readPos - data));
	memcpy(buff, addr, bytesToRead);

	return bytesToRead;
}
