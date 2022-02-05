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

namespace IG
{

[[nodiscard]]
constexpr bool stringContains(std::string_view sv, auto &&toFind)
{
	return sv.find(IG_forward(toFind)) != std::string_view::npos;
}

[[nodiscard]]
constexpr bool stringContainsAny(std::string_view sv, auto &&...substrs)
{
	return (stringContains(sv, IG_forward(substrs)) || ...);
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

bool stringNoCaseLexCompare(std::string_view s1, std::string_view s2);

}
