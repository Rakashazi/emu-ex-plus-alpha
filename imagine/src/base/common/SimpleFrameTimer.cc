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

#define LOGTAG "SimpleFrameTimer"
#include <imagine/base/SimpleFrameTimer.hh>
#include <imagine/base/Screen.hh>
#include <imagine/time/Time.hh>
#include <imagine/logger/logger.h>

namespace IG
{

SimpleFrameTimer::SimpleFrameTimer(Screen &screen, EventLoop loop):
	timer
	{
		"SimpleFrameTimer",
		[this, &screen]()
		{
			if(!requested)
			{
				if(keepTimer)
				{
					// wait one more tick due to simulated vsync inaccuracy
					keepTimer = false;
					return true;
				}
				else
				{
					return false;
				}
			}
			requested = false;
			auto timestamp = IG::steadyClockTimestamp();
			if(screen.frameUpdate(timestamp))
				scheduleVSync();
			return true;
		}
	},
	eventLoop{loop}
{
	assumeExpr(screen.frameTime().count());
}

void SimpleFrameTimer::scheduleVSync()
{
	if(requested)
	{
		return;
	}
	requested = true;
	keepTimer = true;
	if(timer.isArmed())
	{
		return;
	}
	assert(interval.count());
	timer.runIn(IG::Nanoseconds(1), interval, eventLoop);
}

void SimpleFrameTimer::cancel()
{
	requested = false;
	keepTimer = false;
}

void SimpleFrameTimer::setFrameRate(FrameRate rate)
{
	interval = std::chrono::duration_cast<Nanoseconds>(FloatSeconds{1. / rate});
	logMsg("set frame rate:%.2f (timer interval:%ldns)", rate, long(interval.count()));
	if(timer.isArmed())
	{
		timer.runIn(Nanoseconds{1}, interval, eventLoop);
	}
}

}
