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
#include <imagine/base/Window.hh>
#include <imagine/base/GLContext.hh>
#include <imagine/base/Timer.hh>
#include <imagine/base/sharedLibrary.hh>
#include <imagine/util/system/pagesize.h>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/DelegateFuncSet.hh>
#ifdef __ANDROID__
#include <android/log.h>
#endif
#include <imagine/logger/logger.h>
#include <imagine/time/Time.hh>
#include <cstdlib>
#include <dlfcn.h>

namespace Base
{

const char copyright[] = "Imagine is Copyright 2010-2020 Robert Broglia";

static InterProcessMessageDelegate onInterProcessMessage_;
static DelegateFuncSet<ResumeDelegate> onResume_;
static FreeCachesDelegate onFreeCaches_;
static DelegateFuncSet<ExitDelegate> onExit_;
static ActivityState appState = ActivityState::PAUSED;

void engineInit()
{
	logDMsg("%s", copyright);
	logDMsg("compiled on %s %s", __DATE__, __TIME__);
}

ActivityState activityState()
{
	return appState;
}

void setPausedActivityState()
{
	if(appState == ActivityState::EXITING)
	{
		return; // ignore setting paused state while exiting
	}
	appState = ActivityState::PAUSED;
}

void setRunningActivityState()
{
	assert(appState != ActivityState::EXITING); // should never set running state after exit state
	appState = ActivityState::RUNNING;
}

void setExitingActivityState()
{
	appState = ActivityState::EXITING;
}

bool appIsRunning()
{
	return activityState() == ActivityState::RUNNING;
}

bool appIsPaused()
{
	return activityState() == ActivityState::PAUSED;
}

bool appIsExiting()
{
	return activityState() == ActivityState::EXITING;
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

void dispatchOnFreeCaches(bool running)
{
	onFreeCaches_.callCopySafe(running);
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

void exitWithErrorMessagePrintf(int exitVal, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	auto vaEnd = IG::scopeGuard([&](){ va_end(args); });
	exitWithErrorMessageVPrintf(exitVal, format, args);
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

void Timer::runOnce(Time time, Time repeatTime, EventLoop loop, CallbackDelegate callback)
{
	if(isArmed())
		return;
	run(time, repeatTime, loop, callback);
}

IG::PixelFormat GLBufferConfigAttributes::pixelFormat() const
{
	if(!pixelFormat_)
		return Window::defaultPixelFormat();
	return pixelFormat_;
}

FS::RootPathInfo nearestRootPath(const char *path)
{
	if(!path)
		return {};
	auto location = rootFileLocations();
	const FS::PathLocation *nearestPtr{};
	size_t lastMatchOffset = 0;
	for(const auto &l : location)
	{
		auto subStr = strstr(path, l.path.data());
		if(subStr != path)
			continue;
		auto matchOffset = (size_t)(&path[l.root.length] - path);
		if(matchOffset > lastMatchOffset)
		{
			nearestPtr = &l;
			lastMatchOffset = matchOffset;
		}
	}
	if(!lastMatchOffset)
		return {};
	logMsg("found root location:%s with length:%d", nearestPtr->root.name.data(), (int)nearestPtr->root.length);
	return {nearestPtr->root.name, nearestPtr->root.length};
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

}

namespace IG
{

FrameTime FrameParams::timestampDiff() const
{
	assumeExpr(timestamp_ >= lastTimestamp_);
	return lastTimestamp_.count() ? timestamp_ - lastTimestamp_ : FrameTime{};
}

FrameTime FrameParams::presentTime() const
{
	return timestamp_ + std::chrono::duration_cast<FrameTime>(frameTime_);
}

uint32_t FrameParams::elapsedFrames() const
{
	if(!lastTimestamp_.count())
		return 1;
	assumeExpr(timestamp_ >= lastTimestamp_);
	assumeExpr(frameTime_.count() > 0);
	FrameTime diff = timestamp_ - lastTimestamp_;
	uint32_t elapsed = std::round(FloatSeconds(diff) / frameTime_);
	return std::max(elapsed, 1u);
}

uint32_t FrameParams::elapsedFrames(uint32_t frameCap) const
{
	return std::min(elapsedFrames(), frameCap);
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
