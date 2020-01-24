#pragma once

#include "vice.h"
#include <stdint.h>

#define BYTE uint8_t

typedef signed char SIGNED_CHAR;

typedef uint16_t WORD;
typedef int16_t SWORD;

typedef uint32_t DWORD;
typedef int32_t SDWORD;

typedef DWORD CLOCK;
/* Maximum value of a CLOCK.  */
#define CLOCK_MAX (~((CLOCK)0))

#define vice_ptr_to_int(x) ((int)(long)(x))
#define vice_ptr_to_uint(x) ((unsigned int)(unsigned long)(x))
#define int_to_void_ptr(x) ((void *)(long)(x))
#define uint_to_void_ptr(x) ((void *)(unsigned long)(x))
