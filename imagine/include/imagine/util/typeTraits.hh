#pragma once

#include <type_traits>

namespace IG
{

template<typename T> struct dependentFalse : std::false_type {};

template <typename T>
inline constexpr bool dependentFalseValue = dependentFalse<T>::value;

}
