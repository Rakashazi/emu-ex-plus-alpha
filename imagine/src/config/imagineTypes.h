#pragma once

#include <util/ansiTypes.h>

enum { OK, NO_FREE_ENTRIES, OUT_OF_MEMORY, IO_ERROR, ALREADY_EXISTS,
	INIT_ERROR, INVALID_PARAMETER, OUT_OF_BOUNDS, NOT_FOUND,
	NO_PRECOMP, PERMISSION_DENIED, UNSUPPORTED_OPERATION };

typedef uint CallResult;

// TODO: set this for platforms that don't use float math once proper support is in place
#define CONFIG_TYPES_NO_FLOAT 0

#if !defined(__x86_64__) && !defined(__i386__)
	#define CONFIG_TYPES_NO_DOUBLE 1
#else
	#define CONFIG_TYPES_NO_DOUBLE 0
#endif
