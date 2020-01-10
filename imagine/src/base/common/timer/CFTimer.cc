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

#include <imagine/base/Timer.hh>
#include <imagine/logger/logger.h>
#include <limits>

namespace Base
{

#ifdef NDEBUG
CFTimer::CFTimer() {}
#else
CFTimer::CFTimer(const char *debugLabel): debugLabel{debugLabel ? debugLabel : "unnamed"} {}
#endif

void CFTimer::callbackInCFAbsoluteTime(CallbackDelegate callback, CFAbsoluteTime relTime,
	CFTimeInterval repeatInterval, CFRunLoopRef loop, bool shouldReuseResources)
{
	this->callback = callback;
	if(repeat != repeatInterval || (shouldReuseResources && !reuseResources))
	{
		// re-create timer if:
		// 1. repeat interval changed
		// 2. was previously allocated non-reusable, but now should be re-usable
		deinit();
	}
	reuseResources = shouldReuseResources;
	CFAbsoluteTime time = CFAbsoluteTimeGetCurrent() + relTime;
	if(!timer)
	{
		repeat = repeatInterval;
		CFRunLoopTimerContext context{0};
		context.info = this;
		if(shouldReuseResources && !repeat)
		{
			// set a massive repeat interval to reuse a one-shot timer
			repeatInterval = std::numeric_limits<CFTimeInterval>::max();
		}
		timer = CFRunLoopTimerCreate(nullptr, time, repeatInterval, 0, 0,
			[](CFRunLoopTimerRef timer, void *info)
			{
				using namespace Base;
				logMsg("running callback for timer: %p", timer);
				auto &timerData = *((Timer*)info);
				timerData.armed = timerData.repeat; // disarm timer if non-repeating, can be re-armed in callback()
				timerData.callback();
				if(!timerData.armed && !timerData.reuseResources)
					timerData.deinit();
			}, &context);
		logMsg("creating %stimer %p to run in %f second(s)", reuseResources ? "reusable " : "", timer, (double)relTime);
		if(repeat)
			logMsg("repeats every %f second(s)", repeat);
		if(!loop)
			loop = CFRunLoopGetCurrent();
		CFRunLoopAddTimer(loop, timer, kCFRunLoopDefaultMode);
	}
	else
	{
		logMsg("re-arming %stimer %p to run in %f second(s)", reuseResources ? "reusable " : "", timer, (double)relTime);
		if(repeat)
			logMsg("repeats every %f second(s)", repeat);
		CFRunLoopTimerSetNextFireDate(timer, time);
	}
	armed = true;
}

void Timer::callbackAfterNSec(CallbackDelegate callback, int ns, int repeatNs, EventLoop loop, Flags flags)
{
	callbackInCFAbsoluteTime(callback, ns / 1000000000., repeatNs / 1000000000., loop.nativeObject(), flags & HINT_REUSE);
}

void Timer::callbackAfterMSec(CallbackDelegate callback, int ms, int repeatMs, EventLoop loop, Flags flags)
{
	callbackInCFAbsoluteTime(callback, ms / 1000., repeatMs / 1000., loop.nativeObject(), flags & HINT_REUSE);
}

void Timer::callbackAfterSec(CallbackDelegate callback, int s, int repeatS, EventLoop loop, Flags flags)
{
	callbackInCFAbsoluteTime(callback, s, repeatS, loop.nativeObject(), flags & HINT_REUSE);
}

void Timer::callbackAfterNSec(CallbackDelegate callback, int ns, EventLoop loop)
{
	callbackAfterNSec(callback, ns, 0, loop, HINT_NONE);
}

void Timer::callbackAfterMSec(CallbackDelegate callback, int ms, EventLoop loop)
{
	callbackAfterMSec(callback, ms, 0, loop, HINT_NONE);
}

void Timer::callbackAfterSec(CallbackDelegate callback, int s, EventLoop loop)
{
	callbackAfterSec(callback, s, 0, loop, HINT_NONE);
}

void Timer::cancel()
{
	if(reuseResources)
	{
		if(armed)
		{
			// disarm timer
			assert(timer);
			logMsg("disarming timer: %p", timer);
			// set a massive fire time to "disarm" the timer
			CFRunLoopTimerSetNextFireDate(timer, std::numeric_limits<CFAbsoluteTime>::max());
			armed = false;
		}
	}
	else
		deinit();
}

void CFTimer::deinit()
{
	if(timer)
	{
		logMsg("closing timer: %p", timer);
		CFRunLoopTimerInvalidate(timer);
		CFRelease(timer);
		timer = nullptr;
		armed = false;
	}
}

void Timer::deinit()
{
	CFTimer::deinit();
}

const char *CFTimer::label()
{
	#ifdef NDEBUG
	return nullptr;
	#else
	return debugLabel;
	#endif
}

}
