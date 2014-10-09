#pragma once

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <imagine/mem/mem.h>
#include <imagine/util/algorithm.h>

static void string_toUpper(char *s)
{
	while(*s != '\0')
	{
		*s = toupper(*s);
		s++;
	}
}

static int string_containsChar(const char *s, int c)
{
	auto pos = strchr(s, c);
	int found = 0;
	while(pos)
	{
		found++;
		pos = strchr(pos+1, c);
	}
	return found;
}

static bool string_isHexValue(const char *s, uint maxChars)
{
	if(!maxChars || *s == '\0')
		return 0; // empty string
	iterateTimes(maxChars, i)
	{
		char c = s[i];
		if(c == '\0')
			break;
		bool isHex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
		if (!isHex)
			return 0;
	}
	return 1;
}

static int string_equalNoCase(const char *s1, const char *s2) ATTRS(nonnull);
static int string_equalNoCase(const char *s1, const char *s2)
{
	return strcasecmp(s1, s2) == 0;
}

static int string_equal(const char *s1, const char *s2) ATTRS(nonnull);
static int string_equal(const char *s1, const char *s2)
{
	return strcmp(s1, s2) == 0;
}

static char *string_dup(const char *s) ATTRS(nonnull);
static char *string_dup(const char *s)
{
	auto bytes = strlen(s)+1;
	char *dup = (char*)mem_alloc(bytes);
	if(dup)
		memcpy(dup, s, bytes);
	return dup;
}

#ifdef __cplusplus

#include <array>
#include <algorithm>

// copies at most destSize-1 chars from src until null byte or dest size is reached
// dest is always null terminated
static char *string_copy(char *dest, const char *src, size_t destSize) ATTRS(nonnull);
static char *string_copy(char *dest, const char *src, size_t destSize)
{
	/*iterateTimes(destSize, i)
	{
		if(i == destSize-1) // write a NULL at the last index and stop
		{
			dest[i] = '\0';
			break;
		}
		else
			dest[i] = src[i];
		if(src[i] == '\0') // stop writing after null char
			break;
	}*/

	uint charsToCopy = std::min(destSize-1, strlen(src));
	memcpy(dest, src, charsToCopy);
	dest[charsToCopy] = '\0';

	return dest;
}

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
// need to directly v=call builtin version to get constexpr
#define string_len(s) __builtin_strlen(s)
#else
static constexpr size_t string_len(const char *s) ATTRS(nonnull);
static constexpr size_t string_len(const char *s)
{
	return strlen(s);
	// If compiler doesn't have constexpr the following recursive version also works:
	// return *s ? 1 + string_len(s+1) : 0;
}
#endif

#endif

static char *string_cat(char *dest, const char *src, size_t destSize) ATTRS(nonnull);
static char *string_cat(char *dest, const char *src, size_t destSize)
{
	string_copy(dest + strlen(dest), src, destSize - strlen(dest));
	return dest;
}

#ifdef __cplusplus

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

#endif

#ifdef __cplusplus

// prints format string to buffer and returns number of bytes written
// returns zero if buffer is too small or on error
template<typename... ARGS>
static int string_printf(char *buffer, int buff_size, const char *format, ARGS&&... args)
{
	int ret = snprintf(buffer, buff_size, format, std::forward<ARGS>(args)...);
	// error if text would overflow, or actual error in vsnprintf()
	if(ret >= buff_size || ret < 0)
		return 0;
	return ret;
}

template <size_t S, typename... ARGS>
static int string_printf(char (&buffer)[S], const char *format, ARGS&&... args)
{
	return string_printf(buffer, S, format, std::forward<ARGS>(args)...);
}

template <size_t S, typename... ARGS>
static int string_printf(std::array<char, S> &buffer, const char *format, ARGS&&... args)
{
	return string_printf(buffer.data(), S, format, std::forward<ARGS>(args)...);
}

template <size_t S, typename... ARGS>
static std::array<char, S> string_makePrintf(const char *format, ARGS&&... args)
{
	std::array<char, S> str;
	string_printf(str, format, std::forward<ARGS>(args)...);
	return str;
}

#endif
