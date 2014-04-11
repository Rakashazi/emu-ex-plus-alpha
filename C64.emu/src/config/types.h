#pragma once

#include "vice.h"
#include <imagine/util/ansiTypes.h>

#define BYTE uint8

typedef signed char SIGNED_CHAR;

typedef uint16 WORD;
typedef int16 SWORD;

typedef uint32 DWORD;
typedef int32 SDWORD;

typedef DWORD CLOCK;
/* Maximum value of a CLOCK.  */
#define CLOCK_MAX (~((CLOCK)0))

#define vice_ptr_to_int(x) ((int)(long)(x))
#define vice_ptr_to_uint(x) ((unsigned int)(unsigned long)(x))
#define int_to_void_ptr(x) ((void *)(long)(x))
#define uint_to_void_ptr(x) ((void *)(unsigned long)(x))
