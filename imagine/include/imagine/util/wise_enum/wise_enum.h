#pragma once

#include "wise_enum_detail.h"
#include "wise_enum_generated.h"

#include <algorithm>
#include <array>
#include <type_traits>
#include <utility>

/*
 Macro interface

 The first argument to any macro enum is either the name of the enum, or a
 parenthesized pair, (name, storage). The first form results in unfixed
 underlying type for the enum, while the second form defines it explicitly.
 After that, the macros take a variadic enumerator list. Each entry in the list
 can be either a legal identifier name, or it can be a parenthesized pair where
 the first entry is the identifier and the second entry is the initializer.

 Usage notes:
   - A semi-colon after the macro invocation is not necessary, and your
     compiler may warn for it
*/

// Declare an enum at namespace scope
#define WISE_ENUM(name, ...) WISE_ENUM_IMPL(enum, name, , __VA_ARGS__)

// Declare an enum class at namespace scope
#define WISE_ENUM_CLASS(name, ...)                                             \
  WISE_ENUM_IMPL(enum class, name, , __VA_ARGS__)

// Declare an enum at class scope
#define WISE_ENUM_MEMBER(name, ...)                                            \
  WISE_ENUM_IMPL(enum, name, friend, __VA_ARGS__)

// Declare an enum class at class scope
#define WISE_ENUM_CLASS_MEMBER(name, ...)                                      \
  WISE_ENUM_IMPL(enum class, name, friend, __VA_ARGS__)

/*
 Adapt an existing enum into the wise enum API. This macro must be used at
 global scope. The first argument must be the name of the enum (qualified),
 followed by all the enumerators of the enum.
*/
#define WISE_ENUM_ADAPT(name, ...) WISE_ENUM_IMPL_ADAPT(name, __VA_ARGS__)

namespace wise_enum {

// Returns the string representation of an enumerator
template <class T>
constexpr string_type to_string(T t) {
  return wise_enum_detail_to_string(t, detail::Tag<T>{});
}

// Enumerators trait class. Each value is also available as a template variable
// for C++14 and on
template <class T>
struct enumerators {
  // For a given wise enum type, this variable allows iteration over enumerators
  // and their string names in the declared order. Each iterated object is a
  // struct with members { T value; string_type name; }
  static constexpr decltype(wise_enum_detail_array(detail::Tag<T>{})) range =
      wise_enum_detail_array(detail::Tag<T>{});

  // This variable is equal to the number of enumerators for the wise enum type.
  static constexpr std::size_t size = range.size();
};

template <class T>
constexpr decltype(
    wise_enum_detail_array(detail::Tag<T>{})) enumerators<T>::range;

template <class T>
constexpr std::size_t enumerators<T>::size;

#if __cplusplus >= 201402
template <class T>
constexpr auto &range = enumerators<T>::range;

template <class T>
constexpr std::size_t size = enumerators<T>::size;
#endif

// A type trait; this allows checking if a type is a wise_enum in generic code
template <class T>
using is_wise_enum = detail::is_wise_enum<T>;

#if __cplusplus >= 201402
template <class T>
static constexpr bool is_wise_enum_v = is_wise_enum<T>::value;
#endif

// Converts a string literal into a wise enum. Returns an optional<T>; if no
// enumerator has name matching the string, the optional is returned empty.
template <class T>
WISE_ENUM_CONSTEXPR_14 optional_type<T> from_string(string_type s) {
  auto it =
      std::find_if(enumerators<T>::range.begin(), enumerators<T>::range.end(),
                   [=](const detail::value_and_name<T> &x) {
                     return ::wise_enum::detail::compare(x.name, s);
                   });
  if (it == enumerators<T>::range.end())
    return {};

  return it->value;
}
} // namespace wise_enum
