#pragma once

#include <string.h>
#include <stdio.h>
#include <mem/interface.h>
#include <util/cLang.h>
#include <util/basicMath.hh>

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
	var_copy(bytes, strlen(s)+1);
	char *dup = (char*)mem_alloc(bytes);
	if(dup)
		memcpy(dup, s, bytes);
	return dup;
}

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

	uint charsToCopy = IG::min(destSize-1, strlen(src));
	memcpy(dest, src, charsToCopy);
	dest[charsToCopy] = '\0';

	return dest;
}

#ifdef __cplusplus

template <size_t S>
static char *string_copy(char (&dest)[S], const char *src)
{
	return string_copy(dest, src, S);
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
