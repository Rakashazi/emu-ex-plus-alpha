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

#include <imagine/util/operators.hh>
#include <imagine/util/number.h>

namespace IG
{

template <class T>
struct Point2D : public NotEquals< Point2D<T> >, public Arithmetics< Point2D<T> >
{
	T x = 0, y = 0;

	constexpr Point2D() {}
	constexpr Point2D(T x, T y): x(x), y(y) {}

	bool operator ==(Point2D<T> const& rhs) const
	{
		return x == rhs.x && y == rhs.y;
	}

	Point2D<T> & operator +=(Point2D<T> const& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}

	Point2D<T> & operator -=(Point2D<T> const& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}

	Point2D<T> & operator *=(Point2D<T> const& rhs)
	{
		x *= rhs.x;
		y *= rhs.y;
		return *this;
	}

	Point2D<T> & operator /=(Point2D<T> const& rhs)
	{
		x /= rhs.x;
		y /= rhs.y;
		return *this;
	}

	template <class Ratio>
	constexpr Ratio ratio() const
	{
		return (Ratio)x/(Ratio)y;
	}

	T vectorLength()
	{
		return distance({(T)0, (T)0}, *this);
	}
};

template <class T, class T2>
static Point2D<T> sizesWithRatioBestFit(T2 destAspectRatio, T x, T y)
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
static constexpr Point2D<T> makeFromXWithRatio(T x, T2 r)
{
	return {x, x / r};
}

template <class T, class T2>
static constexpr Point2D<T> makeFromYWithRatio(T y, T2 r)
{
	return {y * r, y};
}

template<class T>
static T distance(const Point2D<T> &v1, const Point2D<T> &v2)
{
	return distance2D(v1.x, v1.y, v2.x, v2.y);
}

template <class T>
static Point2D<T> rotateAboutAxis(T rads, const Point2D<T> &v)
{
	auto temp = v;
	rotateAboutAxis(rads, temp.x, temp.y);
	return temp;
}

}
