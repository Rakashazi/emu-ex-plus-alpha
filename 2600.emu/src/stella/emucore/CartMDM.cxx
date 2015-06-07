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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CartMDM.cxx 3131 2015-01-01 03:49:32Z stephena $
//============================================================================

#include <cstring>

#include "System.hxx"
#include "CartMDM.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeMDM::CartridgeMDM(const uInt8* image, uInt32 size, const Settings& settings)
  : Cartridge(settings),
    myImage(nullptr),
    mySize(size),
    myBankingDisabled(false)
{
  // Allocate array for the ROM image
  myImage = new uInt8[mySize];

  // Copy the ROM image into my buffer
  memcpy(myImage, image, mySize);
  createCodeAccessBase(mySize);

  // Remember startup bank
  myStartBank = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeMDM::~CartridgeMDM()
{
  delete[] myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMDM::reset()
{
  // Upon reset we switch to the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMDM::install(System& system)
{
  mySystem = &system;

  // Get the page accessing methods for the hot spots since they overlap
  // areas within the TIA we'll need to forward requests to the TIA
  myHotSpotPageAccess[0] = mySystem->getPageAccess(0x0800 >> System::PAGE_SHIFT);
  myHotSpotPageAccess[1] = mySystem->getPageAccess(0x0900 >> System::PAGE_SHIFT);
  myHotSpotPageAccess[2] = mySystem->getPageAccess(0x0A00 >> System::PAGE_SHIFT);
  myHotSpotPageAccess[3] = mySystem->getPageAccess(0x0B00 >> System::PAGE_SHIFT);
  myHotSpotPageAccess[4] = mySystem->getPageAccess(0x0C00 >> System::PAGE_SHIFT);
  myHotSpotPageAccess[5] = mySystem->getPageAccess(0x0D00 >> System::PAGE_SHIFT);
  myHotSpotPageAccess[6] = mySystem->getPageAccess(0x0E00 >> System::PAGE_SHIFT);
  myHotSpotPageAccess[7] = mySystem->getPageAccess(0x0F00 >> System::PAGE_SHIFT);

  // Set the page accessing methods for the hot spots
  System::PageAccess access(this, System::PA_READWRITE);
  for(uInt32 i = 0x0800; i < 0x0FFF; i += (1 << System::PAGE_SHIFT))
    mySystem->setPageAccess(i >> System::PAGE_SHIFT, access);

  // Install pages for bank 0
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeMDM::peek(uInt16 address)
{
  // Because of the way we've set up accessing above, we can only
  // get here when the addresses are from 0x800 - 0xFFF
  if(address < 0xC00)
    bank(address & 0x0FF);

  int hotspot = ((address & 0x0F00) >> 8) - 8;
  return myHotSpotPageAccess[hotspot].device->peek(address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMDM::poke(uInt16 address, uInt8 value)
{
  // All possible addresses can appear here, but we only care
  // about those below $1000
  if(!(address & 0x1000))
  {
    if(address < 0xC00)
      bank(address & 0x0FF);

    int hotspot = ((address & 0x0F00) >> 8) - 8;
    myHotSpotPageAccess[hotspot].device->poke(address, value);
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMDM::bank(uInt16 bank)
{ 
  if(bankLocked() || myBankingDisabled) return false;

  // Remember what bank we're in
  // Wrap around to a valid bank number if necessary
  myCurrentBank = bank % bankCount();
  uInt32 offset = myCurrentBank << 12;

  // Setup the page access methods for the current bank
  System::PageAccess access(this, System::PA_READ);

  // Map ROM image into the system
  for(uInt32 address = 0x1000; address < 0x2000;
      address += (1 << System::PAGE_SHIFT))
  {
    access.directPeekBase = &myImage[offset + (address & 0x0FFF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x0FFF)];
    mySystem->setPageAccess(address >> System::PAGE_SHIFT, access);
  }

  // Accesses above bank 127 disable further bankswitching; we're only
  // concerned with the lower byte
  myBankingDisabled = myBankingDisabled || bank > 127;
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeMDM::getBank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeMDM::bankCount() const
{
  return mySize >> 12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMDM::patch(uInt16 address, uInt8 value)
{
  myImage[(myCurrentBank << 12) + (address & 0x0FFF)] = value;
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeMDM::getImage(int& size) const
{
  size = mySize;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMDM::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putShort(myCurrentBank);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeMDM::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMDM::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    myCurrentBank = in.getShort();
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeMDM::load" << endl;
    return false;
  }

  // Remember what bank we were in
  bank(myCurrentBank);

  return true;
}
