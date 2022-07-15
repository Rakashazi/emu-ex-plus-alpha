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

#include <imagine/util/bit.hh>
#include <imagine/util/utility.h>
#include <concepts>
#include <cmath>

namespace IG
{

constexpr auto roundUpPowOf2(std::unsigned_integral auto x)
{
	return 1 << fls(x - 1);
}

constexpr auto roundDownPowOf2(std::unsigned_integral auto x)
{
	return 1 << (fls(x) - 1);
}

constexpr auto pow(std::integral auto base, std::integral auto exp)
{
	decltype(base) result = 1;
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

constexpr bool isEven(std::integral auto x)
{
	return x % 2 == 0;
}

constexpr bool isOdd(std::integral auto x)
{
	return !isEven(x);
}

constexpr auto makeEvenRoundedUp(std::integral auto x)
{
	return isEven(x) ? x : x+1;
}

constexpr auto makeEvenRoundedDown(std::integral auto x)
{
	return isEven(x) ? x : x-1;
}

constexpr bool isPowerOf2(std::integral auto x)
{
	return x && !( (x-1) & x );
	// return ((x != 0) && ((x & (~x + 1)) == x)); // alternate method
}

constexpr auto alignRoundedUp(std::unsigned_integral auto addr, unsigned int align)
{
	assumeExpr(isPowerOf2(align));
	return (addr+(align-1)) & ~(align-1);
}

// divide integer rounding-upwards
constexpr auto divRoundUp(std::integral auto x, std::integral auto y)
{
	return (x + (y - 1)) / y;
}

// divide rounding to closest integer
constexpr auto divRoundClosest(std::unsigned_integral auto x, std::unsigned_integral auto y)
{
	return (x > 0) ?
	 (x + (y / 2)) / y :
	 (x - (y / 2)) / y;
}

constexpr auto divRoundClosest(std::floating_point auto x, std::floating_point auto y)
{
	return std::round(x / y);
}

}
