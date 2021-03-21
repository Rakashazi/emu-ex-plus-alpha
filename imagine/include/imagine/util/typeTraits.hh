#pragma once

#include <type_traits>

namespace IG
{

template<typename T> struct dependentFalse : std::false_type {};

template <typename T>
inline constexpr bool dependentFalseValue = dependentFalse<T>::value;

template <class T, T VALUE, class Tag = bool>
struct EmptyConstant
{
	// accept dummy assignment from any value
	constexpr EmptyConstant() {}
	template <class ...Args>
	constexpr EmptyConstant(Args&& ...) {}

	constexpr operator T() const { return VALUE; };
};

template <class T, class Tag = bool>
struct EmptyType
{
	// accept dummy assignment from any value
	constexpr EmptyType() {}
	template <class ...Args>
	constexpr EmptyType(Args&& ...) {}

	constexpr operator T() const { return {}; };
};

// selects either type T and an empty type that converts to T and returns VALUE,
// used in combination with [[no_unique_address]] and a unique Tag type to declare
// a class member that conditionally doesn't consume any space without using the C-preprocessor
template<bool CONDITION, class T, T VALUE, class Tag = bool>
using UseTypeIfOrConstant = std::conditional_t<CONDITION, T, EmptyConstant<T, VALUE, Tag>>;

#define IG_enableMemberIfOrConstant(c, t, v, name) \
	struct name ## EmptyTag{}; \
	[[no_unique_address]] IG::UseTypeIfOrConstant<(c), t, v, name ## EmptyTag> name

// same as above but always returns a default constructed value so class types can be used
template<bool CONDITION, class T, class Tag = bool>
using UseTypeIf = std::conditional_t<CONDITION, T, EmptyType<T, Tag>>;

#define IG_enableMemberIf(c, t, name) \
	struct name ## EmptyTag{}; \
	[[no_unique_address]] IG::UseTypeIf<(c), t, name ## EmptyTag> name

}

#define static_assertIsStandardLayout(type) static_assert(std::is_standard_layout_v<type>, #type " isn't standard-layout")
#define static_assertHasTrivialDestructor(type) static_assert(std::is_trivially_destructible_v<type>, #type " has non-trivial destructor")
