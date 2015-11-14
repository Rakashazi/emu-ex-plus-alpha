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

#include <imagine/util/typeTraits.hh>

namespace IG
{

// Method using Log Base 2 Approximation With One Extra Babylonian Steps
// http://ilab.usc.edu/wiki/index.php/Fast_Square_Root
static float sqrtFast(float x)
{
	union
	{
		int i;
		float x;
	} u;
	u.x = x;
	u.i = (1<<29) + (u.i >> 1) - (1<<22);

	// One Babylonian Step
	u.x = 0.5f * (u.x + x/u.x);

	return u.x;
}

template <typename T, ENABLE_IF_COND(std::is_unsigned<T>)>
static T sqrtFast(T remainder)
{
	T place = (T)1 << (sizeof(T) * 8 - 2); // calculated by precompiler = same runtime as: place = 0x40000000
	while (place > remainder)
		place /= 4; // optimized by complier as place >>= 2

	T root = 0;
	while (place)
	{
		if (remainder >= root+place)
		{
			remainder -= root+place;
			root += place * 2;
		}
		root /= 2;
		place /= 4;
	}
	return root;
}

}
