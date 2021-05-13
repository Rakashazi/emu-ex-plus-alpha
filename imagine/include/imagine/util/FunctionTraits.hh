#pragma once

namespace IG
{

template <typename T>
struct FunctionTraits : public FunctionTraits<decltype(&T::operator())> {};

template <typename C, typename R, typename... Args>
struct FunctionTraits<R(C::*)(Args...) const>
{
	using Result = R;
	static constexpr unsigned arity = sizeof...(Args);
};

template <typename T>
using FunctionTraitsR = typename FunctionTraits<T>::Result;

template <typename T>
inline constexpr unsigned functionTraitsArity = FunctionTraits<T>::arity;

}
