#pragma once

#include <type_traits>

namespace IG
{

template<typename T> struct dependentFalse : std::false_type {};

template <typename T>
inline constexpr bool dependentFalseValue = dependentFalse<T>::value;

template <class T, T VALUE = T{}>
struct EmptyConstant
{
	// accept dummy assignment from any value
	template <class ...Args>
	constexpr EmptyConstant(Args ...) {}

	constexpr operator T() const { return VALUE; };
};

// selects either type T and an empty type that converts to T and returns VALUE,
// used in combination with [[no_unique_address]] to declare and class member
// that conditionally doesn't consume any space without using the C-preprocessor
template<bool CONDITION, class T, T VALUE = T{}>
using UseTypeIf = std::conditional_t<CONDITION, T, EmptyConstant<T, VALUE>>;

}

#define static_assertIsStandardLayout(type) static_assert(std::is_standard_layout_v<type>, #type " isn't standard-layout")
#define static_assertHasTrivialDestructor(type) static_assert(std::is_trivially_destructible_v<type>, #type " has non-trivial destructor")
