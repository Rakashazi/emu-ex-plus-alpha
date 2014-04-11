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

#include <imagine/util/utility.h>

// integer iteration
// 0 to [counts] - 1, i_s caches the value of counts (type of value is maintained)
#define iterateTimesTyped(counts, i) for(typeof (counts) i = 0, i ## _s = counts; i < (i ## _s); i++)
// 0 to [counts] - 1, i_s caches the value of counts
#define iterateTimes(counts, i) for(unsigned int i = 0, i ## _s = counts; i < (i ## _s); i++)
// [start] to ([start] + ([counts] - 1)), i_s caches the value of counts
#define iterateTimesFromStart(counts, start, i) for(int i = start, i ## _s = counts; i < start + (i ## _s); i++)
// [limit] - 1 to 0
#define iterateTimesRev(limit, i) for(int i = (limit)-1; i >= 0; i--)

#ifdef __cplusplus

// when added after a for-loop initializer, makes sure the loop runs only once
#define forLoopExecOnceDummy *DMY_VAR = NULL; DMY_VAR == NULL; DMY_VAR++

// iterates over elements of array [a] with the current element pointer label 'e'
// array number of elements can be accessed as 'e'_s
// array index can be accessed as 'e'_i
#define forEachInArray(a, e) \
for(typeof(*a) *e = a, forLoopExecOnceDummy) \
for(typeof(sizeofArray(a)) e ## _i = 0, e ## _s = sizeofArray(a); e ## _i < e ## _s; e ## _i++, e++ )

// iterates over elements of array [a] with the current element dereferenced label 'e'
// array number of elements can be accessed as 'e'_s
// array index can be accessed as 'e'_i, current element pointer label 'e'_p

#define forEachDInArray(a, e) \
for(size_t e ## _s  = sizeofArray(a), forLoopExecOnceDummy) \
iterateTimes(sizeofArray(a), e ## _i) \
for(typeof(*a) e = a[e ## _i], forLoopExecOnceDummy)

template <class T, size_t S>
static bool equalsAny(const T val, const T (&possible)[S])
{
	iterateTimes(S, i)
	{
		if(val == possible[i])
			return 1;
	}
	return 0;
}

template <class T, class S>
static bool equalsAny(const T val, const T possible[], S num)
{
	iterateTimes(num, i)
	{
		if(val == possible[i])
			return 1;
	}
	return 0;
}

#endif
