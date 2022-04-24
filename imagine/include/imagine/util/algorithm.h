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
#include <imagine/util/concepts.hh>
#include <algorithm>
#include <iterator>
#endif

// integer iteration
// 0 to [counts] - 1, i_s caches the value of counts (type of value is maintained)
#define iterateTimesTyped(counts, i) for(typeof (counts) i = 0, i ## _s = counts; i < (i ## _s); i++)
// 0 to [counts] - 1, i_s caches the value of counts
#define iterateTimes(counts, i) for(int i = 0, i ## _s = counts; i < (i ## _s); i++)
// [start] to ([start] + ([counts] - 1)), i_s caches the value of counts
#define iterateTimesFromStart(counts, start, i) for(int i = start, i ## _s = counts; i < start + (i ## _s); i++)
// [limit] - 1 to 0
#define iterateTimesRev(limit, i) for(int i = (limit)-1; i >= 0; i--)

#ifdef __cplusplus

namespace IG
{

template <class InputIterator1, class Size, class InputIterator2>
static bool equal_n(InputIterator1 first1, Size size, InputIterator2 first2)
{
	return std::equal(first1, first1 + size, first2);
}

constexpr static auto emptyIteratorValue(Iterable auto &&c)
{
	return std::remove_cvref_t<decltype(*std::begin(c))>{};
}

static void fill(Iterable auto &c, auto val)
{
	std::fill(std::begin(c), std::end(c), val);
}

static void fill(Iterable auto &c)
{
	fill(c, emptyIteratorValue(c));
}

static auto find_if(Iterable auto &&c, auto pred)
{
	return std::find_if(std::begin(c), std::end(c), pred);
}

static int findIndex(Iterable auto &&c, const auto &val, int notFound = -1)
{
	auto it = std::find(std::begin(c), std::end(c), val);
	if(it == std::end(c))
		return notFound;
	return std::distance(std::begin(c), it);
}

static int findIndexIf(Iterable auto &&c, auto pred, int notFound = -1)
{
	auto it = std::find_if(std::begin(c), std::end(c), pred);
	if(it == std::end(c))
		return notFound;
	return std::distance(std::begin(c), it);
}

static bool contains(Iterable auto &&c, const auto &val)
{
	return std::find(std::begin(c), std::end(c), val) != c.end();
}

static bool containsIf(Iterable auto &&c, auto pred)
{
	return std::find_if(std::begin(c), std::end(c), pred) != c.end();
}

static bool eraseFirst(Iterable auto &c, const auto &val)
{
	auto it = std::find(c.begin(), c.end(), val);
	if(it == c.end())
		return false;
	c.erase(it);
	return true;
}

static auto moveOutIf(Iterable auto &c, auto pred)
{
	if(auto it = std::find_if(c.begin(), c.end(), pred);
		it != c.end())
	{
		auto val = std::move(*it);
		c.erase(it);
		return val;
	}
	else
	{
		return emptyIteratorValue(c);
	}
}

template<typename InputIt, class Size, typename OutputIt, typename UnaryOperation>
OutputIt transformNOverlapped(InputIt first, Size count,
	OutputIt result, UnaryOperation unary_op)
{
	return std::transform(first, first + count, result, unary_op);
}

// wrapper functions for iterators to non-overlapping memory regions
// to improve compiler optimization opportunities
template<typename InputIt, typename OutputIt, typename UnaryOperation>
OutputIt transform(InputIt __restrict__ first, InputIt last,
	OutputIt __restrict__ result, UnaryOperation unary_op)
{
	return std::transform(first, last, result, unary_op);
}

template<typename InputIt, class Size, typename OutputIt, typename UnaryOperation>
OutputIt transformN(InputIt __restrict__ first, Size count,
	OutputIt __restrict__ result, UnaryOperation unary_op)
{
	return std::transform(first, first + count, result, unary_op);
}

template< class InputIt, class OutputIt>
OutputIt copy(InputIt __restrict__ first, InputIt last, OutputIt __restrict__ d_first)
{
	return std::copy(first, last, d_first);
}

template< class InputIt, class Size, class OutputIt>
OutputIt copy_n(InputIt __restrict__ first, Size count, OutputIt __restrict__ d_first)
{
	return std::copy_n(first, count, d_first);
}

}

#endif
