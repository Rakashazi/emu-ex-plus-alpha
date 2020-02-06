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
#include "CartCV.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCV::CartridgeCV(const ByteBuffer& image, size_t size,
                         const string& md5, const Settings& settings)
  : Cartridge(settings, md5),
    mySize(size)
{
  if(mySize == myImage.size())
  {
    // Copy the ROM data into my buffer
    std::copy_n(image.get(), myImage.size(), myImage.begin());
  }
  else if(mySize == 4_KB)
  {
    // The game has something saved in the RAM
    // Useful for MagiCard program listings

    // Copy the ROM data into my buffer
    std::copy_n(image.get() + myImage.size(), myImage.size(), myImage.begin());

    // Copy the RAM image into a buffer for use in reset()
    std::copy_n(image.get(), myInitialRAM.size(), myInitialRAM.begin());
  }
  createCodeAccessBase(myImage.size() + myRAM.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCV::reset()
{
  if(mySize == 4_KB)
  {
    // Copy the RAM image into my buffer
    myRAM = myInitialRAM;
  }
  else
    initializeRAM(myRAM.data(), myRAM.size());

  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCV::install(System& system)
{
  mySystem = &system;

  System::PageAccess access(this, System::PageAccessType::READ);

  // Map ROM image into the system
  for(uInt16 addr = 0x1800; addr < 0x2000; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[addr & 0x07FF];
    access.codeAccessBase = &myCodeAccessBase[addr & 0x07FF];
    mySystem->setPageAccess(addr, access);
  }

  // Set the page accessing method for the RAM writing pages
  // Map access to this class, since we need to inspect all accesses to
  // check if RWP happens
  access.directPeekBase = nullptr;
  access.codeAccessBase = nullptr;
  access.type = System::PageAccessType::WRITE;
  for(uInt16 addr = 0x1400; addr < 0x1800; addr += System::PAGE_SIZE)
    mySystem->setPageAccess(addr, access);

  // Set the page accessing method for the RAM reading pages
  access.directPokeBase = nullptr;
  access.type = System::PageAccessType::READ;
  for(uInt16 addr = 0x1000; addr < 0x1400; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myRAM[addr & 0x03FF];
    access.codeAccessBase = &myCodeAccessBase[2048 + (addr & 0x03FF)];
    mySystem->setPageAccess(addr, access);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeCV::peek(uInt16 address)
{
  // The only way we can get to this method is if we attempt to read from
  // the write port (0xF400 - 0xF7FF, 1024 bytes), in which case an
  // unwanted write is potentially triggered
  return peekRAM(myRAM[address & 0x03FF], address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCV::poke(uInt16 address, uInt8 value)
{
  pokeRAM(myRAM[address & 0x03FF], address, value);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCV::patch(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  if(address < 0x0800)
  {
    // Normally, a write to the read port won't do anything
    // However, the patch command is special in that ignores such
    // cart restrictions
    // The following will work for both reads and writes
    myRAM[address & 0x03FF] = value;
  }
  else
    myImage[address & 0x07FF] = value;

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeCV::getImage(size_t& size) const
{
  size = myImage.size();
  return myImage.data();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCV::save(Serializer& out) const
{
  try
  {
    out.putByteArray(myRAM.data(), myRAM.size());
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeCV::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCV::load(Serializer& in)
{
  try
  {
    in.getByteArray(myRAM.data(), myRAM.size());
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeCV::load" << endl;
    return false;
  }

  return true;
}
