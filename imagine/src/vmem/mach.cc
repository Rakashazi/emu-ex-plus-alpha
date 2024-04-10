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
#include <mach/mach.h>
#include <mach/vm_map.h>
#include <mach/machine/vm_param.h>

namespace IG
{

constexpr SystemLogger log{"VMem"};
uintptr_t pageSize = PAGE_SIZE;

std::span<uint8_t> vAlloc(size_t bytes)
{
	vm_address_t addr;
	if(vm_allocate(mach_task_self(), &addr, bytes, VM_FLAGS_ANYWHERE) != KERN_SUCCESS) [[unlikely]]
	{
		log.error("error in vm_allocate");
		return {};
	}
	return {reinterpret_cast<uint8_t*>(addr), bytes};
}

void vFree(std::span<uint8_t> buff)
{
	if(!buff.data())
		return;
	if(vm_deallocate(mach_task_self(), vm_address_t(buff.data()), buff.size()) != KERN_SUCCESS)
	{
		log.error("error in vm_deallocate");
	}
}

std::span<uint8_t> vAllocMirrored(size_t bytes)
{
	assert(bytes == roundPageSize(bytes));
	// allocate enough pages for the buffer + the mirrored pages
	auto buff = vAlloc(bytes * 2);
	if(!buff.data()) [[unlikely]]
	{
		return {};
	}
	#ifdef __ARM_ARCH_6K__
	// VM_FLAGS_OVERWRITE isn't supported on iOS <= 4.2.1 (the max deployment target for ARMv6)
	// so deallocate the 2nd half of the buffer first. This introduces a race condition but the
	// chance of it causing a problem is very low.
	if(vm_deallocate(mach_task_self(), (vm_address_t)buff.data() + bytes, bytes) != KERN_SUCCESS)
	{
		log.warn("error in vm_deallocate for 2nd half, buffer may not stay in sync");
	}
	#endif
	vm_prot_t currProtect, maxProtect;
	vm_address_t mirrorAddr = vm_address_t(buff.data()) + bytes;
	if(vm_remap(mach_task_self(), &mirrorAddr, bytes, 0,
		VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE,
		mach_task_self(), vm_address_t(buff.data()), 0,
		&currProtect, &maxProtect, VM_INHERIT_COPY) != KERN_SUCCESS)
	{
		log.error("error in vm_remap");
		vFree(buff);
		return {};
	}
	return buff;
}

}
