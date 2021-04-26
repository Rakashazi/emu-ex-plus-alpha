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

#include <imagine/util/AssignmentArithmetics.hh>
#include <numeric>
#include <compare>
#include <cmath>

namespace IG
{

template <class T>
struct Point2D : public AssignmentArithmetics< Point2D<T> >
{
	T x{}, y{};

	constexpr Point2D() {}
	constexpr Point2D(T x, T y): x{x}, y{y} {}

	constexpr bool operator ==(Point2D const& rhs) const = default;

	constexpr Point2D operator +(Point2D const& rhs) const
	{
		return {x + rhs.x, y + rhs.y};
	}

	constexpr Point2D operator -(Point2D const& rhs) const
	{
		return {x - rhs.x, y - rhs.y};
	}

	constexpr Point2D operator *(Point2D const& rhs) const
	{
		return {x * rhs.x, y * rhs.y};
	}

	constexpr Point2D operator /(Point2D const& rhs) const
	{
		return {x / rhs.x, y / rhs.y};
	}

	constexpr Point2D operator-() const
	{
		return {-x, -y};
	}

	constexpr Point2D operator +(T const& rhs) const
	{
		return {x + rhs, y + rhs};
	}

	constexpr Point2D operator -(T const& rhs) const
	{
		return {x - rhs, y - rhs};
	}

	constexpr Point2D operator *(T const& rhs) const
	{
		return {x * rhs, y * rhs};
	}

	constexpr Point2D operator /(T const& rhs) const
	{
		return {x / rhs, y / rhs};
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
		return std::midpoint(x, y);
	}

	constexpr T distance()
	{
		return std::abs(x - y);
	}

	constexpr T distance(Point2D other)
	{
		auto dx = x - other.x;
		auto dy = y - other.y;
		return std::sqrt(dx * dx + dy * dy);
	}
};

}
