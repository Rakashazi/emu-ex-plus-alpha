#pragma once

#include <array>
#include <type_traits>
#include <utility>

// optional type needed for interface
#ifndef WISE_ENUM_OPTIONAL_TYPE
#if __cplusplus >= 201703L
#include <optional>
namespace wise_enum {
template <class T>
using optional_type = std::optional<T>;
}
#else
#include "optional.h"
namespace wise_enum {
template <class T>
using optional_type = wise_enum::optional<T>;
}
#endif
#else
namespace wise_enum {
template <class T>
using optional_type = WISE_ENUM_OPTIONAL_TYPE<T>;
}
#endif

// Choice of string_view if type defined, otherwise use string literal
#ifndef WISE_ENUM_STRING_TYPE
#if __cplusplus >= 201703L
#include <string_view>
namespace wise_enum {
using string_type = std::string_view;
}
#else
namespace wise_enum {
using string_type = const char *;
}
#endif
#else
namespace wise_enum {
using string_type = WISE_ENUM_STRING_TYPE;
}
#endif

#if __cplusplus == 201103
#define WISE_ENUM_CONSTEXPR_14
#else
#define WISE_ENUM_CONSTEXPR_14 constexpr
#endif

namespace wise_enum {
namespace detail {

template <class T>
struct value_and_name {
  T value;
  string_type name;
};

template <class T>
struct Tag {};

constexpr void wise_enum_detail_array(...);

template <class T>
struct is_wise_enum
    : std::integral_constant<
          bool, !std::is_same<void, decltype(wise_enum_detail_array(
                                        Tag<T>{}))>::value> {};

WISE_ENUM_CONSTEXPR_14 inline int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2))
    s1++, s2++;
  if (*s1 < *s2) {
    return -1;
  }
  if (*s1 > *s2) {
    return 1;
  } else {
    return 0;
  }
}

WISE_ENUM_CONSTEXPR_14 inline bool compare(const char *s1, const char *s2) {
  return strcmp(s1, s2) == 0;
}

template <class U, class = typename std::enable_if<
                       !std::is_same<U, const char *>::value>::type>
WISE_ENUM_CONSTEXPR_14 bool compare(U u1, U u2) {
  return u1 == u2;
}
} // namespace detail
} // namespace wise_enum


// Needed for expansion of variadic macro arguments in MSVC
// MSVC expands __VA_ARGS__ after passing it, while gcc expands it before
#define WISE_ENUM_IMPL_EXPAND(x) x

#define WISE_ENUM_IMPL_NARG(...)                                               \
  WISE_ENUM_IMPL_NARG_(__VA_ARGS__, WISE_ENUM_IMPL_RSEQ_N())
#define WISE_ENUM_IMPL_NARG_(...) WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_ARG_N(__VA_ARGS__))

// ARG_N and RSEQ_N defined in wise_enum_generated.h

// Building blocks; credit to:
// https://github.com/pfultz2/Cloak/wiki/C-Preprocessor-tricks,-tips,-and-idioms

#define WISE_ENUM_IMPL_COMMA() ,
#define WISE_ENUM_IMPL_NOTHING()

#define WISE_ENUM_IMPL_CAT(a, ...) WISE_ENUM_IMPL_PRIMITIVE_CAT(a, __VA_ARGS__)
#define WISE_ENUM_IMPL_PRIMITIVE_CAT(a, ...) a##__VA_ARGS__

#define WISE_ENUM_IMPL_XSTR(s) #s
#define WISE_ENUM_IMPL_STR(s) WISE_ENUM_IMPL_XSTR(s)

#define WISE_ENUM_IMPL_IIF(c) WISE_ENUM_IMPL_CAT(WISE_ENUM_IMPL_IIF_, c)
#define WISE_ENUM_IMPL_IIF_0(t, f) f
#define WISE_ENUM_IMPL_IIF_1(t, f) t

#define WISE_ENUM_IMPL_CHECK_N(x, n, ...) n
#define WISE_ENUM_IMPL_CHECK(...)                                              \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_CHECK_N(__VA_ARGS__, 0, ))
#define WISE_ENUM_IMPL_PROBE(x) x, 1,

#define WISE_ENUM_IMPL_IS_PAREN(x)                                             \
  WISE_ENUM_IMPL_CHECK(WISE_ENUM_IMPL_IS_PAREN_PROBE x)
#define WISE_ENUM_IMPL_IS_PAREN_PROBE(...) WISE_ENUM_IMPL_PROBE(~)

#define WISE_ENUM_IMPL_FIRST_ARG(x, ...) x

#define WISE_ENUM_IMPL_ONLY_OR_FIRST(x)                                        \
  WISE_ENUM_IMPL_IIF(WISE_ENUM_IMPL_IS_PAREN(x))                               \
  (WISE_ENUM_IMPL_FIRST_ARG x, x)

