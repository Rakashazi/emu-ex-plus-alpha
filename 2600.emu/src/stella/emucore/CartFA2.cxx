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

#include "OSystem.hxx"
#include "Serializer.hxx"
#include "System.hxx"
#include "TimerManager.hxx"
#include "CartFA2.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFA2::CartridgeFA2(const ByteBuffer& image, size_t size,
                           const string& md5, const Settings& settings)
  : Cartridge(settings, md5)
{
  // 29/32K version of FA2 has valid data @ 1K - 29K
  const uInt8* img_ptr = image.get();
  if(size >= 29_KB)
    img_ptr += 1_KB;
  else if(size < mySize)
    mySize = size;

  // Copy the ROM image into my buffer
  std::copy_n(img_ptr, mySize, myImage.begin());
  createCodeAccessBase(mySize);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA2::reset()
{
  initializeRAM(myRAM.data(), myRAM.size());
  initializeStartBank(0);

  // Upon reset we switch to the startup bank
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA2::install(System& system)
{
  mySystem = &system;

  System::PageAccess access(this, System::PageAccessType::READ);

  // Set the page accessing method for the RAM writing pages
  // Map access to this class, since we need to inspect all accesses to
  // check if RWP happens
  access.type = System::PageAccessType::WRITE;
  for(uInt16 addr = 0x1000; addr < 0x1100; addr += System::PAGE_SIZE)
  {
    access.codeAccessBase = &myCodeAccessBase[addr & 0x00FF];
    mySystem->setPageAccess(addr, access);
  }

  // Set the page accessing method for the RAM reading pages
  access.directPokeBase = nullptr;
  access.type = System::PageAccessType::READ;
  for(uInt16 addr = 0x1100; addr < 0x1200; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myRAM[addr & 0x00FF];
    access.codeAccessBase = &myCodeAccessBase[0x100 + (addr & 0x00FF)];
    mySystem->setPageAccess(addr, access);
  }

  // Install pages for the startup bank
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeFA2::peek(uInt16 address)
{
  uInt16 peekAddress = address;
  address &= 0x0FFF;

  // Switch banks if necessary
  switch(address)
  {
    case 0x0FF4:
      // Load/save RAM to/from Harmony cart flash
      if(mySize == 28_KB && !bankLocked())
        return ramReadWrite();
      break;

    case 0x0FF5:
      // Set the current bank to the first 4k bank
      bank(0);
      break;

    case 0x0FF6:
      // Set the current bank to the second 4k bank
      bank(1);
      break;

    case 0x0FF7:
      // Set the current bank to the third 4k bank
      bank(2);
      break;

    case 0x0FF8:
      // Set the current bank to the fourth 4k bank
      bank(3);
      break;

    case 0x0FF9:
      // Set the current bank to the fifth 4k bank
      bank(4);
      break;

    case 0x0FFA:
      // Set the current bank to the sixth 4k bank
      bank(5);
      break;

    case 0x0FFB:
      // Set the current bank to the seventh 4k bank
      // This is only available on 28K ROMs
      if(mySize == 28_KB)  bank(6);
      break;

    default:
      break;
  }

  if(address < 0x0100)  // Write port is at 0xF000 - 0xF0FF (256 bytes)
    return peekRAM(myRAM[address], peekAddress);
  else
    return myImage[myBankOffset + address];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFA2::poke(uInt16 address, uInt8 value)
{
  // Switch banks if necessary
  switch(address & 0x0FFF)
  {
    case 0x0FF4:
      // Load/save RAM to/from Harmony cart flash
      if(mySize == 28_KB && !bankLocked())
        ramReadWrite();
      return false;

    case 0x0FF5:
      // Set the current bank to the first 4k bank
      bank(0);
      return false;

    case 0x0FF6:
      // Set the current bank to the second 4k bank
      bank(1);
      return false;

    case 0x0FF7:
      // Set the current bank to the third 4k bank
      bank(2);
      return false;

    case 0x0FF8:
      // Set the current bank to the fourth 4k bank
      bank(3);
      return false;

    case 0x0FF9:
      // Set the current bank to the fifth 4k bank
      bank(4);
      return false;

    case 0x0FFA:
      // Set the current bank to the sixth 4k bank
      bank(5);
      return false;

    case 0x0FFB:
      // Set the current bank to the seventh 4k bank
      // This is only available on 28K ROMs
      if(mySize == 28_KB)  bank(6);
      return false;

    default:
      break;
  }

  if(!(address & 0x100))
  {
    pokeRAM(myRAM[address & 0x00FF], address, value);
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFA2::bank(uInt16 bank)
{
  if(bankLocked()) return false;

  // Remember what bank we're in
  myBankOffset = bank << 12;

  System::PageAccess access(this, System::PageAccessType::READ);

  // Set the page accessing methods for the hot spots
  for(uInt16 addr = (0x1FF4 & ~System::PAGE_MASK); addr < 0x2000;
      addr += System::PAGE_SIZE)
  {
    access.codeAccessBase = &myCodeAccessBase[myBankOffset + (addr & 0x0FFF)];
    mySystem->setPageAccess(addr, access);
  }

  // Setup the page access methods for the current bank
  for(uInt16 addr = 0x1200; addr < static_cast<uInt16>(0x1FF4U & ~System::PAGE_MASK);
      addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[myBankOffset + (addr & 0x0FFF)];
    access.codeAccessBase = &myCodeAccessBase[myBankOffset + (addr & 0x0FFF)];
    mySystem->setPageAccess(addr, access);
  }
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeFA2::getBank(uInt16) const
{
  return myBankOffset >> 12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeFA2::bankCount() const
{
  return uInt16(mySize / 4_KB);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFA2::patch(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  if(address < 0x0200)
  {
    // Normally, a write to the read port won't do anything
    // However, the patch command is special in that ignores such
    // cart restrictions
    myRAM[address & 0x00FF] = value;
  }
  else
    myImage[myBankOffset + address] = value;

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeFA2::getImage(size_t& size) const
{
  size = mySize;
  return myImage.data();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFA2::save(Serializer& out) const
{
  try
  {
    out.putShort(myBankOffset);
    out.putByteArray(myRAM.data(), myRAM.size());
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeFA2::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFA2::load(Serializer& in)
{
  try
  {
    myBankOffset = in.getShort();
    in.getByteArray(myRAM.data(), myRAM.size());
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeFA2::load" << endl;
    return false;
  }

  // Remember what bank we were in
  bank(myBankOffset >> 12);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA2::setNVRamFile(const string& nvramdir, const string& romfile)
{
  myFlashFile = nvramdir + romfile + "_flash.dat";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeFA2::ramReadWrite()
{
  /* The following algorithm implements accessing Harmony cart flash

    1. Wait for an access to hotspot location $1FF4 (return 1 in bit 6
       while busy).

    2. Read byte 256 of RAM+ memory to determine the operation requested
       (1 = read, 2 = write).

    3. Save or load the entire 256 bytes of RAM+ memory to a file.

    4. Set byte 256 of RAM+ memory to zero to indicate success (will
       always happen in emulation).

    5. Return 0 (in bit 6) on the next access to $1FF4, if enough time has
       passed to complete the operation on a real system (0.5 ms for read,
       101 ms for write).
  */

  // First access sets the timer
  if(myRamAccessTimeout == 0)
  {
    // Remember when the first access was made
    myRamAccessTimeout = TimerManager::getTicks();

    // We go ahead and do the access now, and only return when a sufficient
    // amount of time has passed
    Serializer serializer(myFlashFile);
    if(serializer)
    {
      if(myRAM[255] == 1)       // read
      {
        try
        {
          serializer.getByteArray(myRAM.data(), myRAM.size());
        }
        catch(...)
        {
          myRAM.fill(0);
        }
        myRamAccessTimeout += 500;  // Add 0.5 ms delay for read
      }
      else if(myRAM[255] == 2)  // write
      {
        try
        {
          serializer.putByteArray(myRAM.data(), myRAM.size());
        }
        catch(...)
        {
          // Maybe add logging here that save failed?
          cerr << name() << ": ERROR saving score table" << endl;
        }
        myRamAccessTimeout += 101000;  // Add 101 ms delay for write
      }
    }
    // Bit 6 is 1, busy
    return myImage[myBankOffset + 0xFF4] | 0x40;
  }
  else
  {
    // Have we reached the timeout value yet?
    if(TimerManager::getTicks() >= myRamAccessTimeout)
    {
      myRamAccessTimeout = 0;  // Turn off timer
      myRAM[255] = 0;          // Successful operation

      // Bit 6 is 0, ready/success
      return myImage[myBankOffset + 0xFF4] & ~0x40;
    }
    else
      // Bit 6 is 1, busy
      return myImage[myBankOffset + 0xFF4] | 0x40;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA2::flash(uInt8 operation)
{
  Serializer serializer(myFlashFile);
  if(serializer)
  {
    if(operation == 0)       // erase
    {
      try
      {
        std::array<uInt8, 256> buf = {};
        serializer.putByteArray(buf.data(), buf.size());
      }
      catch(...)
      {
      }
    }
    else if(operation == 1)  // read
    {
      try
      {
        serializer.getByteArray(myRAM.data(), myRAM.size());
      }
      catch(...)
      {
        myRAM.fill(0);
      }
    }
    else if(operation == 2)  // write
    {
      try
      {
        serializer.putByteArray(myRAM.data(), myRAM.size());
      }
      catch(...)
      {
        // Maybe add logging here that save failed?
        cerr << name() << ": ERROR saving score table" << endl;
      }
    }
  }
}
