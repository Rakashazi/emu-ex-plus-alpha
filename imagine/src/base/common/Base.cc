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

#if defined __unix__ || defined __APPLE__
#include <unistd.h>
#include <sys/resource.h>
#endif
#include "basePrivate.hh"
#include <imagine/base/Base.hh>
#include <imagine/util/system/pagesize.h>
#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace Base
{

const char copyright[] = "Imagine is Copyright 2010-2014 Robert Broglia";

void engineInit()
{
	#ifdef CONFIG_INITPAGESIZE
	initPageSize();
	#endif
	#if defined __unix__ || (defined __APPLE__ && !TARGET_OS_IPHONE)
	struct rlimit stack;
	getrlimit(RLIMIT_STACK, &stack);
	stack.rlim_cur = 16 * 1024 * 1024;
	assert(stack.rlim_cur <= stack.rlim_max);
	setrlimit(RLIMIT_STACK, &stack);
		#ifndef NDEBUG
		getrlimit(RLIMIT_STACK, &stack);
		logMsg("stack limit %u:%u", (uint)stack.rlim_cur, (uint)stack.rlim_max);
		#endif
	#endif

	logDMsg("%s", copyright);
	logDMsg("compiled on %s %s", __DATE__, __TIME__);
	mem_init();
}

// needed by GCC when not compiling with libstdc++/libsupc++, or to override it
CLINK [[gnu::weak]] void __cxa_pure_virtual() { bug_exit("called pure virtual"); }

#if defined(__unix__) || defined(__APPLE__)
void sleepUs(int us)
{
	usleep(us);
}

void sleepMs(int ms)
{
	sleepUs(ms*1000);
}
#endif

}

#if defined(__has_feature)
	#if __has_feature(address_sanitizer) && defined CONFIG_BASE_CUSTOM_NEW_DELETE
	#undef CONFIG_BASE_NO_CUSTOM_NEW_DELETE
	#warning "cannot use custom new/delete with address sanitizer"
	#endif
#endif

#ifdef CONFIG_BASE_CUSTOM_NEW_DELETE

void* operator new (std::size_t size)
#ifdef __EXCEPTIONS
	throw (std::bad_alloc)
#endif
{ return mem_alloc(size); }

void* operator new[] (std::size_t size)
#ifdef __EXCEPTIONS
	throw (std::bad_alloc)
#endif
{ return mem_alloc(size); }

#ifdef CONFIG_BASE_PS3
void *operator new(_CSTD size_t size, _CSTD size_t align)
	_THROW1(_XSTD bad_alloc)
{
	//logMsg("called aligned new, size %d @ %d byte boundary", (int)size, (int)align);
	return memalign(size, align);
}
#endif

void operator delete (void *o) noexcept { mem_free(o); }
void operator delete[] (void *o) noexcept { mem_free(o); }

#endif

#ifdef __EXCEPTIONS
namespace __gnu_cxx
{

EVISIBLE void __verbose_terminate_handler()
{
	logErr("terminated by uncaught exception");
  abort();
}

}
#endif

CLINK void bug_doExit(const char *msg, ...)
{
	#ifdef __ANDROID__
	va_list args;
	va_start(args, msg);
	char str[256];
	vsnprintf(str, sizeof(str), msg, args);
	__android_log_assert("%s", "imagine", str);
	#else
	va_list args;
	va_start(args, msg);
	logger_vprintf(LOG_E, msg, args);
	va_end(args);
	Base::abort();
	#endif
}
