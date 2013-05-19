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
#include <util/ansiTypes.h>
#include <util/cLang.h>
#include <util/builtins.h>
#include <util/branch.h>
#include <util/bits.h>
#include <util/basicMath.hh>
#include <util/operators.hh>
#include <util/Rational.hh>
#include <config/imagineTypes.h>
#include <cmath>
#include <type_traits>

namespace IG
{

static const bool supportsFloat = !CONFIG_TYPES_NO_FLOAT;
static const bool supportsDouble = !CONFIG_TYPES_NO_DOUBLE;

template<class T>
static constexpr T toRadians(T degrees) { return degrees * (T)(M_PI / 180.0); }
template<class T>
static constexpr T toDegrees(T radians) { return radians * (T)(180.0 / M_PI); }

static void testFloatSupport() { if(!supportsFloat) bug_exit("float used without support"); }
static void testDoubleSupport() { if(!supportsDouble) bug_exit("double used without support"); }

template <class T, class T2>
static T pow(T base, T2 exp)
{
	int result = 1;
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
static double sqrt(double x) { testDoubleSupport(); return std::sqrt(x); }

// Method using Log Base 2 Approximation With One Extra Babylonian Steps
// http://ilab.usc.edu/wiki/index.php/Fast_Square_Root
static float sqrt(float x)
{
	testFloatSupport();
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

template <typename T>
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

//#ifndef _STLP_PLATFORM // stlport already has float sqrt

template<class T>
static bool isEven(T num)
{
	return num % 2 == 0 ? 1 : 0;
}
template<class T>
static bool isOdd(T num)
{
	return !isEven(num);
}

// divide integer rounding-upwards
template<class T>
constexpr static T divUp(T x, T y)
{
	return (x + y - 1) / y;
}

template <class T>
struct Point2D : public Arithmetics< Point2D<T> >
{
	T x = 0, y = 0;

	constexpr Point2D() { }
	constexpr Point2D(T x, T y) : x(x), y(y) { }

	Point2D<T> & operator +=(Point2D<T> const& summand)
	{
		x += summand.x;
		y += summand.y;
		return *this;
	}

	Point2D<T> & operator -=(Point2D<T> const& diminuend)
	{
		x -= diminuend.x;
		y -= diminuend.y;
		return *this;
	}

	Point2D<T> & operator *=(Point2D<T> const& factor)
	{
		x *= factor.x;
		y *= factor.y;
		return *this;
	}

	Point2D<T> & operator /=(Point2D<T> const& divisor)
	{
		x /= divisor.x;
		y /= divisor.y;
		return *this;
	}
};


// ceil/floor/round "n" to the nearest multiple "mult"
template<class T>
static T ceilMult(T n, T mult) { return ceil(n / mult) * mult; }
template<class T>
static T floorMult(T n, T mult) { return floor(n / mult) * mult; }
template<class T>
static T roundMult(T n, T mult) { return round(n / mult) * mult; }

template<class T>
static T midpoint(T a, T b) { return (a+b) / (T)2; }

template<class T>
static T distance1D(T x, T y)
{
	return absv(x - y);
}

template<class T>
static T distance2D(T x1, T y1, T x2, T y2)
{
	return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}

template<class T>
static T distance3D(T x1, T y1, T z1, T x2, T y2, T z2)
{
	return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2) + pow(z1 - z2, 2));
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
static T adjustSignTo(T num, T signToChangeTo)
{
	return signOf(num) == signOf(signToChangeTo) ? num : -num;
}

template <class T>
static T multBySignOf(T num, T multSign)
{
	return signOf(num) == signOf(multSign) ? adjustSignTo(num, (T)1) : adjustSignTo(num, (T)-1);
}

template <class T>
static T clipToLowBound(T val, T bound) { return val < bound ? bound : val; }

template <class T>
static T clipToHighBoundExclusive(T val, T bound) { return val >= bound ? (bound - 1) : val; }

template <class T>
static T clipToHighBound(T val, T bound) { return val > bound ? (bound) : val; }

template <class T>
static T clipToBounds(T val, T low, T high) { return clipToLowBound(clipToHighBound(val, high), low); }

template <class T>
static T clipToHalfOpenBounds(T val, T low, T high) { return clipToLowBound(clipToHighBoundExclusive(val, high), low); }

template <class T, class C>
static T clipToBoundsAndConfirm(T val, T low, T high, C *confirm)
{
	T newVal = clipToBounds(val, low, high);
	if(newVal != val)
		*confirm = 1;
	else
		*confirm = 0;
	return newVal;
}

template <class T, class C>
static T clipToHalfOpenBoundsAndConfirm(T val, T low, T high, C *confirm)
{
	T newVal = clipToHalfOpenBounds(val, low, high);
	if(newVal != val)
		*confirm = 1;
	else
		*confirm = 0;
	return newVal;
}

template <class T>
static T clipToLowZero(T val)
{
	return clipToLowBound(val, (T)0);
}

template <class T>
static T clipToZeroSigned(T val, T offset)
{
	T sign = signOf(val);
	val += offset;
	if(signOf(val) != sign)
	{
		val = 0;
	}
	return val;
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
		xSize = std::is_integral<T>::value ? roundf(res) : res;
	}
}

template <class T, class T2>
static void setSizesWithRatioX(T &xSize, T &ySize, T2 aspectRatio, T x)
{
	xSize = x;
	if(aspectRatio) // treat 0 AR as a no-op, ySize doesn't get modified
	{
		T2 res = (T2)x / aspectRatio;
		ySize = std::is_integral<T>::value ? roundf(res) : res;
	}
}

template <class T, class T2>
static void setSizesWithRatioBestFit(T &xSize, T &ySize, T2 destAspectRatio, T x, T y)
{
	Rational sourceRat {x,y};
	auto sourceAspectRatio = (T2)sourceRat;
	logMsg("ar %f %f, %d %d", sourceAspectRatio, destAspectRatio, x, y);
	if(destAspectRatio == sourceAspectRatio)
	{
		xSize = x;
		ySize = y;
	}
	else if(destAspectRatio > sourceAspectRatio)
	{
		IG::setSizesWithRatioX(xSize, ySize, destAspectRatio, x);
	}
	else
	{
		IG::setSizesWithRatioY(xSize, ySize, destAspectRatio, y);
	}
}

template <class T, class T2>
static IG::Point2D<T> sizesWithRatioBestFit(T2 destAspectRatio, T x, T y)
{
	//Rational sourceRat {x,y};
	auto sourceAspectRatio = (T2)x/(T2)y;
	T xSize = 0, ySize = 0;
	if(destAspectRatio == sourceAspectRatio)
	{
		xSize = x;
		ySize = y;
	}
	else if(destAspectRatio > sourceAspectRatio)
	{
		IG::setSizesWithRatioX(xSize, ySize, destAspectRatio, x);
	}
	else
	{
		IG::setSizesWithRatioY(xSize, ySize, destAspectRatio, y);
	}
	return {xSize, ySize};
}

template <class T>
static void setLinked(T &var, T newVal, T &linkedVar)
{
	linkedVar += newVal - var;
	var = newVal;
}
/*template <class T>
static void centerRange(T &valLow, T &valHigh, T low, T high)
{
	T origRangeSize = valHigh - valLow + 1;
	T newRangeSize = high - low + 1;
	T rangeOffset = (low - valLow) + (newRangeSize/2) - (origRangeSize/2);
	valLow += rangeOffset;
	valHigh += rangeOffset;
}

template <class T>
static int fitRangeMinToBoundAndConfirm(T &valLow, T &valHigh, T low)
{
	if(valLow < low)
	{
		valHigh += valLow - low;
		valLow = low;
		return 1;
	}
	return 0;
}

template <class T>
static int fitRangeMaxToBoundAndConfirm(T &valLow, T &valHigh, T high)
{
	if(valHigh > high)
	{
		valLow -= valHigh - high;
		valHigh = high;
		return 1;
	}
	return 0;
}

template <class T>
static int fitRangeMaxExclusiveToBoundAndConfirm(T &valLow, T &valHigh, T high)
{
	if(valHigh >= high)
	{
		valLow -= (valHigh - high) + 1;
		valHigh = high - 1;
		return 1;
	}
	return 0;
}

enum { BOUNDED_LOW = 1, BOUNDED_HIGH };
template <class T>
static int fitRangeToBoundsAndConfirm(T &valLow, T &valHigh, T low, T high)
{
	char lowConfirm = 0, highConfirm = 0;
	lowConfirm = fitRangeMinToBoundAndConfirm(valLow, valHigh, low);
	if(lowConfirm)
		return BOUNDED_LOW;
	else
	{
		highConfirm = fitRangeMaxToBoundAndConfirm(valLow, valHigh, high);
		if(highConfirm)
			return BOUNDED_HIGH;
		else
			return 0;
	}
}

template <class T>
static int fitRangeExclusiveToBoundsAndConfirm(T &valLow, T &valHigh, T low, T high)
{
	char _lowConfirm = 0, _highConfirm = 0;
	_lowConfirm = fitRangeMinToBoundAndConfirm(valLow, valHigh, low);
	if(_lowConfirm)
		return BOUNDED_LOW;
	else
	{
		_highConfirm = fitRangeMaxExclusiveToBoundAndConfirm(valLow, valHigh, high);
		if(_highConfirm)
			return BOUNDED_HIGH;
		else
			return 0;
	}
}*/

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
static void rotateAboutPoint(T rads, T *x, T *y, T ox, T oy)
{
	T oldX = *x, oldY = *y;
	*x = std::cos(rads) * (oldX-ox) - std::sin(rads) * (oldY-oy) + ox;
	*y = std::sin(rads) * (oldX-ox) + std::cos(rads) * (oldY-oy) + oy;
}

// to rotate about z axis, pass x,y
// to rotate about x axis, pass y,z
// to rotate about y axis, pass z,x
template <class T>
static void rotateAboutAxis(T rads, T *x, T *y)
{
	rotateAboutPoint(rads, x, y, (T)0, (T)0);
}

template <class T>
static T perspectiveFovViewSpaceHeight(T fovy)
{
	return (T)1/tan(fovy/(T)2);
}

static float perspectiveFovViewSpaceHeight(float fovy)
{
	return 1.f/tanf(fovy/2.f);
}

template <class T>
static T perspectiveFovViewSpaceWidth(T h, T aspect)
{
	return h / aspect;
}

}
