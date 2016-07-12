#pragma once

#ifdef __cplusplus
#include <utility>
#include <cstddef>
#endif

// make a copy of [var] named [name], automatically setting the type
#define var_copy(name, val) typeof(val) name = val
#define var_isConst(E) __builtin_constant_p(E)

#define static_assertIsPod(type) static_assert(__is_pod(type), #type " isn't POD")
#define static_assertHasTrivialDestructor(type) static_assert(__has_trivial_destructor(type), #type " has non-trivial destructor")

#define bcase break; case
#define bdefault break; default

#define likely(E) __builtin_expect(!!(E), 1)
#define unlikely(E) __builtin_expect(!!(E), 0)

#define PP_STRINGIFY(A) #A
#define PP_STRINGIFY_EXP(A) PP_STRINGIFY(A)

// logical xor
#define lxor(a, b) ( !(a) != !(b) )

#ifdef __cplusplus

// Use when supplying template parameters with a [type] + [non-type] pair,
// such as template<class T, T foo> class Bar { }, and avoid having to specify the type.
// const int x = 1; Bar<template_ntype(x)> bar;
#define template_ntype(var) typeof(var), var

namespace IG
{

// make and return a copy of the variable, clearing the original
template <class T>
static T moveAndClear(T &v)
{
	auto temp = std::move(v);
	v = {};
	return temp;
}

template<class T>
constexpr static void swap(T& a, T& b)
{
	T tmp = std::move(a);
	a = std::move(b);
	b = std::move(tmp);
}

}

#endif
