#pragma once

#include <imagine/util/builtins.h>

#if !defined __PPU__ && !defined __ANDROID__ && !defined __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__
#include_next <assert.h>
#elif !defined NDEBUG
// custom assert() implementation
#define assert(condition) if(!(condition)) \
	{ bug_doExit("assert failed: %s in " __FILE__ ", line %d , in function %s", #condition, __LINE__, __PRETTY_FUNCTION__); }
#else
#define assert(condition) ((void)(0))
#endif

#ifndef NDEBUG
CLINK void bug_doExit(const char *msg, ...)  __attribute__ ((format (printf, 1, 2)));
#define bug_exit(msg, ...) bug_doExit(msg " in %s", ## __VA_ARGS__, __PRETTY_FUNCTION__)
#else
#define bug_exit(msg, ...) ((void)(0))
#endif

#define bug_branch(fmtStr, val) bug_exit("Bad branch value " fmtStr, val)
