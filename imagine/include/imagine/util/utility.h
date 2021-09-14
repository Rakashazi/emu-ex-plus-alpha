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

#include <assert.h>

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#ifdef __clang__
#define CLANG_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#endif

#ifdef __cplusplus
#define CLINK extern "C"
#define BEGIN_C_DECLS extern "C" {
#define END_C_DECLS }
#else
#define CLINK
#define BEGIN_C_DECLS
#define END_C_DECLS
#endif

// Make symbol remain visible after linking
#define LVISIBLE __attribute__((visibility("default")))

#define bcase break; case
#define bdefault break; default

#define PP_STRINGIFY(A) #A
#define PP_STRINGIFY_EXP(A) PP_STRINGIFY(A)

// Inform the compiler an expression must be true
// or about unreachable locations to optimize accordingly.

#ifdef NDEBUG
#define assumeExpr(E) ((void)(__builtin_expect(!(E), 0) ? __builtin_unreachable(), 0 : 0))
#define bug_unreachable(msg, ...) __builtin_unreachable()
#else
CLINK void bug_doExit(const char *msg, ...)  __attribute__ ((format (printf, 1, 2)));
#define assumeExpr(E) assert(E)
#define bug_unreachable(msg, ...) bug_doExit("bug: " msg " @" __FILE__ ", line:%d , func:%s", ## __VA_ARGS__, __LINE__, __PRETTY_FUNCTION__)
#endif
