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
#include "CartCVPlus.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCVPlus::CartridgeCVPlus(const ByteBuffer& image, size_t size,
                                 const string& md5, const Settings& settings)
  : Cartridge(settings, md5),
    mySize(size)
{
  // Allocate array for the ROM image
  myImage = make_unique<uInt8[]>(mySize);

  // Copy the ROM image into my buffer
  std::copy_n(image.get(), mySize, myImage.get());
  createCodeAccessBase(mySize + myRAM.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCVPlus::reset()
{
  initializeRAM(myRAM.data(), myRAM.size());
  initializeStartBank(0);

  // We'll map the startup bank into the first segment upon reset
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCVPlus::install(System& system)
{
  mySystem = &system;

  System::PageAccess access(this, System::PageAccessType::READWRITE);

  // The hotspot ($3D) is in TIA address space, so we claim it here
  for(uInt16 addr = 0x00; addr < 0x40; addr += System::PAGE_SIZE)
    mySystem->setPageAccess(addr, access);

  // Set the page accessing method for the RAM writing pages
  // Map access to this class, since we need to inspect all accesses to
  // check if RWP happens
  access.directPeekBase = access.directPokeBase = nullptr;
  access.codeAccessBase = nullptr;
  access.type = System::PageAccessType::WRITE;
  for(uInt16 addr = 0x1400; addr < 0x1800; addr += System::PAGE_SIZE)
  {
    access.codeAccessBase = &myCodeAccessBase[mySize + (addr & 0x03FF)];
    mySystem->setPageAccess(addr, access);
  }

  // Set the page accessing method for the RAM reading pages
  access.type = System::PageAccessType::READ;
  for(uInt16 addr = 0x1000; addr < 0x1400; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myRAM[addr & 0x03FF];
    access.codeAccessBase = &myCodeAccessBase[mySize + (addr & 0x03FF)];
    mySystem->setPageAccess(addr, access);
  }

  // Install pages for the startup bank into the first segment
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeCVPlus::peek(uInt16 address)
{
  if((address & 0x0FFF) < 0x0800)  // Write port is at 0xF400 - 0xF7FF (1024 bytes)
    return peekRAM(myRAM[address & 0x03FF], address);
  else
    return myImage[(address & 0x07FF) + (myCurrentBank << 11)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCVPlus::poke(uInt16 address, uInt8 value)
{
  uInt16 pokeAddress = address;
  address &= 0x0FFF;

  if(address < 0x0040)
  {
    // Switch banks if necessary
    if(address == 0x003D)
      bank(value);

    // Handle TIA space that we claimed above
    return mySystem->tia().poke(address, value);
  }
  else
    pokeRAM(myRAM[address & 0x03FF], pokeAddress, value);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCVPlus::bank(uInt16 bank)
{
  if(bankLocked()) return false;

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
  for(uInt16 addr = 0x1800; addr < 0x2000; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[offset + (addr & 0x07FF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (addr & 0x07FF)];
    mySystem->setPageAccess(addr, access);
  }

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeCVPlus::getBank(uInt16) const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeCVPlus::bankCount() const
{
  return uInt16(mySize >> 11);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCVPlus::patch(uInt16 address, uInt8 value)
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
    myImage[(address & 0x07FF) + (myCurrentBank << 11)] = value;

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeCVPlus::getImage(size_t& size) const
{
  size = mySize;
  return myImage.get();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCVPlus::save(Serializer& out) const
{
  try
  {
    out.putShort(myCurrentBank);
    out.putByteArray(myRAM.data(), myRAM.size());
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeCVPlus::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCVPlus::load(Serializer& in)
{
  try
  {
    myCurrentBank = in.getShort();
    in.getByteArray(myRAM.data(), myRAM.size());
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeCVPlus::load" << endl;
    return false;
  }

  // Now, go to the current bank
  bank(myCurrentBank);

  return true;
}
