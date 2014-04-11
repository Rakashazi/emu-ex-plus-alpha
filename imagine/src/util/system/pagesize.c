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

#include <imagine/util/system/pagesize.h>
#if defined __linux__ && !defined __ANDROID__
#include <unistd.h>
#define USE_GETPAGESIZE
#elif defined __APPLE__
#include <mach/machine/vm_param.h>
#else
#include <limits.h>
#endif
#include <assert.h>

static int pageSize_ = 0;

void initPageSize()
{
	#ifdef USE_GETPAGESIZE
	pageSize_ = getpagesize();
	assert(pageSize_);
	#endif
}

int pageSize()
{
	#ifdef USE_GETPAGESIZE
	assert(pageSize_);
	return pageSize_;
	#else
	return PAGE_SIZE;
	#endif
}

static uintptr_t pageSizeAsPtrSize()
{
	return (uintptr_t)pageSize();
}

uintptr_t roundDownToPageSize(uintptr_t val)
{
	return val & ~(pageSizeAsPtrSize());
}

uintptr_t roundUpToPageSize(uintptr_t val)
{
	return (val + pageSizeAsPtrSize()-1) & ~(pageSizeAsPtrSize()-1);
}
