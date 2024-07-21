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

#include <imagine/config/defs.hh>
#include <imagine/base/baseDefs.hh>
#include <imagine/base/EventLoop.hh>
#include <imagine/util/concepts.hh>

#if defined __linux__
#include <imagine/base/timer/TimerFD.hh>
#elif defined __APPLE__
#include <imagine/base/timer/CFTimer.hh>
#endif

#include <chrono>

namespace IG
{

struct TimerDesc
{
	const char* debugLabel{};
	EventLoop eventLoop{};
};

struct Timer : public TimerImpl
{
public:
	using Time = TimePoint::duration;

	Timer(TimerDesc desc, CallbackDelegate del): TimerImpl{desc, del} {}
	void run(Time time, Time repeatTime, bool isAbsoluteTime = false, CallbackDelegate c = {});
	void cancel();
	void setCallback(CallbackDelegate);
	void setEventLoop(EventLoop);
	void dispatchEarly();
	bool isArmed();
	explicit operator bool() const;

	void runIn(ChronoDuration auto time,
		ChronoDuration auto repeatTime,
		CallbackDelegate f = {})
	{
		run(time, repeatTime, false, f);
	}

	void runAt(TimePoint time,
		ChronoDuration auto repeatTime,
		CallbackDelegate f = {})
	{
		run(time.time_since_epoch(), repeatTime, true, f);
	}

	// non-repeating timer
	void runIn(ChronoDuration auto time, CallbackDelegate f = {})
	{
		run(time, Time{}, false, f);
	}

	void runAt(TimePoint time, CallbackDelegate f = {})
	{
		run(time.time_since_epoch(), Time{}, true, f);
	}
};

}
