#pragma once

#include <util/builtins.h>

#if !defined(__PPU__) && !defined(__ANDROID__) && !defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__)
	#include_next <assert.h>
#elif !defined(NDEBUG)
	// custom assert() implementation
	#include <logger/interface.h>
	#include <stdlib.h>
	#if defined(__PPU__) // for PS3
		CLINK void base_abort();
		#define ABORT_FUNC base_abort
	#else
		#define ABORT_FUNC abort
	#endif
	#ifdef __cplusplus
		#define assert(condition) if(!(condition)) \
		{ logger_printfn(0, "assert failed: %s in " __FILE__ ", line %d , in function %s", #condition, __LINE__, __PRETTY_FUNCTION__); ::ABORT_FUNC(); }
	#else
		#define assert(condition) if(!(condition)) \
		{ logger_printfn(0, "assert failed: %s in " __FILE__ ", line %d , in function %s", #condition, __LINE__, __PRETTY_FUNCTION__); ABORT_FUNC(); }
	#endif
#else
	#define assert(condition) { }
#endif
