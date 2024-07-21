#pragma once

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

namespace IG
{

template<class Frequency>
class PausableTimer
{
public:
	PausableTimer(Frequency f, TimerDesc desc, CallbackDelegate del):timer{desc, del}, frequency{f} {}

	void start()
	{
		if(!frequency.count() || timer.isArmed())
			return;
		timer.run(nextFireTime(), frequency);
		startTime = SteadyClock::now();
	}

	void pause()
	{
		if(!startTime.time_since_epoch().count())
			return;
		elapsedTime += SteadyClock::now() - startTime;
		startTime = {};
		timer.cancel();
	}

	void cancel()
	{
		elapsedTime = {};
		startTime = {};
		timer.cancel();
	}

	void reset()
	{
		cancel();
		start();
	}

	void update()
	{
		if(!startTime.time_since_epoch().count())
			return;
		startTime = SteadyClock::now();
		elapsedTime = {};
	}

	SteadyClockTime nextFireTime() const
	{
		if(elapsedTime < frequency)
			return frequency - elapsedTime;
		return Nanoseconds{1};
	}

private:
	Timer timer;
	SteadyClockTimePoint startTime{};
	SteadyClockTime elapsedTime{};
public:
	Frequency frequency{};
};

}
