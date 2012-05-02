#pragma once

#include <config.h>
#include <util/ansiTypes.h>

typedef struct
{
	void* addr;
	size_t size;
} Memrange;

#ifdef CONFIG_MEMRANGE_MALLOC
	#include <memrange/malloc/malloc.h>
#else
	const uint memrange_builtinRanges;
	void memrange_builtinRange(uint index, Memrange* range);
	const uint memrange_canAlloc;
	void* memrange_alloc(size_t amount) ATTRS(malloc, alloc_size(1));
	void* memrange_calloc(size_t nelem, size_t amount) ATTRS(malloc, alloc_size(1,2));
	void* memrange_realloc(void *range, size_t amount) ATTRS(alloc_size(2));
	void memrange_free(void *range);
#endif
