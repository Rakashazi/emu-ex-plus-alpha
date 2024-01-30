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
#include <imagine/util/enum.hh>
#include <chrono>
#include <concepts>

namespace IG
{

using std::chrono::duration_cast;
using std::chrono::round;

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

template<ChronoDuration T>
constexpr T fromSeconds(std::floating_point auto secs)
{
	return std::chrono::round<T>(FloatSeconds{secs});
}

constexpr double toHz(ChronoDuration auto t)
{
	return 1. / std::chrono::duration_cast<FloatSeconds>(t).count();
}

template<ChronoDuration T>
constexpr T fromHz(std::floating_point auto hz)
{
	return fromSeconds<T>(1. / hz);
}

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


WISE_ENUM_CLASS((FrameTimeSource, uint8_t),
	Unset,
	Renderer,
	Screen,
	Timer);

class FrameParams
{
public:
	SteadyClockTimePoint timestamp;
	SteadyClockTime frameTime;
	FrameTimeSource timeSource;

	SteadyClockTimePoint presentTime(int frames) const;
	int elapsedFrames(SteadyClockTimePoint lastTimestamp) const;
	static int elapsedFrames(SteadyClockTimePoint timestamp, SteadyClockTimePoint lastTimestamp, SteadyClockTime frameTime);
	bool isFromRenderer() const { return timeSource == FrameTimeSource::Renderer; }
};

using FrameRate = float;

}
