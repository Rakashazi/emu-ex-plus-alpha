#pragma once

#include <stdint.h>

// Types without strict aliasing
typedef uint8_t __attribute__((__may_alias__)) uint8a;
typedef int8_t __attribute__((__may_alias__)) int8a;
typedef uint16_t __attribute__((__may_alias__)) uint16a;
typedef int16_t __attribute__((__may_alias__)) int16a;
typedef uint32_t __attribute__((__may_alias__)) uint32a;
typedef int32_t __attribute__((__may_alias__)) int32a;
typedef uint64_t __attribute__((__may_alias__)) uint64a;
typedef int64_t __attribute__((__may_alias__)) int64a;
