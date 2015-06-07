//============================================================================
//
//  BBBBB    SSSS   PPPPP   FFFFFF
//  BB  BB  SS  SS  PP  PP  FF
//  BB  BB  SS      PP  PP  FF
//  BBBBB    SSSS   PPPPP   FFFF    --  "Brad's Simple Portability Framework"
//  BB  BB      SS  PP      FF
//  BB  BB  SS  SS  PP      FF
//  BBBBB    SSSS   PP      FF
//
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: bspf.hxx 3131 2015-01-01 03:49:32Z stephena $
//============================================================================

#ifndef BSPF_HXX
#define BSPF_HXX

/**
  This file defines various basic data types and preprocessor variables
  that need to be defined for different operating systems.

  @author Bradford W. Mott
  @version $Id: bspf.hxx 3131 2015-01-01 03:49:32Z stephena $
*/

#include <cstdint>
// Types for 8-bit signed and unsigned integers
using Int8  = int8_t;
using uInt8 = uint8_t;
// Types for 16-bit signed and unsigned integers
using Int16  = int16_t;
using uInt16 = uint16_t;
// Types for 32-bit signed and unsigned integers
using Int32  = int32_t;
using uInt32 = uint32_t;
// Types for 64-bit signed and unsigned integers
using Int64  = int64_t;
using uInt64 = uint64_t;

// The following code should provide access to the standard C++ objects and
// types: cout, cerr, string, ostream, istream, etc.
#include <algorithm>
#include <ostream>
#include <istream>
#include <iomanip>
#include <memory>
#include <string>
#include <sstream>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <utility>
#include <vector>
using namespace std;

// Common array types
using IntArray = vector<Int32>;
using BoolArray = vector<bool>;
using ByteArray = vector<uInt8>;
using StringList = vector<string>;

// Defines to help with path handling
#if (defined(BSPF_UNIX) || defined(BSPF_MAC_OSX))
  #define BSPF_PATH_SEPARATOR  "/"
#elif (defined(BSPF_DOS) || defined(BSPF_WINDOWS) || defined(BSPF_OS2))
  #define BSPF_PATH_SEPARATOR  "\\"
#else
  #error Update src/common/bspf.hxx for path separator
#endif

// CPU architecture type
// This isn't complete yet, but takes care of all the major platforms
#if defined(__i386__) || defined(_M_IX86)
  #define BSPF_ARCH "i386"
#elif defined(__x86_64__) || defined(_WIN64)
  #define BSPF_ARCH "x86_64"
#elif defined(__powerpc__) || defined(__ppc__)
  #define BSPF_ARCH "ppc"
#else
  #define BSPF_ARCH "NOARCH"
#endif

// I wish Windows had a complete POSIX layer
#if defined BSPF_WINDOWS && !defined __GNUG__
  #define BSPF_snprintf _snprintf
  #define BSPF_vsnprintf _vsnprintf
#else
  #define HAVE_UNISTD_H   // needed for building zlib
  #include <strings.h>
  #define BSPF_snprintf snprintf
  #define BSPF_vsnprintf vsnprintf
#endif

static const string EmptyString("");

//////////////////////////////////////////////////////////////////////
// Some convenience functions

// Initialize C++11 unique_ptr, at least until std::make_unique()
// becomes part of the standard (C++14)
template <typename Value, typename ... Arguments>
std::unique_ptr<Value> make_ptr(Arguments && ... arguments_for_constructor)
{
  return std::unique_ptr<Value>(
      new Value(std::forward<Arguments>(arguments_for_constructor)...)
  );
}

template<typename T> inline void BSPF_swap(T& a, T& b) { std::swap(a, b); }
template<typename T> inline T BSPF_abs (T x) { return (x>=0) ? x : -x; }
template<typename T> inline T BSPF_min (T a, T b) { return (a<b) ? a : b; }
template<typename T> inline T BSPF_max (T a, T b) { return (a>b) ? a : b; }
template<typename T> inline T BSPF_clamp (T a, T l, T u) { return (a<l) ? l : (a>u) ? u : a; }

// Compare two strings, ignoring case
inline int BSPF_compareIgnoreCase(const string& s1, const string& s2)
{
#if defined BSPF_WINDOWS && !defined __GNUG__
  return _stricmp(s1.c_str(), s2.c_str());
#else
  return strcasecmp(s1.c_str(), s2.c_str());
#endif
}
inline int BSPF_compareIgnoreCase(const char* s1, const char* s2)
{
#if defined BSPF_WINDOWS && !defined __GNUG__
  return _stricmp(s1, s2);
#else
  return strcasecmp(s1, s2);
#endif
}

// Test whether the first string starts with the second one (case insensitive)
inline bool BSPF_startsWithIgnoreCase(const string& s1, const string& s2)
{
#if defined BSPF_WINDOWS && !defined __GNUG__
  return _strnicmp(s1.c_str(), s2.c_str(), s2.length()) == 0;
#else
  return strncasecmp(s1.c_str(), s2.c_str(), s2.length()) == 0;
#endif
}
inline bool BSPF_startsWithIgnoreCase(const char* s1, const char* s2)
{
#if defined BSPF_WINDOWS && !defined __GNUG__
  return _strnicmp(s1, s2, strlen(s2)) == 0;
#else
  return strncasecmp(s1, s2, strlen(s2)) == 0;
#endif
}

// Test whether two strings are equal (case insensitive)
inline bool BSPF_equalsIgnoreCase(const string& s1, const string& s2)
{
  return BSPF_compareIgnoreCase(s1, s2) == 0;
}

// Find location (if any) of the second string within the first,
// starting from 'startpos' in the first string
inline size_t BSPF_findIgnoreCase(const string& s1, const string& s2, int startpos = 0)
{
  auto pos = std::search(s1.begin()+startpos, s1.end(),
    s2.begin(), s2.end(), [](char ch1, char ch2) {
      return toupper((uInt8)ch1) == toupper((uInt8)ch2);
    });
  return pos == s1.end() ? string::npos : pos - (s1.begin()+startpos);
}

// Test whether the first string ends with the second one (case insensitive)
inline bool BSPF_endsWithIgnoreCase(const string& s1, const string& s2)
{
  if(s1.length() >= s2.length())
  {
    const char* end = s1.c_str() + s1.length() - s2.length();
    return BSPF_compareIgnoreCase(end, s2.c_str()) == 0;
  }
  return false;
}

// Test whether the first string contains the second one (case insensitive)
inline bool BSPF_containsIgnoreCase(const string& s1, const string& s2)
{
  return BSPF_findIgnoreCase(s1, s2) != string::npos;
}

#endif
