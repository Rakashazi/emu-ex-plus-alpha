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

#include <imagine/util/concepts.hh>
#include <algorithm>
#include <iterator>

namespace IG
{

constexpr static auto emptyIteratorValue(Iterable auto &&c)
{
	return std::remove_cvref_t<decltype(*std::begin(c))>{};
}

constexpr void fill(Iterable auto &c)
{
	std::ranges::fill(c, emptyIteratorValue(c));
}

constexpr int findIndex(Iterable auto &&c, const auto &val, int notFound = -1)
{
	auto it = std::find(std::begin(c), std::end(c), val);
	if(it == std::end(c))
		return notFound;
	return std::distance(std::begin(c), it);
}

constexpr int findIndexIf(Iterable auto &&c, auto pred, int notFound = -1)
{
	auto it = std::find_if(std::begin(c), std::end(c), pred);
	if(it == std::end(c))
		return notFound;
	return std::distance(std::begin(c), it);
}

constexpr bool contains(Iterable auto &&c, const auto &val)
{
	return std::find(std::begin(c), std::end(c), val) != std::end(c);
}

constexpr bool containsIf(Iterable auto &&c, auto pred)
{
	return std::find_if(std::begin(c), std::end(c), pred) != std::end(c);
}

constexpr bool eraseFirst(Iterable auto &c, const auto &val)
{
	auto it = std::find(c.begin(), c.end(), val);
	if(it == c.end())
		return false;
	c.erase(it);
	return true;
}

constexpr auto moveOutIf(Iterable auto &c, auto pred)
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
constexpr OutputIt transformNOverlapped(InputIt first, Size count,
	OutputIt result, UnaryOperation unary_op)
{
	return std::transform(first, first + count, result, unary_op);
}

// wrapper functions for iterators to non-overlapping memory regions
// to improve compiler optimization opportunities
template<typename InputIt, typename OutputIt, typename UnaryOperation>
constexpr OutputIt transform(InputIt __restrict__ first, InputIt last,
	OutputIt __restrict__ result, UnaryOperation unary_op)
{
	return std::transform(first, last, result, unary_op);
}

template<typename InputIt, class Size, typename OutputIt, typename UnaryOperation>
constexpr OutputIt transformN(InputIt __restrict__ first, Size count,
	OutputIt __restrict__ result, UnaryOperation unary_op)
{
	return std::transform(first, first + count, result, unary_op);
}

template< class InputIt, class OutputIt>
constexpr OutputIt copy(InputIt __restrict__ first, InputIt last, OutputIt __restrict__ d_first)
{
	return std::copy(first, last, d_first);
}

template< class InputIt, class Size, class OutputIt>
constexpr OutputIt copy_n(InputIt __restrict__ first, Size count, OutputIt __restrict__ d_first)
{
	return std::copy_n(first, count, d_first);
}

}
