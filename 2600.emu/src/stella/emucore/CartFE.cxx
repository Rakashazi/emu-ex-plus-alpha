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

#include "M6532.hxx"
#include "System.hxx"
#include "CartFE.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFE::CartridgeFE(const ByteBuffer& image, size_t size,
                         const string& md5, const Settings& settings)
  : Cartridge(settings, md5)
{
  // Copy the ROM image into my buffer
  std::copy_n(image.get(), std::min(myImage.size(), size), myImage.begin());
  createCodeAccessBase(myImage.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFE::reset()
{
  // Decathlon requires this, since there is no startup vector in bank 1
  initializeStartBank(0);

  bank(startBank());
  myLastAccessWasFE = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFE::install(System& system)
{
  mySystem = &system;

  // The hotspot $01FE is in a mirror of zero-page RAM
  // We need to claim access to it here, and deal with it in peek/poke below
  System::PageAccess access(this, System::PageAccessType::READWRITE);
  for(uInt16 addr = 0x180; addr < 0x200; addr += System::PAGE_SIZE)
    mySystem->setPageAccess(addr, access);

  // Map all of the cart accesses to call peek and poke
  access.type = System::PageAccessType::READ;
  for(uInt16 addr = 0x1000; addr < 0x2000; addr += System::PAGE_SIZE)
    mySystem->setPageAccess(addr, access);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeFE::peek(uInt16 address)
{
  uInt8 value = (address < 0x200) ? mySystem->m6532().peek(address) :
      myImage[myBankOffset + (address & 0x0FFF)];

  // Check if we hit hotspot
  checkBankSwitch(address, value);

  return value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::poke(uInt16 address, uInt8 value)
{
  if(address < 0x200)
    mySystem->m6532().poke(address, value);

  // Check if we hit hotspot
  checkBankSwitch(address, value);

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFE::checkBankSwitch(uInt16 address, uInt8 value)
{
  if(bankLocked())
    return;

  // Did we detect $01FE on the last address bus access?
  // If so, we bankswitch according to the upper 3 bits of the data bus
  // NOTE: see the header file for the significance of 'value & 0x20'
  if(myLastAccessWasFE)
    bank((value & 0x20) ? 0 : 1);

  // On the next cycle, we use the (then) current data bus value to decode
  // the bank to use
  myLastAccessWasFE = address == 0x01FE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::bank(uInt16 bank)
{
  if(bankLocked())
    return false;

  myBankOffset = bank << 12;
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeFE::getBank(uInt16) const
{
  return myBankOffset >> 12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeFE::bankCount() const
{
  return 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::patch(uInt16 address, uInt8 value)
{
  myImage[myBankOffset + (address & 0x0FFF)] = value;
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeFE::getImage(size_t& size) const
{
  size = myImage.size();
  return myImage.data();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::save(Serializer& out) const
{
  try
  {
    out.putShort(myBankOffset);
    out.putBool(myLastAccessWasFE);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeFE::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::load(Serializer& in)
{
  try
  {
    myBankOffset = in.getShort();
    myLastAccessWasFE = in.getBool();
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeF8SC::load" << endl;
    return false;
  }

  return true;
}
