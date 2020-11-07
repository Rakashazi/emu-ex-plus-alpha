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

namespace IG
{

template <class T>
struct ReverseRange
{
private:
	T &o;

public:
	constexpr ReverseRange(T &o): o(o) {}
	auto begin() const -> decltype(this->o.rbegin()) { return o.rbegin(); }
	auto end() const -> decltype(this->o.rend()) { return o.rend(); }
};

template <class T>
struct ConstReverseRange
{
private:
	const T &o;

public:
	constexpr ConstReverseRange(const T &o): o(o) {}
	auto cbegin() const -> decltype(this->o.crbegin()) { return o.crbegin(); }
	auto cend() const -> decltype(this->o.crend()) { return o.crend(); }
};

template <class T>
static ReverseRange<T> makeReverseRange(T &o)
{
	return ReverseRange<T>(o);
}

template <class T>
static ConstReverseRange<T> makeReverseRange(const T &o)
{
	return ConstReverseRange<T>(o);
}

}
