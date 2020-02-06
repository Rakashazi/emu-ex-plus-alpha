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
#include "CartE0.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeE0::CartridgeE0(const ByteBuffer& image, size_t size,
                         const string& md5, const Settings& settings)
  : Cartridge(settings, md5)
{
  // Copy the ROM image into my buffer
  std::copy_n(image.get(), std::min(myImage.size(), size), myImage.begin());
  createCodeAccessBase(myImage.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE0::reset()
{
  // Setup segments to some default slices
  if(randomStartBank())
  {
    segmentZero(mySystem->randGenerator().next() % 8);
    segmentOne(mySystem->randGenerator().next() % 8);
    segmentTwo(mySystem->randGenerator().next() % 8);
  }
  else
  {
    segmentZero(4);
    segmentOne(5);
    segmentTwo(6);
  }
  myCurrentSlice[3] = 7; // fixed

  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE0::install(System& system)
{
  mySystem = &system;

  System::PageAccess access(this, System::PageAccessType::READ);

  // Set the page acessing methods for the first part of the last segment
  for(uInt16 addr = 0x1C00; addr < static_cast<uInt16>(0x1FE0U & ~System::PAGE_MASK);
      addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[0x1C00 + (addr & 0x03FF)];
    access.codeAccessBase = &myCodeAccessBase[0x1C00 + (addr & 0x03FF)];
    mySystem->setPageAccess(addr, access);
  }

  // Set the page accessing methods for the hot spots in the last segment
  access.directPeekBase = nullptr;
  access.codeAccessBase = &myCodeAccessBase[0x1FC0]; // TJ: is this the correct address (or 0x1FE0)?
  access.type = System::PageAccessType::READ;
  for(uInt16 addr = (0x1FE0 & ~System::PAGE_MASK); addr < 0x2000;
      addr += System::PAGE_SIZE)
    mySystem->setPageAccess(addr, access);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeE0::getBank(uInt16 address) const
{
  return myCurrentSlice[(address & 0xFFF) >> 10]; // 1K slices
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeE0::bankCount() const
{
  return 8;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeE0::peek(uInt16 address)
{
  address &= 0x0FFF;

  // Switch banks if necessary
  if((address >= 0x0FE0) && (address <= 0x0FE7))
  {
    segmentZero(address & 0x0007);
  }
  else if((address >= 0x0FE8) && (address <= 0x0FEF))
  {
    segmentOne(address & 0x0007);
  }
  else if((address >= 0x0FF0) && (address <= 0x0FF7))
  {
    segmentTwo(address & 0x0007);
  }

  return myImage[(myCurrentSlice[address >> 10] << 10) + (address & 0x03FF)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeE0::poke(uInt16 address, uInt8)
{
  address &= 0x0FFF;

  // Switch banks if necessary
  if((address >= 0x0FE0) && (address <= 0x0FE7))
  {
    segmentZero(address & 0x0007);
  }
  else if((address >= 0x0FE8) && (address <= 0x0FEF))
  {
    segmentOne(address & 0x0007);
  }
  else if((address >= 0x0FF0) && (address <= 0x0FF7))
  {
    segmentTwo(address & 0x0007);
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE0::segmentZero(uInt16 slice)
{
  if(bankLocked()) return;

  // Remember the new slice
  myCurrentSlice[0] = slice;
  uInt16 offset = slice << 10;

  // Setup the page access methods for the current bank
  System::PageAccess access(this, System::PageAccessType::READ);

  for(uInt16 addr = 0x1000; addr < 0x1400; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[offset + (addr & 0x03FF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (addr & 0x03FF)];
    mySystem->setPageAccess(addr, access);
  }
  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE0::segmentOne(uInt16 slice)
{
  if(bankLocked()) return;

  // Remember the new slice
  myCurrentSlice[1] = slice;
  uInt16 offset = slice << 10;

  // Setup the page access methods for the current bank
  System::PageAccess access(this, System::PageAccessType::READ);

  for(uInt16 addr = 0x1400; addr < 0x1800; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[offset + (addr & 0x03FF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (addr & 0x03FF)];
    mySystem->setPageAccess(addr, access);
  }
  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE0::segmentTwo(uInt16 slice)
{
  if(bankLocked()) return;

  // Remember the new slice
  myCurrentSlice[2] = slice;
  uInt16 offset = slice << 10;

  // Setup the page access methods for the current bank
  System::PageAccess access(this, System::PageAccessType::READ);

  for(uInt16 addr = 0x1800; addr < 0x1C00; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[offset + (addr & 0x03FF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (addr & 0x03FF)];
    mySystem->setPageAccess(addr, access);
  }
  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeE0::patch(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;
  myImage[(myCurrentSlice[address >> 10] << 10) + (address & 0x03FF)] = value;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeE0::getImage(size_t& size) const
{
  size = myImage.size();
  return myImage.data();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeE0::save(Serializer& out) const
{
  try
  {
    out.putShortArray(myCurrentSlice.data(), myCurrentSlice.size());
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeE0::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeE0::load(Serializer& in)
{
  try
  {
    in.getShortArray(myCurrentSlice.data(), myCurrentSlice.size());
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeE0::load" << endl;
    return false;
  }

  return true;
}
