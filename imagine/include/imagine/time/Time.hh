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

#include <imagine/util/utility.h>
#include <chrono>
#if defined __APPLE__
#include <TargetConditionals.h>
#endif

namespace IG
{

using Nanoseconds = std::chrono::nanoseconds;
using Microseconds = std::chrono::microseconds;
using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;
using FloatSeconds = std::chrono::duration<double>;
using Minutes = std::chrono::minutes;

using SteadyClock = std::chrono::steady_clock;
using WallClock = std::chrono::system_clock;
using SteadyClockTimePoint = SteadyClock::time_point;
using WallClockTimePoint = WallClock::time_point;
using SteadyClockTime = SteadyClock::duration;
using WallClockTime = WallClock::duration;

template <class T>
concept ChronoDuration =
	requires
	{
		typename T::rep;
		typename T::period;
	};

template <class T>
concept ChronoTimePoint =
	requires
	{
		typename T::clock;
		typename T::duration;
	};

constexpr bool hasTime(ChronoTimePoint auto t) { return t.time_since_epoch().count(); }

inline SteadyClockTime timeFunc(auto &&func, auto &&...args)
{
	auto before = SteadyClock::now();
	func(IG_forward(args)...);
	return SteadyClock::now() - before;
}

inline SteadyClockTime timeFuncDebug(auto &&func, auto &&...args)
{
	#ifdef NDEBUG
	// execute directly without timing
	func(IG_forward(args)...);
	return {};
	#else
	return timeFunc(IG_forward(func), IG_forward(args)...);
	#endif
}

class FrameParams
{
public:
	constexpr FrameParams(SteadyClockTimePoint timestamp_, FloatSeconds frameTime_):
		timestamp_{timestamp_}, frameTime_{frameTime_} {}
	SteadyClockTimePoint timestamp() const { return timestamp_; }
	FloatSeconds frameTime() const { return frameTime_; }
	SteadyClockTimePoint presentTime(int frames) const;
	int elapsedFrames(SteadyClockTimePoint lastTimestamp) const;
	static int elapsedFrames(SteadyClockTimePoint timestamp, SteadyClockTimePoint lastTimestamp, FloatSeconds frameTime);

protected:
	SteadyClockTimePoint timestamp_;
	SteadyClockTimePoint lastTimestamp_;
	FloatSeconds frameTime_;
};

using FrameRate = float;

}
