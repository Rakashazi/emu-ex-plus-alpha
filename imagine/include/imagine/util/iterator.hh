#pragma once

namespace IG
{

template <class C>
static constexpr auto size(const C& c) -> decltype(c.size())
{
	return c.size();
}

template <class T, size_t N>
static constexpr size_t size(const T (&array)[N]) noexcept
{
	return N;
}

template <class C>
static constexpr auto data(C& c) -> decltype(c.data())
{
	return c.data();
}

template <class C>
static constexpr auto data(const C& c) -> decltype(c.data())
{
	return c.data();
}

template <class T, size_t N> constexpr T* data(T (&array)[N]) noexcept
{
	return array;
}

}
