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

#include "TIA.hxx"
#include "M6502.hxx"
#include "System.hxx"
#include "CartWD.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeWD::CartridgeWD(const ByteBuffer& image, size_t size,
                         const string& md5, const Settings& settings)
  : Cartridge(settings, md5),
    mySize(std::min<size_t>(8_KB + 3, size))
{
  // Copy the ROM image into my buffer
  if (mySize == 8_KB + 3)
  {
    // swap slices 2 & 3
    std::copy_n(image.get(),            1_KB * 2, myImage.begin());
    std::copy_n(image.get() + 1_KB * 3, 1_KB * 1, myImage.begin() + 1_KB * 2);
    std::copy_n(image.get() + 1_KB * 2, 1_KB * 1, myImage.begin() + 1_KB * 3);
    std::copy_n(image.get() + 1_KB * 4, 1_KB * 4, myImage.begin() + 1_KB * 4);
  }
  else
    std::copy_n(image.get(), mySize, myImage.begin());
  createCodeAccessBase(8_KB);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWD::reset()
{
  initializeRAM(myRAM.data(), myRAM.size());
  initializeStartBank(0);

  myCyclesAtBankswitchInit = 0;
  myPendingBank = 0xF0;  // one more than the allowable bank #

  // Setup segments to some default slices
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWD::install(System& system)
{
  mySystem = &system;

  // Set the page accessing method for the RAM reading pages
  System::PageAccess read(this, System::PageAccessType::READ);
  for(uInt16 addr = 0x1000; addr < 0x1040; addr += System::PAGE_SIZE)
  {
    read.directPeekBase = &myRAM[addr & 0x003F];
    read.codeAccessBase = &myCodeAccessBase[addr & 0x003F];
    mySystem->setPageAccess(addr, read);
  }

  // Set the page accessing method for the RAM writing pages
  // Map access to this class, since we need to inspect all accesses to
  // check if RWP happens
  System::PageAccess write(this, System::PageAccessType::WRITE);
  for(uInt16 addr = 0x1040; addr < 0x1080; addr += System::PAGE_SIZE)
  {
    write.codeAccessBase = &myCodeAccessBase[addr & 0x003F];
    mySystem->setPageAccess(addr, write);
  }

  // Mirror all access in TIA; by doing so we're taking responsibility
  // for that address space in peek and poke below.
  mySystem->tia().installDelegate(system, *this);

  // Setup segments to some default slices
  bank(startBank());
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
      // Reading from the write port @ $1040 - $107F triggers an unwanted write
      return peekRAM(myRAM[address & 0x003F], peekAddress);
    else if(address < 0x0400)
      return myImage[myOffset[0] + (address & 0x03FF)];
    else if(address < 0x0800)
      return myImage[myOffset[1] + (address & 0x03FF)];
    else if(address < 0x0C00)
      return myImage[myOffset[2] + (address & 0x03FF)];
    else
      return mySegment3[address & 0x03FF];
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeWD::poke(uInt16 address, uInt8 value)
{
  if(!(address & 0x1000))  // TIA addresses
    return mySystem->tia().poke(address, value);
  else
  {
    if(address & 0x040)
    {
      pokeRAM(myRAM[address & 0x003F], address, value);
      return true;
    }
    else
    {
      // Writing to the read port should be ignored, but trigger a break if option enabled
      uInt8 dummy;

      pokeRAM(dummy, address, value);
      myRamWriteAccess = address;
      return false;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeWD::bank(uInt16 bank)
{
  if(bankLocked() || bank > 15) return false;

  myCurrentBank = bank;

  segmentZero(ourBankOrg[bank & 0x7].zero);
  segmentOne(ourBankOrg[bank & 0x7].one);
  segmentTwo(ourBankOrg[bank & 0x7].two);
  segmentThree(ourBankOrg[bank & 0x7].three);

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWD::segmentZero(uInt8 slice)
{
  uInt16 offset = slice << 10;
  System::PageAccess access(this, System::PageAccessType::READ);

  // Skip first 128 bytes; it is always RAM
  for(uInt16 addr = 0x1080; addr < 0x1400; addr += System::PAGE_SIZE)
  {
    access.codeAccessBase = &myCodeAccessBase[offset + (addr & 0x03FF)];
    mySystem->setPageAccess(addr, access);
  }
  myOffset[0] = offset;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWD::segmentOne(uInt8 slice)
{
  uInt16 offset = slice << 10;
  System::PageAccess access(this, System::PageAccessType::READ);

  for(uInt16 addr = 0x1400; addr < 0x1800; addr += System::PAGE_SIZE)
  {
    access.codeAccessBase = &myCodeAccessBase[offset + (addr & 0x03FF)];
    mySystem->setPageAccess(addr, access);
  }
  myOffset[1] = offset;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWD::segmentTwo(uInt8 slice)
{
  uInt16 offset = slice << 10;
  System::PageAccess access(this, System::PageAccessType::READ);

  for(uInt16 addr = 0x1800; addr < 0x1C00; addr += System::PAGE_SIZE)
  {
    access.codeAccessBase = &myCodeAccessBase[offset + (addr & 0x03FF)];
    mySystem->setPageAccess(addr, access);
  }
  myOffset[2] = offset;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWD::segmentThree(uInt8 slice)
{
  uInt16 offset = slice << 10;

  // Make a copy of the address space pointed to by the slice
  // Then overwrite one byte with 0
  std::copy_n(myImage.begin()+offset, mySegment3.size(), mySegment3.begin());
  mySegment3[0x3FC] = 0;

  System::PageAccess access(this, System::PageAccessType::READ);

  for(uInt16 addr = 0x1C00; addr < 0x2000; addr += System::PAGE_SIZE)
  {
    access.codeAccessBase = &myCodeAccessBase[offset + (addr & 0x03FF)];
    mySystem->setPageAccess(addr, access);
  }
  myOffset[3] = offset;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeWD::getBank(uInt16) const
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
const uInt8* CartridgeWD::getImage(size_t& size) const
{
  size = mySize;
  return myImage.data();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeWD::save(Serializer& out) const
{
  try
  {
    out.putShort(myCurrentBank);
    out.putByteArray(myRAM.data(), myRAM.size());
    out.putLong(myCyclesAtBankswitchInit);
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
    myCurrentBank = in.getShort();
    in.getByteArray(myRAM.data(), myRAM.size());
    myCyclesAtBankswitchInit = in.getLong();
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
const std::array<CartridgeWD::BankOrg, 8> CartridgeWD::ourBankOrg = {{
                   //            0 1 2 3 4 5 6 7
  { 0, 0, 1, 3 },  // Bank 0, 8  2 1 - 1 - - - -
  { 0, 1, 2, 3 },  // Bank 1, 9  1 1 1 1 - - - -
  { 4, 5, 6, 7 },  // Bank 2, 10  - - - - 1 1 1 1
  { 7, 4, 2, 3 },  // Bank 3, 11  - - 1 1 1 - - 1
  { 0, 0, 6, 7 },  // Bank 4, 12  2 - - - - - 1 1
  { 0, 1, 7, 6 },  // Bank 5, 13  1 1 - - - - 1 1
  { 2, 3, 4, 5 },  // Bank 6, 14  - - 1 1 1 1 - -
  { 6, 0, 5, 1 }   // Bank 7, 15  1 1 - - - 1 1 -
                   // count       7 4 3 4 3 3 4 4
}};
