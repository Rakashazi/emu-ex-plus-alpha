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

#include <imagine/base/SimpleFrameTimer.hh>
#include <imagine/base/Screen.hh>
#include <imagine/time/Time.hh>
#include <imagine/logger/logger.h>

namespace IG
{

constexpr SystemLogger log{"SimpleFrameTimer"};

SimpleFrameTimer::SimpleFrameTimer(Screen &screen, EventLoop loop):
	timer
	{
		{.debugLabel = "SimpleFrameTimer", .eventLoop = loop},
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
			if(screen.frameUpdate(SteadyClock::now()))
				scheduleVSync();
			return true;
		}
	},
	interval{fromHz<Nanoseconds>(screen.frameRate())} {}

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
	timer.runIn(Nanoseconds{1}, interval);
}

void SimpleFrameTimer::cancel()
{
	requested = false;
	keepTimer = false;
}

void SimpleFrameTimer::setFrameRate(FrameRate rate)
{
	interval = fromHz<Nanoseconds>(rate);
	log.info("set frame rate:{:g} (timer interval:{}ns)", rate, interval.count());
	if(timer.isArmed())
	{
		timer.runIn(Nanoseconds{1}, interval);
	}
}

void SimpleFrameTimer::setEventsOnThisThread(ApplicationContext)
{
	timer.setEventLoop({});
}

void NullFrameTimer::setEventsOnThisThread(ApplicationContext) {}

}
