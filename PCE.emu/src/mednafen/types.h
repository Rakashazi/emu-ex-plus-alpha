#ifndef __MDFN_TYPES_H
#define __MDFN_TYPES_H

#ifdef HAVE_CONFIG_H
 #include <mednafen-config.h>
#endif

//
//
//

#if defined(__x86_64__) && defined(__code_model_large__)
 #error "Compiling with large memory model is not recommended, for performance reasons."
#endif
//
//
//

#include <stddef.h>
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#ifdef __cplusplus
#include <limits>
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <initializer_list>
#include <utility>
#include <memory>
#include <algorithm>
#include <string>
#include <vector>
#include <array>
#include <list>
#endif

#include <imagine/util/builtins.h>

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

#if !defined(HAVE_NATIVE64BIT) && (SIZEOF_VOID_P >= 8 || defined(__x86_64__))
#define HAVE_NATIVE64BIT 1
#endif

#if defined(__GNUC__) || defined(__clang__) || defined(__ICC) || defined(__INTEL_COMPILER)
 #define HAVE_COMPUTED_GOTO 1
#endif

#if defined(__clang__)
  //
  // Begin clang
  //
  #define MDFN_MAKE_CLANGV(maj,min,pl) (((maj)*100*100) + ((min) * 100) + (pl))
  #define MDFN_CLANG_VERSION	MDFN_MAKE_CLANGV(__clang_major__, __clang_minor__, __clang_patchlevel__)

  #define INLINE inline __attribute__((always_inline))
  #define NO_INLINE
  #define NO_CLONE

  #if defined(__386__) || defined(__i386__) || defined(__i386) || defined(_M_IX86) || defined(_M_I386)
    #define MDFN_FASTCALL __attribute__((fastcall))
  #else
    #define MDFN_FASTCALL
  #endif

  #define MDFN_FORMATSTR(a,b,c)
  #define MDFN_WARN_UNUSED_RESULT __attribute__ ((warn_unused_result))
  #define MDFN_NOWARN_UNUSED __attribute__((unused))

  #define MDFN_UNLIKELY(n) __builtin_expect((n) != 0, 0)
  #define MDFN_LIKELY(n) __builtin_expect((n) != 0, 1)

  #define MDFN_COLD __attribute__((cold))
  #define MDFN_HOT __attribute__((hot))

  #if MDFN_CLANG_VERSION >= MDFN_MAKE_CLANGV(3,6,0)
   #define MDFN_ASSUME_ALIGNED(p, align) ((decltype(p))__builtin_assume_aligned((p), (align)))
  #else
   #define MDFN_ASSUME_ALIGNED(p, align) (p)
  #endif
#elif defined(__GNUC__)
  //
  // Begin gcc
  //
  #define MDFN_MAKE_GCCV(maj,min,pl) (((maj)*100*100) + ((min) * 100) + (pl))
  #define MDFN_GCC_VERSION	MDFN_MAKE_GCCV(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)

  #define INLINE inline __attribute__((always_inline))
  #define NO_INLINE

  #if MDFN_GCC_VERSION >= MDFN_MAKE_GCCV(4,5,0)
   #define NO_CLONE __attribute__((noclone))
  #else
   #define NO_CLONE
  #endif

  #if MDFN_GCC_VERSION < MDFN_MAKE_GCCV(4,8,0)
   #define alignas(n) __attribute__ ((aligned (n)))	// Kludge for 4.7.x, remove eventually when 4.8+ are not so new.
  #endif

  //
  // Just avoid using fastcall with gcc before 4.1.0, as it(and similar regparm)
  // tend to generate bad code on the older versions(between about 3.1.x and 4.0.x, at least)
  //
  // http://gcc.gnu.org/bugzilla/show_bug.cgi?id=12236
  // http://gcc.gnu.org/bugzilla/show_bug.cgi?id=7574
  // http://gcc.gnu.org/bugzilla/show_bug.cgi?id=17025
  //
  #if MDFN_GCC_VERSION >= MDFN_MAKE_GCCV(4,1,0)
   #if defined(__386__) || defined(__i386__) || defined(__i386) || defined(_M_IX86) || defined(_M_I386)
     #define MDFN_FASTCALL __attribute__((fastcall))
   #else
     #define MDFN_FASTCALL
   #endif
  #else
   #define MDFN_FASTCALL
  #endif

  #define MDFN_FORMATSTR(a,b,c) __attribute__ ((format (a, b, c)))
  #define MDFN_WARN_UNUSED_RESULT __attribute__ ((warn_unused_result))
  #define MDFN_NOWARN_UNUSED __attribute__((unused))

  #define MDFN_UNLIKELY(n) __builtin_expect((n) != 0, 0)
  #define MDFN_LIKELY(n) __builtin_expect((n) != 0, 1)

  #if MDFN_GCC_VERSION >= MDFN_MAKE_GCCV(4,3,0)
   #define MDFN_COLD __attribute__((cold))
   #define MDFN_HOT __attribute__((hot))
  #else
   #define MDFN_COLD
   #define MDFN_HOT
  #endif

  #if MDFN_GCC_VERSION >= MDFN_MAKE_GCCV(4,7,0)
   #define MDFN_ASSUME_ALIGNED(p, align) ((decltype(p))__builtin_assume_aligned((p), (align)))
  #else
   #define MDFN_ASSUME_ALIGNED(p, align) (p)
  #endif
#elif defined(_MSC_VER)
  //
  // Begin MSVC
  //
  #pragma message("Compiling with MSVC, untested")

  #define INLINE __forceinline
  #define NO_INLINE __declspec(noinline)
  #define NO_CLONE

  #define MDFN_FASTCALL __fastcall

  #define MDFN_FORMATSTR(a,b,c)

  #define MDFN_WARN_UNUSED_RESULT

  #define MDFN_NOWARN_UNUSED

  #define MDFN_UNLIKELY(n) ((n) != 0)
  #define MDFN_LIKELY(n) ((n) != 0)

  #define MDFN_COLD
  #define MDFN_HOT

  #define MDFN_ASSUME_ALIGNED(p, align) (p)
#else
  #define INLINE inline
  #define NO_INLINE
  #define NO_CLONE

  #define MDFN_FASTCALL

  #define MDFN_FORMATSTR(a,b,c)

  #define MDFN_WARN_UNUSED_RESULT

  #define MDFN_NOWARN_UNUSED

  #define MDFN_UNLIKELY(n) ((n) != 0)
  #define MDFN_LIKELY(n) ((n) != 0)

  #define MDFN_COLD
  #define MDFN_HOT

  #define MDFN_ASSUME_ALIGNED(p, align) (p)
#endif

#ifndef FALSE
 #define FALSE 0
#endif

#ifndef TRUE
 #define TRUE 1
#endif

#if !defined(MSB_FIRST) && !defined(LSB_FIRST)
 #error "Define MSB_FIRST or LSB_FIRST!"
#elif defined(MSB_FIRST) && defined(LSB_FIRST)
 #error "Define only one of MSB_FIRST or LSB_FIRST, not both!"
#endif

#ifdef LSB_FIRST
 #define MDFN_IS_BIGENDIAN false
#else
 #define MDFN_IS_BIGENDIAN true
#endif

#ifdef __cplusplus
template<typename T> typename std::remove_all_extents<T>::type* MDAP(T* v) { return (typename std::remove_all_extents<T>::type*)v; }
#include "error.h"
#include "math_ops.h"
#include "endian.h"
#endif

#endif
