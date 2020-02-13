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

#define LOGTAG "MirroredBuffer"
#include <imagine/util/ringbuffer/mirroredBuffer.hh>
#include <imagine/util/system/pagesize.h>
#include <imagine/logger/logger.h>
#include <imagine/config/env.hh>
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

uint32_t adjustMirroredBufferAllocSize(uint32_t size)
{
	return roundUpToPageSize(size);
}

void *allocMirroredBuffer(uint32_t size)
{
	if(Config::DEBUG_BUILD && size != adjustMirroredBufferAllocSize(size))
	{
		logErr("size:%u is not a multiple of page size", size);
	}
	// allocate enough pages for the buffer + the mirrored pages
	char *buff = (char*)mmap(nullptr, size*2, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if(buff == MAP_FAILED)
	{
		logErr("error in mmap");
		return nullptr;
	}
	// pass 0 to old_size to create a mirror of the buffer in the 2nd half of the mapping
	auto mirror = mremap(buff, 0, size, MREMAP_MAYMOVE | MREMAP_FIXED, buff + size);
	if(mirror == MAP_FAILED)
	{
		logErr("error in mremap");
		freeMirroredBuffer(buff, size);
		return nullptr;
	}
	return buff;
}

void freeMirroredBuffer(void *buff, uint32_t size)
{
	if(!buff)
		return;
	if(munmap(buff, size*2) == -1)
	{
		logWarn("error in unmap");
	}
}

}
