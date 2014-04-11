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

#include <imagine/util/ansiTypes.h>

enum { OK, NO_FREE_ENTRIES, OUT_OF_MEMORY, IO_ERROR, ALREADY_EXISTS,
	INIT_ERROR, INVALID_PARAMETER, OUT_OF_BOUNDS, NOT_FOUND,
	NO_PRECOMP, PERMISSION_DENIED, UNSUPPORTED_OPERATION };

typedef uint CallResult;

#if defined __arm__ && (defined __SOFTFP__ || defined __ARM_ARCH_6K__)
#define CONFIG_TYPES_SOFT_FLOAT
#endif

#if !defined(__x86_64__) && !defined(__i386__)
#define CONFIG_TYPES_NO_DOUBLE
#endif
