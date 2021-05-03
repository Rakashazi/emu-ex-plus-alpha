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

#define LOGTAG "VMem"
#include <imagine/config/env.hh>
#include <imagine/vmem/memory.hh>
#include <imagine/util/utility.h>
#include <imagine/util/system/pagesize.h>
#include <imagine/logger/logger.h>
#include <sys/mman.h>

#if defined __ANDROID__ && __ANDROID_API__ <= 24
#define NEEDS_MREMAP_SYSCALL
#endif

#ifdef NEEDS_MREMAP_SYSCALL
#include <unistd.h>
#include <sys/syscall.h>

// Bionic pre-Android 7 is missing extended mremap with new_address parameter
static void *mremap(void *old_address, size_t old_size, size_t new_size, int flags, void *new_address)
{
	return (void*)syscall(__NR_mremap, old_address, old_size, new_size, flags, new_address);
}
#endif

namespace IG
{

static void *allocVMem(size_t size, bool shared)
{
	if(Config::DEBUG_BUILD && size != adjustVMemAllocSize(size))
	{
		logErr("size:%lu is not a multiple of page size", (unsigned long)size);
	}
	int flags = (shared ? MAP_SHARED : MAP_PRIVATE) | MAP_ANONYMOUS;
	void *buff = mmap(nullptr, size, PROT_READ | PROT_WRITE, flags, -1, 0);
	if(buff == MAP_FAILED) [[unlikely]]
	{
		logErr("error in mmap");
		return nullptr;
	}
	return buff;
}

void *allocVMem(size_t size)
{
	return allocVMem(size, false);
}

void freeVMem(void *vMemPtr, size_t size)
{
	if(!vMemPtr)
		return;
	if(munmap(vMemPtr, size) == -1)
	{
		logWarn("error in unmap");
	}
}

size_t adjustVMemAllocSize(size_t size)
{
	return roundUpToPageSize(size);
}

void *allocMirroredBuffer(size_t size)
{
	// allocate enough pages for the buffer + the mirrored pages
	char *buff = (char*)allocVMem(size * 2, true);
	if(!buff) [[unlikely]]
	{
		return nullptr;
	}
	// pass 0 to old_size to create a mirror of the buffer in the 2nd half of the mapping
	auto mirror = mremap(buff, 0, size, MREMAP_MAYMOVE | MREMAP_FIXED, buff + size);
	if(mirror == MAP_FAILED) [[unlikely]]
	{
		logErr("error in mremap");
		freeMirroredBuffer(buff, size);
		return nullptr;
	}
	return buff;
}

void freeMirroredBuffer(void *vMemPtr, size_t size)
{
	freeVMem(vMemPtr, size * 2);
}

}
