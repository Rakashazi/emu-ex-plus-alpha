#pragma once

#include <config.h>
#include <stddef.h>
#include <util/ansiTypes.h>
#include <assert.h>
#include <util/algorithm.h>
#include <logger/interface.h>

#ifdef CONFIG_MEM_DIRECT
#include <memrange/interface.h>
static void mem_init() { }

static void* mem_alloc(size_t size) ATTRS(malloc, alloc_size(1));
static void* mem_alloc(size_t size) { return memrange_alloc(size); }

static void* mem_calloc(size_t nelem, size_t size) ATTRS(malloc, alloc_size(1,2));
static void* mem_calloc(size_t nelem, size_t size) { return memrange_calloc(nelem, size); }

static void* mem_realloc(void* buffer, size_t newSize) ATTRS(alloc_size(2));
static void* mem_realloc(void* buffer, size_t newSize) { return memrange_realloc(buffer, newSize); }

static void mem_free(void* buffer) { memrange_free(buffer); }
#else
void mem_init() ATTRS(cold);
void* mem_alloc(size_t size) ATTRS(malloc, alloc_size(1));
void* mem_calloc(size_t nelem, size_t size) ATTRS(malloc, alloc_size(1,2));
void* mem_realloc(void* buffer, size_t newSize) ATTRS(alloc_size(2));
void mem_free(void* buffer);
#endif

static void mem_freeSafe(void* buffer) { if(buffer != NULL) mem_free(buffer); }

#ifdef __cplusplus

static void* mem_calloc(size_t size) ATTRS(malloc, alloc_size(1));
static void* mem_calloc(size_t size) { return mem_calloc(1, size); }

#include <new>

// TODO: this only works if the class has a trivial destructor
template <class T>
static T* mem_newRealloc(T* o, size_t newCount)
{
	static_assertHasTrivialDestructor(T);
	T *newPtr = static_cast<T*>(::mem_realloc(o, newCount * sizeof(T)));
	if(!newPtr)
	{
		logWarn("realloc failed");
	}

	if(!__has_trivial_constructor(T))
	{
		//logWarn("run realloc constructors");
		iterateTimes(newCount, i)
		{
			new (static_cast<void*>(&newPtr[i])) T();
		}
	}

	return newPtr;

	// generic version
	/*if(o)
	{
		delete[] o;
	}
	return new T[newSize];*/
}

#endif
