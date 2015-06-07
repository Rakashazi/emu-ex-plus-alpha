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
// $Id: CartDASH.cxx 3131 2015-01-01 03:49:32Z stephena $
//============================================================================

#include <cstring>

#include "System.hxx"
#include "TIA.hxx"
#include "CartDASH.hxx"

//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDASH::CartridgeDASH(const uInt8* image, uInt32 size, const Settings& settings)
  : Cartridge(settings),
    mySize(size),
    myImage(nullptr)
{
  // Allocate array for the ROM image
  myImage = new uInt8[mySize];

  // Copy the ROM image into my buffer
  memcpy(myImage, image, mySize);
  createCodeAccessBase(mySize + RAM_TOTAL_SIZE);

  // Remember startup bank (0 per spec, rather than last per 3E scheme).
  // Set this to go to 3rd 1K Bank.
  myStartBank = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDASH::~CartridgeDASH()
{
  delete[] myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDASH::reset() {

  // Initialize RAM
  if (mySettings.getBool("ramrandom"))
    for (uInt32 i = 0; i < RAM_TOTAL_SIZE; ++i)
      myRAM[i] = mySystem->randGenerator().next();
  else
    memset(myRAM, 0, RAM_TOTAL_SIZE);

  // Initialise bank values for all ROM/RAM access
  // This is used to reverse-lookup from address to bank location
  for (uInt32 b = 0; b < 8; b++)
  {
    bankInUse[b] = BANK_UNDEFINED;        // bank is undefined and inaccessible!
    segmentInUse[b/2] = BANK_UNDEFINED;
  }
  initializeBankState();

  // We'll map the startup banks 0 and 3 from the image into the third 1K bank upon reset
  bankROM((0 << BANK_BITS) | 0);
  bankROM((3 << BANK_BITS) | 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDASH::install(System& system) {

  mySystem = &system;

  System::PageAccess access(this, System::PA_READWRITE);

  // Set the page accessing methods for the hot spots (for 100% emulation
  // we need to chain any accesses below 0x40 to the TIA. Our poke() method
  // does this via mySystem->tiaPoke(...), at least until we come up with a
  // cleaner way to do it).
  for (uInt32 i = 0x00; i < 0x40; i += (1 << System::PAGE_SHIFT))
    mySystem->setPageAccess(i >> System::PAGE_SHIFT, access);

  // Initialise bank values for all ROM/RAM access
  // This is used to reverse-lookup from address to bank location
  for (uInt32 b = 0; b < 8; b++)
  {
    bankInUse[b] = BANK_UNDEFINED;        // bank is undefined and inaccessible!
    segmentInUse[b/2] = BANK_UNDEFINED;
  }
  initializeBankState();

  // Setup the last segment (of 4, each 1K) to point to the first ROM slice
  // Actually we DO NOT want "always". It's just on bootup, and can be out switched later
  bankROM((0 << BANK_BITS) | 0);
  bankROM((3 << BANK_BITS) | 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeDASH::peek(uInt16 address) {

  uInt16 peekAddress = address;
  address &= 0x0FFF;    // restrict to 4K address range

  uInt8 value = 0;
  uInt32 bank = (address >> (ROM_BANK_TO_POWER - 1)) & 7;   // convert to 512 byte bank index (0-7)
  uInt16 imageBank = bankInUse[bank];        			          // the ROM/RAM bank that's here

  if (imageBank == BANK_UNDEFINED) {						// an uninitialised bank?

    // accessing invalid bank, so return should be... random?
    value = mySystem->randGenerator().next();

  } else if (imageBank & BITMASK_ROMRAM) {      // a RAM bank

    // Reading from the write port triggers an unwanted write
    value = mySystem->getDataBusState(0xFF);

    if(bankLocked())
      return value;
    else
    {
      triggerReadFromWritePort(peekAddress);

      Int32 ramBank = imageBank & BIT_BANK_MASK;    // discard irrelevant bits
      Int32 offset = ramBank << RAM_BANK_TO_POWER;  // base bank address in RAM
      offset += (address & BITMASK_RAM_BANK);       // + byte offset in RAM bank
      return myRAM[offset] = value;
    }
  }

  return value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDASH::poke(uInt16 address, uInt8 value) {

  bool changed = false;

  // Check for write to the bank switch address. RAM/ROM and bank # are encoded in 'value'
  // There are NO mirrored hotspots.

  if (address == BANK_SWITCH_HOTSPOT_RAM)
    changed = bankRAM(value);

  else if (address == BANK_SWITCH_HOTSPOT_ROM)
    changed = bankROM(value);

  // Pass the poke through to the TIA. In a real Atari, both the cart and the
  // TIA see the address lines, and both react accordingly. In Stella, each
  // 64-byte chunk of address space is "owned" by only one device. If we
  // don't chain the poke to the TIA, then the TIA can't see it...
  mySystem->tia().poke(address, value);

  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDASH::bankRAM(uInt8 bank) {

  if (bankLocked())  // debugger can lock RAM
    return false;

  // Each RAM bank uses two slots, separated by 0x800 in memory -- one read, one write.
  bankRAMSlot(bank | BITMASK_ROMRAM | 0);
  bankRAMSlot(bank | BITMASK_ROMRAM | BITMASK_LOWERUPPER);

  // Remember that this hotspot was accessed for RAM
  segmentInUse[(bank >> BANK_BITS) & 3] = bank | BITMASK_ROMRAM;

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDASH::bankRAMSlot(uInt16 bank) {

  uInt16 bankNumber = (bank >> BANK_BITS) & 3;  // which bank # we are switching TO (BITS D6,D7) to 512 byte block
  uInt16 currentBank = bank & BIT_BANK_MASK;    // Wrap around/restrict to valid range
  bool upper = bank & BITMASK_LOWERUPPER;       // is this the read or write port

  uInt32 startCurrentBank = currentBank << RAM_BANK_TO_POWER;  // Effectively * 512 bytes

  // Setup the page access methods for the current bank
  System::PageAccess access(this, System::PA_READ);

  if (upper) {  // We're mapping the write port

    bankInUse[bankNumber + 4] = (Int16) bank;
    access.type = System::PA_WRITE;

  } else {      // We're mapping the read port

    bankInUse[bankNumber] = (Int16) bank;
    access.type = System::PA_READ;

  }

  uInt32 start = 0x1000 + (bankNumber << RAM_BANK_TO_POWER) + (upper ? RAM_WRITE_OFFSET : 0);
  uInt32 end = start + RAM_BANK_SIZE - 1;

  for (uInt32 address = start; address <= end; address += (1 << System::PAGE_SHIFT)) {
    if(upper)
      access.directPokeBase = &myRAM[startCurrentBank + (address & (RAM_BANK_SIZE - 1))];
    else
      access.directPeekBase = &myRAM[startCurrentBank + (address & (RAM_BANK_SIZE - 1))];
    access.codeAccessBase = &myCodeAccessBase[mySize + startCurrentBank + (address & (RAM_BANK_SIZE - 1))];
    mySystem->setPageAccess(address >> System::PAGE_SHIFT, access);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDASH::bankROM(uInt8 bank) {

  if (bankLocked())  // debugger can lock ROM
    return false;

  // Map ROM bank image into the system into the correct slot
  // Memory map is 1K slots at 0x1000, 0x1400, 0x1800, 0x1C00
  // Each ROM uses 2 consecutive 512 byte slots
  bankROMSlot(bank | 0);
  bankROMSlot(bank | BITMASK_LOWERUPPER);

  // Remember that this hotspot was accessed for ROM
  segmentInUse[(bank >> BANK_BITS) & 3] = bank;

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDASH::bankROMSlot(uInt16 bank) {

  uInt16 bankNumber = (bank >> BANK_BITS) & 3;    // which bank # we are switching TO (BITS D6,D7)
  uInt16 currentBank = bank & BIT_BANK_MASK;      // Wrap around/restrict to valid range
  bool upper = bank & BITMASK_LOWERUPPER;         // is this the lower or upper 512b

  bankInUse[bankNumber * 2 + (upper ? 1 : 0)] = (Int16) bank; // Record which bank switched in (as ROM)

  uInt32 startCurrentBank = currentBank << ROM_BANK_TO_POWER;     // Effectively *1K

  // Setup the page access methods for the current bank
  System::PageAccess access(this, System::PA_READ);

  uInt32 start = 0x1000 + (bankNumber << ROM_BANK_TO_POWER) + (upper ? ROM_BANK_SIZE / 2 : 0);
  uInt32 end = start + ROM_BANK_SIZE / 2 - 1;

  for (uInt32 address = start; address <= end; address += (1 << System::PAGE_SHIFT)) {
    access.directPeekBase = &myImage[startCurrentBank + (address & (ROM_BANK_SIZE - 1))];
    access.codeAccessBase = &myCodeAccessBase[startCurrentBank + (address & (ROM_BANK_SIZE - 1))];
    mySystem->setPageAccess(address >> System::PAGE_SHIFT, access);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDASH::initializeBankState() {

  // Switch in each 512b slot
  for (uInt32 b = 0; b < 8; b++) {
    if (bankInUse[b] == BANK_UNDEFINED) {

      // All accesses point to peek/poke above
      System::PageAccess access(this, System::PA_READ);
      uInt32 start = 0x1000 + (b << RAM_BANK_TO_POWER);
      uInt32 end = start + RAM_BANK_SIZE - 1;
      for (uInt32 address = start; address <= end; address += (1 << System::PAGE_SHIFT))
        mySystem->setPageAccess(address >> System::PAGE_SHIFT, access);

    }
    else if (bankInUse[b] & BITMASK_ROMRAM)
      bankRAMSlot(bankInUse[b]);
    else
      bankROMSlot(bankInUse[b]);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDASH::patch(uInt16 address, uInt8 value) {
#if 0
  // Patch the cartridge ROM (for debugger)

  myBankChanged = true;

  uInt32 bankNumber = (address >> RAM_BANK_TO_POWER) & 7;   // now 512 byte bank # (ie: 0-7)
  Int16 whichBankIsThere = bankInUse[bankNumber];           // ROM or RAM bank reference

  if (whichBankIsThere == BANK_UNDEFINED) {

    // We're trying to access undefined memory (no bank here yet). Fail!
    myBankChanged = false;

  } else if (whichBankIsThere & BITMASK_ROMRAM) { // patching RAM (512 byte banks)

    uInt32 byteOffset = address & BITMASK_RAM_BANK;
    uInt32 baseAddress = ((whichBankIsThere & BIT_BANK_MASK) << RAM_BANK_TO_POWER) + byteOffset;
    myRAM[baseAddress] = value;     // write to RAM

    // TODO: Stephen -- should we set 'myBankChanged' true when there's a RAM write?

  } else {  // patching ROM (1K banks)

    uInt32 byteOffset = address & BITMASK_ROM_BANK;
    uInt32 baseAddress = (whichBankIsThere << ROM_BANK_TO_POWER) + byteOffset;
    myImage[baseAddress] = value;   // write to the image
  }

  return myBankChanged;
#else
  return false;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeDASH::getImage(int& size) const {
  size = mySize;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDASH::save(Serializer& out) const {

  try {
    out.putString(name());
    out.putShortArray(bankInUse, 8);
    out.putShortArray(segmentInUse, 4);
    out.putByteArray(myRAM, RAM_TOTAL_SIZE);
  } catch (...) {
    cerr << "ERROR: CartridgeDASH::save" << endl;
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDASH::load(Serializer& in) {

  try {
    if (in.getString() != name())
      return false;
    in.getShortArray(bankInUse, 8);
    in.getShortArray(segmentInUse, 4);
    in.getByteArray(myRAM, RAM_TOTAL_SIZE);
  } catch (...) {
    cerr << "ERROR: CartridgeDASH::load" << endl;
    return false;
  }

  initializeBankState();
  return true;
}
