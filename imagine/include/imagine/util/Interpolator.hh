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

#include <imagine/util/typeTraits.hh>
#include <chrono>
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

template <class T, class Time = std::chrono::nanoseconds, InterpolatorType INTERPOLATOR_TYPE = InterpolatorType::UNSET>
class Interpolator
{
public:
	constexpr Interpolator() {}

	constexpr Interpolator(T start, T dest, InterpolatorType type, Time startTime, Time destTime):
		startTime{startTime},
		destTime{destTime},
		startVal{start},
		destVal{dest},
		startDestValSize{destVal - startVal},
		type{type}
	{}

	bool update(Time currentTime, T &val) const
	{
		if(currentTime >= destTime)
		{
			val = destVal;
			return false;
		}
		else
		{
			float t = (float)(currentTime - startTime).count();
			float d = (float)(destTime - startTime).count();
			float b = startVal;
			float c = startDestValSize;
			val = getFormula(type, t, b, d, c);
			return true;
		}
	}

	static float getFormula(InterpolatorType type, float t, float b, float d, float c)
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

	Time duration() const
	{
		return destTime - startTime;
	}

protected:
	Time startTime{};
	Time destTime{};
	T startVal{};
	T destVal{};
	T startDestValSize{};
	IG_enableMemberIfOrConstant(INTERPOLATOR_TYPE == InterpolatorType::UNSET,
		InterpolatorType, INTERPOLATOR_TYPE, type){InterpolatorType::LINEAR};
};

template <class T, class Time = std::chrono::nanoseconds, InterpolatorType INTERPOLATOR_TYPE = InterpolatorType::UNSET>
class InterpolatorValue : public Interpolator<T, Time, INTERPOLATOR_TYPE>
{
public:
	struct AbsoluteTimeInit{};

	constexpr InterpolatorValue() {}

	template<class Time1, class Time2>
	constexpr InterpolatorValue(T start, T dest, InterpolatorType type, Time1 startTime, Time2 duration):
		InterpolatorValue{start, dest, type, startTime, startTime + duration, AbsoluteTimeInit{}}
	{}

	template<class Time1, class Time2>
	constexpr InterpolatorValue(T start, T dest, InterpolatorType type, Time1 startTime, Time2 destTime, struct AbsoluteTimeInit):
		Interpolator<T, Time, INTERPOLATOR_TYPE>{start, dest, type,
			std::chrono::duration_cast<Time>(startTime),
			std::chrono::duration_cast<Time>(destTime)},
		val{start}
	{}

	constexpr InterpolatorValue(T dest):
		InterpolatorValue{dest, dest, {}, Time{}, Time{}}
	{}

	InterpolatorValue reverse() const
	{
		return {destVal, startVal, type, startTime, destTime};
	}

	bool update(Time time)
	{
		return Interpolator<T, Time, INTERPOLATOR_TYPE>::update(time, val);
	}

	operator T() const
	{
		return val;
	}

	bool isFinished() const
	{
		return val == destVal;
	}

	void finish()
	{
		val = destVal;
	}

protected:
	T val{};
	using Interpolator<T, Time, INTERPOLATOR_TYPE>::startTime;
	using Interpolator<T, Time, INTERPOLATOR_TYPE>::destTime;
	using Interpolator<T, Time, INTERPOLATOR_TYPE>::type;
	using Interpolator<T, Time, INTERPOLATOR_TYPE>::startVal;
	using Interpolator<T, Time, INTERPOLATOR_TYPE>::destVal;
	using Interpolator<T, Time, INTERPOLATOR_TYPE>::duration;
};

}
