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

#ifndef BRANCH_H_PRINTF
	#include <stdio.h>
	#define BRANCH_H_PRINTF(str, ...) printf(str "\n", ## __VA_ARGS__)
#endif

#ifndef BRANCH_H_EXIT
	#include <stdlib.h>
	#define BRANCH_H_EXIT() abort()
#endif

#ifndef NDEBUG
	#define bug_exit(msg, ...) { BRANCH_H_PRINTF(msg " in %s", ## __VA_ARGS__, __PRETTY_FUNCTION__); BRANCH_H_EXIT(); }
#else
	#define bug_exit(msg, ...) {  }
#endif

#define bug_branch(fmtStr, val) bug_exit("Bad branch value " fmtStr, val)

#define bcase break; case
#define bdefault break; default

// call a function pointer if non-null
#define callSafe(funcPtr, ...) { if(likely(funcPtr != 0)) funcPtr(__VA_ARGS__); }
// call a member function if object pointer non-null
#define callMSafe(objPtr, memberFunc, ...) { if(likely(objPtr != 0)) objPtr->memberFunc(__VA_ARGS__); }
