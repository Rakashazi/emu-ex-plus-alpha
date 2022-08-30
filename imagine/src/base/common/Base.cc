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
#include <imagine/thread/Thread.hh>
#include <imagine/util/math/Point2D.hh>
#include <imagine/util/ranges.hh>
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
#if defined __APPLE__
#include <pthread.h>
#endif

namespace IG
{

std::string_view asString(OrientationMask o)
{
	using enum OrientationMask;
	switch(o)
	{
		case UNSET: return "Unset";
		case PORTRAIT: return "Portrait";
		case LANDSCAPE_RIGHT: return "Landscape Right";
		case PORTRAIT_UPSIDE_DOWN: return "Portrait Upside-Down";
		case LANDSCAPE_LEFT: return "Landscape Left";
		case ALL_LANDSCAPE: return "Either Landscape";
		case ALL_PORTRAIT: return "Either Portrait";
		case ALL_BUT_UPSIDE_DOWN: return "All But Upside-Down";
		case ALL: return "All";
	}
	return "Unknown";
}

FDEventSource::FDEventSource(const char *debugLabel, MaybeUniqueFileDescriptor fd, EventLoop loop, PollEventDelegate callback, uint32_t events):
	FDEventSource{debugLabel, std::move(fd)}
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

const char *lastOpenSharedLibraryError()
{
	return dlerror();
}

GLContext GLManager::makeContext(GLContextAttributes attr, GLBufferConfig config)
{
	return makeContext(attr, config, {});
}

void GLManager::resetCurrentContext() const
{
	display().resetCurrentContext();
}

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
	auto elapsed = (uint32_t)std::round(FloatSeconds(diff) / frameTime);
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

ThreadId thisThreadId()
{
	#ifdef __linux__
	return gettid();
	#else
	uint64_t id{};
	pthread_threadid_np(nullptr, &id);
	return id;
	#endif
}

WRect Viewport::relRect(WP pos, WP size, _2DOrigin posOrigin, _2DOrigin screenOrigin) const
{
	// adjust to the requested origin on the screen
	auto newX = LT2DO.adjustX(pos.x, width(), screenOrigin.invertYIfCartesian());
	auto newY = LT2DO.adjustY(pos.y, height(), screenOrigin.invertYIfCartesian());
	WRect rect;
	rect.setPosRel({newX, newY}, size, posOrigin);
	return rect;
}

WRect Viewport::relRectBestFit(WP pos, float aspectRatio, _2DOrigin posOrigin, _2DOrigin screenOrigin) const
{
	auto size = sizesWithRatioBestFit(aspectRatio, width(), height());
	return relRect(pos, size, posOrigin, screenOrigin);
}

}

#ifndef __ANDROID__
static void logBacktrace()
{
	void *arr[10];
	auto size = backtrace(arr, 10);
	char **backtraceStrings = backtrace_symbols(arr, size);
	for(auto i : IG::iotaCount(size))
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
