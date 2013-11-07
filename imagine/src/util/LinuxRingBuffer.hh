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
#include <sys/mman.h>
#include <limits.h>
#ifdef __ANDROID__
#include <sys/syscall.h>
#endif

template <class COUNT = std::atomic_uint, class SIZE = uint>
class LinuxRingBuffer
{
public:
	constexpr LinuxRingBuffer() {}

	#ifdef __ANDROID__
	// Bionic is missing extended mremap with new_address parameter
	static void *mremap(void *old_address, size_t old_size, size_t new_size, int flags, void *new_address)
	{
		return (void*)syscall(__NR_mremap, old_address, old_size, new_size, flags, new_address);
	}
	#endif

	static uint roundUpToPageSize(uint size)
	{
		return (size + PAGESIZE-1) & ~(PAGESIZE-1);
	}

	bool init(SIZE size)
	{
		deinit();
		buffSize = size;
		allocBuffSize = roundUpToPageSize(size);
		logMsg("allocating ring buffer with size %d (%d rounded up)", size, allocBuffSize);
		buff = (char*)mmap(nullptr, allocBuffSize*2, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		if(buff == MAP_FAILED)
		{
			logErr("error in mmap");
			buff = nullptr;
			return false;
		}
		{
			// undocumented mremap feature that mirrors/aliases mappings when old_size == 0
			auto mirror = mremap(buff, 0, allocBuffSize, MREMAP_MAYMOVE | MREMAP_FIXED, buff + allocBuffSize);
			if(mirror == MAP_FAILED)
			{
				logErr("error in mremap");
				if(munmap(buff, allocBuffSize*2) == -1)
				{
					buff = nullptr;
					logWarn("error in unmap");
				}
				return false;
			}
		}
		reset();
		return true;
	}

	void deinit()
	{
		if(buff)
		{
			if(munmap(buff, allocBuffSize*2) == -1)
			{
				logWarn("error in unmap");
			}
			buff = nullptr;
			buffSize = 0;
			written = 0;
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

	char *wrapPtr(char *ptr)
	{
		if(ptr >= &buff[allocBuffSize])
			ptr -= allocBuffSize;
		return ptr;
	}

	SIZE write(const void *buff, SIZE size)
	{
		if(size > freeSpace())
			size = freeSpace();
		memcpy(end, buff, size);
		end = advancePos(end, size);
		written += size;
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

	void rewindWrite(SIZE size)
	{
		assert(size <= (SIZE)written);
		end = advancePos(end, -size);
		written -= size;
	}

	SIZE read(void *buff, SIZE size)
	{
		if(size > (SIZE)written)
			size = written;
		memcpy(buff, start, size);
		start = advancePos(start, size);
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
		return wrapPtr(ptr + size);
	}

	COUNT written {0};
	SIZE buffSize = 0;
	uint allocBuffSize = 0;
private:
	char *buff = nullptr;
	char *start = nullptr, *end = nullptr;
};
