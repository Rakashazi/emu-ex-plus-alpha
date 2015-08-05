#pragma once

#ifdef __cplusplus
#include <array>
#include <cstring>
#include <cctype>
#include <cstdio>
#else
#include <string.h>
#include <ctype.h>
#endif
#include <imagine/util/builtins.h>

BEGIN_C_DECLS

void string_toUpper(char *s);
int string_containsChar(const char *s, int c);
bool string_isHexValue(const char *s, uint maxChars);
int string_equalNoCase(const char *s1, const char *s2) ATTRS(nonnull);
int string_equal(const char *s1, const char *s2) ATTRS(nonnull);
char *string_dup(const char *s) ATTRS(nonnull);
char *string_cat(char *dest, const char *src, size_t destSize) ATTRS(nonnull);

// copies at most destSize-1 chars from src until null byte or dest size is reached
// dest is always null terminated
char *string_copy(char *dest, const char *src, size_t destSize) ATTRS(nonnull);

END_C_DECLS

#ifdef __cplusplus

template <size_t S>
static char *string_copy(char (&dest)[S], const char *src)
{
	return string_copy(dest, src, S);
}

template <size_t S>
static char *string_copy(std::array<char, S> &dest, const char *src)
{
	return string_copy(dest.data(), src, S);
}

#ifdef __clang__
// need to directly call builtin version to get constexpr
#define string_len(s) __builtin_strlen(s)
#else
[[gnu::nonnull, gnu::pure]] static constexpr size_t string_len(const char *s)
{
	return std::strlen(s);
	// If compiler doesn't have constexpr the following recursive version also works:
	// return *s ? 1 + string_len(s+1) : 0;
}
#endif

template <size_t S>
static char *string_cat(char (&dest)[S], const char *src)
{
	return string_cat(dest, src, S);
}

template <size_t S>
static char *string_cat(std::array<char, S> &dest, const char *src)
{
	return string_cat(dest.data(), src, S);
}

template <size_t S, typename... ARGS>
static int string_printf(char (&buffer)[S], ARGS&&... args)
{
	return snprintf(buffer, S, std::forward<ARGS>(args)...);
}

template <size_t S, typename... ARGS>
static int string_printf(std::array<char, S> &buffer, ARGS&&... args)
{
	return snprintf(buffer.data(), S, std::forward<ARGS>(args)...);
}

template <size_t S, typename... ARGS>
static std::array<char, S> string_makePrintf(ARGS&&... args)
{
	std::array<char, S> str;
	snprintf(str.data(), S, std::forward<ARGS>(args)...);
	return str;
}

#endif
