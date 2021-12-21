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

#include <imagine/config/defs.hh>
#include <imagine/util/string.h>
#include <imagine/util/utility.h>
#include <imagine/util/utf.hh>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <system_error>

namespace IG
{

[[gnu::nonnull]]
static std::errc convertCharCode(const char** sourceStart, uint32_t &c)
{
	if(Config::UNICODE_CHARS)
	{
		switch(UTF::ConvertUTF8toUTF32((const uint8_t**)sourceStart, UTF::strictConversion, c))
		{
			case UTF::conversionOK: return {};
			case UTF::reachedNullChar: return std::errc::result_out_of_range;
			default: return std::errc::invalid_argument;
		}
	}
	else
	{
		c = **sourceStart;
		if(c == '\0')
			return std::errc::result_out_of_range;
		*sourceStart += 1;
		return {};
	}
}

std::u16string makeUTF16String(std::string_view strView)
{
	if(!strView.size())
		return {};
	std::u16string u16String{};
	unsigned utf16Len = 0;
	const char *s = strView.data();
	uint32_t c = 0;
	while(!(bool)convertCharCode(&s, c))
	{
		if(c > UINT16_MAX)
			continue;
		utf16Len++;
	}
	u16String.reserve(utf16Len);
	s = strView.data();
	while(!(bool)convertCharCode(&s, c))
	{
		if(c > UINT16_MAX)
			continue;
		u16String.push_back(c);
	}
	return u16String;
}

bool stringNoCaseLexCompare(std::string_view s1, std::string_view s2)
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
