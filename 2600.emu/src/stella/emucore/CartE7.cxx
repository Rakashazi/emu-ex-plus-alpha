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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "System.hxx"
#include "CartE7.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeE7::CartridgeE7(const ByteBuffer& image, size_t size,
                         const string& md5, const Settings& settings)
  : CartridgeMNetwork(image, size, md5, settings)
{
  initialize(image, size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE7::checkSwitchBank(uInt16 address)
{
  // Switch banks if necessary
  if((address >= 0x0FE0) && (address <= 0x0FE7))
  {
    bank(address & 0x0007);
  }
  else if((address >= 0x0FE8) && (address <= 0x0FEB))
  {
    bankRAM(address & 0x0003);
  }
}
