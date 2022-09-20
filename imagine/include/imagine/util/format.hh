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
#include <imagine/fmt/core.h>
#include <array>

namespace IG
{

template <class... T>
constexpr auto formatTo(IG::Container auto &c, fmt::format_string<T...> fmt, const T&... args)
{
	if constexpr(requires {c.push_back('0');})
	{
		return fmt::vformat_to(std::back_inserter(c), fmt, fmt::make_format_args(args...));
	}
	else
	{
		// static array case
		return fmt::vformat_to_n(std::data(c), std::size(c) - 1, fmt, fmt::make_format_args(args...));
	}
}

template <IG::Container Container, class... T>
constexpr auto format(fmt::format_string<T...> fmt, const T&... args)
{
	Container c{};
	formatTo(c, fmt, args...);
	return c;
}

template <size_t S, class... T>
constexpr auto formatArray(fmt::format_string<T...> fmt, const T&... args)
{
	std::array<char, S> arr{};
	formatTo(arr, fmt, args...);
	return arr;
}

}
