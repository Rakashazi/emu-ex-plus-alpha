#pragma once

#include <stddef.h>

// C-Language helper macros

// make a copy of <var> named <name>, automatically setting the type
#define var_copy(name, val) typeof(val) name = val

// make a reference of <var> named <name>, automatically setting the type
#define var_ref(name, val) typeof(val)& name = val

// assign variable to class member variable of the same name
#define var_selfSet(name) this->name = name;
#define var_selfs(name) var_selfSet(name)

// integer iteration
// from <start> to <end>
#define iterateRange(start, end, iterator) for(var_copy(iterator, start); iterator <= (end); iterator++)
// 0 to <counts> - 1, i_s caches the value of counts (type of value is maintained)
#define iterateTimesTyped(counts, i) for(typeof (counts) i = 0, i ## _s = counts; i < (i ## _s); i++)
// 0 to <counts> - 1, i_s caches the value of counts
#define iterateTimes(counts, i) for(unsigned int i = 0, i ## _s = counts; i < (i ## _s); i++)
// <limit> - 1 to 0
#define iterateTimesRev(limit, i) for(int i = (limit)-1; i >= 0; i--)

#ifdef __cplusplus

template <class T, size_t S>
static size_t sizeofArray(const T (&a)[S])
{
	return S;
}

// static sized array iteration
// when added after a for-loop initializer, makes sure the loop runs only once
#define forLoopExecOnceDummy *DMY_VAR = NULL; DMY_VAR == NULL; DMY_VAR++

// iterates over elements of 'a' with the current element pointer label 'e'
// array number of elements can be accessed as 'e'_s
// array index can be accessed as 'e'_i
#define forEachInArray(a, e) \
for(typeof(*a) *e = a, forLoopExecOnceDummy) \
for(typeof(sizeofArray(a)) e ## _i = 0, e ## _s = sizeofArray(a); e ## _i < e ## _s; e ## _i++, e++ )

// iterates over elements of 'a' with the current element dereferenced label 'e'
// array number of elements can be accessed as 'e'_s
// array index can be accessed as 'e'_i, current element pointer label 'e'_p

// Old version, can't handle const types
/*#define forEachDInArray(a, e) \
for(typeof(*a) e = *a, forLoopExecOnceDummy) \
for(typeof(*a) *e ## _p  = a, forLoopExecOnceDummy) \
for(typeof(sizeofArray(a)) e ## _i = 0, e ## _s = sizeofArray(a); e ## _i < e ## _s; e ## _i++, e ## _p++, e = *e ## _p )*/

#define forEachDInArray(a, e) \
for(size_t e ## _s  = sizeofArray(a), forLoopExecOnceDummy) \
iterateTimes(sizeofArray(a), e ## _i) \
for(typeof(*a) e = a[e ## _i], forLoopExecOnceDummy)

template <class T, size_t S>
static bool equalsAny(const T val, const T (&possible)[S])
{
	iterateTimes(S, i)
	{
		if(val == possible[i])
			return 1;
	}
	return 0;
}

template <class T, class S>
static bool equalsAny(const T val, const T possible[], S num)
{
	iterateTimes(num, i)
	{
		if(val == possible[i])
			return 1;
	}
	return 0;
}

// Increments [counter] once per call until it equals [val],
// then returns 1 and resets counter to 0.
// Can replace code such as:
// if(counter == 10) { doFunction(); counter = 0; } else counter++;
// with:
// countToValueLooped(counter, 10) { doFunction(); }
template <class T>
static bool countToValueLooped(T &counter, T val)
{
	if(counter == val)
	{
		counter = 0;
		return 1;
	}
	else
	{
		counter++;
		return 0;
	}
}

// Use when supplying template parameters with a [type] + [non-type] pair,
// such as template<class T, T foo> class Bar { }, and avoid having to specify the type.
// const int x = 1; Bar<template_ntype(x)> bar;
#define template_ntype(var) typeof(var), var

#endif
