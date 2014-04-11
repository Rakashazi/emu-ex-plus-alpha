#pragma once

#undef NOMINMAX
#define NOMINMAX 1
#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

// re-map or remove clashing macros
#undef DELETE
#define WINNT_DELETE 0x00010000L
#undef near
#undef far
