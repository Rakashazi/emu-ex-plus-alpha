#pragma once

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <mem/interface.h>
#include <util/cLang.h>

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

static constexpr size_t string_len(const char *s) ATTRS(pure, nonnull);
static constexpr size_t string_len(const char *s)
{
#ifdef __clang__
	// TODO: remove when clang supports constexpr strlen
	return *s ? 1 + string_len(s+1) : 0;
#else
	return strlen(s);
#endif
}

#endif

// prints format string to buffer and returns number of bytes written
// returns zero if buffer is too small or on error
static int string_printf(char *buffer, int buff_size, const char *format, ... ) __attribute__ ((format (printf, 3, 4)));
static int string_printf(char *buffer, int buff_size, const char *format, ... )
{
	va_list args;
	va_start(args, format);
	int ret = vsnprintf(buffer, buff_size, format, args);
	va_end(args);
	// error if text would overflow, or actual error in vsnprintf()
	if(ret >= buff_size || ret < 0)
		return 0;
	return ret;
}

#ifdef __cplusplus

template <size_t S>
static int string_printf(char (&buffer)[S], const char *format, ... ) __attribute__ ((format (printf, 2, 3)));
template <size_t S>
static int string_printf(char (&buffer)[S], const char *format, ... )
{
	va_list args;
	va_start(args, format);
	int ret = vsnprintf(buffer, S, format, args);
	va_end(args);
	// error if text would overflow, or actual error in vsnprintf()
	if(ret >= (int)S || ret < 0)
		return 0;
	return ret;
}

#endif
