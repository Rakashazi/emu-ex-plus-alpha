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

#include <imagine/util/bits.h>
#include <imagine/util/utility.h>
#include <cmath>
#include <algorithm>
#include <type_traits>

namespace IG
{

template<class T>
static constexpr T radians(T degrees)
{
	static_assert(std::is_floating_point<T>::value, "expected floating point parameter");
	return degrees * (T)(M_PI / 180.0);
}

template<class T>
static constexpr T degrees(T radians)
{
	static_assert(std::is_floating_point<T>::value, "expected floating point parameter");
	return radians * (T)(180.0 / M_PI);
}

template <class T>
static constexpr T pow2(T base)
{
	return base * base;
}

// ceil/floor/round "n" to the nearest multiple "mult"
template<class T>
static constexpr T ceilMult(T n, T mult)
{
	return std::ceil(n / mult) * mult;
}

template<class T>
static constexpr T floorMult(T n, T mult)
{
	return std::floor(n / mult) * mult;
}

template<class T>
static constexpr T roundMult(T n, T mult)
{
	return std::round(n / mult) * mult;
}

template<class T>
static bool isInRange(T val, T min, T max)
{
	return val >= min && val <= max;
}

// treat zeros as positive
template <class T>
static constexpr T sign(T num)
{
	return num >= (T)0 ? (T)1 : (T)-1;
}

template <class T>
static constexpr T clamp(T val, T low, T high)
{
	return std::min(std::max(val, low), high);
}

template<class RET, class T>
constexpr static RET scaleDecToBits(T val, unsigned int bits)
{
	return (T)makeFullBits<RET>(bits) * val;
}

template <class T>
static constexpr T wrapMax(T x, T max)
{
	if constexpr(std::is_floating_point<T>::value)
	{
		return std::fmod(max + std::fmod(x, max), max);
	}
	else
	{
		return (max + (x % max)) % max;
	}
}

template <class T>
static constexpr T wrapMinMax(T x, T min, T max)
{
	return min + wrapMax(x - min, max - min);
}

}
