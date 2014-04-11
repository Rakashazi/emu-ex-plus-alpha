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

#include <atomic>
#include <sys/mman.h>
#include <unistd.h>
#include <imagine/util/system/pagesize.h>
#ifdef __ANDROID__
#include <sys/syscall.h>
#endif

template <class COUNT = std::atomic_uint, class SIZE = unsigned int>
class StaticLinuxRingBuffer
{
public:
	constexpr StaticLinuxRingBuffer() {}

	bool init(SIZE size)
	{
		deinit();
		buffSize = size;
		allocBuffSize = roundUpToPageSize(size);
		logMsg("allocating ring buffer with size %d (%d rounded up)", size, allocBuffSize);
		auto addr = mmap(nullptr, allocBuffSize*2, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		if(addr == MAP_FAILED)
		{
			logErr("error in mmap");
			deinit();
			return false;
		}
		buff = (char*)addr;
		{
			// undocumented mremap feature that mirrors/aliases mappings when old_size == 0
			auto mirror = mremap(buff, 0, allocBuffSize, MREMAP_MAYMOVE | MREMAP_FIXED, buff + allocBuffSize);
			if(mirror == MAP_FAILED)
			{
				logErr("error in mremap");
				deinit();
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
		return freeSpace();
	}

	SIZE writtenSize() const
	{
		return written;
	}

	SIZE write(const void *buff, SIZE size)
	{
		if(size > freeSpace())
			size = freeSpace();
		memcpy(end, buff, size);
		end = advanceAddr(end, size);
		written += size;
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
		memcpy(buff, start, size);
		start = advanceAddr(start, size);
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

	char *advanceAddr(char *ptr, SIZE size) const
	{
		return wrapPtr(ptr + size);
	}

private:
	char *buff = nullptr;
	char *start = nullptr, *end = nullptr;
	COUNT written {0};
	SIZE buffSize {0};
	SIZE allocBuffSize {0};

	#ifdef __ANDROID__
	// Bionic is missing extended mremap with new_address parameter
	static void *mremap(void *old_address, size_t old_size, size_t new_size, int flags, void *new_address)
	{
		return (void*)syscall(__NR_mremap, old_address, old_size, new_size, flags, new_address);
	}
	#endif

	char *wrapPtr(char *ptr) const
	{
		if(ptr >= &buff[allocBuffSize])
			ptr -= allocBuffSize;
		return ptr;
	}
};

template <class COUNT = std::atomic_uint, class SIZE = unsigned int>
class LinuxRingBuffer : public StaticLinuxRingBuffer<COUNT, SIZE>
{
public:
	using StaticLinuxRingBuffer<COUNT, SIZE>::StaticLinuxRingBuffer;

	~LinuxRingBuffer()
	{
		StaticLinuxRingBuffer<COUNT, SIZE>::deinit();
	}
};
