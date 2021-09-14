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

#include <imagine/fs/FSDefs.hh>
#include <imagine/util/concepts.hh>
#include <imagine/fmt/core.h>
#include <array>

namespace IG
{

template <class... T>
static auto formatTo(IG::Container auto &dest, fmt::format_string<T...> fmt, const T&... args)
{
	auto result = vformat_to_n(std::data(dest), std::size(dest) - 1, fmt, fmt::make_format_args(args...));
	*result.out = '\0'; // null terminate
	return result.size;
}

template <size_t S, class... T>
static auto formatToArray(fmt::format_string<T...> fmt, const T&... args)
{
	std::array<char, S> arr;
	formatTo(arr, fmt, args...);
	return arr;
}

template <class... T>
static FS::FileString formatToFileString(fmt::format_string<T...> fmt, const T&... args)
{
	return formatToArray<sizeof(FS::FileString)>(fmt, args...);
}

template <class... T>
static FS::PathString formatToPathString(fmt::format_string<T...> fmt, const T&... args)
{
	return formatToArray<sizeof(FS::PathString)>(fmt, args...);
}

}
