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
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>
#include <limits>

namespace IG
{

constexpr SystemLogger log{"Timer"};

CFTimer::CFTimer(TimerDesc desc, CallbackDelegate del):
	debugLabel_{desc.debugLabel ? desc.debugLabel : "unnamed"},
	info{std::make_unique<CFTimerInfo>(del, CFRunLoopRef{}, desc.eventLoop.nativeObject() ?: EventLoop::forThread().nativeObject())} {}

CFTimer::CFTimer(CFTimer&& o) noexcept
{
	*this = std::move(o);
}

CFTimer& CFTimer::operator=(CFTimer&& o) noexcept
{
	deinit();
	timer = std::exchange(o.timer, {});
	info = std::move(o.info);
	return *this;
}

CFTimer::~CFTimer()
{
	deinit();
}

static double makeRelativeSecs(CFAbsoluteTime futureAbsTime)
{
	return futureAbsTime - CFAbsoluteTimeGetCurrent();
}

void CFTimer::callbackInCFAbsoluteTime(CFAbsoluteTime absTime, CFTimeInterval repeatInterval, CFRunLoopRef loop)
{
	auto realRepeatInterval = repeatInterval ? repeatInterval
		: std::numeric_limits<CFTimeInterval>::max(); // set a massive repeat interval to reuse a one-shot timer
	if(timer && CFRunLoopTimerGetInterval(timer) != realRepeatInterval)
	{
		// re-create timer if repeat interval changed
		deinit();
	}
	bool createdTimer = false;
	if(!timer) [[unlikely]]
	{
		CFRunLoopTimerContext context{};
		context.info = info.get();
		timer = CFRunLoopTimerCreate(nullptr, absTime, realRepeatInterval, 0, 0,
			[](CFRunLoopTimerRef timer, void *infoPtr)
			{
				log.info("running callback for timer:{}", (void*)timer);
				auto &info = *((CFTimerInfo*)infoPtr);
				bool keep = info.callback();
				if(!keep)
				{
					CFRunLoopRemoveTimer(info.loop, timer, kCFRunLoopDefaultMode);
					info.loop = {};
				}
			}, &context);
		createdTimer = true;
		log.info("created timer:{} ({})", (void*)timer, debugLabel());
	}
	else
	{
		CFRunLoopTimerSetNextFireDate(timer, absTime);
	}
	if(Config::DEBUG_BUILD)
	{
		log.info("{}timer:{} ({}) to run in:{}s repeats:{}s",
			createdTimer ? "created " : "",
			(void*)timer, debugLabel(), makeRelativeSecs(absTime), (double)repeatInterval);
	}
	if(loop != info->loop)
	{
		if(info->loop)
			CFRunLoopRemoveTimer(info->loop, timer, kCFRunLoopDefaultMode);
		CFRunLoopAddTimer(loop, timer, kCFRunLoopDefaultMode);
		info->loop = loop;
	}
}

void Timer::run(Time time, Time repeatTime, bool isAbsTime, CallbackDelegate callback)
{
	if(callback)
		setCallback(callback);
	CFAbsoluteTime absTime = duration_cast<FloatSeconds>(time).count();
	if(!isAbsTime)
		absTime += CFAbsoluteTimeGetCurrent();
	callbackInCFAbsoluteTime(absTime, repeatTime.count(), info->setLoop);
}

void Timer::cancel()
{
	if(!info->loop)
		return;
	CFRunLoopRemoveTimer(info->loop, timer, kCFRunLoopDefaultMode);
	info->loop = {};
}

void Timer::setCallback(CallbackDelegate callback)
{
	info->callback = callback;
}

void Timer::setEventLoop(EventLoop loop)
{
	cancel();
	info->setLoop = loop.nativeObject() ?: EventLoop::forThread().nativeObject();
}

void Timer::dispatchEarly()
{
	cancel();
	info->callback();
}

bool Timer::isArmed()
{
	return info->loop;
}

void CFTimer::deinit()
{
	if(!timer)
		return;
	log.info("closing timer:{}", (void*)timer);
	CFRunLoopTimerInvalidate(timer);
	CFRelease(timer);
	timer = {};
	info->loop = {};
}

}
