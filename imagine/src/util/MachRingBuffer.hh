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
#include <mach/mach.h>
#include <mach/vm_map.h>

template <class COUNT = std::atomic_uint, class SIZE = uint>
class MachRingBuffer
{
public:
	bool init(SIZE size)
	{
		deinit();
		buffSize = size;
		allocBuffSize = round_page(size);
		vm_address_t addr;
		logMsg("allocating ring buffer with size %d (%d rounded up)", size, allocBuffSize);
		if(vm_allocate(mach_task_self(), &addr, allocBuffSize*2, VM_FLAGS_ANYWHERE) != KERN_SUCCESS)
		{
			logErr("error in vm_allocate");
			return false;
		}
		{
			vm_prot_t currProtect, maxProtect;
			vm_address_t mirrorAddr = addr + allocBuffSize;
			if(vm_remap(mach_task_self(), &mirrorAddr, allocBuffSize, 0,
				VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE, mach_task_self(), addr,
				0, &currProtect, &maxProtect, VM_INHERIT_COPY) != KERN_SUCCESS)
			{
				logErr("error in vm_remap");
				if(vm_deallocate(mach_task_self(), addr, allocBuffSize*2) != KERN_SUCCESS)
				{
					logWarn("error in vm_deallocate");
				}
				return false;
			}
		}
		buff = (uchar*)addr;
		reset();
		return true;
	}

	void deinit()
	{
		if(buff)
		{
			if(vm_deallocate(mach_task_self(), (vm_address_t)buff, allocBuffSize*2) != KERN_SUCCESS)
			{
				logWarn("error in vm_deallocate");
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

	uchar *wrapPtr(uchar *ptr)
	{
		if(ptr >= &buff[allocBuffSize])
			ptr -= allocBuffSize;
		return ptr;
	}

	SIZE write(uchar *buff, SIZE size)
	{
		if(size > freeSpace())
			size = freeSpace();
		memcpy(end, buff, size);
		end = wrapPtr(end + size);
		written += size;
		//logMsg("wrote %d bytes", (int)size);
		return size;
	}

	uchar *writePos()
	{
		return end;
	}

	void advanceWritePos(SIZE size)
	{
		assert(size <= freeSpace());
		end = wrapPtr(end + size);
		written += size;
	}

	SIZE read(uchar *buff, SIZE size)
	{
		if(size > (SIZE)written)
			size = written;
		memcpy(buff, start, size);
		start = wrapPtr(start + size);
		written -= size;
		//logMsg("read %d bytes", (int)size);
		return size;
	}

	COUNT written {0};
	SIZE buffSize = 0;
	uint allocBuffSize = 0;
private:
	uchar *buff = nullptr;
	uchar *start = nullptr, *end = nullptr;
};
