//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef KEY_VALUE_REPOSITORY_HXX
#define KEY_VALUE_REPOSITORY_HXX

#include <map>

#include "Variant.hxx"
#include "bspf.hxx"

class KeyValueRepository
{
  public:

    virtual ~KeyValueRepository() = default;

    virtual std::map<string, Variant> load() = 0;

    virtual void save(const std::map<string, Variant>& values) = 0;

    virtual void save(const string& key, const Variant& value) = 0;
};

#endif // KEY_VALUE_REPOSITORY_HXX
