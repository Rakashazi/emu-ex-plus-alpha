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

#define LOGTAG "FrameTimer"
#include <imagine/base/Screen.hh>
#include <imagine/time/Time.hh>
#include <imagine/input/Input.hh>
#include <imagine/logger/logger.h>
#include "SimpleFrameTimer.hh"

namespace Base
{

FrameTimer::~FrameTimer() {}

SimpleFrameTimer::SimpleFrameTimer(EventLoop loop)
{
	eventLoop = loop;
	constexpr double NSEC_PER_SEC = 1000000000.;
	assumeExpr(Screen::screen(0)->frameTime());
	intervalNS = std::round(Screen::screen(0)->frameTime() * NSEC_PER_SEC);
}

SimpleFrameTimer::~SimpleFrameTimer()
{
	timer.deinit();
}

void SimpleFrameTimer::scheduleVSync()
{
	cancelled = false;
	if(requested)
	{
		return;
	}
	requested = true;
	if(timer)
	{
		return; // timer already armed
	}
	timer.callbackAfterNSec(
		[this]()
		{
			auto timestamp = IG::steadyClockTimestamp();
			requested = false;
			if(cancelled)
			{
				cancelled = false;
				timer.cancel();
				return; // frame request was cancelled
			}
			Input::flushEvents();
			auto s = Screen::screen(0);
			if(s->isPosted())
			{
				s->frameUpdate(timestamp);
				s->prevFrameTimestamp = timestamp;
			}
			if(!requested)
			{
				cancel();
			}
		}, 1, intervalNS, eventLoop, Timer::HINT_REUSE);
}

void SimpleFrameTimer::cancel()
{
	cancelled = true;
}

}
