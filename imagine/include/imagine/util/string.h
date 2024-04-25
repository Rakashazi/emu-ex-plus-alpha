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
#include <imagine/util/ctype.hh>
#include <imagine/util/algorithm.h>
#include <string_view>
#include <concepts>

namespace IG
{

using namespace std::string_view_literals;

[[nodiscard]]
constexpr bool containsAny(std::string_view s, auto &&...substrs)
{
	return (s.contains(std::string_view{IG_forward(substrs)}) || ...);
}

[[nodiscard]]
constexpr bool equalsToLower(char lhs, char rhs)
{
	return toLower<char>(lhs) == toLower<char>(rhs);
}

[[nodiscard]]
constexpr bool endsWithAnyCaseless(std::string_view s, std::convertible_to<std::string_view> auto &&...endings)
{
	return (ends_with(s, std::string_view{IG_forward(endings)}, equalsToLower) || ...);
}

[[nodiscard]]
constexpr bool equalsCaseless(std::string_view lhs, std::string_view rhs)
{
	return std::ranges::equal(lhs, rhs, equalsToLower);
}

template <class String = std::string>
[[nodiscard]]
constexpr auto toUpperCase(std::string_view s)
{
	String dest;
	dest.reserve(s.size());
	for(auto c : s)
	{
		dest.push_back(toUpper(c));
	}
	return dest;
}

[[nodiscard]]
constexpr std::string_view withoutDotExtension(std::string_view s)
{
	auto dotOffset = s.rfind('.');
	if(dotOffset != s.npos)
		return {s.data(), dotOffset};
	else
		return s;
}

[[nodiscard]]
constexpr std::string_view withoutDotExtension(std::convertible_to<std::string_view> auto &&s)
{
	return withoutDotExtension(std::string_view{IG_forward(s)});
}

[[nodiscard]]
constexpr std::string_view dotExtension(std::string_view s)
{
	auto dotOffset = s.rfind('.');
	if(dotOffset != s.npos)
	{
		s.remove_prefix(dotOffset + 1);
		return s;
	}
	return {};
}

[[nodiscard]]
constexpr std::string_view dotExtension(std::convertible_to<std::string_view> auto &&s)
{
	return dotExtension(std::string_view{IG_forward(s)});
}

[[nodiscard]]
constexpr bool caselessLexCompare(std::string_view s1, std::string_view s2)
{
	return std::ranges::lexicographical_compare(s1, s2, std::ranges::less{}, toLower<char>, toLower<char>);
}

}
