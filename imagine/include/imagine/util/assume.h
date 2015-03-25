#pragma once

#include <assert.h>

// Inform the compiler an expression must be true and
// to optimize accordingly.
// Use assumeExpr(0) to make the code following
// the statement unreachable.

#define assumeExpr(expr) \
{ \
	assert(expr); \
	if(!(expr)) \
		__builtin_unreachable(); \
}
