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
#include "CartFC.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFC::CartridgeFC(const ByteBuffer& image, size_t size,
                         const string& md5, const Settings& settings)
  : Cartridge(settings, md5),
    mySize(size)
{
  // Copy the ROM image into my buffer
  std::copy_n(image.get(), std::min(myImage.size(), size), myImage.begin());
  createCodeAccessBase(myImage.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFC::reset()
{
  initializeStartBank(0);
  myTargetBank = 0;

  // Upon reset we switch to the reset bank
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFC::install(System& system)
{
  mySystem = &system;

  // Install pages for the startup bank
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeFC::peek(uInt16 address)
{
  address &= 0x0FFF;

  // Switch banks if necessary
  if(address == 0x0FFC)
  {
    // Trigger the bank switch
    bank(myTargetBank);
  }

  return myImage[myBankOffset + address];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFC::poke(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  // Switch banks if necessary
  switch (address)
  {
    case 0x0FF8:
      // Set the two lowest bits of target 4k bank
      myTargetBank = value & 0b11;
      break;

    case 0x0FF9:
      // Set the high bits of target 4k bank
      if (value << 2 < bankCount())
      {
        myTargetBank += value << 2;
        myTargetBank %= bankCount();
      }
      else
        // special handling when both values are identical (e.g. 4/4 or 5/5)
        myTargetBank = value % bankCount();
      break;

    case 0x0FFC:
      // Trigger the bank switch
      bank(myTargetBank);
      break;

    default:
      break;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFC::bank(uInt16 bank)
{
  if (bankLocked()) return false;

  // Remember what bank we're in
  myBankOffset = bank << 12;

  System::PageAccess access(this, System::PageAccessType::READ);

  // Set the page accessing methods for the hot spots
  for (uInt16 addr = (0x1FF8 & ~System::PAGE_MASK); addr < 0x2000;
       addr += System::PAGE_SIZE)
  {
    access.codeAccessBase = &myCodeAccessBase[myBankOffset + (addr & 0x0FFF)];
    mySystem->setPageAccess(addr, access);
  }

  // Setup the page access methods for the current bank
  for (uInt16 addr = 0x1000; addr < static_cast<uInt16>(0x1FF8U & ~System::PAGE_MASK);
       addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[myBankOffset + (addr & 0x0FFF)];
    access.codeAccessBase = &myCodeAccessBase[myBankOffset + (addr & 0x0FFF)];
    mySystem->setPageAccess(addr, access);
  }
  myCurrentBank = myTargetBank;
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeFC::getBank(uInt16) const
{
  return myBankOffset >> 12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeFC::bankCount() const
{
  return uInt16(mySize >> 12);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFC::patch(uInt16 address, uInt8 value)
{
  myImage[myBankOffset + (address & 0x0FFF)] = value;
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeFC::getImage(size_t& size) const
{
  size = mySize;
  return myImage.data();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFC::save(Serializer& out) const
{
  try
  {
    out.putShort(myBankOffset);
  }
  catch (...)
  {
    cerr << "ERROR: CartridgeFC::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFC::load(Serializer& in)
{
  try
  {
    myBankOffset = in.getShort();
  }
  catch (...)
  {
    cerr << "ERROR: CartridgeFC::load" << endl;
    return false;
  }

  // Remember what bank we were in
  bank(myBankOffset >> 12);

  return true;
}
