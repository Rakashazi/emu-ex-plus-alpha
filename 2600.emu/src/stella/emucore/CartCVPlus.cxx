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
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CartCVPlus.cxx 3316 2016-08-24 23:57:07Z stephena $
//============================================================================

#include "System.hxx"
#include "TIA.hxx"
#include "CartCVPlus.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCVPlus::CartridgeCVPlus(const uInt8* image, uInt32 size,
                                 const Settings& settings)
  : Cartridge(settings),
    mySize(size),
    myCurrentBank(0)
{
  // Allocate array for the ROM image
  myImage = make_ptr<uInt8[]>(mySize);

  // Copy the ROM image into my buffer
  memcpy(myImage.get(), image, mySize);
  createCodeAccessBase(mySize + 1024);

  // Remember startup bank
  myStartBank = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCVPlus::reset()
{
  initializeRAM(myRAM, 1024);

  // We'll map the startup bank into the first segment upon reset
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCVPlus::install(System& system)
{
  mySystem = &system;

  System::PageAccess access(this, System::PA_READWRITE);

  // Set the page accessing methods for the hot spots (for 100% emulation
  // we need to chain any accesses below 0x40 to the TIA. Our poke() method
  // does this via mySystem->tiaPoke(...), at least until we come up with a
  // cleaner way to do it).
  for(uInt32 i = 0x00; i < 0x40; i += (1 << System::PAGE_SHIFT))
    mySystem->setPageAccess(i >> System::PAGE_SHIFT, access);

  // Set the page accessing method for the RAM writing pages
  access.directPeekBase = 0;
  access.codeAccessBase = 0;
  access.type = System::PA_WRITE;
  for(uInt32 j = 0x1400; j < 0x1800; j += (1 << System::PAGE_SHIFT))
  {
    access.directPokeBase = &myRAM[j & 0x03FF];
    access.codeAccessBase = &myCodeAccessBase[mySize + (j & 0x03FF)];
    mySystem->setPageAccess(j >> System::PAGE_SHIFT, access);
  }

  // Set the page accessing method for the RAM reading pages
  access.directPokeBase = 0;
  access.type = System::PA_READ;
  for(uInt32 k = 0x1000; k < 0x1400; k += (1 << System::PAGE_SHIFT))
  {
    access.directPeekBase = &myRAM[k & 0x03FF];
    access.codeAccessBase = &myCodeAccessBase[mySize + (k & 0x03FF)];
    mySystem->setPageAccess(k >> System::PAGE_SHIFT, access);
  }

  // Install pages for the startup bank into the first segment
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeCVPlus::peek(uInt16 address)
{
  if((address & 0x0FFF) < 0x0800)  // Write port is at 0xF400 - 0xF800 (1024 bytes)
  {                                // Read port is handled in ::install()
    // Reading from the write port triggers an unwanted write
    uInt8 value = mySystem->getDataBusState(0xFF);

    if(bankLocked())
      return value;
    else
    {
      triggerReadFromWritePort(address);
      return myRAM[address & 0x03FF] = value;
    }
  }
  else
    return myImage[(address & 0x07FF) + (myCurrentBank << 11)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCVPlus::poke(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  // Switch banks if necessary
  if(address == 0x003D)
    bank(value);

  // Pass the poke through to the TIA. In a real Atari, both the cart and the
  // TIA see the address lines, and both react accordingly. In Stella, each
  // 64-byte chunk of address space is "owned" by only one device. If we
  // don't chain the poke to the TIA, then the TIA can't see it...
  mySystem->tia().poke(address, value);

  return false;
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
  System::PageAccess access(this, System::PA_READ);

  // Map ROM image into the system
  for(uInt32 address = 0x1800; address < 0x2000;
      address += (1 << System::PAGE_SHIFT))
  {
    access.directPeekBase = &myImage[offset + (address & 0x07FF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x07FF)];
    mySystem->setPageAccess(address >> System::PAGE_SHIFT, access);
  }

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeCVPlus::getBank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeCVPlus::bankCount() const
{
  return mySize >> 11;
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
const uInt8* CartridgeCVPlus::getImage(int& size) const
{
  size = mySize;
  return myImage.get();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCVPlus::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putShort(myCurrentBank);
    out.putByteArray(myRAM, 1024);
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
    if(in.getString() != name())
      return false;

    myCurrentBank = in.getShort();
    in.getByteArray(myRAM, 1024);
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
