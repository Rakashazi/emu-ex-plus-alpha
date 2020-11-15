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

SimpleFrameTimer::SimpleFrameTimer(EventLoop loop, Screen &screen):
	timer
	{
		"SimpleFrameTimer",
		[this, &screen]()
		{
			if(!requested)
			{
				return false;
			}
			requested = false;
			Input::flushEvents();
			auto timestamp = IG::steadyClockTimestamp();
			if(screen.isPosted())
			{
				screen.frameUpdate(timestamp);
				screen.prevFrameTimestamp = timestamp;
			}
			return true;
		}
	},
	eventLoop{loop}
{
	assumeExpr(Screen::screen(0)->frameTime().count());
	interval = std::chrono::duration_cast<IG::Nanoseconds>(Screen::screen(0)->frameTime());
}

SimpleFrameTimer::~SimpleFrameTimer() {}

void SimpleFrameTimer::scheduleVSync()
{
	if(requested)
	{
		return;
	}
	requested = true;
	timer.runOnce(IG::Nanoseconds(1), interval);
}

void SimpleFrameTimer::cancel()
{
	requested = false;
}

}
