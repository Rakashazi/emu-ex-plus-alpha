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
	bool init(SIZE size)
	{
		deinit();
		buff = (char*)malloc(size);
		buffSize = size;
		reset();
		return true;
	}

	void deinit()
	{
		if(buff)
		{
			free(buff);
			buff = nullptr;
		}
	}

	void reset()
	{
		start = end = buff;
		written = 0;
	}

	SIZE freeSpace()
	{
		return buffSize - written;
	}

	SIZE write(const void *buff, SIZE size)
	{
		if(size > freeSpace())
			size = freeSpace();

		char *writePos = end;
		iterateTimes(size, i)
		{
			//logMsg("addr %p", writePos);
			*writePos = ((char*)buff)[i];
			writePos = advancePos(writePos, 1);
		}
		end = writePos;
		written += size;

		assert((SIZE)written <= buffSize);

		//logMsg("wrote %d bytes", (int)size);
		return size;
	}

	char *writePos()
	{
		return end;
	}

	void advanceWritePos(SIZE size)
	{
		assert(size <= freeSpace());
		end = advancePos(end, size);
		written += size;
	}

	SIZE read(void *buff, SIZE size)
	{
		if(size > (SIZE)written)
			size = written;

		char *readPos = start;
		iterateTimes(size, i)
		{
			//logMsg("addr %p", readPos);
			((char*)buff)[i] = *readPos;
			readPos = advancePos(readPos, 1);
		}
		start = readPos;
		written -= size;

		//logMsg("read %d bytes", (int)size);
		return size;
	}

	char *readPos()
	{
		return start;
	}

	void advanceReadPos(SIZE size)
	{
		assert(size <= (SIZE)written);
		start = advancePos(start, size);
		written -= size;
	}

	char *data() const
	{
		return buff;
	}

	char *advancePos(char *ptr, int size)
	{
		assert(ptr >= buff && ptr < buff+buffSize*2);
		ptr += size;
		if(ptr >= buff+buffSize)
			ptr -= buffSize;
		return ptr;
	}

	COUNT written;
	SIZE buffSize;
private:
	char *buff;
	char *start, *end;
};
