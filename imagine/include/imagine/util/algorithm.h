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

#ifdef __cplusplus
#include <imagine/util/iterator.hh>
#include <imagine/util/typeTraits.hh>
#include <algorithm>
#endif

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

namespace IG
{

template <class T>
static constexpr T const& cmax(const T& a, const T& b)
{
	return (b < a) ? a : b;
}

template <class T>
static constexpr T const& cmin(const T& a, const T& b)
{
	return (a < b) ? a : b;
}

template <class InputIterator1, class Size, class InputIterator2>
static bool equal_n(InputIterator1 first1, Size size, InputIterator2 first2)
{
	return std::equal(first1, first1 + size, first2);
}

template <class C, class V>
static void fillData(C &c, const V &val)
{
	std::fill_n(IG::data(c), IG::size(c), val);
}

template <class C>
static void fillData(C &c)
{
	fillData(c, typeof(*IG::data(c)){});
}

template <class C, class UnaryPredicate>
static auto findData_if(C &c, UnaryPredicate pred) -> decltype(IG::data(c))
{
	return std::find_if(IG::data(c), IG::data(c) + IG::size(c), pred);
}

template <class C, class V>
static bool contains(C c, const V& val)
{
	return std::find(c.begin(), c.end(), val) != c.end();
}

template <class T>
static void setLinked(T &var, T newVal, T &linkedVar)
{
	linkedVar += newVal - var;
	var = newVal;
}

}

#endif
