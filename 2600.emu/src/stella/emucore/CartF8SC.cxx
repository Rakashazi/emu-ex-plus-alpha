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

#include "System.hxx"
#include "CartF8SC.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeF8SC::CartridgeF8SC(const ByteBuffer& image, size_t size,
                             const string& md5, const Settings& settings)
  : Cartridge(settings, md5)
{
  // Copy the ROM image into my buffer
  std::copy_n(image.get(), std::min(myImage.size(), size), myImage.begin());
  createCodeAccessBase(myImage.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeF8SC::reset()
{
  initializeRAM(myRAM.data(), myRAM.size());
  initializeStartBank(1);

  // Upon reset we switch to the startup bank
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeF8SC::install(System& system)
{
  mySystem = &system;

  System::PageAccess access(this, System::PageAccessType::READ);

  // Set the page accessing method for the RAM writing pages
  // Map access to this class, since we need to inspect all accesses to
  // check if RWP happens
  access.type = System::PageAccessType::WRITE;
  for(uInt16 addr = 0x1000; addr < 0x1080; addr += System::PAGE_SIZE)
  {
    access.codeAccessBase = &myCodeAccessBase[addr & 0x007F];
    mySystem->setPageAccess(addr, access);
  }

  // Set the page accessing method for the RAM reading pages
  access.type = System::PageAccessType::READ;
  for(uInt16 addr = 0x1080; addr < 0x1100; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myRAM[addr & 0x007F];
    access.codeAccessBase = &myCodeAccessBase[0x80 + (addr & 0x007F)];
    mySystem->setPageAccess(addr, access);
  }

  // Install pages for the startup bank
  bank(startBank());
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

  if(address < 0x0080)  // Write port is at 0xF000 - 0xF07F (128 bytes)
    return peekRAM(myRAM[address], peekAddress);
  else
    return myImage[myBankOffset + address];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF8SC::poke(uInt16 address, uInt8 value)
{
  // Switch banks if necessary
  switch(address & 0x0FFF)
  {
    case 0x0FF8:
      // Set the current bank to the lower 4k bank
      bank(0);
      return false;

    case 0x0FF9:
      // Set the current bank to the upper 4k bank
      bank(1);
      return false;

    default:
      break;
  }

  if (!(address & 0x080))
  {
    pokeRAM(myRAM[address & 0x007F], address, value);
    return true;
  }
  else
  {
    // Writing to the read port should be ignored, but trigger a break if option enabled
    uInt8 dummy;

    pokeRAM(dummy, address, value);
    myRamWriteAccess = address;
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF8SC::bank(uInt16 bank)
{
  if(bankLocked()) return false;

  // Remember what bank we're in
  myBankOffset = bank << 12;

  System::PageAccess access(this, System::PageAccessType::READ);

  // Set the page accessing methods for the hot spots
  for(uInt16 addr = (0x1FF8 & ~System::PAGE_MASK); addr < 0x2000;
      addr += System::PAGE_SIZE)
  {
    access.codeAccessBase = &myCodeAccessBase[myBankOffset + (addr & 0x0FFF)];
    mySystem->setPageAccess(addr, access);
  }

  // Setup the page access methods for the current bank
  for(uInt16 addr = 0x1100; addr < static_cast<uInt16>(0x1FF8U & ~System::PAGE_MASK);
      addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[myBankOffset + (addr & 0x0FFF)];
    access.codeAccessBase = &myCodeAccessBase[myBankOffset + (addr & 0x0FFF)];
    mySystem->setPageAccess(addr, access);
  }
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeF8SC::getBank(uInt16) const
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
const uInt8* CartridgeF8SC::getImage(size_t& size) const
{
  size = myImage.size();
  return myImage.data();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF8SC::save(Serializer& out) const
{
  try
  {
    out.putShort(myBankOffset);
    out.putByteArray(myRAM.data(), myRAM.size());
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
    myBankOffset = in.getShort();
    in.getByteArray(myRAM.data(), myRAM.size());
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
