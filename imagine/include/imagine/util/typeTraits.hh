#pragma once

#include <type_traits>
#include <tuple>

#define ENABLE_IF_EXPR(...) std::enable_if_t< __VA_ARGS__ >* = nullptr
#define DISABLE_IF_EXPR(...) std::enable_if_t< !(__VA_ARGS__) >* = nullptr

// TODO: remove when enabling C++17
namespace std
{

template<class T, class U>
constexpr bool is_same_v = is_same<T, U>::value;

template<class From, class To>
constexpr bool is_convertible_v = is_convertible<From, To>::value;

template<class T>
constexpr bool is_integral_v = is_integral<T>::value;

template<class T>
constexpr bool is_floating_point_v = is_floating_point<T>::value;

template<class T>
constexpr bool is_unsigned_v = is_unsigned<T>::value;

template<class T>
constexpr bool is_arithmetic_v = is_arithmetic<T>::value;

}

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
