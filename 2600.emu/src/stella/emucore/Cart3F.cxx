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
#include "TIA.hxx"
#include "Cart3F.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3F::Cartridge3F(const BytePtr& image, uInt32 size,
                         const Settings& settings)
  : Cartridge(settings),
    mySize(size),
    myCurrentBank(0)
{
  // Allocate array for the ROM image
  myImage = make_unique<uInt8[]>(mySize);

  // Copy the ROM image into my buffer
  memcpy(myImage.get(), image.get(), mySize);
  createCodeAccessBase(mySize);

  // Remember startup bank
  myStartBank = bankCount() - 1; // last bank
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3F::reset()
{
  // define random startup banks
  // Note: This works for all Tigervision ROMs except for one version of Polaris
  //   (md5: 203049f4d8290bb4521cc4402415e737) which requires 3 as startup bank
  //   (or non-randomized RAM). All other ROMs take care of other startup banks.
  //   The problematic version is most likely an incorrect dump with wrong
  //   startup vectors.
  randomizeStartBank();

  // We'll map the startup bank into the first segment upon reset
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3F::install(System& system)
{
  mySystem = &system;

  System::PageAccess access(this, System::PA_READWRITE);

  // The hotspot ($3F) is in TIA address space, so we claim it here
  for(uInt16 addr = 0x00; addr < 0x40; addr += System::PAGE_SIZE)
    mySystem->setPageAccess(addr, access);

  // Setup the second segment to always point to the last ROM slice
  access.type = System::PA_READ;
  for(uInt16 addr = 0x1800; addr < 0x2000; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[(mySize - 2048) + (addr & 0x07FF)];
    access.codeAccessBase = &myCodeAccessBase[(mySize - 2048) + (addr & 0x07FF)];
    mySystem->setPageAccess(addr, access);
  }

  // Install pages for startup bank into the first segment
  bank(myStartBank);
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
  System::PageAccess access(this, System::PA_READ);

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
uInt16 Cartridge3F::getBank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Cartridge3F::bankCount() const
{
  return mySize >> 11;
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
const uInt8* Cartridge3F::getImage(uInt32& size) const
{
  size = mySize;
  return myImage.get();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3F::save(Serializer& out) const
{
  try
  {
    out.putString(name());
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
    if(in.getString() != name())
      return false;

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
