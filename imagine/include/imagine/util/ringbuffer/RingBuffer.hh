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

#include <imagine/util/algorithm.h>
#include <atomic>
#include <cstdlib>

template <class COUNT = std::atomic_uint, class SIZE = unsigned int>
class StaticRingBuffer
{
public:
	constexpr StaticRingBuffer() {}

	bool init(SIZE size)
	{
		deinit();
		buff = (char*)malloc(size);
		if(!buff)
			return false;
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
		buffSize = 0;
		reset();
	}

	void reset()
	{
		start = end = buff;
		written = 0;
	}

	SIZE freeSpace() const
	{
		return buffSize - written;
	}

	SIZE freeContiguousSpace() const
	{
		return ((uintptr_t)buff + buffSize) - (uintptr_t)end;
	}

	SIZE writtenSize() const
	{
		return written;
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
			writePos = advanceAddr(writePos, 1);
		}
		end = writePos;
		written += size;

		assert((SIZE)written <= buffSize);

		//logMsg("wrote %d bytes", (int)size);
		return size;
	}

	char *writeAddr() const
	{
		return end;
	}

	void commitWrite(SIZE size)
	{
		assert(size <= freeSpace());
		end = advanceAddr(end, size);
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
			readPos = advanceAddr(readPos, 1);
		}
		start = readPos;
		written -= size;

		//logMsg("read %d bytes", (int)size);
		return size;
	}

	char *readAddr() const
	{
		return start;
	}

	void commitRead(SIZE size)
	{
		assert(size <= (SIZE)written);
		start = advanceAddr(start, size);
		written -= size;
	}

	// given an address inside the ring buffer, return the address
	// after moving the pointer forward, wrapping as needed
	char *advanceAddr(char *ptr, SIZE size) const
	{
		assert(ptr >= buff && ptr < buff+buffSize*2);
		ptr += size;
		if(ptr >= buff+buffSize)
			ptr -= buffSize;
		return ptr;
	}

private:
	char *buff = nullptr;
	char *start = nullptr, *end = nullptr;
	COUNT written {0};
	SIZE buffSize {0};
};

template <class COUNT = std::atomic_uint, class SIZE = unsigned int>
class RingBuffer : public StaticRingBuffer<COUNT, SIZE>
{
public:
	using StaticRingBuffer<COUNT, SIZE>::StaticRingBuffer;

	~RingBuffer()
	{
		StaticRingBuffer<COUNT, SIZE>::deinit();
	}
};
