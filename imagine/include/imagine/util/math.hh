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

#include <imagine/util/bit.hh>
#include <imagine/util/utility.h>
#include <concepts>
#include <numbers>
#include <cmath>
#include <bit>
#include <type_traits>

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

constexpr auto roundUpPowOf2(std::unsigned_integral auto x)
{
	return 1 << fls(x - 1);
}

constexpr auto roundDownPowOf2(std::unsigned_integral auto x)
{
	return 1 << (fls(x) - 1);
}

constexpr auto pow(std::integral auto base, std::integral auto exp)
{
	decltype(base) result = 1;
	while(exp)
	{
		if(exp & 1) // exp % 2 == 1
		{
			result *= base;
		}
		exp >>= 1; // exp /= 2
		base *= base;
	}
	return result;
}

constexpr bool isEven(std::integral auto x)
{
	return x % 2 == 0;
}

constexpr bool isOdd(std::integral auto x)
{
	return !isEven(x);
}

constexpr auto makeEvenRoundedUp(std::integral auto x)
{
	return isEven(x) ? x : x+1;
}

constexpr auto makeEvenRoundedDown(std::integral auto x)
{
	return isEven(x) ? x : x-1;
}

template<std::integral T>
constexpr bool isPowerOf2(T x)
{
	assumeExpr(x >= 0);
	return std::has_single_bit(std::make_unsigned_t<T>(x));
}

constexpr auto alignRoundedUp(std::unsigned_integral auto addr, unsigned int align)
{
	assumeExpr(isPowerOf2(align));
	return (addr+(align-1)) & ~(align-1);
}

// divide integer rounding-upwards
constexpr auto divRoundUp(std::integral auto x, std::integral auto y)
{
	return (x + (y - 1)) / y;
}

// divide rounding to closest integer
constexpr auto divRoundClosestPositive(std::integral auto x, std::integral auto y)
{
	return (x + (y / 2)) / y;
}

constexpr auto divRoundClosestNegative(std::integral auto x, std::integral auto y)
{
	return (x - (y / 2)) / y;
}

constexpr auto divRoundClosest(std::integral auto x, std::integral auto y)
{
	return (x >= 0) ? divRoundClosestPositive(x, y) : divRoundClosestNegative(x, y);
}

constexpr auto divRoundClosest(std::floating_point auto x, std::floating_point auto y)
{
	return std::round(x / y);
}

template<class T>
constexpr T distance3D(T x1, T y1, T z1, T x2, T y2, T z2)
{
	return std::sqrt(pow2(x1 - x2) + pow2(y1 - y2) + pow2(z1 - z2));
}

template <class T>
constexpr void adjust2DSizeToFit(T &xBound, T &yBound, T aR)
{
	T boundAR = xBound / yBound;
	T xSize = aR >= boundAR ? xBound : yBound * aR;
	T ySize = aR >= boundAR ? xBound / aR : yBound;
	xBound = xSize;
	yBound = ySize;
}

template <class T, class T2>
constexpr void setSizesWithRatioY(T &xSize, T &ySize, T2 aspectRatio, T y)
{
	ySize = y;
	if(aspectRatio) // treat 0 AR as a no-op, xSize doesn't get modified
	{
		T2 res = (T2)y * aspectRatio;
		xSize = std::is_integral_v<T> ? std::round(res) : res;
	}
}

template <class T, class T2>
constexpr void setSizesWithRatioX(T &xSize, T &ySize, T2 aspectRatio, T x)
{
	xSize = x;
	if(aspectRatio) // treat 0 AR as a no-op, ySize doesn't get modified
	{
		T2 res = (T2)x / aspectRatio;
		ySize = std::is_integral_v<T> ? std::round(res) : res;
	}
}

template <class T>
constexpr bool valIsWithinStretch(T val, T val2, T stretch)
{
	return val + stretch >= val2 && val - stretch <= val2;
}

template <class T>
constexpr void rotateAboutPoint(T rads, T &x, T &y, T ox, T oy)
{
	T oldX = x, oldY = y;
	x = std::cos(rads) * (oldX-ox) - std::sin(rads) * (oldY-oy) + ox;
	y = std::sin(rads) * (oldX-ox) + std::cos(rads) * (oldY-oy) + oy;
}

// to rotate about z axis, pass x,y
// to rotate about x axis, pass y,z
// to rotate about y axis, pass z,x
template <class T>
constexpr void rotateAboutAxis(T rads, T &x, T &y)
{
	rotateAboutPoint(rads, x, y, (T)0, (T)0);
}

}
