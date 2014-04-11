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

#include <imagine/util/number.h>

enum INTERPOLATOR_TYPE
{
	INTERPOLATOR_TYPE_LINEAR,
	INTERPOLATOR_TYPE_EASEINQUAD,
	INTERPOLATOR_TYPE_EASEOUTQUAD,
	INTERPOLATOR_TYPE_EASEINOUTQUAD,
	INTERPOLATOR_TYPE_EASEINCUBIC,
	INTERPOLATOR_TYPE_EASEOUTCUBIC,
	INTERPOLATOR_TYPE_EASEINOUTCUBIC,
	INTERPOLATOR_TYPE_EASEINQUART,
	INTERPOLATOR_TYPE_EASEINEXPO,
	INTERPOLATOR_TYPE_EASEOUTEXPO,
};

template <class T>
class Interpolator
{
protected:
	int startTime = 0;
	int destTime = 0;
	INTERPOLATOR_TYPE type = INTERPOLATOR_TYPE_LINEAR;
	T startVal {0};
	T destVal {0};
	T startDestValSize {0};

public:
	constexpr Interpolator() {}

	void set(T start, T dest, INTERPOLATOR_TYPE type, int duration)
	{
		startTime = 0;
		destTime = startTime + duration;
		this->type = type;
		startVal = start;
		destVal = dest;
		startDestValSize = destVal - startVal;
	}

	bool update(int currentTime, T &val) const
	{
		if(currentTime >= destTime)
		{
			val = destVal;
			return false;
		}
		else
		{
			float t = (float)(currentTime - startTime);
			float d = (float)(destTime - startTime);
			float b = startVal;
			float c = startDestValSize;
			val = getFormula(type, t, b, d, c);
			return true;
		}
	}

	static float getFormula(INTERPOLATOR_TYPE type, float t, float b, float d, float c)
	{
		float t1;
		switch(type)
		{
			case INTERPOLATOR_TYPE_LINEAR:
				// simple linear interpolation - no easing
				return (c * t / d + b);

			case INTERPOLATOR_TYPE_EASEINQUAD:
				// quadratic (t^2) easing in - accelerating from zero velocity
				t1 = t / d;
				return (c * t1 * t1 + b);

			case INTERPOLATOR_TYPE_EASEOUTQUAD:
				// quadratic (t^2) easing out - decelerating to zero velocity
				t1 = t / d;
				return (-c * t1 * (t1-2) + b);

			case INTERPOLATOR_TYPE_EASEINOUTQUAD:
				// quadratic easing in/out - acceleration until halfway, then deceleration
				t1 = t / (d / 2);
				if (t1 < 1)
					return ( c/2 * t1 * t1 + b);
				else
				{
					t1 = t1 -1;
					return (-c/2 * (t1 * (t1-2) - 1) + b);
				}
			case INTERPOLATOR_TYPE_EASEINCUBIC:
				// cubic easing in - accelerating from zero velocity
				t1 = t / d;
				return (c * t1 * t1 * t1 + b);

			case INTERPOLATOR_TYPE_EASEOUTCUBIC:
				// cubic easing in - accelerating from zero velocity
				t1 = t / d - 1;
				return (c * (t1 * t1 * t1 + 1) + b);

			case INTERPOLATOR_TYPE_EASEINOUTCUBIC:
				// cubic easing in - accelerating from zero velocity
				t1 = t / (d / 2);

				if ( t1 < 1)
					return (c/2 * t1 * t1 * t1 + b);
				else
				{
					t1 -= 2;
					return (c/2 * (t1 * t1 * t1 + 2 ) + b);
				}
			case INTERPOLATOR_TYPE_EASEINQUART:
				// quartic easing in - accelerating from zero velocity
				t1 = t / d;
				return (c * t1 * t1 * t1 * t1 + b);

			case INTERPOLATOR_TYPE_EASEINEXPO:
				// exponential (2^t) easing in - accelerating from zero velocity
				if (t==0)
					return b;
				else
					return (c*std::pow(2.f,(10*(t/d-1)))+b);

			case INTERPOLATOR_TYPE_EASEOUTEXPO:
				// exponential (2^t) easing out - decelerating to zero velocity
				if (t==d)
					return (b+c);
				else
					return (c * (-std::pow(2.f,-10*t/d)+1)+b);
			default:
				return 0;
		}
	}

	int duration() const
	{
		return destTime - startTime;
	}
};

template <class T>
class TimedInterpolator : public Interpolator<T>
{
public:
	int clock = 0;
	using Interpolator<T>::destTime;
	using Interpolator<T>::type;
	using Interpolator<T>::startVal;
	using Interpolator<T>::destVal;
	using Interpolator<T>::duration;

	constexpr TimedInterpolator() {}

  void set(T start, T dest, INTERPOLATOR_TYPE type, int duration)
  {
  	Interpolator<T>::set(start, dest, type, duration);
  	clock = 0;
  }

  void set(T dest)
  {
  	set(dest, dest, type, 0);
  }

  void setReversed()
  {
  	set(destVal, startVal, type, duration());
  }

  bool update(int ticksAdvanced, T &val)
  {
  	bool didUpdate = Interpolator<T>::update(clock + ticksAdvanced, val);
  	if(clock < destTime)
  		clock += ticksAdvanced;
  	//logMsg("did update: %d, clock: %d, val: %f", didUpdate, clock, (double)val);
  	return didUpdate;
  }

  bool update(int ticksAdvanced)
  {
  	T dummy;
  	bool didUpdate = update(ticksAdvanced, dummy);
  	return didUpdate;
  }

  T now() const
  {
  	T val;
  	Interpolator<T>::update(clock, val);
  	return val;
  }

  bool isComplete() const
  {
  	return clock >= destTime;
  }
};
