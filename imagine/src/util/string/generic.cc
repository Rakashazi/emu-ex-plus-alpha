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

#include <imagine/util/strings.h>
#include <imagine/util/ansiTypes.h>
#ifdef CONFIG_UNICODE_CHARS
#include <imagine/util/utf.hh>
#endif
#include <cstring>
#include <assert.h>

static const char pathSeparator[] = { '/'
#ifdef CONFIG_BASE_WIN32
		, '\\'
#endif
};
static const uint numPathSeparators = sizeofArray(pathSeparator);

template <class T>
static T *dirNameCutoffPoint(T *path)
{
	T *cutoffPoint = nullptr;
	for(uint i = 0; i < numPathSeparators; i++)
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
		pathOut = (char*)mem_alloc(cpySize + 1);
		memcpy(pathOut, path, cpySize);
		pathOut[cpySize] = 0;
	}
	else
	{
		pathOut = (char*)mem_alloc(2);
		strcpy(pathOut, ".");
	}
	return pathOut;
}

template <class T>
T baseNamePos(T path)
{
	T pos = path;
	for(uint i = 0; i < numPathSeparators; i++)
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

int hexToInt(char c)
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

int numCharInString(const char *s, int search)
{
	int count = 0;
	for(char c = *s; c != 0; c = *(++s))
	{
		if(c == search) count++;
	}
	return count;
}

int string_hasDotExtension(const char *s, const char *extension)
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

int charIsDrawableAscii(int c)
{
	if(c >= firstDrawableAsciiChar && c <= lastDrawableAsciiChar)
		return 1;
	else return 0;
}

int charIsDrawableUnicode(int c)
{
	return !(
			(c >= 0x0 && c < '!')
			|| (c > '~' && c < 0xA1)
			|| (c >= 0x2000 && c <= 0x200F)
			|| (c == 0x3000)
			);
}

void string_copyUpToLastCharInstance(char *dest, const char *src, char c)
{
	const char *limit = strrchr(src, c);
	if(!limit)
		limit = &src[strlen(src)];
	memcpy(dest, src, limit-src);
	dest[limit-src] = 0;
}

int string_numCharsInLine(const char *s)
{
	int count = 0;
	for(const char *c = s; *c != '\n' && *c != '\0'; c++)
	{
		count++;
	}
	return count;
}

void string_copyNCharsInLine(char *dest, const char *src, uint destSize)
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

CallResult string_convertCharCode(const char** sourceStart, uint &c)
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

void string_toUpper(char *s)
{
	while(*s != '\0')
	{
		*s = toupper(*s);
		s++;
	}
}

int string_containsChar(const char *s, int c)
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

bool string_isHexValue(const char *s, uint maxChars)
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

int string_equalNoCase(const char *s1, const char *s2)
{
	return strcasecmp(s1, s2) == 0;
}

int string_equal(const char *s1, const char *s2)
{
	return strcmp(s1, s2) == 0;
}

char *string_dup(const char *s)
{
	auto bytes = strlen(s)+1;
	char *dup = (char*)mem_alloc(bytes);
	if(dup)
		memcpy(dup, s, bytes);
	return dup;
}

char *string_copy(char *dest, const char *src, size_t destSize)
{
	uint charsToCopy = std::min(destSize-1, strlen(src));
	memcpy(dest, src, charsToCopy);
	dest[charsToCopy] = '\0';
	return dest;
}

char *string_cat(char *dest, const char *src, size_t destSize)
{
	string_copy(dest + strlen(dest), src, destSize - strlen(dest));
	return dest;
}
