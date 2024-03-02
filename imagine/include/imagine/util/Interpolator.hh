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

#include <imagine/util/used.hh>
#include <imagine/time/Time.hh>
#include <cmath>

namespace IG
{

enum class InterpolatorType : uint8_t
{
	UNSET,
	LINEAR,
	EASEINQUAD,
	EASEOUTQUAD,
	EASEINOUTQUAD,
	EASEINCUBIC,
	EASEOUTCUBIC,
	EASEINOUTCUBIC,
	EASEINQUART,
	EASEINEXPO,
	EASEOUTEXPO,
};

template <class T>
concept FloatScalable =
	requires(T &&v, float scale)
	{
		v * scale;
		v / scale;
	};

template <FloatScalable T, ChronoTimePoint TimePoint = SteadyClockTimePoint,
		InterpolatorType INTERPOLATOR_TYPE = InterpolatorType::UNSET>
class Interpolator
{
public:
	constexpr Interpolator() = default;

	constexpr Interpolator(T start, T end, InterpolatorType type, TimePoint startTime, TimePoint endTime):
		startTime_{startTime},
		endTime_{endTime},
		startVal{start},
		endVal{end},
		endValDiff{endVal - startVal},
		type{type} {}

	bool update(TimePoint currentTime, T &val) const
	{
		if(currentTime >= endTime_)
		{
			val = endVal;
			return false;
		}
		else
		{
			float t = (float)(currentTime - startTime_).count();
			float d = (float)(endTime_ - startTime_).count();
			auto b = startVal;
			auto c = endValDiff;
			val = getFormula(type, t, b, d, c);
			return true;
		}
	}

	static T getFormula(InterpolatorType type, float t, T b, float d, T c)
	{
		float t1;
		switch(type)
		{
			case InterpolatorType::LINEAR:
				// simple linear interpolation - no easing
				return (c * t / d + b);

			case InterpolatorType::EASEINQUAD:
				// quadratic (t^2) easing in - accelerating from zero velocity
				t1 = t / d;
				return (c * t1 * t1 + b);

			case InterpolatorType::EASEOUTQUAD:
				// quadratic (t^2) easing out - decelerating to zero velocity
				t1 = t / d;
				return (-c * t1 * (t1-2) + b);

			case InterpolatorType::EASEINOUTQUAD:
				// quadratic easing in/out - acceleration until halfway, then deceleration
				t1 = t / (d / 2);
				if (t1 < 1)
					return ( c/2 * t1 * t1 + b);
				else
				{
					t1 = t1 -1;
					return (-c/2 * (t1 * (t1-2) - 1) + b);
				}
			case InterpolatorType::EASEINCUBIC:
				// cubic easing in - accelerating from zero velocity
				t1 = t / d;
				return (c * t1 * t1 * t1 + b);

			case InterpolatorType::EASEOUTCUBIC:
				// cubic easing in - accelerating from zero velocity
				t1 = t / d - 1;
				return (c * (t1 * t1 * t1 + 1) + b);

			case InterpolatorType::EASEINOUTCUBIC:
				// cubic easing in - accelerating from zero velocity
				t1 = t / (d / 2);

				if ( t1 < 1)
					return (c/2 * t1 * t1 * t1 + b);
				else
				{
					t1 -= 2;
					return (c/2 * (t1 * t1 * t1 + 2 ) + b);
				}
			case InterpolatorType::EASEINQUART:
				// quartic easing in - accelerating from zero velocity
				t1 = t / d;
				return (c * t1 * t1 * t1 * t1 + b);

			case InterpolatorType::EASEINEXPO:
				// exponential (2^t) easing in - accelerating from zero velocity
				if (t==0)
					return b;
				else
					return (c*std::pow(2.f,(10*(t/d-1)))+b);

			case InterpolatorType::EASEOUTEXPO:
				// exponential (2^t) easing out - decelerating to zero velocity
				if (t==d)
					return (b+c);
				else
					return (c * (-std::pow(2.f,-10*t/d)+1)+b);
			default:
				return b;
		}
	}

	constexpr auto duration() const { return endTime_ - startTime_; }
	constexpr TimePoint startTime() const { return startTime_; }
	constexpr TimePoint endTime() const { return endTime_; }

protected:
	TimePoint startTime_{};
	TimePoint endTime_{};
	T startVal{};
	T endVal{};
	T endValDiff{};
	ConditionalMemberOr<INTERPOLATOR_TYPE == InterpolatorType::UNSET,
		InterpolatorType, INTERPOLATOR_TYPE> type{InterpolatorType::LINEAR};
};

template <class T, ChronoTimePoint TimePoint = SteadyClockTimePoint, InterpolatorType INTERPOLATOR_TYPE = InterpolatorType::UNSET>
class InterpolatorValue : public Interpolator<T, TimePoint, INTERPOLATOR_TYPE>
{
public:
	struct AbsoluteTimeInit{};
	using InterpolatorBase = Interpolator<T, TimePoint, INTERPOLATOR_TYPE>;
	T val{};

	constexpr InterpolatorValue() = default;

	constexpr InterpolatorValue(T start, T end, InterpolatorType type,
		TimePoint startTime, ChronoDuration auto duration):
		InterpolatorValue{start, end, type,
			startTime, startTime + duration,
			AbsoluteTimeInit{}} {}

	constexpr InterpolatorValue(T start, T end, InterpolatorType type,
		TimePoint startTime, TimePoint endTime,
		struct AbsoluteTimeInit):
		InterpolatorBase{start, end, type,
			startTime, endTime},
		val{start} {}

	constexpr InterpolatorValue(T end):
		InterpolatorValue{end, end, {}, {}, Seconds{}} {}

	InterpolatorValue reverse() const
	{
		return {endVal, startVal, type, startTime, endTime};
	}

	bool update(TimePoint time)
	{
		return InterpolatorBase::update(time, val);
	}

	operator T() const
	{
		return val;
	}

	bool isFinished() const
	{
		return val == endVal;
	}

	void finish()
	{
		val = endVal;
	}

	using InterpolatorBase::startTime;
	using InterpolatorBase::endTime;

protected:
	using InterpolatorBase::type;
	using InterpolatorBase::startVal;
	using InterpolatorBase::endVal;
	using InterpolatorBase::duration;
};

}
