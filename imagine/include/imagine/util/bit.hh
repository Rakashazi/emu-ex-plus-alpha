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

#include <concepts>
#include <bit>

namespace IG
{

constexpr int ctz(unsigned int x)
{
	return __builtin_ctz(x);
}

constexpr int ctz(unsigned long x)
{
	return __builtin_ctzl(x);
}

constexpr int ctz(unsigned long long x)
{
	return __builtin_ctzll(x);
}

constexpr int clz(unsigned int x)
{
	return __builtin_clz(x);
}

constexpr int clz(unsigned long x)
{
	return __builtin_clzl(x);
}

constexpr int clz(unsigned long long x)
{
	return __builtin_clzll(x);
}

constexpr int fls(std::unsigned_integral auto x)
{
	return x ? sizeof(x) * 8 - clz(x) : 0;
}

}
