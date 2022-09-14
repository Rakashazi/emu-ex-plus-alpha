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

#include <imagine/vmem/pageSize.hh>
#include <imagine/util/utility.h>
#if defined __linux__
#include <unistd.h>
#define USE_GETPAGESIZE
#elif defined __APPLE__
#include <mach/machine/vm_param.h>
#else
#include <limits.h>
#endif

namespace IG
{

uintptr_t pageSize()
{
	#ifdef USE_GETPAGESIZE
	static uintptr_t pageSize_ = sysconf(_SC_PAGESIZE);
	assumeExpr(pageSize_);
	return pageSize_;
	#else
	return PAGE_SIZE;
	#endif
}

}
