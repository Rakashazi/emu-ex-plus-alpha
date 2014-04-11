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

#include <imagine/util/builtins.h>

#define ASCII_LF 0xA
#define ASCII_CR 0xD

#ifdef __cplusplus

template <class T>
T baseNamePos(T path);

#endif

BEGIN_C_DECLS

void baseName(const char *path, char *pathOut);
void baseNameInPlace(char *path);
void dirName(const char *path, char *pathOut);
void dirNameInPlace(char *path);

END_C_DECLS
