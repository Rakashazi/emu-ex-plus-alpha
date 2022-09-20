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
#include <imagine/util/algorithm.h>
#include <concepts>
#include <string>
#include <string_view>

namespace IG
{

// Constants from Unicode Inc.

/*
 * Index into the table below with the first byte of a UTF-8 sequence to
 * get the number of trailing bytes that are supposed to follow it.
 * Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
 * left as-is for anyone who may want to do such conversion, which was
 * allowed in earlier algorithms.
 */
constexpr size_t trailingBytesForUTF8[256]
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/*
 * Magic values subtracted from a buffer value during UTF8 conversion.
 * This table contains as many values as there might be trailing bytes
 * in a UTF-8 sequence.
 */
constexpr uint32_t offsetsFromUTF8[4]
{
	0x00000000UL, 0x00003080UL, 0x000E2080UL, 0x03C82080UL
};

class UTF8CodeUnit
{
public:
	constexpr UTF8CodeUnit() = default;

	constexpr UTF8CodeUnit(std::string_view srcStr)
	{
		extraSize = srcStr.size() - 1;
		switch(srcStr.size())
		{
			default: bug_unreachable("invalid UTF8CodeUnit string size");
			case 1: copy_n(srcStr.data(), 1, c.data()); break;
			case 2: copy_n(srcStr.data(), 2, c.data()); break;
			case 3: copy_n(srcStr.data(), 3, c.data()); break;
			case 4: copy_n(srcStr.data(), 4, c.data()); break;
		}
	}

	[[nodiscard]]
	constexpr auto size() const { return extraSize + 1; }
	constexpr const auto &operator[](size_t i) const { return c[i]; }

	[[nodiscard]]
	constexpr uint32_t toUTF32()
	{
		uint32_t utf32{};
		switch(extraSize)
		{
			default: bug_unreachable("invalid UTF8CodeUnit string size");
			case 0:
				utf32 += c[0]; break;
			case 1:
				utf32 += c[0]; utf32 <<= 6;
				utf32 += c[1]; break;
			case 2:
				utf32 += c[0]; utf32 <<= 6;
				utf32 += c[1]; utf32 <<= 6;
				utf32 += c[2]; break;
			case 3:
				utf32 += c[0]; utf32 <<= 6;
				utf32 += c[1]; utf32 <<= 6;
				utf32 += c[2]; utf32 <<= 6;
				utf32 += c[3]; break;
		}
		utf32 -= offsetsFromUTF8[extraSize];
		return utf32;
	}

private:
	size_t extraSize{};
	std::array<uint8_t, 4> c{};
};


class UTF8Parser
{
public:
	class Iterator
	{
	public:
		struct Sentinel {};

		constexpr Iterator(std::string_view srcStr):
			srcStr{srcStr}, utf8Code{nextCodeUnit()} {}

		constexpr Iterator &operator++()
		{
			utf8Code = nextCodeUnit();
			return *this;
		}

		constexpr bool operator==(Sentinel) const
		{
			return !utf8Code[0];
		}

		constexpr auto operator*() const
		{
			return utf8Code;
		}

	private:
		std::string_view srcStr;
		UTF8CodeUnit utf8Code;

		constexpr UTF8CodeUnit nextCodeUnit()
		{
			if(!srcStr.size())
				return {};
			const auto utf8Size = 1 + trailingBytesForUTF8[(uint8_t)srcStr[0]];
			if(utf8Size > 4 || utf8Size > srcStr.size()) [[unlikely]]
				return {};
			auto utf8Data = srcStr.data();
			srcStr = {srcStr.data() + utf8Size, srcStr.size() - utf8Size};
			return std::string_view{utf8Data, utf8Size};
		}
	};

	constexpr UTF8Parser(std::string_view srcStr): srcStr{srcStr} {}
	constexpr auto begin() const { return Iterator{srcStr}; }
	constexpr auto end() const { return Iterator::Sentinel{}; }

private:
	std::string_view srcStr;
};

[[nodiscard]]
static constexpr std::u16string toUTF16String(std::string_view strView)
{
	std::u16string u16String;
	u16String.reserve(strView.size());
	for(auto utf8CodeUnit : UTF8Parser(strView))
	{
		u16String.push_back(utf8CodeUnit.toUTF32());
	}
	return u16String;
}

// Simple wrapper around u16string that converts any char-type strings from UTF-8 to UTF-16
class UTF16String : public std::u16string
{
public:
	using std::u16string::u16string;
	using std::u16string::operator=;
	constexpr UTF16String(std::convertible_to<std::u16string> auto &&s):std::u16string{IG_forward(s)} {}
	constexpr UTF16String(std::u16string_view s):std::u16string{s} {}

	// UTF-8 -> UTF-16 conversion
	constexpr UTF16String(std::string_view s):std::u16string{toUTF16String(s)} {}
	constexpr UTF16String(std::convertible_to<std::string_view> auto &&s):UTF16String{std::string_view{IG_forward(s)}} {}
	[[gnu::nonnull]]
	constexpr UTF16String(const char *s):UTF16String{std::string_view{s}} {}
};

template<class T>
concept UTF16Convertible = std::convertible_to<T, UTF16String>;

}
