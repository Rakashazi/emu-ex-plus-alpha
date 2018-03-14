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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef BSPF_HXX
#define BSPF_HXX

/**
  This file defines various basic data types and preprocessor variables
  that need to be defined for different operating systems.

  @author Bradford W. Mott and Stephen Anthony
*/

#include <cstdint>
// Types for 8/16/32/64-bit signed and unsigned integers
using Int8   = int8_t;
using uInt8  = uint8_t;
using Int16  = int16_t;
using uInt16 = uint16_t;
using Int32  = int32_t;
using uInt32 = uint32_t;
using Int64  = int64_t;
using uInt64 = uint64_t;

// The following code should provide access to the standard C++ objects and
// types: cout, cerr, string, ostream, istream, etc.
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <memory>
#include <string>
#include <sstream>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <utility>
#include <vector>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::istream;
using std::ostream;
using std::fstream;
using std::iostream;
using std::ifstream;
using std::ofstream;
using std::ostringstream;
using std::istringstream;
using std::stringstream;
using std::unique_ptr;
using std::shared_ptr;
using std::make_unique;
using std::make_shared;
using std::array;
using std::vector;
using std::runtime_error;
using std::memcpy;

// Common array types
using IntArray = std::vector<Int32>;
using uIntArray = std::vector<uInt32>;
using BoolArray = std::vector<bool>;
using ByteArray = std::vector<uInt8>;
using ShortArray = std::vector<uInt16>;
using StringList = std::vector<std::string>;
using BytePtr = std::unique_ptr<uInt8[]>;

static const string EmptyString("");

namespace BSPF
{
  // Defines to help with path handling
  #if defined(BSPF_UNIX) || defined(BSPF_MAC_OSX)
    static const string PATH_SEPARATOR = "/";
    #define ATTRIBUTE_FMT_PRINTF __attribute__((__format__ (__printf__, 2, 0)))
  #elif defined(BSPF_WINDOWS)
    static const string PATH_SEPARATOR = "\\";
    #pragma warning (disable : 4146)  // unary minus operator applied to unsigned type
    #pragma warning(2:4264)  // no override available for virtual member function from base 'class'; function is hidden
    #pragma warning(2:4265)  // class has virtual functions, but destructor is not virtual
    #pragma warning(2:4266)  // no override available for virtual member function from base 'type'; function is hidden
    #define ATTRIBUTE_FMT_PRINTF
  #else
    #error Update src/common/bspf.hxx for path separator
  #endif

  // CPU architecture type
  // This isn't complete yet, but takes care of all the major platforms
  #if defined(__i386__) || defined(_M_IX86)
    static const string ARCH = "i386";
  #elif defined(__x86_64__) || defined(_WIN64)
    static const string ARCH = "x86_64";
  #elif defined(__powerpc__) || defined(__ppc__)
    static const string ARCH = "ppc";
  #else
    static const string ARCH = "NOARCH";
  #endif

  // Combines 'max' and 'min', and clamps value to the upper/lower value
  // if it is outside the specified range
  template<class T> inline T clamp(T val, T lower, T upper)
  {
    return (val < lower) ? lower : (val > upper) ? upper : val;
  }
  template<class T> inline void clamp(T& val, T lower, T upper, T setVal)
  {
    if(val < lower || val > upper)  val = setVal;
  }

  // Compare two strings, ignoring case
  inline int compareIgnoreCase(const string& s1, const string& s2)
  {
  #if defined BSPF_WINDOWS && !defined __GNUG__
    return _stricmp(s1.c_str(), s2.c_str());
  #else
    return strcasecmp(s1.c_str(), s2.c_str());
  #endif
  }
  inline int compareIgnoreCase(const char* s1, const char* s2)
  {
  #if defined BSPF_WINDOWS && !defined __GNUG__
    return _stricmp(s1, s2);
  #else
    return strcasecmp(s1, s2);
  #endif
  }

  // Test whether the first string starts with the second one (case insensitive)
  inline bool startsWithIgnoreCase(const string& s1, const string& s2)
  {
  #if defined BSPF_WINDOWS && !defined __GNUG__
    return _strnicmp(s1.c_str(), s2.c_str(), s2.length()) == 0;
  #else
    return strncasecmp(s1.c_str(), s2.c_str(), s2.length()) == 0;
  #endif
  }
  inline bool startsWithIgnoreCase(const char* s1, const char* s2)
  {
  #if defined BSPF_WINDOWS && !defined __GNUG__
    return _strnicmp(s1, s2, strlen(s2)) == 0;
  #else
    return strncasecmp(s1, s2, strlen(s2)) == 0;
  #endif
  }

  // Test whether two strings are equal (case insensitive)
  inline bool equalsIgnoreCase(const string& s1, const string& s2)
  {
    return compareIgnoreCase(s1, s2) == 0;
  }

  // Find location (if any) of the second string within the first,
  // starting from 'startpos' in the first string
  inline size_t findIgnoreCase(const string& s1, const string& s2, size_t startpos = 0)
  {
    auto pos = std::search(s1.cbegin()+startpos, s1.cend(),
      s2.cbegin(), s2.cend(), [](char ch1, char ch2) {
        return toupper(uInt8(ch1)) == toupper(uInt8(ch2));
      });
    return pos == s1.cend() ? string::npos : pos - (s1.cbegin()+startpos);
  }

  // Test whether the first string ends with the second one (case insensitive)
  inline bool endsWithIgnoreCase(const string& s1, const string& s2)
  {
    if(s1.length() >= s2.length())
    {
      const char* end = s1.c_str() + s1.length() - s2.length();
      return compareIgnoreCase(end, s2.c_str()) == 0;
    }
    return false;
  }

  // Test whether the first string contains the second one (case insensitive)
  inline bool containsIgnoreCase(const string& s1, const string& s2)
  {
    return findIgnoreCase(s1, s2) != string::npos;
  }

  // Test whether the first string matches the second one (case insensitive)
  // - the first character must match
  // - the following characters must appear in the order of the first string
  inline bool matches(const string& s1, const string& s2)
  {
    if(BSPF::startsWithIgnoreCase(s1, s2.substr(0, 1)))
    {
      size_t pos = 1;
      for(uInt32 j = 1; j < s2.size(); j++)
      {
        size_t found = BSPF::findIgnoreCase(s1, s2.substr(j, 1), pos);
        if(found == string::npos)
          return false;
        pos += found + 1;
      }
      return true;
    }
    return false;
  }
} // namespace BSPF

#endif
