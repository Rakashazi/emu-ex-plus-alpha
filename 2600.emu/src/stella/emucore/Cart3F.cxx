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
#include "TIA.hxx"
#include "Cart3F.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3F::Cartridge3F(const ByteBuffer& image, size_t size,
                         const string& md5, const Settings& settings)
  : Cartridge(settings, md5),
    mySize(size)
{
  // Allocate array for the ROM image
  myImage = make_unique<uInt8[]>(mySize);

  // Copy the ROM image into my buffer
  std::copy_n(image.get(), mySize, myImage.get());
  createCodeAccessBase(mySize);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3F::reset()
{
  initializeStartBank(bankCount() - 1);

  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3F::install(System& system)
{
  mySystem = &system;

  System::PageAccess access(this, System::PageAccessType::READWRITE);

  // The hotspot ($3F) is in TIA address space, so we claim it here
  for(uInt16 addr = 0x00; addr < 0x40; addr += System::PAGE_SIZE)
    mySystem->setPageAccess(addr, access);

  // Setup the second segment to always point to the last ROM slice
  access.type = System::PageAccessType::READ;
  for(uInt16 addr = 0x1800; addr < 0x2000; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[(mySize - 2048) + (addr & 0x07FF)];
    access.codeAccessBase = &myCodeAccessBase[(mySize - 2048) + (addr & 0x07FF)];
    mySystem->setPageAccess(addr, access);
  }

  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Cartridge3F::peek(uInt16 address)
{
  address &= 0x0FFF;

  if(address < 0x0800)
    return myImage[(address & 0x07FF) + (myCurrentBank << 11)];
  else
    return myImage[(address & 0x07FF) + mySize - 2048];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3F::poke(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  // Switch banks if necessary
  if(address <= 0x003F)
    bank(value);

  // Handle TIA space that we claimed above
  mySystem->tia().poke(address, value);

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3F::bank(uInt16 bank)
{
  if(bankLocked())
    return false;

  // Make sure the bank they're asking for is reasonable
  if((uInt32(bank) << 11) < mySize)
  {
    myCurrentBank = bank;
  }
  else
  {
    // Oops, the bank they're asking for isn't valid so let's wrap it
    // around to a valid bank number
    myCurrentBank = bank % (mySize >> 11);
  }

  uInt32 offset = myCurrentBank << 11;

  // Setup the page access methods for the current bank
  System::PageAccess access(this, System::PageAccessType::READ);

  // Map ROM image into the system
  for(uInt16 addr = 0x1000; addr < 0x1800; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[offset + (addr & 0x07FF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (addr & 0x07FF)];
    mySystem->setPageAccess(addr, access);
  }
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Cartridge3F::getBank(uInt16 address) const
{
  if (address & 0x800)
    return uInt16((mySize >> 11) - 1); // 2K slices, fixed bank
  else
    return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Cartridge3F::bankCount() const
{
  return uInt16(mySize >> 11); // 2K slices
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3F::patch(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  if(address < 0x0800)
    myImage[(address & 0x07FF) + (myCurrentBank << 11)] = value;
  else
    myImage[(address & 0x07FF) + mySize - 2048] = value;

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* Cartridge3F::getImage(size_t& size) const
{
  size = mySize;
  return myImage.get();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3F::save(Serializer& out) const
{
  try
  {
    out.putShort(myCurrentBank);
  }
  catch(...)
  {
    cerr << "ERROR: Cartridge3F::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3F::load(Serializer& in)
{
  try
  {
    myCurrentBank = in.getShort();
  }
  catch(...)
  {
    cerr << "ERROR: Cartridge3F::load" << endl;
    return false;
  }

  // Now, go to the current bank
  bank(myCurrentBank);

  return true;
}
