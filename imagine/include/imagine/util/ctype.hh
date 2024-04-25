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

#include <concepts>

namespace IG
{

constexpr bool isAlpha(std::integral auto c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

constexpr int isBlank(std::integral auto c)
{
	return c == ' ' || c == '\t';
}

constexpr int isCntrl(std::integral auto c)
{
	return static_cast<unsigned>(c) < ' ' || c == 0x7f;
}

/** Returns true if `ch` is in `[0-9]`. */
constexpr int isDigit(std::integral auto c)
{
	return c >= '0' && c <= '9';
}

constexpr int isGraph(std::integral auto c)
{
	return c >= '!' && c <= '~';
}

constexpr int isLower(std::integral auto c)
{
	return c >= 'a' && c <= 'z';
}

constexpr int isPrint(std::integral auto c)
{
	return c >= ' ' && c <= '~';
}

constexpr int isSpace(std::integral auto c)
{
	return c == ' ' || (c >= '\t' && c <= '\r');
}

constexpr int isUpper(std::integral auto c)
{
	return c >= 'A' && c <= 'Z';
}

constexpr int isXdigit(std::integral auto c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

template<std::integral T>
constexpr T toLower(T c)
{
	if(c >= 'A' && c <= 'Z')
		return c | 0x20;
	return c;
}

template<std::integral T>
constexpr T toUpper(T c)
{
	if(c >= 'a' && c <= 'z')
		return c ^ 0x20;
	return c;
}

/** Returns true if `ch` is less than 0x80. */
constexpr int isAscii(std::integral auto c)
{
	return static_cast<unsigned>(c) < 0x80;
}

constexpr int toAscii(std::integral auto c)
{
	return c & 0x7f;
}

}
