#pragma once

#include <type_traits>
#include <tuple>

namespace IG
{

template <typename T>
struct functionTraits : public functionTraits<decltype(&T::operator())>
{};

// specialize for pointers to member function
template <typename C, typename R, typename... ARGS>
struct functionTraits<R(C::*)(ARGS...) const>
{
	enum { arity = sizeof...(ARGS) };

	typedef R resultType;

	template <size_t i>
	struct arg
	{
		typedef typename std::tuple_element<i, std::tuple<ARGS...>>::type type;
		// the i-th argument is equivalent to the i-th tuple element of a tuple
		// composed of those arguments.
	};
};

template <typename T>
using functionTraitsRType = typename functionTraits<T>::resultType;

template <typename T>
constexpr size_t functionTraitsArity = functionTraits<T>::arity;

}
