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
#include "Cart2K.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge2K::Cartridge2K(const BytePtr& image, uInt32 size,
                         const Settings& settings)
  : Cartridge(settings)
{
  // Size can be a maximum of 2K
  if(size > 2048) size = 2048;

  // Set image size to closest power-of-two for the given size
  mySize = 1;
  while(mySize < size)
    mySize <<= 1;

  // We can't use a size smaller than the minimum page size in Stella
  mySize = std::max<uInt32>(mySize, System::PAGE_SIZE);

  // Initialize ROM with illegal 6502 opcode that causes a real 6502 to jam
  myImage = make_unique<uInt8[]>(mySize);
  memset(myImage.get(), 0x02, mySize);

  // Copy the ROM image into my buffer
  memcpy(myImage.get(), image.get(), size);
  createCodeAccessBase(mySize);

  // Set mask for accessing the image buffer
  // This is guaranteed to work, as mySize is a power of two
  myMask = mySize - 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge2K::reset()
{
  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge2K::install(System& system)
{
  mySystem = &system;

  // Map ROM image into the system
  // Note that we don't need our own peek/poke methods, since the mapping
  // takes care of the entire address space
  System::PageAccess access(this, System::PA_READ);
  for(uInt16 addr = 0x1000; addr < 0x2000; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[addr & myMask];
    access.codeAccessBase = &myCodeAccessBase[addr & myMask];
    mySystem->setPageAccess(addr, access);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge2K::patch(uInt16 address, uInt8 value)
{
  myImage[address & myMask] = value;
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* Cartridge2K::getImage(uInt32& size) const
{
  size = mySize;
  return myImage.get();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge2K::save(Serializer& out) const
{
  try
  {
    out.putString(name());
  }
  catch(...)
  {
    cerr << "ERROR: Cartridge2K::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge2K::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;
  }
  catch(...)
  {
    cerr << "ERROR: Cartridge2K::load" << endl;
    return false;
  }

  return true;
}
