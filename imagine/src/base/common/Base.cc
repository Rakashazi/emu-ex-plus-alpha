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

std::string_view asString(Orientations o)
{
	switch(o)
	{
		case Orientations{}: return "Unset";
		case Orientations{.portrait = 1}: return "Portrait";
		case Orientations{.landscapeRight = 1}: return "Landscape Right";
		case Orientations{.portraitUpsideDown = 1}: return "Portrait Upside-Down";
		case Orientations{.landscapeLeft = 1}: return "Landscape Left";
		case Orientations::allLandscape(): return "Either Landscape";
		case Orientations::allPortrait(): return "Either Portrait";
		case Orientations::allButUpsideDown(): return "All But Upside-Down";
		case Orientations::all(): return "All";
	}
	return "Unknown";
}

SharedLibraryRef openSharedLibrary(const char *name, OpenSharedLibraryFlags flags)
{
	int mode = flags.resolveAllSymbols ? RTLD_NOW : RTLD_LAZY;
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

GLBufferConfig GLManager::makeBufferConfig(ApplicationContext ctx, const GLBufferRenderConfigAttributes& attrs) const
{
	return makeBufferConfig(ctx, std::span{&attrs, 1});
}

GLBufferConfig GLManager::makeBufferConfig(ApplicationContext ctx, std::span<const GLBufferRenderConfigAttributes> attrsSpan) const
{
	for(const auto &attrs : attrsSpan)
	{
		auto config = tryBufferConfig(ctx, attrs);
		if(config)
			return *config;
	}
	throw std::runtime_error("Error finding a GL configuration");
}

SteadyClockTimePoint FrameParams::presentTime(int frames) const
{
	if(frames <= 0)
		return {};
	return frameTime * frames + timestamp;
}

int FrameParams::elapsedFrames(SteadyClockTimePoint lastTimestamp) const
{
	return elapsedFrames(timestamp, lastTimestamp, frameTime);
}

int FrameParams::elapsedFrames(SteadyClockTimePoint timestamp, SteadyClockTimePoint lastTimestamp, SteadyClockTime frameTime)
{
	if(!hasTime(lastTimestamp))
		return 1;
	assumeExpr(timestamp >= lastTimestamp);
	assumeExpr(frameTime.count() > 0);
	auto diff = timestamp - lastTimestamp;
	auto elapsed = divRoundClosestPositive(diff.count(), frameTime.count());
	return std::max(elapsed, decltype(elapsed){1});
}

WRect Viewport::relRect(WPt pos, WSize size, _2DOrigin posOrigin, _2DOrigin screenOrigin) const
{
	// adjust to the requested origin on the screen
	auto newX = LT2DO.adjustX(pos.x, width(), screenOrigin.invertYIfCartesian());
	auto newY = LT2DO.adjustY(pos.y, height(), screenOrigin.invertYIfCartesian());
	WRect rect;
	rect.setPosRel({newX, newY}, size, posOrigin);
	return rect;
}

WRect Viewport::relRectBestFit(WPt pos, float aspectRatio, _2DOrigin posOrigin, _2DOrigin screenOrigin) const
{
	auto size = sizesWithRatioBestFit(aspectRatio, width(), height());
	return relRect(pos, size, posOrigin, screenOrigin);
}

}

#ifndef __ANDROID__
inline void logBacktrace()
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
