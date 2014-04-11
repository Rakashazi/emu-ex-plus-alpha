#pragma once

#include <stddef.h>
#include <assert.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/builtins.h>
//#include <imagine/logger/logger.h>

CLINK void mem_init() ATTRS(cold);
CLINK void* mem_alloc(size_t size) ATTRS(malloc, alloc_size(1));
CLINK void* mem_calloc(size_t nelem, size_t size) ATTRS(malloc, alloc_size(1,2));
CLINK void* mem_realloc(void* buffer, size_t newSize) ATTRS(alloc_size(2));
CLINK void mem_free(void* buffer);

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
		return nullptr;
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
