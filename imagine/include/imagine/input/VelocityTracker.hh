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
#include <imagine/time/Time.hh>
#include <imagine/util/typeTraits.hh>
#include <array>

namespace Input
{

template <class T, uint32_t D>
class IntegratingVelocityTracker
{
public:
	static_assert(std::is_floating_point<T>::value, "IntegratingVelocityTracker needs floating point type");
	using ValArray = std::array<T, D>;

	constexpr IntegratingVelocityTracker() {}
	constexpr IntegratingVelocityTracker(IG::Time time, std::array<T, D> pos): updateTime{time}, pos{pos} {}

	void update(IG::Time time, std::array<T, D> pos_)
	{
		const uint64 MIN_TIME_DELTA = 2000000; // in nanosecs
		const T FILTER_TIME_CONSTANT = .010; // in secs

		if(time.nSecs() <= updateTime.nSecs() + MIN_TIME_DELTA)
			return;
		T dt = (time - updateTime).nSecs() * (T)0.000000001;
		updateTime = time;
		ValArray vel_;
		iterateTimes(D, i)
		{
			vel_[i] = (pos_[i] - pos[i]) / dt;
		}
		if(degree == 0)
		{
			iterateTimes(D, i)
			{
				vel[i] = vel_[i];
			}
			degree = 1;
		}
		else
		{
			T alpha = dt / (FILTER_TIME_CONSTANT + dt);
			iterateTimes(D, i)
			{
				vel[i] += (vel_[i] - vel[i]) * alpha;
			}
		}
		iterateTimes(D, i)
		{
			pos[i] = pos_[i];
		}
	}

	T velocity(uint32_t idx) const { return vel[idx]; }

protected:
	IG::Time updateTime{};
	ValArray pos{};
	ValArray vel{};
	uint8 degree{};
};

template <class T, uint32_t D>
using VelocityTracker = IntegratingVelocityTracker<T, D>;

}
