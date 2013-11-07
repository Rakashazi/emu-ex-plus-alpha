#pragma once

#include <util/ansiTypes.h>

enum { OK, NO_FREE_ENTRIES, OUT_OF_MEMORY, IO_ERROR, ALREADY_EXISTS,
	INIT_ERROR, INVALID_PARAMETER, OUT_OF_BOUNDS, NOT_FOUND,
	NO_PRECOMP, PERMISSION_DENIED, UNSUPPORTED_OPERATION };

typedef uint CallResult;

#if defined __arm__ && (defined __SOFTFP__ || defined __ARM_ARCH_6K__)
#define CONFIG_TYPES_SOFT_FLOAT
#endif

#if !defined(__x86_64__) && !defined(__i386__)
#define CONFIG_TYPES_NO_DOUBLE
#endif
