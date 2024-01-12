#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <concepts>
#include <bit>
#include <limits>
#include <array>
#include <cstdint>

namespace IG
{

template <class T>
concept BitSet =
	requires(T &&v)
	{
		~v;
		v | T{};
		v & T{};
		v ^ T{};
	};

template <class T>
constexpr inline auto bitSize = std::numeric_limits<T>::digits;

template <std::unsigned_integral T = unsigned>
[[nodiscard]]
constexpr T bit(int bitIdx)
{
	return T{1} << bitIdx;
}

template <std::unsigned_integral T = unsigned>
[[nodiscard]]
constexpr T bits(int numBits)
{
	return numBits ? std::numeric_limits<T>::max() >> (bitSize<T> - numBits) : 0;
}

[[nodiscard]]
constexpr auto clearBits(BitSet auto x, BitSet auto mask)
{
	return x & ~mask; // AND with the NOT of mask to unset
}

[[nodiscard]]
constexpr auto setOrClearBits(BitSet auto x, BitSet auto mask, bool condition)
{
	return condition ? (x | mask) : clearBits(x, mask);
}

[[nodiscard]]
constexpr auto updateBits(BitSet auto x, BitSet auto mask, BitSet auto updateMask)
{
	return clearBits(x, updateMask) | mask;
}

[[nodiscard]]
constexpr auto swapBits(std::integral auto x, std::integral auto range1, std::integral auto range2, std::integral auto rangeSize)
{
	auto t = ((x >> range1) ^ (x >> range2)) & ((1 << rangeSize) - 1); // XOR temporary
	return x ^ ((t << range1) | (t << range2));
}

[[nodiscard]]
constexpr bool isBitMaskSet(BitSet auto x, BitSet auto mask)
{
	return (x & mask) == mask; //AND mask, if the result equals mask, all bits match
}

[[nodiscard]]
constexpr auto addressAsBytes(auto &v)
{
	return std::bit_cast<std::array<uint8_t, sizeof(&v)>>(&v);
}

[[nodiscard]]
constexpr int ctz(unsigned int x)
{
	return __builtin_ctz(x);
}

[[nodiscard]]
constexpr int ctz(unsigned long x)
{
	return __builtin_ctzl(x);
}

[[nodiscard]]
constexpr int ctz(unsigned long long x)
{
	return __builtin_ctzll(x);
}

[[nodiscard]]
constexpr int clz(unsigned int x)
{
	return __builtin_clz(x);
}

[[nodiscard]]
constexpr int clz(unsigned long x)
{
	return __builtin_clzl(x);
}

[[nodiscard]]
constexpr int clz(unsigned long long x)
{
	return __builtin_clzll(x);
}

[[nodiscard]]
constexpr int fls(std::unsigned_integral auto x)
{
	return x ? sizeof(x) * 8 - clz(x) : 0;
}

// Utility functions for classes representing bit sets, define BitSetClassInt as the underlying int type
template <class T>
concept BitSetClass = requires {typename T::BitSetClassInt;};

template<BitSetClass T>
constexpr auto asInt(const T &val) {return std::bit_cast<typename T::BitSetClassInt>(val); }

template<BitSetClass T>
constexpr T operator~(T val)
{
	return std::bit_cast<T>(typename T::BitSetClassInt(~asInt(val)));
};

template<BitSetClass T>
constexpr T operator|(T lhs, T rhs)
{
	return std::bit_cast<T>(typename T::BitSetClassInt(asInt(lhs) | asInt(rhs)));
};

template<BitSetClass T>
constexpr T& operator|=(T &lhs, T rhs) { lhs = lhs | rhs; return lhs; }

template<BitSetClass T>
constexpr T operator&(T lhs, T rhs)
{
	return std::bit_cast<T>(typename T::BitSetClassInt(asInt(lhs) & asInt(rhs)));
};

template<BitSetClass T>
constexpr T& operator&=(T &lhs, T rhs) { lhs = lhs & rhs; return lhs; }

template<BitSetClass T>
constexpr T operator^(T lhs, T rhs)
{
	return std::bit_cast<T>(typename T::BitSetClassInt(asInt(lhs) ^ asInt(rhs)));
};

template<BitSetClass T>
constexpr T& operator^=(T &lhs, T rhs) { lhs = lhs ^ rhs; return lhs; }


}
