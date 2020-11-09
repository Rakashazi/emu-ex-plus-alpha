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
#include <imagine/util/math/math.hh>
#include <compare>

namespace IG
{

template <class T>
struct Point2D : public Arithmetics< Point2D<T> >
{
	T x = 0, y = 0;

	constexpr Point2D() {}
	constexpr Point2D(T x, T y): x(x), y(y) {}

	constexpr bool operator ==(Point2D<T> const& rhs) const = default;

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

	constexpr T vectorLength()
	{
		return distance({(T)0, (T)0});
	}

	constexpr T midpoint()
	{
		return (x + y) / (T)2;
	}

	constexpr T distance()
	{
		return std::abs(x - y);
	}

	constexpr T distance(Point2D<T> other)
	{
		return std::sqrt(pow2(x - other.x) + pow2(y - other.y));
	}
};

}
