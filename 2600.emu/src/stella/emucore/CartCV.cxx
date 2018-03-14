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
#include "CartCV.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCV::CartridgeCV(const BytePtr& image, uInt32 size,
                         const Settings& settings)
  : Cartridge(settings),
    mySize(size)
{
  if(mySize == 2048)
  {
    // Copy the ROM data into my buffer
    memcpy(myImage, image.get(), 2048);
  }
  else if(mySize == 4096)
  {
    // The game has something saved in the RAM
    // Useful for MagiCard program listings

    // Copy the ROM data into my buffer
    memcpy(myImage, image.get() + 2048, 2048);

    // Copy the RAM image into a buffer for use in reset()
    myInitialRAM = make_unique<uInt8[]>(1024);
    memcpy(myInitialRAM.get(), image.get(), 1024);
  }
  createCodeAccessBase(2048+1024);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCV::reset()
{
  if(myInitialRAM)
  {
    // Copy the RAM image into my buffer
    memcpy(myRAM, myInitialRAM.get(), 1024);
  }
  else
    initializeRAM(myRAM, 1024);

  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCV::install(System& system)
{
  mySystem = &system;

  System::PageAccess access(this, System::PA_READ);

  // Map ROM image into the system
  for(uInt16 addr = 0x1800; addr < 0x2000; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[addr & 0x07FF];
    access.codeAccessBase = &myCodeAccessBase[addr & 0x07FF];
    mySystem->setPageAccess(addr, access);
  }

  // Set the page accessing method for the RAM writing pages
  access.directPeekBase = nullptr;
  access.codeAccessBase = nullptr;
  access.type = System::PA_WRITE;
  for(uInt16 addr = 0x1400; addr < 0x1800; addr += System::PAGE_SIZE)
  {
    access.directPokeBase = &myRAM[addr & 0x03FF];
    mySystem->setPageAccess(addr, access);
  }

  // Set the page accessing method for the RAM reading pages
  access.directPokeBase = nullptr;
  access.type = System::PA_READ;
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
  // the write port (0xF400 - 0xF800, 1024 bytes), in which case an
  // unwanted write is triggered
  uInt8 value = mySystem->getDataBusState(0xFF);

  if(bankLocked())
    return value;
  else
  {
    triggerReadFromWritePort(address);
    return myRAM[address & 0x03FF] = value;
  }
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
const uInt8* CartridgeCV::getImage(uInt32& size) const
{
  size = 2048;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCV::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putByteArray(myRAM, 1024);
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
    if(in.getString() != name())
      return false;

    in.getByteArray(myRAM, 1024);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeCV::load" << endl;
    return false;
  }

  return true;
}
