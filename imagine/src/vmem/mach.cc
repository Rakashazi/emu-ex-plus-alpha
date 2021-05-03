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
#include <imagine/logger/logger.h>
#include <mach/mach.h>
#include <mach/vm_map.h>

namespace IG
{

void *allocVMem(size_t size)
{
	if(Config::DEBUG_BUILD && size != adjustVMemAllocSize(size))
	{
		logErr("size:%lu is not a multiple of page size", (unsigned long)size);
	}
	vm_address_t addr;
	if(vm_allocate(mach_task_self(), &addr, size, VM_FLAGS_ANYWHERE) != KERN_SUCCESS) [[unlikely]]
	{
		logErr("error in vm_allocate");
		return nullptr;
	}
	return (void*)addr;
}

void freeVMem(void *vMemPtr, size_t size)
{
	if(!vMemPtr)
		return;
	if(vm_deallocate(mach_task_self(), (vm_address_t)vMemPtr, size) != KERN_SUCCESS)
	{
		logWarn("error in vm_deallocate");
	}
}

size_t adjustVMemAllocSize(size_t size)
{
	return round_page(size);
}

void *allocMirroredBuffer(size_t size)
{
	// allocate enough pages for the buffer + the mirrored pages
	vm_address_t addr = (vm_address_t)allocVMem(size * 2);
	if(!addr) [[unlikely]]
	{
		return nullptr;
	}
	#ifdef __ARM_ARCH_6K__
	// VM_FLAGS_OVERWRITE isn't supported on iOS <= 4.2.1 (the max deployment target for ARMv6)
	// so deallocate the 2nd half of the buffer first. This introduces a race condition but the
	// chance of it causing a problem is very low.
	if(vm_deallocate(mach_task_self(), addr + size, size) != KERN_SUCCESS)
	{
		logWarn("error in vm_deallocate for 2nd half, buffer may not stay in sync");
	}
	#endif
	vm_prot_t currProtect, maxProtect;
	vm_address_t mirrorAddr = addr + size;
	if(vm_remap(mach_task_self(), &mirrorAddr, size, 0,
		VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE, mach_task_self(), addr,
		0, &currProtect, &maxProtect, VM_INHERIT_COPY) != KERN_SUCCESS)
	{
		logErr("error in vm_remap");
		freeMirroredBuffer((void*)addr, size);
		return nullptr;
	}
	return (void*)addr;
}

void freeMirroredBuffer(void *vMemPtr, size_t size)
{
	freeVMem(vMemPtr, size * 2);
}

}
