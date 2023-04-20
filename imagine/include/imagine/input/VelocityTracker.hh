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
#include <imagine/util/ranges.hh>
#include <concepts>
#include <chrono>
#include <array>

namespace IG::Input
{

template <std::floating_point T, size_t D>
class IntegratingVelocityTracker
{
public:
	using TimePoint = std::chrono::steady_clock::time_point;
	using ValArray = std::array<T, D>;

	constexpr IntegratingVelocityTracker() = default;
	constexpr IntegratingVelocityTracker(TimePoint time, std::array<T, D> pos): updateTime{time}, pos{pos} {}

	void update(TimePoint time, std::array<T, D> pos_)
	{
		using namespace std::chrono_literals;
		const auto MIN_TIME_DELTA = 2000000ns; // in nanosecs
		const T FILTER_TIME_CONSTANT = .010; // in secs

		if(time <= updateTime + MIN_TIME_DELTA)
			return;
		T dt = (time - updateTime).count() * (T)0.000000001;
		updateTime = time;
		ValArray vel_;
		for(auto i : iotaCount(D))
		{
			vel_[i] = (pos_[i] - pos[i]) / dt;
		}
		if(degree == 0)
		{
			for(auto i : iotaCount(D))
			{
				vel[i] = vel_[i];
			}
			degree = 1;
		}
		else
		{
			T alpha = dt / (FILTER_TIME_CONSTANT + dt);
			for(auto i : iotaCount(D))
			{
				vel[i] += (vel_[i] - vel[i]) * alpha;
			}
		}
		for(auto i : iotaCount(D))
		{
			pos[i] = pos_[i];
		}
	}

	T velocity(size_t idx) const { return vel[idx]; }

protected:
	TimePoint updateTime{};
	ValArray pos{};
	ValArray vel{};
	uint8_t degree{};
};

template <class T, size_t D>
using VelocityTracker = IntegratingVelocityTracker<T, D>;

}
