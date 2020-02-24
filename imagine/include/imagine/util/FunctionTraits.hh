#pragma once

#include <type_traits>
#include <tuple>

namespace IG
{

template <typename T>
struct FunctionTraits : public FunctionTraits<decltype(&std::decay_t<T>::operator())>
{};

// specialize for pointers to member function
template <typename C, typename R, typename... Args>
struct FunctionTraits<R(C::*)(Args...) const>
{
	enum { arity = sizeof...(Args) };

	typedef R Result;

	template <size_t i>
	struct Arg
	{
		typedef typename std::tuple_element_t<i, std::tuple<Args...>> type;
		// the i-th argument is equivalent to the i-th tuple element of a tuple
		// composed of those arguments.
	};
};

template <typename T>
using FunctionTraitsR = typename FunctionTraits<T>::Result;

template <typename T, size_t i>
using FunctionTraitsArg = typename FunctionTraits<T>::template Arg<i>::type;

template <typename T>
inline constexpr size_t functionTraitsArity = FunctionTraits<T>::arity;

}
