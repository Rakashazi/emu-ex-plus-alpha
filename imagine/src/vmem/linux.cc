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

#include <imagine/config/defs.hh>
#include <imagine/vmem/memory.hh>
#include <imagine/util/utility.h>
#include <imagine/logger/logger.h>
#include <sys/mman.h>
#include <unistd.h>

#if defined __ANDROID__ && ANDROID_MIN_API <= 24
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

constexpr SystemLogger log{"VMem"};
uintptr_t pageSize = sysconf(_SC_PAGESIZE);

static std::span<uint8_t> vAlloc(size_t bytes, bool shared)
{
	int flags = (shared ? MAP_SHARED : MAP_PRIVATE) | MAP_ANONYMOUS;
	void *buff = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, flags, -1, 0);
	if(buff == MAP_FAILED) [[unlikely]]
	{
		log.error("error in mmap");
		return {};
	}
	return {static_cast<uint8_t*>(buff), bytes};
}

std::span<uint8_t> vAlloc(size_t bytes)
{
	return vAlloc(bytes, false);
}

void vFree(std::span<uint8_t> buff)
{
	if(!buff.data())
		return;
	if(munmap(buff.data(), buff.size_bytes()) == -1)
	{
		log.error("error in unmap");
	}
}

std::span<uint8_t> vAllocMirrored(size_t bytes)
{
	assert(bytes == roundPageSize(bytes));
	// allocate enough pages for the buffer + the mirrored pages
	auto buff = vAlloc(bytes * 2, true);
	if(!buff.data()) [[unlikely]]
	{
		return {};
	}
	// pass 0 to old_size to create a mirror of the buffer in the 2nd half of the mapping
	auto mirror = mremap(buff.data(), 0, bytes, MREMAP_MAYMOVE | MREMAP_FIXED, buff.data() + bytes);
	if(mirror == MAP_FAILED) [[unlikely]]
	{
		log.error("error in mremap");
		vFree(buff);
		return {};
	}
	return buff;
}

}
