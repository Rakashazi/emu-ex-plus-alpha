#pragma once

#include <config/env.hh>

// Always use malloc-based memory allocator
#define CONFIG_MEM_DIRECT
#define CONFIG_MEMRANGE_MALLOC

#ifndef CONFIG_BASE_ANDROID
	#define CONFIG_GFX_SOFT_ORIENTATION 1
#endif

#include <config/defaultIncludes.h>
