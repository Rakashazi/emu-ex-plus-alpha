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
#include "CartFA.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFA::CartridgeFA(const ByteBuffer& image, size_t size,
                         const string& md5, const Settings& settings)
  : Cartridge(settings, md5)
{
  // Copy the ROM image into my buffer
  std::copy_n(image.get(), std::min(myImage.size(), size), myImage.begin());
  createCodeAccessBase(myImage.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA::reset()
{
  initializeRAM(myRAM.data(), myRAM.size());
  initializeStartBank(2);

  // Upon reset we switch to the startup bank
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA::install(System& system)
{
  mySystem = &system;

  System::PageAccess access(this, System::PageAccessType::READ);

  // Set the page accessing method for the RAM writing pages
  // Map access to this class, since we need to inspect all accesses to
  // check if RWP happens
  access.type = System::PageAccessType::WRITE;
  for(uInt16 addr = 0x1000; addr < 0x1100; addr += System::PAGE_SIZE)
  {
    access.codeAccessBase = &myCodeAccessBase[addr & 0x00FF];
    mySystem->setPageAccess(addr, access);
  }

  // Set the page accessing method for the RAM reading pages
  access.type = System::PageAccessType::READ;
  for(uInt16 addr = 0x1100; addr < 0x1200; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myRAM[addr & 0x00FF];
    access.codeAccessBase = &myCodeAccessBase[0x100 + (addr & 0x00FF)];
    mySystem->setPageAccess(addr, access);
  }

  // Install pages for the startup bank
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeFA::peek(uInt16 address)
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
      // Set the current bank to the middle 4k bank
      bank(1);
      break;

    case 0x0FFA:
      // Set the current bank to the upper 4k bank
      bank(2);
      break;

    default:
      break;
  }

  if(address < 0x0100)  // Write port is at 0xF000 - 0xF0FF (256 bytes)
    return peekRAM(myRAM[address], peekAddress);
  else
    return myImage[myBankOffset + address];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFA::poke(uInt16 address, uInt8 value)
{
  // Switch banks if necessary
  switch(address & 0x0FFF)
  {
    case 0x0FF8:
      // Set the current bank to the lower 4k bank
      bank(0);
      return false;

    case 0x0FF9:
      // Set the current bank to the middle 4k bank
      bank(1);
      return false;

    case 0x0FFA:
      // Set the current bank to the upper 4k bank
      bank(2);
      return false;

    default:
      break;
  }

  if (!(address & 0x100))
  {
    pokeRAM(myRAM[address & 0x00FF], address, value);
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
bool CartridgeFA::bank(uInt16 bank)
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
  for(uInt16 addr = 0x1200; addr < static_cast<uInt16>(0x1FF8U & ~System::PAGE_MASK);
      addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[myBankOffset + (addr & 0x0FFF)];
    access.codeAccessBase = &myCodeAccessBase[myBankOffset + (addr & 0x0FFF)];
    mySystem->setPageAccess(addr, access);
  }
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeFA::getBank(uInt16) const
{
  return myBankOffset >> 12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeFA::bankCount() const
{
  return 3;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFA::patch(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  if(address < 0x0200)
  {
    // Normally, a write to the read port won't do anything
    // However, the patch command is special in that ignores such
    // cart restrictions
    myRAM[address & 0x00FF] = value;
  }
  else
    myImage[myBankOffset + address] = value;

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeFA::getImage(size_t& size) const
{
  size = myImage.size();
  return myImage.data();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFA::save(Serializer& out) const
{
  try
  {
    out.putShort(myBankOffset);
    out.putByteArray(myRAM.data(), myRAM.size());
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeFA::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFA::load(Serializer& in)
{
  try
  {
    myBankOffset = in.getShort();
    in.getByteArray(myRAM.data(), myRAM.size());
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeFA::load" << endl;
    return false;
  }

  // Remember what bank we were in
  bank(myBankOffset >> 12);

  return true;
}
