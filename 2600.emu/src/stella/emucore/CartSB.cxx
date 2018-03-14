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
#include "CartSB.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeSB::CartridgeSB(const BytePtr& image, uInt32 size,
                         const Settings& settings)
  : Cartridge(settings),
    mySize(size),
    myBankOffset(0)
{
  // Allocate array for the ROM image
  myImage = make_unique<uInt8[]>(mySize);

  // Copy the ROM image into my buffer
  memcpy(myImage.get(), image.get(), mySize);
  createCodeAccessBase(mySize);

  // Remember startup bank
  myStartBank = bankCount() - 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeSB::reset()
{
  // Upon reset we switch to the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeSB::install(System& system)
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

  System::PageAccess access(this, System::PA_READ);

  // Set the page accessing methods for the hot spots
  for(uInt16 addr = 0x0800; addr < 0x0FFF; addr += System::PAGE_SIZE)
    mySystem->setPageAccess(addr, access);

  // Install pages for startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeSB::peek(uInt16 address)
{
  address &= (0x17FF + (mySize >> 12));

  // Switch banks if necessary
  if ((address & 0x1800) == 0x0800)
    bank(address & myStartBank);

  if(!(address & 0x1000))
  {
    // Because of the way we've set up accessing above, we can only
    // get here when the addresses are from 0x800 - 0xFFF
    int hotspot = ((address & 0x0F00) >> 8) - 8;
    return myHotSpotPageAccess[hotspot].device->peek(address);
  }

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeSB::poke(uInt16 address, uInt8 value)
{
  address &= (0x17FF + (mySize >> 12));

  // Switch banks if necessary
  if((address & 0x1800) == 0x0800)
    bank(address & myStartBank);

  if(!(address & 0x1000))
  {
    // Because of the way we've set up accessing above, we can only
    // get here when the addresses are from 0x800 - 0xFFF
    int hotspot = ((address & 0x0F00) >> 8) - 8;
    myHotSpotPageAccess[hotspot].device->poke(address, value);
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeSB::bank(uInt16 bank)
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
uInt16 CartridgeSB::getBank() const
{
  return myBankOffset >> 12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeSB::bankCount() const
{
  return mySize >> 12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeSB::patch(uInt16 address, uInt8 value)
{
  myImage[myBankOffset + (address & 0x0FFF)] = value;
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeSB::getImage(uInt32& size) const
{
  size = mySize;
  return myImage.get();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeSB::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putInt(myBankOffset);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeSB::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeSB::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    myBankOffset = in.getInt();
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeSB::load" << endl;
    return false;
  }

  // Remember what bank we were in
  bank(myBankOffset >> 12);

  return true;
}
