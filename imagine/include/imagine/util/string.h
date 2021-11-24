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
#include <imagine/util/concepts.hh>
#include <system_error>
#include <string>
#include <string_view>

namespace IG
{

[[gnu::nonnull]]
std::errc convertCharCode(const char** sourceStart, uint32_t &c);

[[gnu::nonnull]]
char *decodeUri(IG::CStringView input, char *output);

[[nodiscard]]
static constexpr bool stringContains(std::string_view sv, std::string_view viewToFind)
{
	return sv.find(viewToFind) != std::string_view::npos;
}

[[nodiscard]]
static constexpr bool stringContainsAny(std::string_view sv, auto &... substrs)
{
	return (stringContains(sv, substrs) || ...);
}

[[nodiscard]]
static constexpr bool stringEndsWithAny(std::string_view sv, auto &... endings)
{
	return (sv.ends_with(endings) || ...);
}

template <class String>
[[nodiscard]]
static auto stringToUpper(std::string_view str)
{
	String destStr{};
	destStr.reserve(str.size());
	for(auto c : str)
	{
		destStr.push_back(std::toupper(c));
	}
	return destStr;
}

template <class Return = void>
[[nodiscard]]
static constexpr auto stringWithoutDotExtension(auto &&str)
{
	auto dotOffset = str.rfind('.');
	// If Return isn't specified, return result as argument type
	using R = std::conditional_t<std::is_same_v<Return, void>, std::decay_t<decltype(str)>, Return>;
	if(dotOffset != str.npos)
		return R{str.data(), dotOffset};
	else
		return R{std::forward<decltype(str)>(str)};
}

}
