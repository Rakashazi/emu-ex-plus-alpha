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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CartWD.cxx 3136 2015-01-01 16:50:18Z stephena $
//============================================================================

#include <cstring>

#include "TIA.hxx"
#include "M6502.hxx"
#include "System.hxx"
#include "CartWD.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeWD::CartridgeWD(const uInt8* image, uInt32 size,
                         const Settings& settings)
  : Cartridge(settings),
    mySize(BSPF_min(8195u, size))
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image, mySize);
  createCodeAccessBase(8192);

  // Remember startup bank
  myStartBank = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeWD::~CartridgeWD()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWD::reset()
{
  // Initialize RAM
  if(mySettings.getBool("ramrandom"))
    for(uInt32 i = 0; i < 64; ++i)
      myRAM[i] = mySystem->randGenerator().next();
  else
    memset(myRAM, 0, 64);

  myCyclesAtBankswitchInit = 0;
  myPendingBank = 0xF0;  // one more than the allowable bank #

  // Setup segments to some default slices
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWD::install(System& system)
{
  mySystem = &system;

  // Set the page accessing method for the RAM reading pages
  System::PageAccess read(this, System::PA_READ);
  for(uInt32 k = 0x1000; k < 0x1040; k += (1 << System::PAGE_SHIFT))
  {
    read.directPeekBase = &myRAM[k & 0x003F];
    read.codeAccessBase = &myCodeAccessBase[k & 0x003F];
    mySystem->setPageAccess(k >> System::PAGE_SHIFT, read);
  }

  // Set the page accessing method for the RAM writing pages
  System::PageAccess write(this, System::PA_WRITE);
  for(uInt32 j = 0x1040; j < 0x1080; j += (1 << System::PAGE_SHIFT))
  {
    write.directPokeBase = &myRAM[j & 0x003F];
    write.codeAccessBase = &myCodeAccessBase[j & 0x003F];
    mySystem->setPageAccess(j >> System::PAGE_SHIFT, write);
  }

  // Mirror all access in TIA; by doing so we're taking responsibility
  // for that address space in peek and poke below.
  mySystem->tia().install(system, *this);

  // Setup segments to some default slices
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeWD::peek(uInt16 address)
{
  // Is it time to do an actual bankswitch?
  if(myPendingBank != 0xF0 && !bankLocked() &&
     mySystem->cycles() > (myCyclesAtBankswitchInit + 3))
  {
    bank(myPendingBank);
    myPendingBank = 0xF0;
  }

  if(!(address & 0x1000))   // Hotspots below 0x1000 are also TIA addresses
  {
    // Hotspots at $30 - $3F
    // Note that a hotspot read triggers a bankswitch after at least 3 cycles
    // have passed, so we only initiate the switch here
    if(!bankLocked() && (address & 0x00FF) >= 0x30 && (address & 0x00FF) <= 0x3F)
    {
      myCyclesAtBankswitchInit = mySystem->cycles();
      myPendingBank = address & 0x000F;
    }
    return mySystem->tia().peek(address);
  }
  else
  {
    uInt16 peekAddress = address;
    address &= 0x0FFF;

    if(address < 0x0040)        // RAM read port
      return myRAM[address];
    else if(address < 0x0080)   // RAM write port
    {
      // Reading from the write port @ $1040 - $107F triggers an unwanted write
      uInt8 value = mySystem->getDataBusState(0xFF);

      if(bankLocked())
        return value;
      else
      {
        triggerReadFromWritePort(peekAddress);
        return myRAM[address & 0x003F] = value;
      }
    }
    else if(address < 0x0400)
      return myImage[myOffset[0] + (address & 0x03FF)];
    else if(address < 0x0800)
      return myImage[myOffset[1] + (address & 0x03FF)];
    else if(address < 0x0C00)
      return myImage[myOffset[2] + (address & 0x03FF)];
    else
      return mySegment3[address & 0x03FF];
  }

  return 0;  // We'll never reach this
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeWD::poke(uInt16 address, uInt8 value)
{
  // Only TIA writes will reach here
  if(!(address & 0x1000))
    return mySystem->tia().poke(address, value);
  else  
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeWD::bank(uInt16 bank)
{
  if(bankLocked() || bank > 15) return false;

  myCurrentBank = bank;

  segmentZero(ourBankOrg[bank].zero);
  segmentOne(ourBankOrg[bank].one);
  segmentTwo(ourBankOrg[bank].two);
  segmentThree(ourBankOrg[bank].three, ourBankOrg[bank].map3bytes);

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWD::segmentZero(uInt8 slice)
{
  uInt16 offset = slice << 10;
  System::PageAccess access(this, System::PA_READ);

  // Skip first 128 bytes; it is always RAM
  for(uInt32 address = 0x1080; address < 0x1400;
      address += (1 << System::PAGE_SHIFT))
  {
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x03FF)];
    mySystem->setPageAccess(address >> System::PAGE_SHIFT, access);
  }
  myOffset[0] = offset;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWD::segmentOne(uInt8 slice)
{
  uInt16 offset = slice << 10;
  System::PageAccess access(this, System::PA_READ);

  for(uInt32 address = 0x1400; address < 0x1800;
      address += (1 << System::PAGE_SHIFT))
  {
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x03FF)];
    mySystem->setPageAccess(address >> System::PAGE_SHIFT, access);
  }
  myOffset[1] = offset;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWD::segmentTwo(uInt8 slice)
{
  uInt16 offset = slice << 10;
  System::PageAccess access(this, System::PA_READ);

  for(uInt32 address = 0x1800; address < 0x1C00;
      address += (1 << System::PAGE_SHIFT))
  {
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x03FF)];
    mySystem->setPageAccess(address >> System::PAGE_SHIFT, access);
  }
  myOffset[2] = offset;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWD::segmentThree(uInt8 slice, bool map3bytes)
{
  uInt16 offset = slice << 10;

  // Make a copy of the address space pointed to by the slice
  // Then map in the extra 3 bytes, if required
  memcpy(mySegment3, myImage+offset, 1024);
  if(mySize == 8195 && map3bytes)
  {
    mySegment3[0x3FC] = myImage[0x2000+0];
    mySegment3[0x3FD] = myImage[0x2000+1];
    mySegment3[0x3FE] = myImage[0x2000+2];
  }

  System::PageAccess access(this, System::PA_READ);

  for(uInt32 address = 0x1C00; address < 0x2000;
      address += (1 << System::PAGE_SHIFT))
  {
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x03FF)];
    mySystem->setPageAccess(address >> System::PAGE_SHIFT, access);
  }
  myOffset[3] = offset;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeWD::getBank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeWD::bankCount() const
{
  return 16;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeWD::patch(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  uInt16 idx = address >> 10;
  myImage[myOffset[idx] + (address & 0x03FF)] = value;

  // The upper segment is mirrored, so we need to patch its buffer too
  if(idx == 3)
    mySegment3[(address & 0x03FF)] = value;

  return true;
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeWD::getImage(int& size) const
{
  size = mySize;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeWD::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putShort(myCurrentBank);
    out.putByteArray(myRAM, 64);
    out.putInt(myCyclesAtBankswitchInit);
    out.putShort(myPendingBank);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeWD::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeWD::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    myCurrentBank = in.getShort();
    in.getByteArray(myRAM, 64);
    myCyclesAtBankswitchInit = in.getInt();
    myPendingBank = in.getShort();

    bank(myCurrentBank);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeWD::load" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeWD::BankOrg CartridgeWD::ourBankOrg[16] = {
  { 0, 0, 1, 2, false },  // Bank 0
  { 0, 1, 3, 2, false },  // Bank 1
  { 4, 5, 6, 7, false },  // Bank 2
  { 7, 4, 3, 2, false },  // Bank 3
  { 0, 0, 6, 7, false },  // Bank 4
  { 0, 1, 7, 6, false },  // Bank 5
  { 3, 2, 4, 5, false },  // Bank 6
  { 6, 0, 5, 1, false },  // Bank 7
  { 0, 0, 1, 2, false },  // Bank 8
  { 0, 1, 3, 2, false },  // Bank 9
  { 4, 5, 6, 7, false },  // Bank 10
  { 7, 4, 3, 2, false },  // Bank 11
  { 0, 0, 6, 7, true  },  // Bank 12
  { 0, 1, 7, 6, true  },  // Bank 13
  { 3, 2, 4, 5, true  },  // Bank 14
  { 6, 0, 5, 1, true  }   // Bank 15
};
