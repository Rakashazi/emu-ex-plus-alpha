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
#include "CartUA.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeUA::CartridgeUA(const ByteBuffer& image, size_t size,
                         const string& md5, const Settings& settings,
                         bool swapHotspots)
  : Cartridge(settings, md5),
    mySwappedHotspots(swapHotspots)
{
  // Copy the ROM image into my buffer
  std::copy_n(image.get(), std::min(myImage.size(), size), myImage.begin());
  createCodeAccessBase(myImage.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeUA::reset()
{
  // Upon reset we switch to the startup bank
  initializeStartBank(0);
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeUA::install(System& system)
{
  mySystem = &system;

  // Get the page accessing methods for the hot spots since they overlap
  // areas within the TIA we'll need to forward requests to the TIA
  myHotSpotPageAccess[0] = mySystem->getPageAccess(0x0220);
  myHotSpotPageAccess[1] = mySystem->getPageAccess(0x0220 | 0x80);

  // Set the page accessing methods for the hot spots
  System::PageAccess access(this, System::PageAccessType::READ);
  mySystem->setPageAccess(0x0220, access);
  mySystem->setPageAccess(0x0240, access);
  mySystem->setPageAccess(0x0220 | 0x80, access);
  mySystem->setPageAccess(0x0240 | 0x80, access);

  // Install pages for the startup bank
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeUA::peek(uInt16 address)
{
  address &= 0x1FFF;

  // Switch banks if necessary
  switch(address & 0x1260)
  {
    case 0x0220:
      // Set the current bank to the lower 4k bank
      bank(mySwappedHotspots ? 1 : 0);
      break;

    case 0x0240:
      // Set the current bank to the upper 4k bank
      bank(mySwappedHotspots ? 0 : 1);
      break;

    default:
      break;
  }

  // Because of the way accessing is set up, we will only get here
  // when doing a TIA read
  int hotspot = ((address & 0x80) >> 7);
  return myHotSpotPageAccess[hotspot].device->peek(address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeUA::poke(uInt16 address, uInt8 value)
{
  address &= 0x1FFF;

  // Switch banks if necessary
  switch(address & 0x1260)
  {
    case 0x0220:
      // Set the current bank to the lower 4k bank
      bank(mySwappedHotspots ? 1 : 0);
      break;

    case 0x0240:
      // Set the current bank to the upper 4k bank
      bank(mySwappedHotspots ? 0 : 1);
      break;

    default:
      break;
  }

  // Because of the way accessing is set up, we will may get here by
  // doing a write to TIA or cart; we ignore the cart write
  if (!(address & 0x1000))
  {
    int hotspot = ((address & 0x80) >> 7);
    myHotSpotPageAccess[hotspot].device->poke(address, value);
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeUA::bank(uInt16 bank)
{
  if(bankLocked()) return false;

  // Remember what bank we're in
  myBankOffset = bank << 12;

  // Setup the page access methods for the current bank
  System::PageAccess access(this, System::PageAccessType::READ);

  // Map ROM image into the system
  for(uInt16 addr = 0x1000; addr < 0x2000; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[myBankOffset + (addr & 0x0FFF)];
    access.codeAccessBase = &myCodeAccessBase[myBankOffset + (addr & 0x0FFF)];
    mySystem->setPageAccess(addr, access);
  }
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeUA::getBank(uInt16) const
{
  return myBankOffset >> 12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeUA::bankCount() const
{
  return 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeUA::patch(uInt16 address, uInt8 value)
{
  myImage[myBankOffset + (address & 0x0FFF)] = value;
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeUA::getImage(size_t& size) const
{
  size = myImage.size();
  return myImage.data();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeUA::save(Serializer& out) const
{
  try
  {
    out.putShort(myBankOffset);
  }
  catch(...)
  {
    cerr << "ERROR: " << name() << "::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeUA::load(Serializer& in)
{
  try
  {
    myBankOffset = in.getShort();
  }
  catch(...)
  {
    cerr << "ERROR: " << name() << "::load" << endl;
    return false;
  }

  // Remember what bank we were in
  bank(myBankOffset >> 12);

  return true;
}
