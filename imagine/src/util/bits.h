#pragma once

#include <util/ansiTypes.h>
#include <util/builtins.h>
#include <assert.h>

// bit mask macro for use in the functions below, passing 0 results in the first bit and so on
// OR them together for operations on multiple bits
#ifdef BIT
	#warning "Overriding existing BIT(x) macro"
	#undef BIT
#endif
#define BIT(x) ( 1 << (x) )

#ifdef __cplusplus

// returns a bit mask with the first [numBits] set to one
// same as (2 exponent [numBits]) - 1
template <class T>
constexpr static T bit_fullMask(uint numBits)
{
	//assert(numBits <= sizeof(T)*8);
	return (T)(~0) >> ((sizeof(T)*8) - numBits);
}

template <class T, class T2>
static void setBits(T &value, T2 bitMask) { value |= bitMask; } //OR bitMask to set

template <class T, class T2>
static void toggleBits(T &value, T2 bitMask) { value ^= bitMask; } //XOR bitMask to toggle

template <class T, class T2>
static void unsetBits(T &value, T2 bitMask) { value &= ~bitMask; } //AND with the NOT of bitMask to unset

template <class T, class T2>
static void updateBits(T &value, T2 bitMask, T2 updateMask)
{
	unsetBits(value, updateMask);
	setBits(value, bitMask);
}

template <class T>
static T swapBits(T val, uint range1, uint range2, uint rangeSize)
{
	T x = ((val >> range1) ^ (val >> range2)) & ((1U << rangeSize) - 1); // XOR temporary
	return val ^ ((x << range1) | (x << range2));
}

namespace Bits
{

template <class T>
struct InType
{
	static constexpr uint bits = sizeof(T) * 8;
};

// num elements of integer array needed to hold bits
template <class T>
static constexpr uint elemsToHold(uint bits)
{
	return (bits + InType<T>::bits - 1) / InType<T>::bits;
}

template <class T, size_t S>
static bool isSetInArray(const T (&arr)[S], uint bit)
{
    return !!(arr[bit / InType<T>::bits] & ((T)1 << (bit % InType<T>::bits)));
}

}

#endif

static uint bit_isMaskSet(uint value, uint bitMask)
{
	return (value & bitMask) == bitMask; //AND bitMask, if the result equals bitMask, all bits match
}

static uint bit_isAtLeastOneSet(uint value, uint bitMask)
{
	return value & bitMask; //AND bitMask, if the result is not zero, at least one bit matched
}

static uint bit_trailingZeros(uint32 num)
{
	const uint mod37BitPosition[] = // maps a bit value mod 37 to its position
	{
		32, 0, 1, 26, 2, 23, 27, 0, 3, 16, 24, 30, 28, 11, 0, 13, 4,
		7, 17, 0, 25, 22, 31, 15, 29, 10, 12, 6, 0, 21, 14, 9, 5,
		20, 8, 19, 18
	};

	return mod37BitPosition[(-num & num) % 37];
}

/*static void updateBitRange(uint *value, uint rangeMask, uint update)
{
	unsetBits(*value, rangeMask);
	*value |= update << bit_trailingZeros(rangeMask);
	//printf("Bits: set 0x%X with range mask 0x%X, update was offset by %d bits\n", *value, rangeMask, bit_trailingZeros(rangeMask));
}

#define getBitRange(value, rangeMask) ((value & (rangeMask)) >> bit_trailingZeros(rangeMask))*/

static uint bit_numSet(uint num)
{
	uint bits;
	for(bits = 0; num; bits++)
	{
		num &= num - 1; // clear the least significant bit set
	}
	return bits;
}
