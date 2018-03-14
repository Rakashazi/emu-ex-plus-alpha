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
#include "CartF8SC.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeF8SC::CartridgeF8SC(const BytePtr& image, uInt32 size,
                             const Settings& settings)
  : Cartridge(settings),
    myBankOffset(0)
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image.get(), std::min(8192u, size));
  createCodeAccessBase(8192);

  // Remember startup bank
  myStartBank = 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeF8SC::reset()
{
  // define startup bank
  randomizeStartBank();

  initializeRAM(myRAM, 128);

  // Upon reset we switch to the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeF8SC::install(System& system)
{
  mySystem = &system;

  System::PageAccess access(this, System::PA_READ);

  // Set the page accessing method for the RAM writing pages
  access.type = System::PA_WRITE;
  for(uInt16 addr = 0x1000; addr < 0x1080; addr += System::PAGE_SIZE)
  {
    access.directPokeBase = &myRAM[addr & 0x007F];
    access.codeAccessBase = &myCodeAccessBase[addr & 0x007F];
    mySystem->setPageAccess(addr, access);
  }

  // Set the page accessing method for the RAM reading pages
  access.directPokeBase = nullptr;
  access.type = System::PA_READ;
  for(uInt16 addr = 0x1080; addr < 0x1100; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myRAM[addr & 0x007F];
    access.codeAccessBase = &myCodeAccessBase[0x80 + (addr & 0x007F)];
    mySystem->setPageAccess(addr, access);
  }

  // Install pages for the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeF8SC::peek(uInt16 address)
{
  uInt16 peekAddress = address;
  address &= 0x0FFF;

  // Switch banks if necessary
  switch(address)
  {
    case 0x0FF8:
      // Set the current bank to the lower 4k bank
      bank(0);
      break;

    case 0x0FF9:
      // Set the current bank to the upper 4k bank
      bank(1);
      break;

    default:
      break;
  }

  if(address < 0x0080)  // Write port is at 0xF000 - 0xF080 (128 bytes)
  {
    // Reading from the write port triggers an unwanted write
    uInt8 value = mySystem->getDataBusState(0xFF);

    if(bankLocked())
      return value;
    else
    {
      triggerReadFromWritePort(peekAddress);
      return myRAM[address] = value;
    }
  }
  else
    return myImage[myBankOffset + address];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF8SC::poke(uInt16 address, uInt8)
{
  address &= 0x0FFF;

  // Switch banks if necessary
  switch(address)
  {
    case 0x0FF8:
      // Set the current bank to the lower 4k bank
      bank(0);
      break;

    case 0x0FF9:
      // Set the current bank to the upper 4k bank
      bank(1);
      break;

    default:
      break;
  }

  // NOTE: This does not handle accessing RAM, however, this function
  // should never be called for RAM because of the way page accessing
  // has been setup
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF8SC::bank(uInt16 bank)
{
  if(bankLocked()) return false;

  // Remember what bank we're in
  myBankOffset = bank << 12;

  System::PageAccess access(this, System::PA_READ);

  // Set the page accessing methods for the hot spots
  for(uInt16 addr = (0x1FF8 & ~System::PAGE_MASK); addr < 0x2000;
      addr += System::PAGE_SIZE)
  {
    access.codeAccessBase = &myCodeAccessBase[myBankOffset + (addr & 0x0FFF)];
    mySystem->setPageAccess(addr, access);
  }

  // Setup the page access methods for the current bank
  for(uInt16 addr = 0x1100; addr < (0x1FF8U & ~System::PAGE_MASK);
      addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[myBankOffset + (addr & 0x0FFF)];
    access.codeAccessBase = &myCodeAccessBase[myBankOffset + (addr & 0x0FFF)];
    mySystem->setPageAccess(addr, access);
  }
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeF8SC::getBank() const
{
  return myBankOffset >> 12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeF8SC::bankCount() const
{
  return 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF8SC::patch(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  if(address < 0x0100)
  {
    // Normally, a write to the read port won't do anything
    // However, the patch command is special in that ignores such
    // cart restrictions
    myRAM[address & 0x007F] = value;
  }
  else
    myImage[myBankOffset + address] = value;

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeF8SC::getImage(uInt32& size) const
{
  size = 8192;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF8SC::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putShort(myBankOffset);
    out.putByteArray(myRAM, 128);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeF8SC::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF8SC::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    myBankOffset = in.getShort();
    in.getByteArray(myRAM, 128);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeF8SC::load" << endl;
    return false;
  }

  // Remember what bank we were in
  bank(myBankOffset >> 12);

  return true;
}