// Use building blocks to conditionally process enumerators; they can either be
// just an identifier, or (identifier, value)
#define WISE_ENUM_IMPL_ENUM_INIT_2(x, ...) x = __VA_ARGS__
#define WISE_ENUM_IMPL_ENUM_INIT(name, x)                                      \
  WISE_ENUM_IMPL_IIF(WISE_ENUM_IMPL_IS_PAREN(x))                               \
  (WISE_ENUM_IMPL_ENUM_INIT_2 x, x)

#define WISE_ENUM_IMPL_ENUM_STR(x)                                             \
  WISE_ENUM_IMPL_STR(WISE_ENUM_IMPL_ONLY_OR_FIRST(x))

#define WISE_ENUM_IMPL_DESC_PAIR(name, x)                                      \
  { name::WISE_ENUM_IMPL_ONLY_OR_FIRST(x), WISE_ENUM_IMPL_ENUM_STR(x) }

#define WISE_ENUM_IMPL_SWITCH_CASE(name, x)                                    \
  case name::WISE_ENUM_IMPL_ONLY_OR_FIRST(x):                                  \
    return WISE_ENUM_IMPL_ENUM_STR(x);

#define WISE_ENUM_IMPL_STORAGE_2(x, y) y

#define WISE_ENUM_IMPL_STORAGE(x)                                              \
  WISE_ENUM_IMPL_IIF(WISE_ENUM_IMPL_IS_PAREN(x))                               \
  ( : WISE_ENUM_IMPL_STORAGE_2 x, )

#define WISE_ENUM_IMPL(type, name_storage, friendly, ...)                      \
  WISE_ENUM_IMPL_2(type, WISE_ENUM_IMPL_ONLY_OR_FIRST(name_storage),           \
                   WISE_ENUM_IMPL_STORAGE(name_storage), friendly,             \
                   WISE_ENUM_IMPL_NARG(__VA_ARGS__), __VA_ARGS__)

#define WISE_ENUM_IMPL_2(type, name, storage, friendly, num_enums, ...)        \
  WISE_ENUM_IMPL_3(type, name, storage, friendly, num_enums,                   \
                   WISE_ENUM_IMPL_CAT(WISE_ENUM_IMPL_LOOP_, num_enums),        \
                   __VA_ARGS__)


#define WISE_ENUM_IMPL_3(type, name, storage, friendly, num_enums, loop, ...)  \
  type name storage{                                                           \
      WISE_ENUM_IMPL_EXPAND(loop(WISE_ENUM_IMPL_ENUM_INIT, _,                  \
                             WISE_ENUM_IMPL_COMMA, __VA_ARGS__))};             \
  WISE_ENUM_IMPL_ADAPT_3(name, friendly, num_enums, loop, __VA_ARGS__)

#define WISE_ENUM_IMPL_ADAPT(name, ...)                                        \
  namespace wise_enum {                                                        \
  namespace detail {                                                           \
  WISE_ENUM_IMPL_ADAPT_2(name, WISE_ENUM_IMPL_NARG(__VA_ARGS__), __VA_ARGS__)  \
  }                                                                            \
  }

#define WISE_ENUM_IMPL_ADAPT_2(name, num_enums, ...)                           \
  WISE_ENUM_IMPL_ADAPT_3(name, , num_enums,                                    \
                         WISE_ENUM_IMPL_CAT(WISE_ENUM_IMPL_LOOP_, num_enums),  \
                         __VA_ARGS__)

#define WISE_ENUM_IMPL_ADAPT_3(name, friendly, num_enums, loop, ...)           \
  friendly constexpr std::array<::wise_enum::detail::value_and_name<name>,     \
                                num_enums>                                     \
  wise_enum_detail_array(::wise_enum::detail::Tag<name>) {                     \
    return {{WISE_ENUM_IMPL_EXPAND(loop(WISE_ENUM_IMPL_DESC_PAIR, name,        \
                  WISE_ENUM_IMPL_COMMA, __VA_ARGS__))}};                       \
  }                                                                            \
                                                                               \
  template <class T>                                                           \
  friendly WISE_ENUM_CONSTEXPR_14 ::wise_enum::string_type                     \
  wise_enum_detail_to_string(T e, ::wise_enum::detail::Tag<name>) {            \
    switch (e) {                                                               \
      WISE_ENUM_IMPL_EXPAND(loop(WISE_ENUM_IMPL_SWITCH_CASE, name,             \
           WISE_ENUM_IMPL_NOTHING, __VA_ARGS__))                               \
    }                                                                          \
    return {};                                                                 \
  }
