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

#include <imagine/base/baseDefs.hh>
#include <imagine/base/EventLoop.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/used.hh>
#include <time.h>
#include <memory>

namespace IG
{

class TimerFD
{
public:
	using TimePoint = SteadyClockTimePoint;

	TimerFD(TimerDesc, CallbackDelegate);
	const char* debugLabel() const { return fdSrc.debugLabel(); }

protected:
	std::unique_ptr<CallbackDelegate> callback_;
	FDEventSource fdSrc;

	bool arm(timespec ms, timespec repeatInterval, int flags);
};

using TimerImpl = TimerFD;

}
