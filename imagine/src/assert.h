#pragma once

#ifdef IMAGINE_SRC
	#include <config.h>
#endif

#if !defined(CONFIG_BASE_PS3) && !defined(CONFIG_BASE_ANDROID) && !defined(CONFIG_BASE_IOS)
	#include_next <assert.h>
#elif !defined(NDEBUG)
	// custom assert() implementation
	#include <logger/interface.h>
	#include <stdlib.h>
	#ifdef __cplusplus
		#define assert(condition) if(!(condition)) \
		{ logger_printfn(0, "assert failed: %s in " __FILE__ ", line %d , in function %s", #condition, __LINE__, __PRETTY_FUNCTION__); ::abort(); }
	#else
		#define assert(condition) if(!(condition)) \
		{ logger_printfn(0, "assert failed: %s in " __FILE__ ", line %d , in function %s", #condition, __LINE__, __PRETTY_FUNCTION__); abort(); }
	#endif
#else
	#define assert(condition) { }
#endif
