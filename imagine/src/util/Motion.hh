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

#include <util/number.h>

enum { MOTION_MOD_NONE = 0, };
enum { MOTION_MOD_AFFECT_START, MOTION_MOD_AFFECT_END, MOTION_MOD_AFFECT_START_END };
enum { TIMED_MOTION_COMPLETE = 0, TIMED_MOTION_INCOMPLETE };

template <class T>
class Motion
{
public:
	constexpr Motion() { }
	//Motion(T now = 0, T vel = 0, T accel = 0) : now(now), vel(vel), accel(accel) { }

	T now = 0;
	T vel = 0;
	T accel = 0;

	T update()
	{
		vel += accel;
		now += vel;
		//logMsg("now %f", now);
		return now;
	}

	T updateStopAccelOnZeroVel()
	{
		if(signOf(vel + accel) != signOf(vel) || vel + accel == 0)
		{
			accel = 0;
		}
		return update();
	}
};

template <class T>
class TimedMotion : public Motion<T>
{
public:
	constexpr TimedMotion() { }
	T start = 0;
	T end = 0;
	int duration = 0;
	int clock = 0;
	//uchar type;
	//uchar affectType;

	using Motion<T>::now;
	using Motion<T>::vel;
	using Motion<T>::accel;

	void moveLinear(T dest, uint time)
	{
		vel = (dest - now) / (T)(time);
		accel = 0;
		start = now;
		end = dest;
		duration = time;
		clock = 0;
	}

	void initLinear(T now, T dest, uint time)
	{
		assert(time != 0);
		this->now = now;
		vel = accel = 0;
		moveLinear(dest, time);
		//logMsg("init linear motion from %f to %f in %d ticks", start, end, duration);
	}

	void init(T now)
	{
		this->now = now;
		vel = accel = 0;
		start = 0;
		end = 0;
		duration = 1;
		clock = 1;
	}

	bool isComplete()
	{
		if(clock == duration)
			return 1;
		else return 0;
	}

	void complete()
	{
		now = end;
		clock = duration;
	}

	uint update()
	{
		if(clock == duration) return(TIMED_MOTION_COMPLETE);

		Motion<T>::update();
		clock++;

		if(clock == duration)
		{
			now = end;
			return(TIMED_MOTION_COMPLETE);
		}
		else return(TIMED_MOTION_INCOMPLETE);
	}

	void reverse()
	{
		vel = -vel;
		IG::swap(start, end);
		clock = duration - clock;
		//logMsg("reverse motion from %f to %f in %d ticks", start, end, duration);
	}

	bool advanceStep(uint steps)
	{
		iterateTimes(steps, i)
		{
			if(update() == TIMED_MOTION_COMPLETE)
				return TIMED_MOTION_COMPLETE;
		}
		return TIMED_MOTION_INCOMPLETE;
	}

};
