#pragma once

#include <stdlib.h>

static const uint memrange_builtinRanges = 0;
static void memrange_builtinRange(uint index, Memrange* range) { }
static const uint memrange_canAlloc = 1;

static void* memrange_alloc(size_t amount) ATTRS(malloc, alloc_size(1));
static void* memrange_alloc(size_t amount) { return malloc(amount); }

static void* memrange_calloc(size_t nelem, size_t amount) ATTRS(malloc, alloc_size(1,2));
static void* memrange_calloc(size_t nelem, size_t amount) { return calloc(nelem, amount); }

static void* memrange_realloc(void *range, size_t amount) ATTRS(alloc_size(2));
static void* memrange_realloc(void *range, size_t amount) { return realloc(range, amount); }

static void memrange_free(void *range) { free(range); }
