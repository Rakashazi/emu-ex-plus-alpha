#pragma once

#include <imagine/util/builtins.h>
#if defined __APPLE__
#include <TargetConditionals.h>
#endif

#if !defined __PPU__ && !defined __ANDROID__ && !(defined __APPLE__ && TARGET_OS_IPHONE)
#include_next <assert.h>
#elif defined NDEBUG
#define assert(condition) ((void)(0))
#else
CLINK void bug_doExit(const char *msg, ...)  __attribute__ ((format (printf, 1, 2)));
// custom assert() implementation
#define assert(condition) ((condition) ? ((void)(0)) : bug_doExit("assert failed: %s in " __FILE__ ", line %d , in function %s", #condition, __LINE__, __PRETTY_FUNCTION__))
#endif
