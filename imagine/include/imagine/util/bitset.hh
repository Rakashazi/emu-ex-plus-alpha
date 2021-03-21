#pragma once

#include <limits>
#include <type_traits>

namespace IG
{

template <class T>
static constexpr unsigned bitSize = std::numeric_limits<T>::digits;

template <class T = unsigned>
constexpr static T bit(unsigned bitIdx)
{
	static_assert(std::is_unsigned_v<T>, "expected unsigned type");
	return (T)1 << bitIdx;
}

template <class T = unsigned>
constexpr static T bits(unsigned numBits)
{
	static_assert(std::is_unsigned_v<T>, "expected unsigned type");
	return std::numeric_limits<T>::max() >> (bitSize<T> - numBits);
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

template <class T>
constexpr static bool isBitMaskSet(T x, T mask)
{
	return (x & mask) == mask; //AND mask, if the result equals mask, all bits match
}

}
