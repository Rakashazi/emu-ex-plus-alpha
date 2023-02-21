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
#include <cmath>

namespace IG
{

template <class T>
concept Point = requires()
{
	T::x; T::y;
};

template <class T>
struct Point2D : public AssignmentArithmetics< Point2D<T> >
{
	using Value = T;

	T x, y;

	constexpr Point2D() = default;
	constexpr Point2D(T x, T y): x{x}, y{y} {}
	constexpr bool operator ==(const Point2D& rhs) const = default;
	constexpr Point2D operator +(const Point2D& rhs) const { return {T(x + rhs.x), T(y + rhs.y)}; }
	constexpr Point2D operator -(const Point2D& rhs) const { return {T(x - rhs.x), T(y - rhs.y)}; }
	constexpr Point2D operator *(const Point2D& rhs) const { return {T(x * rhs.x), T(y * rhs.y)}; }
	constexpr Point2D operator /(const Point2D& rhs) const { return {T(x / rhs.x), T(y / rhs.y)}; }
	constexpr Point2D operator-() const { return {T(-x), T(-y)}; }
	constexpr Point2D negateX() const { return {T(-x), T(y)}; }
	constexpr Point2D negateY() const { return {T(x), T(-y)}; }
	constexpr Point2D operator +(T const& rhs) const { return {T(x + rhs), T(y + rhs)}; }
	constexpr Point2D operator -(T const& rhs) const { return {T(x - rhs), T(y - rhs)}; }
	constexpr Point2D operator *(T const& rhs) const { return {T(x * rhs), T(y * rhs)}; }
	constexpr Point2D operator /(T const& rhs) const { return {T(x / rhs), T(y / rhs)}; }

	template <class Ratio>
	constexpr Ratio ratio() const { return Ratio(x)/Ratio(y); }

	constexpr T vectorLength() { return distance({(T)0, (T)0}); }
	constexpr T midpoint() { return std::midpoint(x, y); }
	constexpr T distance() { return std::abs(x - y); }

	constexpr T distance(Point2D other)
	{
		auto dx = x - other.x;
		auto dy = y - other.y;
		return std::sqrt(dx * dx + dy * dy);
	}

	template<class NewType>
	constexpr Point2D<NewType> as() const { return {NewType(x), NewType(y)}; }
};

using IP = Point2D<int>;
using SP = Point2D<int16_t>;
using FP = Point2D<float>;

}
