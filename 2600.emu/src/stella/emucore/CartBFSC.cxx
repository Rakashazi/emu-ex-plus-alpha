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
#include "CartBFSC.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeBFSC::CartridgeBFSC(const ByteBuffer& image, size_t size,
                             const string& md5, const Settings& settings)
  : Cartridge(settings, md5)
{
  // Copy the ROM image into my buffer
  std::copy_n(image.get(), std::min(myImage.size(), size), myImage.begin());
  createCodeAccessBase(myImage.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBFSC::reset()
{
  initializeRAM(myRAM.data(), myRAM.size());
  initializeStartBank(15);

  // Upon reset we switch to the startup bank
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBFSC::install(System& system)
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
uInt8 CartridgeBFSC::peek(uInt16 address)
{
  uInt16 peekAddress = address;
  address &= 0x0FFF;

  // Switch banks if necessary
  if((address >= 0x0F80) && (address <= 0x0FBF))
    bank(address - 0x0F80);

  if(address < 0x0080)  // Write port is at 0xF000 - 0xF07F (128 bytes)
    return peekRAM(myRAM[address], peekAddress);
  else
    return myImage[myBankOffset + address];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBFSC::poke(uInt16 address, uInt8 value)
{
  uInt16 pokeAddress = address;
  address &= 0x0FFF;

  // Switch banks if necessary
  if((address >= 0x0F80) && (address <= 0x0FBF))
  {
    bank(address - 0x0F80);
    return false;
  }

  if (!(address & 0x080))
  {
    pokeRAM(myRAM[address & 0x007F], pokeAddress, value);
    return true;
  }
  else
  {
    // Writing to the read port should be ignored, but trigger a break if option enabled
    uInt8 dummy;

    pokeRAM(dummy, pokeAddress, value);
    myRamWriteAccess = pokeAddress;
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBFSC::bank(uInt16 bank)
{
  if(bankLocked()) return false;

  // Remember what bank we're in
  myBankOffset = bank << 12;

  System::PageAccess access(this, System::PageAccessType::READ);

  // Set the page accessing methods for the hot spots
  for(uInt16 addr = (0x1F80 & ~System::PAGE_MASK); addr < 0x2000;
      addr += System::PAGE_SIZE)
  {
    access.codeAccessBase = &myCodeAccessBase[myBankOffset + (addr & 0x0FFF)];
    mySystem->setPageAccess(addr, access);
  }

  // Setup the page access methods for the current bank
  for(uInt16 addr = 0x1100; addr < static_cast<uInt16>(0x1F80U & ~System::PAGE_MASK);
      addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[myBankOffset + (addr & 0x0FFF)];
    access.codeAccessBase = &myCodeAccessBase[myBankOffset + (addr & 0x0FFF)];
    mySystem->setPageAccess(addr, access);
  }
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeBFSC::getBank(uInt16) const
{
  return myBankOffset >> 12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeBFSC::bankCount() const
{
  return 64;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBFSC::patch(uInt16 address, uInt8 value)
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
const uInt8* CartridgeBFSC::getImage(size_t& size) const
{
  size = myImage.size();
  return myImage.data();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBFSC::save(Serializer& out) const
{
  try
  {
    out.putInt(myBankOffset);
    out.putByteArray(myRAM.data(), myRAM.size());
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeBFSC::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBFSC::load(Serializer& in)
{
  try
  {
    myBankOffset = in.getInt();
    in.getByteArray(myRAM.data(), myRAM.size());
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeBFSC::load" << endl;
    return false;
  }

  // Remember what bank we were in
  bank(myBankOffset >> 12);

  return true;
}
