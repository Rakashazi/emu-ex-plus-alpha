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

#include <concepts>
#include <numbers>
#include <cmath>

namespace IG
{

template <std::floating_point T>
constexpr auto radians(T degrees)
{
	return degrees * static_cast<T>(std::numbers::pi / 180.0);
}

template <std::floating_point T>
constexpr auto degrees(T radians)
{
	return radians * static_cast<T>(180.0 / std::numbers::pi);
}

constexpr auto pow2(auto base)
{
	return base * base;
}

// ceil/floor/round "n" to the nearest multiple "mult"
constexpr auto ceilMult(std::floating_point auto n, std::floating_point auto mult)
{
	return std::ceil(n / mult) * mult;
}

constexpr auto floorMult(std::floating_point auto n, std::floating_point auto mult)
{
	return std::floor(n / mult) * mult;
}

constexpr auto roundMult(std::floating_point auto n, std::floating_point auto mult)
{
	return std::round(n / mult) * mult;
}

constexpr bool isInRange(auto val, auto min, auto max)
{
	return val >= min && val <= max;
}

// treat zeros as positive
constexpr auto sign(auto num)
{
	return static_cast<decltype(num)>(num >= 0 ? 1 : -1);
}

constexpr auto wrapMax(auto x, auto max)
{
	if constexpr(std::is_floating_point_v<decltype(x)>)
	{
		return std::fmod(max + std::fmod(x, max), max);
	}
	else
	{
		return (max + (x % max)) % max;
	}
}

constexpr auto wrapMinMax(auto x, auto min, auto max)
{
	return min + wrapMax(x - min, max - min);
}

}
