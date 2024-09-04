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

#include <algorithm>
#include <iterator>
#include <ranges>
#include <optional>

namespace IG
{

template<std::ranges::range Range>
constexpr void fill(Range&& r)
{
	std::ranges::fill(r, std::ranges::range_value_t<Range>{});
}

template<std::ranges::range Range>
constexpr std::ranges::iterator_t<Range> findIt(Range&& r, auto&& compare)
{
	if constexpr(requires {std::ranges::find_if(r, compare);})
		return std::ranges::find_if(r, compare);
	else
		return std::ranges::find(r, compare);
}

template<std::ranges::range Range>
constexpr std::optional<std::ranges::iterator_t<Range>> find(Range&& r, auto&& compare)
{
	auto it = findIt(r, compare);
	if(it == std::ranges::end(r))
		return {};
	return it;
}

constexpr ptrdiff_t findIndex(std::ranges::range auto&& r, auto&& compare, int notFound = -1)
{
	auto optIt = find(r, compare);
	if(!optIt)
		return notFound;
	return std::ranges::distance(std::ranges::begin(r), optIt.value());
}

template<std::ranges::range Range>
constexpr std::ranges::range_value_t<Range>::pointer findPtr(Range&& r, auto&& compare)
{
	auto opt = find(r, compare);
	return opt ? opt.value()->get() : nullptr;
}

constexpr bool eraseFirst(std::ranges::range auto&& r, auto&& compare)
{
	auto optIt = find(r, compare);
	if(!optIt)
		return false;
	r.erase(optIt.value());
	return true;
}

template<std::ranges::range Range>
constexpr std::ranges::range_value_t<Range> moveOut(Range&& r, auto&& compare)
{
	auto optIt = find(r, compare);
	if(!optIt)
		return {};
	auto val = std::move(*optIt.value());
	r.erase(optIt.value());
	return val;
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

// std::ranges::ends_with implementation from
// https://en.cppreference.com/w/cpp/algorithm/ranges/ends_with
namespace ranges = std::ranges;
struct ends_with_fn {
  template<std::input_iterator I1, std::sentinel_for<I1> S1,
           std::input_iterator I2, std::sentinel_for<I2> S2,
           class Pred = ranges::equal_to,
           class Proj1 = std::identity, class Proj2 = std::identity>
    requires (std::forward_iterator<I1> || std::sized_sentinel_for<S1, I1>) &&
             (std::forward_iterator<I2> || std::sized_sentinel_for<S2, I2>) &&
             std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
  constexpr bool operator()(I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {},
                            Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    const auto n1 = ranges::distance(first1, last1);
    const auto n2 = ranges::distance(first2, last2);

    if (n1 < n2)
      return false;
    ranges::advance(first1, n1 - n2);
    return ranges::equal(std::move(first1), std::move(last1),
                         std::move(first2), std::move(last2),
                         std::move(pred), std::move(proj1), std::move(proj2));
  }

  template<ranges::input_range R1, ranges::input_range R2,
           class Pred = ranges::equal_to,
           class Proj1 = std::identity, class Proj2 = std::identity>
    requires (ranges::forward_range<R1> || ranges::sized_range<R1>) &&
             (ranges::forward_range<R2> || ranges::sized_range<R2>) &&
             std::indirectly_comparable<ranges::iterator_t<R1>,
                                        ranges::iterator_t<R2>,
                                        Pred, Proj1, Proj2>
  constexpr bool operator()(R1&& r1, R2&& r2, Pred pred = {},
                            Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    return (*this)(ranges::begin(r1), ranges::end(r1),
                   ranges::begin(r2), ranges::end(r2),
                   std::move(pred), std::move(proj1), std::move(proj2));
  }
};

inline constexpr ends_with_fn ends_with{};

constexpr auto remap(auto val, auto origMin, auto origMax, auto newMin, auto newMax)
{
	auto origSize = origMax - origMin;
	auto newSize = newMax - newMin;
	return newMin + (val - origMin) * newSize / origSize;
}

constexpr auto remapClamp(auto val, auto origMin, auto origMax, auto newMin, auto newMax)
{
	auto mappedVal = remap(val, origMin, origMax, newMin, newMax);
	using MappedVal = decltype(mappedVal);
	return std::clamp(mappedVal, static_cast<MappedVal>(newMin), static_cast<MappedVal>(newMax));
}

template <class Limit>
constexpr auto remap(auto val, auto origMin, auto origMax, std::numeric_limits<Limit> limit)
{
	return remap(val, origMin, origMax, limit.min(), limit.max());
}

template <class Limit>
constexpr auto remapClamp(auto val, auto origMin, auto origMax, std::numeric_limits<Limit> limit)
{
	return remapClamp(val, origMin, origMax, limit.min(), limit.max());
}

}
