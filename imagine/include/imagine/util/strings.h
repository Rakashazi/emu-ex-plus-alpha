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

#include <imagine/engine-globals.h>
#include <imagine/util/basicString.h>
#include <imagine/util/string/generic.h>
#include <imagine/util/builtins.h>

static const char firstDrawableAsciiChar = '!';
static const char lastDrawableAsciiChar = '~';
static const uint numDrawableAsciiChars = (lastDrawableAsciiChar - firstDrawableAsciiChar) + 1;

BEGIN_C_DECLS

int hexToInt(char c);
int numCharInString(const char *s, int search);
uchar string_hasDotExtension(const char *s, const char *extension);
int charIsDrawableAscii(int c);
int charIsDrawableUnicode(int c);
void string_copyUpToLastCharInstance(char *dest, const char *src, char c);
int string_numCharsInLine(const char *s);
void string_copyNCharsInLine(char *dest, const char *src, uint destSize);

END_C_DECLS

#ifdef __cplusplus
CallResult string_convertCharCode(const char** sourceStart, uint &c);
#endif
