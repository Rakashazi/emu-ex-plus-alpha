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
#include "CartF0.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeF0::CartridgeF0(const ByteBuffer& image, size_t size,
                         const string& md5, const Settings& settings)
  : Cartridge(settings, md5)
{
  // Copy the ROM image into my buffer
  std::copy_n(image.get(), std::min(myImage.size(), size), myImage.begin());
  createCodeAccessBase(myImage.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeF0::reset()
{
  // Upon reset we switch to the startup bank
  initializeStartBank(15);
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeF0::install(System& system)
{
  mySystem = &system;

  // Install pages for the startup bank
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeF0::peek(uInt16 address)
{
  address &= 0x0FFF;

  // Switch to next bank
  if(address == 0x0FF0)
    incbank();

  return myImage[myBankOffset + address];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF0::poke(uInt16 address, uInt8)
{
  address &= 0x0FFF;

  // Switch to next bank
  if(address == 0x0FF0)
    incbank();

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeF0::incbank()
{
  // Determine current bank, and increment to the next one
  uInt8 nextBank = ((myBankOffset >> 12) + 1) & 0x0F;
  bank(nextBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF0::bank(uInt16 bank)
{
  if(bankLocked()) return false;

  // Remember what bank we're in
  myBankOffset = bank << 12;

  System::PageAccess access(this, System::PageAccessType::READ);

  // Set the page accessing methods for the hot spots
  for(uInt16 addr = (0x1FF0 & ~System::PAGE_MASK); addr < 0x2000;
      addr += System::PAGE_SIZE)
  {
    access.codeAccessBase = &myCodeAccessBase[myBankOffset + (addr & 0x0FFF)];
    mySystem->setPageAccess(addr, access);
  }

  // Setup the page access methods for the current bank
  for(uInt16 addr = 0x1000; addr < static_cast<uInt16>(0x1FF0U & ~System::PAGE_MASK);
      addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[myBankOffset + (addr & 0x0FFF)];
    access.codeAccessBase = &myCodeAccessBase[myBankOffset + (addr & 0x0FFF)];
    mySystem->setPageAccess(addr, access);
  }

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeF0::getBank(uInt16) const
{
  return myBankOffset >> 12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeF0::bankCount() const
{
  return 16;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF0::patch(uInt16 address, uInt8 value)
{
  myImage[myBankOffset + (address & 0x0FFF)] = value;
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeF0::getImage(size_t& size) const
{
  size = myImage.size();
  return myImage.data();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF0::save(Serializer& out) const
{
  try
  {
    out.putShort(myBankOffset);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeF0::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF0::load(Serializer& in)
{
  try
  {
    myBankOffset = in.getShort();
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeF0::load" << endl;
    return false;
  }

  // Remember what bank we were in
  bank(myBankOffset >> 12);

  return true;
}
