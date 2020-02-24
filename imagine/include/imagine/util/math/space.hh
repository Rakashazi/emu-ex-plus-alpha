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

#include <cmath>

namespace IG
{

template<class T>
static T distance3D(T x1, T y1, T z1, T x2, T y2, T z2)
{
	return std::sqrt(pow2(x1 - x2) + pow2(y1 - y2) + pow2(z1 - z2));
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

template <class T>
static void adjust2DSizeToFit(T &xBound, T &yBound, T aR)
{
	T boundAR = xBound / yBound;
	T xSize = aR >= boundAR ? xBound : yBound * aR;
	T ySize = aR >= boundAR ? xBound / aR : yBound;
	xBound = xSize;
	yBound = ySize;
}

template <class T, class T2>
static void setSizesWithRatioY(T &xSize, T &ySize, T2 aspectRatio, T y)
{
	ySize = y;
	if(aspectRatio) // treat 0 AR as a no-op, xSize doesn't get modified
	{
		T2 res = (T2)y * aspectRatio;
		xSize = std::is_integral_v<T> ? std::round(res) : res;
	}
}

template <class T, class T2>
static void setSizesWithRatioX(T &xSize, T &ySize, T2 aspectRatio, T x)
{
	xSize = x;
	if(aspectRatio) // treat 0 AR as a no-op, ySize doesn't get modified
	{
		T2 res = (T2)x / aspectRatio;
		ySize = std::is_integral_v<T> ? std::round(res) : res;
	}
}

template <class T>
static bool valIsWithinStretch(T val, T val2, T stretch)
{
	return val + stretch >= val2 && val - stretch <= val2;
}

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

}
