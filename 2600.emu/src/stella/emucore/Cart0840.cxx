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
#include "Cart0840.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge0840::Cartridge0840(const BytePtr& image, uInt32 size,
                             const Settings& settings)
  : Cartridge(settings),
    myBankOffset(0)
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image.get(), std::min(8192u, size));
  createCodeAccessBase(8192);

  // Remember startup bank
  myStartBank = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge0840::reset()
{
  // Upon reset we switch to the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge0840::install(System& system)
{
  mySystem = &system;

  // Get the page accessing methods for the hot spots since they overlap
  // areas within the TIA we'll need to forward requests to the TIA
  myHotSpotPageAccess[0] = mySystem->getPageAccess(0x0800);
  myHotSpotPageAccess[1] = mySystem->getPageAccess(0x0900);
  myHotSpotPageAccess[2] = mySystem->getPageAccess(0x0A00);
  myHotSpotPageAccess[3] = mySystem->getPageAccess(0x0B00);
  myHotSpotPageAccess[4] = mySystem->getPageAccess(0x0C00);
  myHotSpotPageAccess[5] = mySystem->getPageAccess(0x0D00);
  myHotSpotPageAccess[6] = mySystem->getPageAccess(0x0E00);
  myHotSpotPageAccess[7] = mySystem->getPageAccess(0x0F00);

  // Set the page accessing methods for the hot spots
  System::PageAccess access(this, System::PA_READ);
  for(uInt16 addr = 0x0800; addr < 0x0FFF; addr += System::PAGE_SIZE)
    mySystem->setPageAccess(addr, access);

  // Install pages for bank 0
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Cartridge0840::peek(uInt16 address)
{
  address &= 0x1840;

  // Switch banks if necessary
  switch(address)
  {
    case 0x0800:
      // Set the current bank to the lower 4k bank
      bank(0);
      break;

    case 0x0840:
      // Set the current bank to the upper 4k bank
      bank(1);
      break;

    default:
      break;
  }

  // Because of the way we've set up accessing above, we can only
  // get here when the addresses are from 0x800 - 0xFFF
  int hotspot = ((address & 0x0F00) >> 8) - 8;
  return myHotSpotPageAccess[hotspot].device->peek(address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge0840::poke(uInt16 address, uInt8 value)
{
  address &= 0x1840;

  // Switch banks if necessary
  switch(address)
  {
    case 0x0800:
      // Set the current bank to the lower 4k bank
      bank(0);
      break;

    case 0x0840:
      // Set the current bank to the upper 4k bank
      bank(1);
      break;

    default:
      break;
  }

  // Because of the way accessing is set up, we will may get here by
  // doing a write to 0x800 - 0xFFF or cart; we ignore the cart write
  if(!(address & 0x1000))
  {
    int hotspot = ((address & 0x0F00) >> 8) - 8;
    myHotSpotPageAccess[hotspot].device->poke(address, value);
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge0840::bank(uInt16 bank)
{
  if(bankLocked()) return false;

  // Remember what bank we're in
  myBankOffset = bank << 12;

  // Setup the page access methods for the current bank
  System::PageAccess access(this, System::PA_READ);

  // Map ROM image into the system
  for(uInt16 addr = 0x1000; addr < 0x2000; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[myBankOffset + (addr & 0x0FFF)];
    access.codeAccessBase = &myCodeAccessBase[myBankOffset + (addr & 0x0FFF)];
    mySystem->setPageAccess(addr, access);
  }
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Cartridge0840::getBank() const
{
  return myBankOffset >> 12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Cartridge0840::bankCount() const
{
  return 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge0840::patch(uInt16 address, uInt8 value)
{
  myImage[myBankOffset + (address & 0x0fff)] = value;
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* Cartridge0840::getImage(uInt32& size) const
{
  size = 8192;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge0840::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putShort(myBankOffset);
  }
  catch(...)
  {
    cerr << "ERROR: Cartridge0840::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge0840::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    myBankOffset = in.getShort();
  }
  catch(...)
  {
    cerr << "ERROR: Cartridge0840::load" << endl;
    return false;
  }

  // Remember what bank we were in
  bank(myBankOffset);

  return true;
}
