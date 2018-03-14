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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "System.hxx"
#include "CartE78K.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeE78K::CartridgeE78K(const BytePtr& image, uInt32 size,
                         const Settings& settings)
  : CartridgeMNetwork(image, size, settings)
{
  initialize(image, size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE78K::checkSwitchBank(uInt16 address)
{
  // Switch banks if necessary
  if((address >= 0x0FE4) && (address <= 0x0FE7))
  {
    bank(address & 0x0003);
  }
  else if((address >= 0x0FE8) && (address <= 0x0FEB))
  {
    bankRAM(address & 0x0003);
  }
}
