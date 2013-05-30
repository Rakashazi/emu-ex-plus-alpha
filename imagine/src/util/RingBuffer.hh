#pragma once

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

#include <util/cLang.h>
#include <atomic>

template <class COUNT = std::atomic_uint, class SIZE = uint>
class RingBuffer
{
public:
	bool init(uchar *buff, SIZE size)
	{
		this->buff = buff;
		buffSize = size;
		reset();
		return true;
	}

	void deinit() {}

	void reset()
	{
		start = end = buff;
		written = 0;
	}

	SIZE freeSpace()
	{
		return buffSize - written;
	}

	uchar *advancePtr(uchar *ptr)
	{
		assert(ptr >= buff && ptr < buff+buffSize);
		ptr++;
		if(ptr == buff+buffSize)
			ptr = buff;
		return ptr;
	}

	SIZE write(uchar *buff, SIZE size)
	{
		if(size > freeSpace())
			size = freeSpace();

		uchar *writePos = end;
		iterateTimes(size, i)
		{
			//logMsg("addr %p", writePos);
			*writePos = buff[i];
			writePos = advancePtr(writePos);
		}
		written += size;
		end = writePos;

		assert((SIZE)written <= buffSize);

		//logMsg("wrote %d bytes", (int)size);
		return size;
	}

	SIZE read(uchar *buff, SIZE size)
	{
		if(size > (SIZE)written)
			size = written;

		uchar *readPos = start;
		iterateTimes(size, i)
		{
			//logMsg("addr %p", readPos);
			buff[i] = *readPos;
			readPos = advancePtr(readPos);
		}
		written -= size;
		start = readPos;

		//logMsg("read %d bytes", (int)size);
		return size;
	}

	SIZE readPadded(uchar *buff, SIZE size)
	{
		SIZE readSize = read(buff, size);
		if(readSize < size)
		{
			uint padBytes = size - readSize;
			logMsg("padding %d bytes", padBytes);
			uchar padVal = readSize ? buff[readSize-1] : 0;
			memset(&buff[readSize], padVal, padBytes);
		}
		return readSize;
	}

	SIZE readPadded32(uchar *buff, SIZE size)
	{
		assert(size % 4 == 0);
		SIZE readSize = read(buff, size);
		assert(readSize % 4 == 0);
		if(readSize < size)
		{
			uint padElements = (size - readSize) / 4;
			logMsg("padding %dx4 bytes", padElements);
			uint padVal = 0;
			if(readSize)
				memcpy(&padVal, &buff[readSize-4], 4);
			iterateTimes(padElements, i)
				memcpy(&buff[readSize + (i*4)], &padVal, 4);
		}
		return readSize;
	}

	COUNT written;
	SIZE buffSize;
private:
	uchar *buff;
	uchar *start, *end;
};
