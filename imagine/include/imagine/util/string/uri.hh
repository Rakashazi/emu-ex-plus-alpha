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
#include <string_view>

namespace IG
{

constexpr bool isUri(std::string_view str)
{
	return str.contains("://");
}

constexpr bool isUnreservedUriChar(char c)
{
	switch(c)
	{
		case 'a' ... 'z':
		case 'A' ... 'Z':
		case '0' ... '9':
		case '_':
		case '-':
		case '!':
		case '.':
		case '~':
		case '\'':
		case '(':
		case ')':
		case '*':
		case '"':
			return true;
		default:
			return false;
	}
}

template <class Return>
[[nodiscard]]
constexpr Return encodeUri(std::string_view input)
{
	Return output{};
	output.reserve(input.size());
	for(auto c : input)
	{
		if(isUnreservedUriChar(c))
		{
			output += c;
		}
		else
		{
			auto code = static_cast<unsigned char>(c);
			output += '%';
			output += hexDigitChar(code >> 4);
			output += hexDigitChar(code & 0x0f);
		}
	}
	return output;
}

template <class Return>
[[nodiscard]]
constexpr Return decodeUri(std::string_view input)
{
	Return output{};
	output.reserve(input.size());
	auto isHexChar =
		[](char c)
		{
			switch(c)
			{
				case '0' ... '9':
				case 'a' ... 'f':
				case 'A' ... 'F':
					return true;
				default:
					return false;
			}
		};
	for(auto it = input.begin(); it != input.end(); it++)
	{
		if(*it == '%' && it + 2 < input.end() && isHexChar(*(it+1)) && isHexChar(*(it+2)))
		{
			auto left = charHexDigitInt(*(it+1));
			auto right = charHexDigitInt(*(it+2));
			output += static_cast<char>(16 * left + right);
			it += 2;
		}
		else
		{
			output += *it;
		}
	}
	return output;
}

}
