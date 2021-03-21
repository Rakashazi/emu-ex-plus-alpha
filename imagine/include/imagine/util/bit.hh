#pragma once

#include <bit>

namespace IG
{

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

constexpr static int clz(unsigned int x)
{
	return __builtin_clz(x);
}

constexpr static int clz(unsigned long x)
{
	return __builtin_clzl(x);
}

constexpr static int clz(unsigned long long x)
{
	return __builtin_clzll(x);
}

template <class T>
constexpr static int fls(T x)
{
	static_assert(std::is_unsigned_v<T>, "expected unsigned parameter");
	return x ? sizeof(x) * 8 - clz(x) : 0;
}

}
