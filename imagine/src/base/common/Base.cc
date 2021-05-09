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

#include <imagine/base/EventLoop.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/GLContext.hh>
#include <imagine/base/sharedLibrary.hh>
#include <imagine/time/Time.hh>
#include <imagine/data-type/image/GfxImageSource.hh>
#include <imagine/logger/logger.h>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#if defined __unix__ || defined __APPLE__
#include <unistd.h>
#endif
#ifdef __linux__
#include <sys/resource.h>
#endif
#ifdef __ANDROID__
#include <android/log.h>
#else
#include <execinfo.h>
#endif

namespace Base
{

const char *orientationToStr(Orientation o)
{
	switch(o)
	{
		case VIEW_ROTATE_AUTO: return "Auto";
		case VIEW_ROTATE_0: return "0";
		case VIEW_ROTATE_90: return "90";
		case VIEW_ROTATE_180: return "180";
		case VIEW_ROTATE_270: return "270";
		case VIEW_ROTATE_0 | VIEW_ROTATE_90 | VIEW_ROTATE_270: return "0/90/270";
		case VIEW_ROTATE_0 | VIEW_ROTATE_90 | VIEW_ROTATE_180 | VIEW_ROTATE_270: return "0/90/180/270";
		case VIEW_ROTATE_90 | VIEW_ROTATE_270: return "90/270";
		default: bug_unreachable("o == %d", o); return "";
	}
}

bool orientationIsSideways(Orientation o)
{
	return o == VIEW_ROTATE_90 || o == VIEW_ROTATE_270;
}

FDEventSource::FDEventSource(const char *debugLabel, int fd, EventLoop loop, PollEventDelegate callback, uint32_t events):
	FDEventSource{debugLabel, fd}
{
	attach(loop, callback, events);
}

bool FDEventSource::attach(PollEventDelegate callback, uint32_t events)
{
	return attach({}, callback, events);
}

SharedLibraryRef openSharedLibrary(const char *name, unsigned flags)
{
	int mode = flags & RESOLVE_ALL_SYMBOLS_FLAG ? RTLD_NOW : RTLD_LAZY;
	auto lib = dlopen(name, mode);
	if(Config::DEBUG_BUILD && !lib)
	{
		logErr("dlopen(%s) error:%s", name, dlerror());
	}
	return lib;
}

void closeSharedLibrary(SharedLibraryRef lib)
{
	dlclose(lib);
}

void *loadSymbol(SharedLibraryRef lib, const char *name)
{
	if(!lib)
		lib = RTLD_DEFAULT;
	return dlsym(lib, name);
}

GLContext GLManager::makeContext(GLContextAttributes attr, GLBufferConfig config, IG::ErrorCode &ec)
{
	return makeContext(attr, config, {}, ec);
}

void GLManager::resetCurrentContext() const
{
	display().resetCurrentContext();
}

}

namespace IG
{

FrameTime FrameParams::presentTime() const
{
	return timestamp_ + std::chrono::duration_cast<FrameTime>(frameTime_);
}

uint32_t FrameParams::elapsedFrames(FrameTime lastTimestamp) const
{
	return elapsedFrames(timestamp_, lastTimestamp, frameTime_);
}

uint32_t FrameParams::elapsedFrames(FrameTime timestamp, FrameTime lastTimestamp, FloatSeconds frameTime)
{
	if(!lastTimestamp.count())
		return 1;
	assumeExpr(timestamp >= lastTimestamp);
	assumeExpr(frameTime.count() > 0);
	FrameTime diff = timestamp - lastTimestamp;
	uint32_t elapsed = std::round(FloatSeconds(diff) / frameTime);
	return std::max(elapsed, 1u);
}

void setThisThreadPriority(int nice)
{
	#ifdef __linux__
	assert(nice > -20);
	auto tid = gettid();
	if(setpriority(PRIO_PROCESS, tid, nice) == -1)
	{
		if(Config::DEBUG_BUILD)
		{
			logErr("error:%s setting thread:0x%X nice level:%d", strerror(errno), (unsigned)tid, nice);
		}
	}
	else
	{
		//logDMsg("set thread:0x%X nice level:%d", (unsigned)tid, nice);
	}
	#endif
}

int thisThreadPriority()
{
	#ifdef __linux__
	return getpriority(PRIO_PROCESS, gettid());
	#else
	return 0;
	#endif
}

}

GfxImageSource::~GfxImageSource() {}

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
{ return std::malloc(size); }

void* operator new[] (std::size_t size)
#ifdef __EXCEPTIONS
	throw (std::bad_alloc)
#endif
{ return std::malloc(size); }

void operator delete (void *o) noexcept { std::free(o); }
void operator delete[] (void *o) noexcept { std::free(o); }

#endif

#ifdef __EXCEPTIONS
namespace __gnu_cxx
{

void __verbose_terminate_handler()
{
	logErr("terminated by uncaught exception");
  abort();
}

}
#endif

#ifndef __ANDROID__
static void logBacktrace()
{
	void *arr[10];
	auto size = backtrace(arr, 10);
	char **backtraceStrings = backtrace_symbols(arr, size);
	iterateTimes(size, i)
		logger_printf(LOG_E, "%s\n", backtraceStrings[i]);
}
#endif

CLINK void bug_doExit(const char *msg, ...)
{
	#ifdef __ANDROID__
	va_list args;
	va_start(args, msg);
	char str[256];
	vsnprintf(str, sizeof(str), msg, args);
	logErr("%s", str);
	__android_log_assert("", "imagine", "%s", str);
	#else
	va_list args;
	va_start(args, msg);
	logger_vprintf(LOG_E, msg, args);
	va_end(args);
	logger_printf(LOG_E, "\n");
	usleep(500000); // TODO: need a way to flush every type of log output
	abort();
	#endif
}
