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
#ifndef __ANDROID__
#include <execinfo.h>
#endif
#include "basePrivate.hh"
#include <imagine/base/Base.hh>
#include <imagine/base/EventLoop.hh>
#include <imagine/util/system/pagesize.h>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/DelegateFuncSet.hh>
#ifdef __ANDROID__
#include <android/log.h>
#endif
#include <imagine/logger/logger.h>
#include <imagine/time/Time.hh>
#include <imagine/mem/mem.h>

namespace Base
{

const char copyright[] = "Imagine is Copyright 2010-2020 Robert Broglia";

static InterProcessMessageDelegate onInterProcessMessage_;
static DelegateFuncSet<ResumeDelegate> onResume_;
static FreeCachesDelegate onFreeCaches_;
static DelegateFuncSet<ExitDelegate> onExit_;

void engineInit()
{
	logDMsg("%s", copyright);
	logDMsg("compiled on %s %s", __DATE__, __TIME__);
	mem_init();
}

void setOnInterProcessMessage(InterProcessMessageDelegate del)
{
	onInterProcessMessage_ = del;
}

bool addOnResume(ResumeDelegate del, int priority)
{
	return onResume_.add(del, priority);
}

bool removeOnResume(ResumeDelegate del)
{
	return onResume_.remove(del);
}

bool containsOnResume(ResumeDelegate del)
{
	return onResume_.contains(del);
}

void setOnFreeCaches(FreeCachesDelegate del)
{
	onFreeCaches_ = del;
}

bool addOnExit(ExitDelegate del, int priority)
{
	return onExit_.add(del, priority);
}

bool removeOnExit(ExitDelegate del)
{
	return onExit_.remove(del);
}

bool containsOnExit(ExitDelegate del)
{
	return onExit_.contains(del);
}

void dispatchOnInterProcessMessage(const char *filename)
{
	onInterProcessMessage_.callCopySafe(filename);
}

void dispatchOnResume(bool focused)
{
	onResume_.runAll([&](ResumeDelegate del){ return del(focused); });
}

void dispatchOnFreeCaches()
{
	onFreeCaches_.callCopySafe();
}

void dispatchOnExit(bool backgrounded)
{
	onExit_.runAll([&](ExitDelegate del){ return del(backgrounded); });
}

const InterProcessMessageDelegate &onInterProcessMessage()
{
	return onInterProcessMessage_;
}

const FreeCachesDelegate &onFreeCaches()
{
	return onFreeCaches_;
}

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

Orientation validateOrientationMask(Orientation oMask)
{
	if(IG::bitsSet(oMask & VIEW_ROTATE_ALL) == 0)
	{
		// use default when none of the orientation bits are set
		oMask = defaultSystemOrientations();
	}
	return oMask;
}

FrameTimeBase timeSinceFrameTime(FrameTimeBase time)
{
	return frameTimeBaseFromNSecs(IG::Time::now().nSecs()) - time;
}

void exitWithErrorMessagePrintf(int exitVal, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	auto vaEnd = IG::scopeGuard([&](){ va_end(args); });
	exitWithErrorMessageVPrintf(exitVal, format, args);
}

#ifdef NDEBUG
FDEventSource::FDEventSource(int fd, EventLoop loop, PollEventDelegate callback, uint32_t events):
	FDEventSource{fd}
#else
FDEventSource::FDEventSource(const char *debugLabel, int fd, EventLoop loop, PollEventDelegate callback, uint32_t events):
	FDEventSource{debugLabel, fd}
#endif
{
	addToEventLoop(loop, callback, events);
}

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
	__android_log_assert("", "imagine", "%s", str);
	#else
	va_list args;
	va_start(args, msg);
	logger_vprintf(LOG_E, msg, args);
	va_end(args);
	logger_printf(LOG_E, "\n");
	usleep(500000); // TODO: need a way to flush every type of log output
	Base::abort();
	#endif
}
