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

#define LOGTAG "CustomEvent"
#include <imagine/base/CustomEvent.hh>
#include <imagine/logger/logger.h>

namespace Base
{

void CustomEvent::setEventLoop(EventLoop loop_)
{
	if(!timer)
	{
		CFRunLoopTimerContext context{0};
		context.info = this;
		auto maxTime = std::numeric_limits<CFTimeInterval>::max();
		timer = CFRunLoopTimerCreate(nullptr, maxTime, maxTime, 0, 0,
			[](CFRunLoopTimerRef timer, void *info)
			{
				auto &eventData = *((CustomEvent*)info);
				if(eventData.cancelled)
					return;
				//logDMsg("running callback for custom event:%p", info);
				eventData.callback.callSafe();
			}, &context);
	}
	if(!loop_)
		loop_ = EventLoop::forThread();
	if(loop)
	{
		CFRunLoopRemoveTimer(loop, timer, kCFRunLoopDefaultMode);
	}
	loop = loop_.nativeObject();
	CFRunLoopAddTimer(loop, timer, kCFRunLoopDefaultMode);
}

void CustomEvent::setCallback(CustomEventDelegate c)
{
	callback = c;
}

void CustomEvent::notify()
{
	cancelled = false;
	CFRunLoopTimerSetNextFireDate(timer, CFAbsoluteTimeGetCurrent());
}

void CustomEvent::cancel()
{
	cancelled = true;
}

void CustomEvent::deinit()
{
	cancelled = true;
	if(timer)
	{
		logMsg("closing custom event timer: %p", timer);
		CFRunLoopTimerInvalidate(timer);
		CFRelease(timer);
		timer = nullptr;
	}
}

}
