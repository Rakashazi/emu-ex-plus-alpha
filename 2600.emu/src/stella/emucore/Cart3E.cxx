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
#include "Cart3E.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3E::Cartridge3E(const BytePtr& image, uInt32 size,
                         const Settings& settings)
  : Cartridge(settings),
    mySize(size),
    myCurrentBank(0)
{
  // Allocate array for the ROM image
  myImage = make_unique<uInt8[]>(mySize);

  // Copy the ROM image into my buffer
  memcpy(myImage.get(), image.get(), mySize);
  createCodeAccessBase(mySize + 32768);

  // Remember startup bank
  myStartBank = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3E::reset()
{
  initializeRAM(myRAM, 32768);

  // We'll map the startup bank into the first segment upon reset
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3E::install(System& system)
{
  mySystem = &system;

  System::PageAccess access(this, System::PA_READWRITE);

  // The hotspots ($3E and $3F) are in TIA address space, so we claim it here
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

  // Install pages for the startup bank into the first segment
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Cartridge3E::peek(uInt16 address)
{
  uInt16 peekAddress = address;
  address &= 0x0FFF;

  if(address < 0x0800)
  {
    if(myCurrentBank < 256)
      return myImage[(address & 0x07FF) + (myCurrentBank << 11)];
    else
    {
      if(address < 0x0400)
        return myRAM[(address & 0x03FF) + ((myCurrentBank - 256) << 10)];
      else
      {
        // Reading from the write port triggers an unwanted write
        uInt8 value = mySystem->getDataBusState(0xFF);

        if(bankLocked())
          return value;
        else
        {
          triggerReadFromWritePort(peekAddress);
          return myRAM[(address & 0x03FF) + ((myCurrentBank - 256) << 10)] = value;
        }
      }
    }
  }
  else
  {
    return myImage[(address & 0x07FF) + mySize - 2048];
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3E::poke(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  // Switch banks if necessary. Armin (Kroko) says there are no mirrored
  // hotspots.
  if(address == 0x003F)
  {
    bank(value);
  }
  else if(address == 0x003E)
  {
    bank(value + 256);
  }

  // Handle TIA space that we claimed above
  mySystem->tia().poke(address, value);

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3E::bank(uInt16 bank)
{
  if(bankLocked()) return false;

  if(bank < 256)
  {
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
  }
  else
  {
    bank -= 256;
    bank %= 32;
    myCurrentBank = bank + 256;

    uInt32 offset = bank << 10;

    // Setup the page access methods for the current bank
    System::PageAccess access(this, System::PA_READ);

    // Map read-port RAM image into the system
    for(uInt16 addr = 0x1000; addr < 0x1400; addr += System::PAGE_SIZE)
    {
      access.directPeekBase = &myRAM[offset + (addr & 0x03FF)];
      access.codeAccessBase = &myCodeAccessBase[mySize + offset + (addr & 0x03FF)];
      mySystem->setPageAccess(addr, access);
    }

    access.directPeekBase = nullptr;
    access.type = System::PA_WRITE;

    // Map write-port RAM image into the system
    for(uInt16 addr = 0x1400; addr < 0x1800; addr += System::PAGE_SIZE)
    {
      access.directPokeBase = &myRAM[offset + (addr & 0x03FF)];
      access.codeAccessBase = &myCodeAccessBase[mySize + offset + (addr & 0x03FF)];
      mySystem->setPageAccess(addr, access);
    }
  }
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Cartridge3E::getBank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Cartridge3E::bankCount() const
{
  // Because the RAM banks always start at 256 and above, we require the
  // number of ROM banks to be 256
  // If the RAM banks were simply appended to the number of actual
  // ROM banks, bank numbers would be ambiguous (ie, would bank 128 be
  // the last bank of ROM, or one of the banks of RAM?)
  return 256 + 32;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3E::patch(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  if(address < 0x0800)
  {
    if(myCurrentBank < 256)
      myImage[(address & 0x07FF) + (myCurrentBank << 11)] = value;
    else
      myRAM[(address & 0x03FF) + ((myCurrentBank - 256) << 10)] = value;
  }
  else
    myImage[(address & 0x07FF) + mySize - 2048] = value;

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* Cartridge3E::getImage(uInt32& size) const
{
  size = mySize;
  return myImage.get();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3E::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putShort(myCurrentBank);
    out.putByteArray(myRAM, 32768);
  }
  catch(...)
  {
    cerr << "ERROR: Cartridge3E::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3E::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    myCurrentBank = in.getShort();
    in.getByteArray(myRAM, 32768);
  }
  catch(...)
  {
    cerr << "ERROR: Cartridge3E::load" << endl;
    return false;
  }

  // Now, go to the current bank
  bank(myCurrentBank);

  return true;
}
