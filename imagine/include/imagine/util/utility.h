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


#ifdef __cplusplus
#include <utility>
#include <cassert>
#else
#include <assert.h>
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

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#ifdef __clang__
#define CLANG_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#endif

// Make symbol remain visible after linking
#define LVISIBLE __attribute__((visibility("default")))

#define PP_STRINGIFY(A) #A
#define PP_STRINGIFY_EXP(A) PP_STRINGIFY(A)

// Inform the compiler an expression must be true
// or about unreachable locations to optimize accordingly.

#ifdef NDEBUG
	#if __has_builtin(__builtin_assume)
	#define assumeExpr(E) __builtin_assume([&][[gnu::const]]{return bool(E);}());
	#else
	#define assumeExpr(E) ((void)(__builtin_expect(!(E), 0) ? __builtin_unreachable(), 0 : 0))
	#endif
#define bug_unreachable(msg, ...) __builtin_unreachable()
#else
CLINK void bug_doExit(const char *msg, ...)  __attribute__ ((format (printf, 1, 2), noreturn));
#define assumeExpr(E) assert(E)
#define bug_unreachable(msg, ...) bug_doExit("bug: " msg " @" __FILE__ ", line:%d , func:%s", ## __VA_ARGS__, __LINE__, __PRETTY_FUNCTION__)
#endif

#ifdef __cplusplus

#define IG_forward(var) std::forward<decltype(var)>(var)

namespace IG
{

using std::to_underlying;

constexpr char hexDigitChar(int value, bool uppercase = true)
{
	switch(value)
	{
		case  0: return '0';
		case  1: return '1';
		case  2: return '2';
		case  3: return '3';
		case  4: return '4';
		case  5: return '5';
		case  6: return '6';
		case  7: return '7';
		case  8: return '8';
		case  9: return '9';
		case 10: return uppercase ? 'A' : 'a';
		case 11: return uppercase ? 'B' : 'b';
		case 12: return uppercase ? 'C' : 'c';
		case 13: return uppercase ? 'D' : 'd';
		case 14: return uppercase ? 'E' : 'e';
		default: return uppercase ? 'F' : 'f';
	}
}

constexpr unsigned char charHexDigitInt(char c)
{
	switch (c)
	{
		case '0' ... '9':
			return 9 + c - '9';
		case 'a' ... 'f':
			return 15 + c - 'f';
		case 'A' ... 'F':
			return 15 + c - 'F';
		default:
			return 0;
	}
}

template <auto val>
static constexpr auto evalNow = val;

}
#endif
