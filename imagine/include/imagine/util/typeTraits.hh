#pragma once

#include <type_traits>
#include <tuple>

namespace IG
{

	namespace detail
	{
	enum class Enabler{};
	}

#ifdef __clang__
// TODO: remove work-around for Clang, needed as of version 3.4.2
constexpr detail::Enabler dummy = {};
#define ENABLE_IF_COND(...) IG::EnableIfCond< __VA_ARGS__ > = IG::dummy
#define DISABLE_IF_COND(...) IG::DisableIfCond< __VA_ARGS__ > = IG::dummy
#define ENABLE_IF_BOOL(...) IG::EnableIfBool< __VA_ARGS__ > = IG::dummy
#define DISABLE_IF_BOOL(...) IG::DisableIfBool< __VA_ARGS__ > = IG::dummy
#else
#define ENABLE_IF_COND(...) IG::EnableIfCond< __VA_ARGS__ >...
#define DISABLE_IF_COND(...) IG::DisableIfCond< __VA_ARGS__ >...
#define ENABLE_IF_BOOL(...) IG::EnableIfBool< __VA_ARGS__ >...
#define DISABLE_IF_BOOL(...) IG::DisableIfBool< __VA_ARGS__ >...
#endif

template<typename CONDITION>
using EnableIfCond = typename std::enable_if<CONDITION::value, detail::Enabler>::type;

template<typename CONDITION>
using DisableIfCond = typename std::enable_if<!CONDITION::value, detail::Enabler>::type;

template<bool value>
using EnableIfBool = typename std::enable_if<value, detail::Enabler>::type;

template<bool value>
using DisableIfBool = typename std::enable_if<!value, detail::Enabler>::type;

template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())>
{};

// specialize for pointers to member function
template <typename C, typename R, typename... ARGS>
struct function_traits<R(C::*)(ARGS...) const>
{
	enum { arity = sizeof...(ARGS) };

	typedef R result_type;

	template <size_t i>
	struct arg
	{
		typedef typename std::tuple_element<i, std::tuple<ARGS...>>::type type;
		// the i-th argument is equivalent to the i-th tuple element of a tuple
		// composed of those arguments.
	};
};

}
