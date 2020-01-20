#pragma once

#include <stddef.h>
#include <imagine/util/builtins.h>

CLINK void mem_init() __attribute__((cold));
CLINK void* mem_alloc(size_t size) __attribute__((malloc, alloc_size(1)));
CLINK void* mem_calloc(size_t nelem, size_t size) __attribute__((malloc, alloc_size(1,2)));
CLINK void* mem_realloc(void* buffer, size_t newSize) __attribute__((alloc_size(2)));
CLINK void mem_free(void* buffer);
static void mem_freeSafe(void* buffer) { if(buffer != NULL) mem_free(buffer); }

#ifdef __cplusplus

[[gnu::malloc, gnu::alloc_size(1)]] static void* mem_calloc(size_t size);
static void* mem_calloc(size_t size) { return mem_calloc(1, size); }

#endif
