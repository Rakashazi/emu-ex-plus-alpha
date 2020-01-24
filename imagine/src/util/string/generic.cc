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

#include <imagine/config/defs.hh>
#include <imagine/util/string.h>
#include <imagine/util/string/basename.h>
#include <imagine/util/utility.h>
#include <imagine/util/utf.hh>
#include <assert.h>
#include <system_error>
#include <cstdint>

#if defined __ANDROID__ || defined __APPLE__
#define HAS_STRL_FUNCS
#endif

static const char pathSeparator[] = { '/'
#ifdef CONFIG_BASE_WIN32
		, '\\'
#endif
};
static constexpr uint32_t numPathSeparators = std::size(pathSeparator);

template <class T>
static T *dirNameCutoffPoint(T *path)
{
	T *cutoffPoint = nullptr;
	for(uint32_t i = 0; i < numPathSeparators; i++)
	{
		T *possibleCutoff = strrchr(path, pathSeparator[i]);
		if(possibleCutoff > cutoffPoint)
			cutoffPoint = possibleCutoff;
	}
	return cutoffPoint;
}

void dirNameInPlace(char *path)
{
	char *cutoffPoint = dirNameCutoffPoint(path);

	if(cutoffPoint != nullptr)
		*cutoffPoint = 0;
	else strcpy(path, ".");
}

void dirName(const char *path, char *pathOut)
{
	const char *cutoffPoint = dirNameCutoffPoint(path);

	if(cutoffPoint != nullptr)
	{
		size_t cpySize = cutoffPoint - path;
		memcpy(pathOut, path, cpySize);
		pathOut[cpySize] = 0;
	}
	else strcpy(pathOut, ".");
}

static char *dirNameCpy(char *path)
{
	char *cutoffPoint = dirNameCutoffPoint(path);
	char *pathOut;
	if(cutoffPoint != nullptr)
	{
		size_t cpySize = cutoffPoint - path;
		pathOut = (char*)malloc(cpySize + 1);
		memcpy(pathOut, path, cpySize);
		pathOut[cpySize] = 0;
	}
	else
	{
		pathOut = (char*)malloc(2);
		strcpy(pathOut, ".");
	}
	return pathOut;
}

template <class T>
T baseNamePos(T path)
{
	T pos = path;
	for(uint32_t i = 0; i < numPathSeparators; i++)
	{
		T possiblePos = strrchr(path, pathSeparator[i]);
		if(possiblePos > pos)
			pos = possiblePos+1;
	}
	return pos;
}

template char* baseNamePos<char*>(char* path);
template const char* baseNamePos<const char*>(const char* path);

void baseNameInPlace(char *path)
{
	char *copyPoint = baseNamePos(path);
	if(copyPoint != nullptr)
		strcpy(path, copyPoint);
}

void baseName(const char *path, char *pathOut)
{
	const char *cutoffPoint = baseNamePos(path);

	assert(*cutoffPoint != 0); // TODO: other cases
	strcpy(pathOut, cutoffPoint);
}

int char_hexToInt(char c)
{
	int hex = -1;
	sscanf(string_fromChar(c).data(), "%x", &hex);
	return hex;
}

const char *string_dotExtension(const char *s)
{
	return strrchr(s, '.');
}

bool string_hasDotExtension(const char *s, const char *extension)
{
	const char *extPos = string_dotExtension(s);
	if(!extPos)
	{
		//logMsg("name has no dot to specify extension");
		return false;
	}
	extPos++; //skip past dot
	return string_equalNoCase(extPos, extension);
}

std::errc string_convertCharCode(const char** sourceStart, uint32_t &c)
{
	if(Config::UNICODE_CHARS)
	{
		switch(UTF::ConvertUTF8toUTF32((const uint8_t**)sourceStart, UTF::strictConversion, c))
		{
			case UTF::conversionOK: return {};
			case UTF::reachedNullChar: return std::errc::result_out_of_range;
			default: return std::errc::invalid_argument;
		}
	}
	else
	{
		c = **sourceStart;
		if(c == '\0')
			return std::errc::result_out_of_range;
		*sourceStart += 1;
		return {};
	}
}

void string_toUpper(char *s)
{
	while(*s != '\0')
	{
		*s = toupper(*s);
		s++;
	}
}

bool string_equalNoCase(const char *s1, const char *s2)
{
	return strcasecmp(s1, s2) == 0;
}

bool string_equal(const char *s1, const char *s2)
{
	return strcmp(s1, s2) == 0;
}

size_t string_copy(char *dest, const char *src, size_t destSize)
{
	#ifdef HAS_STRL_FUNCS
	return strlcpy(dest, src, destSize);
	#else
	size_t srcLen = strlen(src);
	if(destSize)
	{
		size_t copyBytes = std::min(destSize-1, srcLen);
		memcpy(dest, src, copyBytes);
		dest[copyBytes] = '\0';
	}
	return srcLen;
	#endif

}

size_t string_cat(char *dest, const char *src, size_t destSize)
{
	#ifdef HAS_STRL_FUNCS
	return strlcat(dest, src, destSize);
	#else
	size_t destLen = strlen(dest);
	assumeExpr(destSize >= destLen);
	dest += destLen;
	destSize -= destLen;
	size_t srcLen = strlen(src);
	size_t copyBytes = std::min(destSize-1, srcLen);
	memcpy(dest, src, copyBytes);
	dest[copyBytes] = '\0';
	size_t newLen = destLen + srcLen;
	return newLen;
	#endif
}

std::array<char, 2> string_fromChar(char c)
{
	return {c, '\0'};
}
