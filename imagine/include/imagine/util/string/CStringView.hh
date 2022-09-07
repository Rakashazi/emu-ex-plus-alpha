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

#include <string>
#include <string_view>
#include <imagine/util/concepts.hh>
#include <imagine/util/utility.h>

namespace IG
{

// Simple const char* wrapper used as a function argument to accept
// non-null C strings or containers of char data
class CStringView
{
public:
	[[gnu::nonnull]]
	constexpr CStringView(const char *str):
		str{str} {}

	constexpr CStringView(const Container auto &c):
		str{std::data(c)} {}

	constexpr const char *data() const { return str; }
	constexpr size_t size() const { return std::char_traits<char>::length(str); }
	constexpr bool empty() const { return !size(); }
	constexpr operator const char *() const { return str; }
	constexpr operator std::string_view() const { return str; }
	constexpr bool contains(auto &&s) const { return static_cast<std::string_view>(*this).contains(IG_forward(s)); }

protected:
	const char *str;
};

}
