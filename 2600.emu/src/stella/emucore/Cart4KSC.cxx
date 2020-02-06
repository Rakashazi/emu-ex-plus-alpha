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
#include "Cart4KSC.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge4KSC::Cartridge4KSC(const ByteBuffer& image, size_t size,
                             const string& md5, const Settings& settings)
  : Cartridge(settings, md5)
{
  // Copy the ROM image into my buffer
  std::copy_n(image.get(), std::min(myImage.size(), size), myImage.begin());
  createCodeAccessBase(myImage.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge4KSC::reset()
{
  initializeRAM(myRAM.data(), myRAM.size());

  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge4KSC::install(System& system)
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

  // Map ROM image into the system
  for(uInt16 addr = 0x1100; addr < 0x2000; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[addr & 0x0FFF];
    access.codeAccessBase = &myCodeAccessBase[addr & 0x0FFF];
    mySystem->setPageAccess(addr, access);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Cartridge4KSC::peek(uInt16 address)
{
  // The only way we can get to this method is if we attempt to read from
  // the write port (0xF000 - 0xF07F, 128 bytes), in which case an
  // unwanted write is potentially triggered
  return peekRAM(myRAM[address & 0x007F], address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge4KSC::poke(uInt16 address, uInt8 value)
{
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
bool Cartridge4KSC::patch(uInt16 address, uInt8 value)
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
    myImage[address & 0xFFF] = value;

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* Cartridge4KSC::getImage(size_t& size) const
{
  size = myImage.size();
  return myImage.data();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge4KSC::save(Serializer& out) const
{
  try
  {
    out.putByteArray(myRAM.data(), myRAM.size());
  }
  catch(...)
  {
    cerr << "ERROR: Cartridge4KSC::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge4KSC::load(Serializer& in)
{
  try
  {
    in.getByteArray(myRAM.data(), myRAM.size());
  }
  catch(...)
  {
    cerr << "ERROR: Cartridge4KSC::load" << endl;
    return false;
  }

  return true;
}
