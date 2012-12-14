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

#define thisModuleName "io:mmap"
#include <logger/interface.h>
#include <mem/interface.h>
#include <string.h>
#include <assert.h>

#include "IoMmap.hh"

void IoMmap::init(const uchar *buffer, size_t size)
{
	logMsg("opening memory as file @ %p of size %zd", buffer, size);
	data = currPos = buffer;
	iSize = size;
}

const uchar *IoMmap::endofBuffer()
{
	return data + iSize;
}

size_t IoMmap::readUpTo(void* buffer, size_t numBytes)
{
	assert(currPos >= data);
	if(currPos >= endofBuffer())
		return 0;
	
	size_t bytesToRead;
	const uchar *posToReadTo = currPos + numBytes;
	if(posToReadTo > endofBuffer())
	{
		bytesToRead = endofBuffer() - currPos;
	}
	else bytesToRead = numBytes;

	//logDMsg("reading %d bytes at offset %d to %p", (int)bytesToRead, int(currPos - data), buffer);
	memcpy(buffer, currPos, bytesToRead);
	currPos += bytesToRead;

	return bytesToRead;
}

int IoMmap::fgetc()
{
	assert(currPos >= data);
	if(currPos >= endofBuffer())
		return EOF;
	
	int val = *currPos;
	currPos++;
	return val;
}

// TODO
size_t IoMmap::fwrite(const void* ptr, size_t size, size_t nmemb) { return 0; }

CallResult IoMmap::tell(ulong *offset)
{
	assert(currPos >= data);
	*offset = currPos - data;
	//logMsg("offset @ %lu", *offset);
	return(OK);
}

CallResult IoMmap::seekU(ulong offset, uint mode)
{
	const uchar *newPos = currPos;
	if(mode == IO_SEEK_ABS)
	{
		newPos = data + offset;
	}
	else if(mode == IO_SEEK_ABS_END)
	{
		newPos = (data + iSize) + offset;
	}
	else if(mode == IO_SEEK_ADD)
	{
		newPos += offset;
	}
	else if(mode == IO_SEEK_SUB)
	{
		newPos -= offset;
	}
	else
	{
		logErr("invalid seek mode");
		return INVALID_PARAMETER;
	}

	if(newPos < data || newPos > (data + iSize))
	{
		logErr("illegal seek position");
		return INVALID_PARAMETER;
	}

	currPos = newPos;
	return OK;
}

CallResult IoMmap::seekS(long offset, uint mode)
{
	const uchar *newPos = currPos;
	if(mode == IO_SEEK_ABS)
	{
		newPos = data + offset;
	}
	else if(mode == IO_SEEK_ABS_END)
	{
		newPos = (data + iSize) + offset;
	}
	else if(mode == IO_SEEK_ADD)
	{
		newPos += offset;
	}
	else if(mode == IO_SEEK_SUB)
	{
		newPos -= offset;
	}
	else
	{
		logErr("invalid seek mode");
		return INVALID_PARAMETER;
	}

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
