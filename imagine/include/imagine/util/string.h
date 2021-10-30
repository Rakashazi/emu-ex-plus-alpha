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

#ifdef __cplusplus
#include <imagine/util/string/CStringView.hh>
#include <array>
#include <cstring>
#include <system_error>
#include <string>
#include <string_view>
#else
#include <string.h>
#include <stdbool.h>
#endif
#include <imagine/util/utility.h>

BEGIN_C_DECLS

int char_hexToInt(char c);
const char *string_dotExtension(const char *s) __attribute__((nonnull));
bool string_hasDotExtension(const char *s, const char *extension) __attribute__((nonnull));
void string_toUpper(char *s) __attribute__((nonnull));
bool string_equalNoCase(const char *s1, const char *s2) __attribute__((nonnull));
bool string_equal(const char *s1, const char *s2) __attribute__((nonnull));
size_t string_cat(char *dest, const char *src, size_t destSize) __attribute__((nonnull));

// copies at most destSize-1 chars from src until null byte or dest size is reached
// dest is always null terminated
size_t string_copy(char *dest, const char *src, size_t destSize) __attribute__((nonnull));

END_C_DECLS

#ifdef __cplusplus

template <class T>
size_t string_copy(T &dest, IG::CStringView src)
{
	return string_copy(std::data(dest), src, std::size(dest));
}

[[gnu::nonnull, gnu::pure]]
static constexpr size_t string_len(IG::CStringView s)
{
	return std::char_traits<char>::length(s);
}

template <class T>
static size_t string_cat(T &dest, IG::CStringView src)
{
	return string_cat(std::data(dest), src, std::size(dest));
}

std::errc string_convertCharCode(const char** sourceStart, uint32_t &c);

std::array<char, 2> string_fromChar(char c);

std::u16string string_makeUTF16(std::string_view);

namespace IG
{

// Simple wrapper around u16string that converts any char-type strings from UTF-8 to UTF-16
class utf16String : public std::u16string
{
public:
	using std::u16string::u16string;
	utf16String(std::u16string_view s):std::u16string{s} {}
	utf16String(std::u16string s):std::u16string{std::move(s)} {}
	utf16String(const char *s) [[gnu::nonnull]]:utf16String{std::string_view{s}} {}
	utf16String(std::string_view s):std::u16string{string_makeUTF16(s)} {}
	utf16String(const std::string &s):utf16String{std::string_view{s}} {}
};

}

#endif
