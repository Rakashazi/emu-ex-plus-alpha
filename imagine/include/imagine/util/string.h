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

#include <imagine/util/string/CStringView.hh>
#include <imagine/util/utility.h>
#include <imagine/util/ctype.hh>
#include <string_view>
#include <type_traits>
#include <algorithm>
#include <cctype>

namespace IG
{

[[nodiscard]]
constexpr bool stringContainsAny(std::string_view sv, auto &&...substrs)
{
	return (sv.contains(IG_forward(substrs)) || ...);
}

[[nodiscard]]
constexpr bool stringEndsWithAny(std::string_view sv, auto &&...endings)
{
	return (sv.ends_with(IG_forward(endings)) || ...);
}

template <class String>
[[nodiscard]]
constexpr auto stringToUpper(std::string_view str)
{
	String destStr{};
	destStr.reserve(str.size());
	for(auto c : str)
	{
		destStr.push_back(toupper(c));
	}
	return destStr;
}

template <class Return = void>
[[nodiscard]]
constexpr auto stringWithoutDotExtension(auto &&str)
{
	auto dotOffset = str.rfind('.');
	// If Return isn't specified, return result as argument type
	using R = std::conditional_t<std::is_same_v<Return, void>, std::decay_t<decltype(str)>, Return>;
	if(dotOffset != str.npos)
		return R{str.data(), dotOffset};
	else
		return R{IG_forward(str)};
}

constexpr bool stringNoCaseLexCompare(std::string_view s1, std::string_view s2)
{
	return std::lexicographical_compare(
		s1.begin(), s1.end(),
		s2.begin(), s2.end(),
		[](char c1, char c2)
		{
			return std::tolower(c1) < std::tolower(c2);
		});
}

}
