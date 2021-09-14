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
#include <imagine/util/concepts.hh>
#include <cmath>

namespace IG
{

static auto roundUpPowOf2(unsigned_integral auto x)
{
	return 1 << fls(x - 1);
}

static auto roundDownPowOf2(unsigned_integral auto x)
{
	return 1 << (fls(x) - 1);
}

constexpr static auto pow(integral auto base, integral auto exp)
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

constexpr static bool isEven(integral auto x)
{
	return x % 2 == 0;
}

constexpr static bool isOdd(integral auto x)
{
	return !isEven(x);
}

constexpr static auto makeEvenRoundedUp(integral auto x)
{
	return isEven(x) ? x : x+1;
}

constexpr static auto makeEvenRoundedDown(integral auto x)
{
	return isEven(x) ? x : x-1;
}

static constexpr bool isPowerOf2(integral auto x)
{
	return x && !( (x-1) & x );
	// return ((x != 0) && ((x & (~x + 1)) == x)); // alternate method
}

static auto alignRoundedUp(unsigned_integral auto addr, unsigned int align)
{
	assumeExpr(isPowerOf2(align));
	return (addr+(align-1)) & ~(align-1);
}

// divide integer rounding-upwards
constexpr static auto divRoundUp(integral auto x, integral auto y)
{
	return (x + (y - 1)) / y;
}

// divide rounding to closest integer
constexpr static auto divRoundClosest(unsigned_integral auto x, unsigned_integral auto y)
{
	return (x > 0) ?
	 (x + (y / 2)) / y :
	 (x - (y / 2)) / y;
}

constexpr static auto divRoundClosest(floating_point auto x, floating_point auto y)
{
	return std::round(x / y);
}

}
