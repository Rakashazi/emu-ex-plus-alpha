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

template <class T>
class Adds
{
public:
	constexpr Adds() {}
	friend T operator+(const T &v1, const T &v2)
	{
		T sum{v1};
		sum += v2;
		return sum;
	}

	constexpr bool operator ==(Adds<T> const& rhs) const = default;
};

template <class T>
class Subtracts
{
public:
	constexpr Subtracts() {}
	friend T operator-(const T &v1, const T &v2)
	{
		T diff{v1};
		diff -= v2;
		return diff;
	}

	constexpr bool operator ==(Subtracts<T> const& rhs) const = default;
};

template <class T>
class Multiplies
{
public:
	constexpr Multiplies() {}
	friend T operator*(const T &v1, const T &v2)
	{
		T prod{v1};
		prod *= v2;
		return prod;
	}

	constexpr bool operator ==(Multiplies<T> const& rhs) const = default;
};

template <class T>
class Divides
{
public:
	constexpr Divides() {}
	friend T operator/(const T &v1, const T &v2)
	{
		T quot{v1};
		quot /= v2;
		return quot;
	}

	constexpr bool operator ==(Divides<T> const& rhs) const = default;
};

template <class T>
class Arithmetics : public Adds<T>, public Subtracts<T>, public Multiplies<T>, public Divides<T>
{
public:
	constexpr bool operator ==(Arithmetics<T> const& rhs) const = default;
};

template <class T>
class PrimitiveOperators : public Arithmetics<T>
{
public:
	friend T &operator +=(T &lhs, const T &rhs)
	{
		auto &val = lhs.primitiveVal();
		val = val + rhs.primitiveVal();
		return lhs;
	}

	friend T &operator -=(T &lhs, const T &rhs)
	{
		auto &val = lhs.primitiveVal();
		val = val - rhs.primitiveVal();
		return lhs;
	}

	friend T &operator *=(T &lhs, const T &rhs)
	{
		auto &val = lhs.primitiveVal();
		val = val * rhs.primitiveVal();
		return lhs;
	}

	friend T &operator /=(T &lhs, const T &rhs)
	{
		auto &val = lhs.primitiveVal();
		val = val / rhs.primitiveVal();
		return lhs;
	}

	explicit operator bool() const
	{
		return static_cast<const T*>(this)->primitiveVal();
	}

	friend bool operator <(const T &lhs, const T &rhs)
	{
		return lhs.primitiveVal() < rhs.primitiveVal();
	}

	friend bool operator >(const T &lhs, const T &rhs)
	{
		return lhs.primitiveVal() > rhs.primitiveVal();
	}

	friend bool operator ==(const T &lhs, const T &rhs)
	{
		return lhs.primitiveVal() == rhs.primitiveVal();
	}

	constexpr bool operator ==(PrimitiveOperators<T> const& rhs) const = default;
};
