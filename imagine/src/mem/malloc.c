#include <imagine/mem/mem.h>
#include <stdlib.h>

void mem_init() {}

void* mem_alloc(size_t size) { return malloc(size); }

void* mem_calloc(size_t nelem, size_t size) { return calloc(nelem, size); }

void* mem_realloc(void* buffer, size_t newSize) { return realloc(buffer, newSize); }

void mem_free(void* buffer) { free(buffer); }
