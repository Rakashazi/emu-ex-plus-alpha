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
#include "M6532.hxx"
#include "TIA.hxx"
#include "CartX07.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeX07::CartridgeX07(const BytePtr& image, uInt32 size,
                           const Settings& settings)
  : Cartridge(settings),
    myCurrentBank(0)
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image.get(), std::min(65536u, size));
  createCodeAccessBase(65536);

  // Remember startup bank
  myStartBank = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeX07::reset()
{
  // Upon reset we switch to the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeX07::install(System& system)
{
  mySystem = &system;

  // Set the page accessing methods for the hot spots
  // The hotspots use almost all addresses below 0x1000, so we simply grab them
  // all and forward the TIA/RIOT calls from the peek and poke methods.
  System::PageAccess access(this, System::PA_READWRITE);
  for(uInt16 addr = 0x00; addr < 0x1000; addr += System::PAGE_SIZE)
    mySystem->setPageAccess(addr, access);

  // Install pages for the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeX07::peek(uInt16 address)
{
  uInt8 value = 0;

  // Check for RAM or TIA mirroring
  uInt16 lowAddress = address & 0x3ff;
  if(lowAddress & 0x80)
    value = mySystem->m6532().peek(address);
  else if(!(lowAddress & 0x200))
    value = mySystem->tia().peek(address);

  // Switch banks if necessary
  if((address & 0x180f) == 0x080d)
    bank((address & 0xf0) >> 4);
  else if((address & 0x1880) == 0)
  {
    if((myCurrentBank & 0xe) == 0xe)
      bank(((address & 0x40) >> 6) | (myCurrentBank & 0xe));
  }

  return value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeX07::poke(uInt16 address, uInt8 value)
{
  // Check for RAM or TIA mirroring
  uInt16 lowAddress = address & 0x3ff;
  if(lowAddress & 0x80)
    mySystem->m6532().poke(address, value);
  else if(!(lowAddress & 0x200))
    mySystem->tia().poke(address, value);

  // Switch banks if necessary
  if((address & 0x180f) == 0x080d)
    bank((address & 0xf0) >> 4);
  else if((address & 0x1880) == 0)
  {
    if((myCurrentBank & 0xe) == 0xe)
      bank(((address & 0x40) >> 6) | (myCurrentBank & 0xe));
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeX07::bank(uInt16 bank)
{
  if(bankLocked()) return false;

  // Remember what bank we're in
  myCurrentBank = (bank & 0x0f);
  uInt32 offset = myCurrentBank << 12;

  // Setup the page access methods for the current bank
  System::PageAccess access(this, System::PA_READ);

  // Map ROM image into the system
  for(uInt16 addr = 0x1000; addr < 0x2000; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[offset + (addr & 0x0FFF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (addr & 0x0FFF)];
    mySystem->setPageAccess(addr, access);
  }
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeX07::getBank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeX07::bankCount() const
{
  return 16;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeX07::patch(uInt16 address, uInt8 value)
{
  myImage[(myCurrentBank << 12) + (address & 0x0FFF)] = value;
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeX07::getImage(uInt32& size) const
{
  size = 65536;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeX07::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putShort(myCurrentBank);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeX07::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeX07::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    myCurrentBank = in.getShort();
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeX07::load" << endl;
    return false;
  }

  // Remember what bank we were in
  bank(myCurrentBank);

  return true;
}
