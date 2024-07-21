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
#include <imagine/util/used.hh>
#include <format>
#include <array>

namespace boost::static_strings
{
template<std::size_t N, typename CharT, typename Traits>
class basic_static_string;
}

namespace IG::FS
{
class PathString;
class FileString;
}

namespace IG
{

class CStringView;

template <class... T>
constexpr auto formatTo(ResizableContainer auto &c, std::format_string<T...> fmt, T&&... args)
{
	return std::vformat_to(std::back_inserter(c), fmt.get(), std::make_format_args(args...));
}

template <ResizableContainer Container, class... T>
constexpr auto format(std::format_string<T...> fmt, T&&... args)
{
	Container c;
	std::vformat_to(std::back_inserter(c), fmt.get(), std::make_format_args(args...));
	return c;
}

template <size_t S, class... T>
constexpr auto formatArray(std::format_string<T...> fmt, T&&... args)
{
	std::array<char, S> arr{};
	if(std::formatted_size(fmt, args...) > S - 1)
		return arr;
	std::vformat_to(arr.begin(), fmt.get(), std::make_format_args(args...));
	return arr;
}

}

template<>
struct std::formatter<IG::CStringView> : std::formatter<std::string_view> {};

template<std::size_t N, typename CharT, typename Traits>
struct std::formatter<boost::static_strings::basic_static_string<N, CharT, Traits>> : std::formatter<std::string_view> {};

template<>
struct std::formatter<IG::FS::PathString> : std::formatter<std::string_view> {};

template<>
struct std::formatter<IG::FS::FileString> : std::formatter<std::string_view> {};

template<class T, int Tag>
struct std::formatter<IG::UnusedType<T, Tag>>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	constexpr auto format(const IG::UnusedType<T, Tag>&, auto &ctx) const { return ctx.out(); }
};
