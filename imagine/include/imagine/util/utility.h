#pragma once

#ifdef __cplusplus
#include <utility>
#include <cstddef>
#endif
#include <assert.h>
#include <imagine/util/builtins.h>

#define var_isConst(E) __builtin_constant_p(E)

#define bcase break; case
#define bdefault break; default

#define likely(E) __builtin_expect(!!(E), 1)
#define unlikely(E) __builtin_expect(!!(E), 0)

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

// logical xor
#define lxor(a, b) ( !(a) != !(b) )
