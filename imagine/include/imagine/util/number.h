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

#include <cstdlib>
#include <assert.h>
#include <imagine/util/ansiTypes.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/builtins.h>
#include <assert.h>
#include <imagine/util/bits.h>
#include <imagine/util/basicMath.hh>
#include <imagine/util/operators.hh>
#include <imagine/util/typeTraits.hh>
#include <imagine/config/imagineTypes.h>
#include <cmath>
#include <algorithm>

#if defined __ANDROID__ && !(defined __x86_64__ || defined __aarch64__)
// missing C99 math functions in Bionic prevent TR1 function definitions in cmath,
// copy needed ones from libstdc++
namespace std
{

constexpr float
round(float x)
{ return __builtin_roundf(x); }

constexpr long double
round(long double x)
{ return __builtin_roundl(x); }

constexpr double
round(double x)
{ return __builtin_round(x); }

template <typename T, ENABLE_IF_COND(std::is_integral<T>)>
constexpr T round(T x)
{ return __builtin_round(x); }

}
#endif

namespace IG
{

template<class T, ENABLE_IF_COND(std::is_floating_point<T>)>
static constexpr T toRadians(T degrees) { return degrees * (T)(M_PI / 180.0); }
template<class T, ENABLE_IF_COND(std::is_floating_point<T>)>
static constexpr T toDegrees(T radians) { return radians * (T)(180.0 / M_PI); }

template <typename T, ENABLE_IF_COND(std::is_integral<T>)>
static T pow(T base, T exp)
{
	T result = 1;
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

template <class T>
static constexpr T square(T base)
{
	return base * base;
}

static uint32 nextHighestPowerOf2(uint32 n)
{
	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n++;
	return n;
}

// sqrt
template <class T>
static constexpr T sqrt(T x) { return std::sqrt(x); }

#ifdef CONFIG_TYPES_SOFT_FLOAT
// Method using Log Base 2 Approximation With One Extra Babylonian Steps
// http://ilab.usc.edu/wiki/index.php/Fast_Square_Root
static float sqrt(float x)
{
	union
	{
		int i;
		float x;
	} u;
	u.x = x;
	u.i = (1<<29) + (u.i >> 1) - (1<<22);

	// One Babylonian Step
	u.x = 0.5f * (u.x + x/u.x);

	return u.x;
}

template <typename T, ENABLE_IF_COND(std::is_integral<T>)>
static T sqrt(T remainder)
{
	if(remainder < 0) // if type is unsigned this will be ignored = no runtime
		return 0; // negative number ERROR

	T place = (T)1 << (sizeof (T) * 8 - 2); // calculated by precompiler = same runtime as: place = 0x40000000
	while (place > remainder)
		place /= 4; // optimized by complier as place >>= 2

	T root = 0;
	while (place)
	{
		if (remainder >= root+place)
		{
			remainder -= root+place;
			root += place * 2;
		}
		root /= 2;
		place /= 4;
	}
	return root;
}
#endif

template<class T, ENABLE_IF_COND(std::is_integral<T>)>
constexpr static bool isEven(T num)
{
	return num % 2 == 0;
}

template<class T, ENABLE_IF_COND(std::is_integral<T>)>
constexpr static bool isOdd(T num)
{
	return !isEven(num);
}

template<class T, ENABLE_IF_COND(std::is_integral<T>)>
constexpr static T makeEvenRoundedUp(T num)
{
	return isEven(num) ? num : num+1;
}

template<class T, ENABLE_IF_COND(std::is_integral<T>)>
constexpr static T makeEvenRoundedDown(T num)
{
	return isEven(num) ? num : num-1;
}

// ceil/floor/round "n" to the nearest multiple "mult"
template<class T>
static T ceilMult(T n, T mult) { return std::ceil(n / mult) * mult; }
template<class T>
static T floorMult(T n, T mult) { return std::floor(n / mult) * mult; }
template<class T>
static T roundMult(T n, T mult) { return std::round(n / mult) * mult; }

template<class T>
static T midpoint(T a, T b) { return (a+b) / (T)2; }

template<class T>
static constexpr T distance1D(T x, T y)
{
	return std::abs(x - y);
}

template<class T>
static T distance2D(T x1, T y1, T x2, T y2)
{
	return sqrt(square(x1 - x2) + square(y1 - y2));
}

template<class T>
static T distance3D(T x1, T y1, T z1, T x2, T y2, T z2)
{
	return sqrt(square(x1 - x2) + square(y1 - y2) + square(z1 - z2));
}

/*static uint maxValOfBits(uint bits)
{
	return IG::pow(2, bits) - 1;
}*/

template<class T>
static T decWrapped(T val, const T &max)
{
	//assert(val >= 0 && val < max);
	if(val <= 0)
		return max-1;
	return --val;
}

template<class T>
static T incWrapped(T val, const T &max)
{
	if(++val >= max)
		return 0;
	return val;
}

template<class T>
static void incWrappedSelf(T &val, const T &max)
{
	val = incWrapped(val, max);
}

static bool isInRange(const int val, const int min, const int max)
{
	assert(min <= max);
	return uint(val - min) < uint(max - min);
}

static bool isInRange(const uint val, const uint min, const uint max)
{
	assert(min <= max);
	return (val - min) < max - min;
}

template<class T>
static T scalePointRange(T val, T origMin, T origMax, T newMin, T newMax)
{
	if(val == origMin) return newMin;
	if(val == origMax) return newMax;
	T origSize = origMax - origMin;
	T newSize = newMax - newMin;
	if(origSize == 0)
		return val;
	T scale = newSize / origSize;

	val -= origMin; // shift so that 0 == origMin
	val *= scale; // scale to new range size
	val += newMin; // shift into new range

	return val;
}

template<class T>
static T scalePointRange(T val, T origMax, T newMax)
{
	return scalePointRange(val, T(0), origMax, T(0), newMax);
}

template<class T>
static T normalizePoint(T val, T origMin, T origMax)
{
	return scalePointOnLine(val, origMin, origMax, (T)0, (T)1);
}


template<class RET, class T>
constexpr static RET scaleDecToBits(T val, uint bits)
{
	return (T)bit_fullMask<RET>(bits) * val;
}

// we treat zeros as positive
template <class T>
static T signOf(T num)
{
	return num >= (T)0 ? (T)1 : (T)-1;
}

template <class T>
static T clamp(T val, T low, T high)
{
	return std::min(std::max(val, low), high);
}

template <class T>
static T wrapToBounds(T val, T low, T high)
{
	return (val >= high) ? val - (high-low) : (val < low) ? val + (high-low) : val;
}

template <class T>
static void adjust2DSizeToFit(T &xBound, T &yBound, T aR)
{
	T boundAR = xBound / yBound;
	T xSize = aR >= boundAR ? xBound : yBound * aR;
	T ySize = aR >= boundAR ? xBound / aR : yBound;
	xBound = xSize;
	yBound = ySize;
}

template <class T>
static constexpr bool isPowerOf2(T x)
{
	return x && !( (x-1) & x );
	// return ((x != 0) && ((x & (~x + 1)) == x)); // alternate method
}

template <class T>
static T alignRoundedUp(T addr, uint align)
{
	assert(isPowerOf2(align));
	return (addr+(align-1)) & ~(align-1);
}

template <class T, class T2>
static void setSizesWithRatioY(T &xSize, T &ySize, T2 aspectRatio, T y)
{
	ySize = y;
	if(aspectRatio) // treat 0 AR as a no-op, xSize doesn't get modified
	{
		T2 res = (T2)y * aspectRatio;
		xSize = std::is_integral<T>::value ? std::round(res) : res;
	}
}

template <class T, class T2>
static void setSizesWithRatioX(T &xSize, T &ySize, T2 aspectRatio, T x)
{
	xSize = x;
	if(aspectRatio) // treat 0 AR as a no-op, ySize doesn't get modified
	{
		T2 res = (T2)x / aspectRatio;
		ySize = std::is_integral<T>::value ? std::round(res) : res;
	}
}

template <class T>
static void setLinked(T &var, T newVal, T &linkedVar)
{
	linkedVar += newVal - var;
	var = newVal;
}

template <class T>
static T wrapToBound(T val, T low, T high)
{
	return (val >= high) ? val - (high-low)
			: (val < low) ? val + (high-low)
			: val;
}

template <class T>
static bool valIsWithinStretch(T val, T val2, T stretch)
{
	return val + stretch >= val2 && val - stretch <= val2;
}

template <class T>
constexpr static T sin(T x)
{
	return std::sin(x);
}

template <class T>
constexpr static T sinD(T x) { return std::sin(toRadians(x)); }

template <class T>
constexpr static T cos(T x)
{
	return std::cos(x);
}

template <class T>
constexpr static T cosD(T x) { return std::cos(toRadians(x)); }

template <class T>
static void rotateAboutPoint(T rads, T &x, T &y, T ox, T oy)
{
	T oldX = x, oldY = y;
	x = std::cos(rads) * (oldX-ox) - std::sin(rads) * (oldY-oy) + ox;
	y = std::sin(rads) * (oldX-ox) + std::cos(rads) * (oldY-oy) + oy;
}

// to rotate about z axis, pass x,y
// to rotate about x axis, pass y,z
// to rotate about y axis, pass z,x
template <class T>
static void rotateAboutAxis(T rads, T &x, T &y)
{
	rotateAboutPoint(rads, x, y, (T)0, (T)0);
}

// divide integer rounding-upwards
template<class T, ENABLE_IF_COND(std::is_integral<T>)>
constexpr static T divRoundUp(T x, T y)
{
	return (x + (y - 1)) / y;
}



// divide rounding to closest integer
template<class T, ENABLE_IF_COND(std::is_unsigned<T>)>
constexpr static T divRoundClosest(T x, T y)
{
	return (x > 0) ?
		(x + (y / 2)) / y :
		(x - (y / 2)) / y;
}

template<class T, ENABLE_IF_COND(std::is_floating_point<T>)>
constexpr static T divRoundClosest(T x, T y)
{
	return std::round(x / y);
}

}
