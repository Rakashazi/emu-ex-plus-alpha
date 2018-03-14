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
#include "Cart4K.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge4K::Cartridge4K(const BytePtr& image, uInt32 size,
                         const Settings& settings)
  : Cartridge(settings)
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image.get(), std::min(4096u, size));
  createCodeAccessBase(4096);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge4K::reset()
{
  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge4K::install(System& system)
{
  mySystem = &system;

  // Map ROM image into the system
  // Note that we don't need our own peek/poke methods, since the mapping
  // takes care of the entire address space
  System::PageAccess access(this, System::PA_READ);
  for(uInt16 addr = 0x1000; addr < 0x2000; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[addr & 0x0FFF];
    access.codeAccessBase = &myCodeAccessBase[addr & 0x0FFF];
    mySystem->setPageAccess(addr, access);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge4K::patch(uInt16 address, uInt8 value)
{
  myImage[address & 0x0FFF] = value;
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* Cartridge4K::getImage(uInt32& size) const
{
  size = 4096;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge4K::save(Serializer& out) const
{
  try
  {
    out.putString(name());
  }
  catch(...)
  {
    cerr << "ERROR: Cartridge4K::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge4K::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;
  }
  catch(...)
  {
    cerr << "ERROR: Cartridge4K::load" << endl;
    return false;
  }

  return true;
}
