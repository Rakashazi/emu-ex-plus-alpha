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

#define LOGTAG "IOMMap"
#include <imagine/logger/logger.h>
#include <cstring>
#include <assert.h>
#include <imagine/io/utils.hh>
#include <imagine/io/IoMmap.hh>

void IoMmap::init(const void *buffer, size_t size)
{
	logMsg("opening memory as file @ %p of size %zd", buffer, size);
	data = currPos = (char*)buffer;
	iSize = size;
}

const char *IoMmap::endofBuffer()
{
	return data + iSize;
}

ssize_t IoMmap::readUpTo(void* buffer, size_t numBytes)
{
	assert(currPos >= data);
	auto bytesRead = readAtPos(buffer, numBytes, currPos);
	currPos += bytesRead;
	return bytesRead;
}

ssize_t IoMmap::readAtPos(void* buffer, size_t numBytes, ulong offset)
{
	return readAtPos(buffer, numBytes, data + offset);
}

ssize_t IoMmap::readAtPos(void* buffer, size_t numBytes, const char *readPos)
{
	if(readPos >= endofBuffer())
		return 0;

	size_t bytesToRead;
	const char *posToReadTo = readPos + numBytes;
	if(posToReadTo > endofBuffer())
	{
		bytesToRead = endofBuffer() - readPos;
	}
	else bytesToRead = numBytes;

	//logDMsg("reading %d bytes at offset %d to %p", (int)bytesToRead, int(readPos - data), buffer);
	memcpy(buffer, readPos, bytesToRead);

	return bytesToRead;
}

// TODO
size_t IoMmap::fwrite(const void* ptr, size_t size, size_t nmemb) { return 0; }

CallResult IoMmap::tell(ulong &offset)
{
	assert(currPos >= data);
	offset = currPos - data;
	//logMsg("offset @ %lu", *offset);
	return(OK);
}

CallResult IoMmap::seek(long offset, uint mode)
{
	if(!transformOffsetToAbsolute(mode, offset, (long)data, (long)(data + iSize), (long)currPos))
	{
		logErr("invalid seek parameter");
		return INVALID_PARAMETER;
	}
	auto newPos = (const char*)offset;
	if(newPos < data || newPos > (data + iSize))
	{
		logErr("illegal seek position");
		return INVALID_PARAMETER;
	}

	currPos = newPos;
	return OK;
}

ulong IoMmap::size()
{
	//log_mPrintf(LOG_MSG, "size of stream %d bytes", i_size);
	return(iSize);
}

int IoMmap::eof()
{
	assert(currPos >= data);
	if(currPos >= endofBuffer())
		return 1;
	else
		return 0;
}

void IoMmap::truncate(ulong offset)
{
	logErr("truncate not supported");
}
