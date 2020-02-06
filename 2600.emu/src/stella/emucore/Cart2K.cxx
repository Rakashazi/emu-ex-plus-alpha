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
#include "Cart2K.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge2K::Cartridge2K(const ByteBuffer& image, size_t size,
                         const string& md5, const Settings& settings)
  : Cartridge(settings, md5)
{
  // Size can be a maximum of 2K
  if(size > 2_KB)  size = 2_KB;

  // Set image size to closest power-of-two for the given size
  mySize = 1;
  while(mySize < size)
    mySize <<= 1;

  // Initialize ROM with illegal 6502 opcode that causes a real 6502 to jam
  size_t bufSize = std::max<size_t>(mySize, System::PAGE_SIZE);
  myImage = make_unique<uInt8[]>(bufSize);
  std::fill_n(myImage.get(), bufSize, 0x02);

  // Handle cases where ROM is smaller than the page size
  // It's much easier to do it this way rather than changing the page size
  if(mySize >= System::PAGE_SIZE)
  {
    // Directly copy the ROM image into the buffer
    std::copy_n(image.get(), mySize, myImage.get());
  }
  else
  {
    // Manually 'mirror' the ROM image into the buffer
    for(size_t i = 0; i < System::PAGE_SIZE; i += mySize)
      std::copy_n(image.get(), mySize, myImage.get() + i);
    mySize = System::PAGE_SIZE;
  }

  createCodeAccessBase(mySize);

  // Set mask for accessing the image buffer
  // This is guaranteed to work, as mySize is a power of two
  myMask = static_cast<uInt16>(mySize) - 1;
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
  System::PageAccess access(this, System::PageAccessType::READ);
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
const uInt8* Cartridge2K::getImage(size_t& size) const
{
  size = mySize;
  return myImage.get();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge2K::save(Serializer&) const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge2K::load(Serializer&)
{
  return true;
}
