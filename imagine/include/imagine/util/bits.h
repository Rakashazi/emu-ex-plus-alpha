#pragma once

#include <assert.h>
#include <stddef.h>
#ifdef __APPLE__
#include <strings.h>
#endif

#ifndef __APPLE__
static int fls(int x)
{
	return x ? sizeof(x) * 8 - __builtin_clz(x) : 0;
}

static int flsl(long x)
{
	return x ? sizeof(x) * 8 - __builtin_clzl(x) : 0;
}
#endif

#ifdef __cplusplus
namespace IG
{

template <class T>
constexpr static T bit(T bitIdx)
{
	return 1 << bitIdx;
}

// returns a bit set with the first [numBits] set to one
// same as (2 exponent [numBits]) - 1
template <class T>
constexpr static T makeFullBits(unsigned int numBits)
{
	//assert(numBits <= sizeof(T)*8);
	return (T)(~0) >> ((sizeof(T)*8) - numBits);
}

template <class T>
constexpr static T setBits(T x, T mask)
{
	return x | mask; // OR mask to set
}

template <class T>
constexpr static T clearBits(T x, T mask)
{
	return x & ~mask; // AND with the NOT of mask to unset
}

template <class T>
constexpr static T setOrClearBits(T x, T mask, bool condition)
{
	return condition ? setBits(x, mask) : clearBits(x, mask);
}

template <class T>
constexpr static T flipBits(T x, T mask)
{
	return x ^ mask; // XOR mask to flip
}

template <class T>
constexpr static T updateBits(T x, T mask, T updateMask)
{
	return setBits(clearBits(x, updateMask), mask);
}

template <class T>
static T swapBits(T x, T range1, T range2, unsigned int rangeSize)
{
	T t = ((x >> range1) ^ (x >> range2)) & ((1 << rangeSize) - 1); // XOR temporary
	return x ^ ((t << range1) | (t << range2));
}

constexpr static int bitsSet(unsigned int x)
{
	return __builtin_popcount(x);
	// alternate generic version
	/*uint bits;
	for(bits = 0; x; bits++)
	{
		x &= x - 1; // clear the least significant bit set
	}
	return bits;*/
}

constexpr static int bitsSet(unsigned long x)
{
	return __builtin_popcountl(x);
}

constexpr static int bitsSet(unsigned long long x)
{
	return __builtin_popcountll(x);
}

template <class T>
constexpr static bool isBitMaskSet(T x, T mask)
{
	return (x & mask) == mask; //AND mask, if the result equals mask, all bits match
}

constexpr static int ctz(unsigned int x)
{
	return __builtin_ctz(x);
}

constexpr static int ctz(unsigned long x)
{
	return __builtin_ctzl(x);
}

constexpr static int ctz(unsigned long long x)
{
	return __builtin_ctzll(x);
}

}

namespace Bits
{

template <class T>
struct InType
{
	static constexpr unsigned int bits = sizeof(T) * 8;
};

// num elements of integer array needed to hold bits
template <class T>
static constexpr unsigned int elemsToHold(unsigned int bits)
{
	return (bits + InType<T>::bits - 1) / InType<T>::bits;
}

template <class T, size_t S>
static bool isSetInArray(const T (&arr)[S], unsigned int bit)
{
    return !!(arr[bit / InType<T>::bits] & ((T)1 << (bit % InType<T>::bits)));
}

}
#else
	#ifdef BIT
	#warning "Overriding existing BIT(x) macro"
	#undef BIT
	#endif
// macro-based for use in C
#define BIT(x) ( 1 << (x) )
#endif
