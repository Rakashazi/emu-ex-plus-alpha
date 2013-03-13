#pragma once

#include <config/env.hh>

// Always use malloc-based memory allocator
#define CONFIG_MEM_DIRECT
#define CONFIG_MEMRANGE_MALLOC

#include <config/defaultIncludes.h>
