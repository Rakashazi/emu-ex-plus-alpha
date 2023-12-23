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
#include <imagine/util/Point2D.hh>
#include <imagine/util/math.hh>

namespace IG
{

template <class T, class T2>
constexpr Point2D<T> sizesWithRatioBestFit(T2 destAspectRatio, T x, T y)
{
	auto sourceAspectRatio = (T2)x/(T2)y;
	T xSize = 0, ySize = 0;
	if(destAspectRatio == sourceAspectRatio)
	{
		xSize = x;
		ySize = y;
	}
	else if(destAspectRatio > sourceAspectRatio)
	{
		setSizesWithRatioX(xSize, ySize, destAspectRatio, x);
	}
	else
	{
		setSizesWithRatioY(xSize, ySize, destAspectRatio, y);
	}
	return {xSize, ySize};
}

template <class T, class T2>
constexpr Point2D<T> makeFromXWithRatio(T x, T2 r)
{
	return {x, x / r};
}

template <class T, class T2>
constexpr Point2D<T> makeFromYWithRatio(T y, T2 r)
{
	return {y * r, y};
}

template <class T>
constexpr Point2D<T> rotateAboutAxis(T rads, const Point2D<T> &v)
{
	auto temp = v;
	rotateAboutAxis(rads, temp.x, temp.y);
	return temp;
}

}
