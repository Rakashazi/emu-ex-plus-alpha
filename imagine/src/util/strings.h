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

#include <engine-globals.h>
#include <util/basicString.h>
#include <assert.h>
#include <mem/interface.h>
#include <util/string/generic.h>
#include <limits.h>
#ifndef CONFIG_BASE_PS3
	#include <libgen.h>
#endif

#if !defined __APPLE__ && !defined __ANDROID__ && !defined CONFIG_BASE_PS3 && defined _GNU_SOURCE
	#define CONFIG_USE_GNU_BASENAME
#endif

#ifdef CONFIG_USE_GNU_BASENAME
	#include <util/string/glibc.h>
#endif

static int hexToInt(char c)
{
	switch(c)
	{
		case '0': return 0;
		case '1': return 1;
		case '2': return 2;
		case '3': return 3;
		case '4': return 4;
		case '5': return 5;
		case '6': return 6;
		case '7': return 7;
		case '8': return 8;
		case '9': return 9;
		case 'a':
		case 'A':
			return 0xa;
		case 'b':
		case 'B':
			return 0xb;
		case 'c':
		case 'C':
			return 0xc;
		case 'd':
		case 'D':
			return 0xd;
		case 'e':
		case 'E':
			return 0xe;
		case 'f':
		case 'F':
			return 0xf;
		default : bug_branch("%d", c); return 0;
	}
}

static int numCharInString(const char *s, int search)
{
	int count = 0;
	for(char c = *s; c != 0; c = *(++s))
	{
		if(c == search) count++;
	}
	return count;
}

static uchar string_hasDotExtension(const char *s, const char *extension)
{
	const char *suffixPos = strrchr(s, '.');
	if(suffixPos == 0)
	{
		//logMsg("name has no dot to specify extension");
		return 0;
	}
	suffixPos++; //skip past dot

	return string_equalNoCase(suffixPos, extension);
}

#define firstDrawableAsciiChar '!'
#define lastDrawableAsciiChar '~'
#define numDrawableAsciiChars ( (lastDrawableAsciiChar - firstDrawableAsciiChar) + 1 )
static int charIsDrawableAscii(int c)
{
	if(c >= firstDrawableAsciiChar && c <= lastDrawableAsciiChar)
		return 1;
	else return 0;
}

static int charIsDrawableUnicode(int c)
{
	return !(
			(c >= 0x0 && c < '!')
			|| (c > '~' && c < 0xA1)
			|| (c >= 0x2000 && c <= 0x200F)
			|| (c == 0x3000)
			);
}

static void string_copyUpToLastCharInstance(char *dest, const char *src, char c)
{
	const char *limit = strrchr(src, c);
	if(!limit)
		limit = &src[strlen(src)];
	memcpy(dest, src, limit-src);
	dest[limit-src] = 0;
}

static int string_numCharsInLine(const char *s)
{
	int count = 0;
	for(const char *c = s; *c != '\n' && *c != '\0'; c++)
	{
		count++;
	}
	return count;
}

static void string_copyNCharsInLine(char *dest, const char *src, uint destSize)
{
	iterateTimes(destSize, i)
	{
		if(src[i] == '\n' || src[i] == '\0' || i == destSize - 1)
		{
			dest[i] = '\0';
			break;
		}
		dest[i] = src[i];
	}
}

#ifdef __cplusplus

template <size_t S>
static char *string_basename(const char *filename, char (&storage)[S])
{
	#ifdef CONFIG_USE_GNU_BASENAME
		return gnu_basename(filename);
	#elif defined __ANDROID__
		return basename(filename);
	#elif defined CONFIG_BASE_PS3
		char str[1024] {0};
		baseName(filename, str);
		return str;
	#else
		// standard version can modify input, and returns a pointer within it
		// BSD version can modify input, but always returns its own allocated storage
		string_copy(storage, filename);
		return basename(storage);
	#endif
}

template <size_t S>
static char *string_dirname(const char *filename, char (&storage)[S])
{
	#if defined __ANDROID__
		return dirname(filename);
	#elif defined CONFIG_BASE_PS3
		char str[1024] {0};
		dirName(filename, str);
		return str;
	#else
		// standard version can modify input, and returns a pointer within it
		// BSD version can modify input, but always returns its own allocated storage
		string_copy(storage, filename);
		return dirname(storage);
	#endif
}

#ifdef CONFIG_UNICODE_CHARS
	#include <util/utf.hh>
#endif

static CallResult string_convertCharCode(const char** sourceStart, uint &c)
{
#ifdef CONFIG_UNICODE_CHARS
	switch(UTF::ConvertUTF8toUTF32((const uint8**)sourceStart, UTF::strictConversion, c))
	{
		case UTF::conversionOK: return OK;
		case UTF::reachedNullChar: return OUT_OF_BOUNDS;
		default: return INVALID_PARAMETER;
	}
#else
	c = **sourceStart;
	if(c == '\0')
		return OUT_OF_BOUNDS;
	*sourceStart += 1;
	return OK;
#endif
}

#endif
