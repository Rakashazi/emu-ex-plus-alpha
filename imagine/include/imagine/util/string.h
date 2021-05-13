#pragma once

#ifdef __cplusplus
#include <array>
#include <cstring>
#include <cctype>
#include <cstdarg>
#include <system_error>
#include <string>
#else
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#endif
#include <imagine/util/builtins.h>

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

template <typename T>
size_t string_copy(T &dest, const char *src)
{
	return string_copy(std::data(dest), src, std::size(dest));
}

#ifdef __clang__
// need to directly call builtin version to get constexpr
#define string_len(s) __builtin_strlen(s)
#else
[[gnu::nonnull, gnu::pure]]
static constexpr size_t string_len(const char *s)
{
	return std::strlen(s);
	// If compiler doesn't have constexpr the following recursive version also works:
	// return *s ? 1 + string_len(s+1) : 0;
}
#endif

template <typename T>
static size_t string_cat(T &dest, const char *src)
{
	return string_cat(std::data(dest), src, std::size(dest));
}

template <typename T>
[[gnu::format(printf, 2, 3)]]
static int string_printf(T &dest, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	auto result = vsnprintf(std::data(dest), std::size(dest), format, args);
	va_end(args);
	return result;
}

template <size_t S>
[[gnu::format(printf, 1, 2)]]
static std::array<char, S> string_makePrintf(const char *format, ...)
{
	std::array<char, S> str;
	va_list args;
	va_start(args, format);
	vsnprintf(str.data(), S, format, args);
	va_end(args);
	return str;
}

std::errc string_convertCharCode(const char** sourceStart, uint32_t &c);

std::array<char, 2> string_fromChar(char c);

std::u16string string_makeUTF16(const char *str);

#endif
