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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <cstring>

#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif
#include "System.hxx"
#include "M6532.hxx"
#include "TIA.hxx"
#include "Thumbulator.hxx"
#include "CartBUS.hxx"
#include "exception/FatalEmulationError.hxx"

// Location of data within the RAM copy of the BUS Driver.
#define DSxPTR        0x06D8
#define DSxINC        0x0720
#define DSMAPS        0x0760
#define WAVEFORM      0x07F4
#define DSRAM         0x0800

#define COMMSTREAM    0x10
#define JUMPSTREAM    0x11

#define BUS_STUFF_ON ((myMode & 0x0F) == 0)
#define DIGITAL_AUDIO_ON ((myMode & 0xF0) == 0)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeBUS::CartridgeBUS(const ByteBuffer& image, size_t size,
                           const string& md5, const Settings& settings)
  : Cartridge(settings, md5),
    myImage{make_unique<uInt8[]>(32_KB)}
{
  // Copy the ROM image into my buffer
  std::copy_n(image.get(), std::min(32_KB, size), myImage.get());

  // Even though the ROM is 32K, only 28K is accessible to the 6507
  createRomAccessArrays(28_KB);

  // Pointer to the program ROM (28K @ 0 byte offset)
  // which starts after the 2K BUS Driver and 2K C Code
  myProgramImage = myImage.get() + 4_KB;

  // Pointer to BUS driver in RAM
  myDriverImage = myRAM.data();

  // Pointer to the display RAM
  myDisplayImage = myRAM.data() + DSRAM;

  // Create Thumbulator ARM emulator
  bool devSettings = settings.getBool("dev.settings");
  myThumbEmulator = make_unique<Thumbulator>(
    reinterpret_cast<uInt16*>(myImage.get()),
    reinterpret_cast<uInt16*>(myRAM.data()),
    static_cast<uInt32>(32_KB),
    0x00000800,
    0x00000808,
    0x40001FDC,
    devSettings ? settings.getBool("dev.thumb.trapfatal") : false,
    Thumbulator::ConfigureFor::BUS,
    this);

  setInitialState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::reset()
{
  initializeRAM(myRAM.data() + 2_KB, 6_KB);

  // BUS always starts in bank 6
  initializeStartBank(6);

  // Update cycles to the current system cycles
  myAudioCycles = myARMCycles = 0;
  myFractionalClocks = 0.0;

  setInitialState();

  // Upon reset we switch to the startup bank
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::setInitialState()
{
  // Copy initial BUS driver to Harmony RAM
  std::copy_n(myImage.get(), 2_KB, myDriverImage);

  myMusicWaveformSize.fill(27);

  // Assuming mode starts out with Fast Fetch off and 3-Voice music,
  // need to confirm with Chris
  myMode = 0xFF;

  myBankOffset = myBusOverdriveAddress =
    mySTYZeroPageAddress = myJMPoperandAddress = 0;

  myFastJumpActive = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::consoleChanged(ConsoleTiming timing)
{
  myThumbEmulator->setConsoleTiming(timing);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::install(System& system)
{
  mySystem = &system;

  // Map all of the accesses to call peek and poke
  System::PageAccess access(this, System::PageAccessType::READ);
  for(uInt16 addr = 0x1000; addr < 0x1040; addr += System::PAGE_SIZE)
    mySystem->setPageAccess(addr, access);

  // Mirror all access in TIA and RIOT; by doing so we're taking responsibility
  // for that address space in peek and poke below.
  mySystem->tia().installDelegate(system, *this);
  mySystem->m6532().installDelegate(system, *this);

  // Install pages for the startup bank
  bank(startBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void CartridgeBUS::updateMusicModeDataFetchers()
{
  // Calculate the number of cycles since the last update
  uInt32 cycles = uInt32(mySystem->cycles() - myAudioCycles);
  myAudioCycles = mySystem->cycles();

  // Calculate the number of BUS OSC clocks since the last update
  double clocks = ((20000.0 * cycles) / 1193191.66666667) + myFractionalClocks;
  uInt32 wholeClocks = uInt32(clocks);
  myFractionalClocks = clocks - double(wholeClocks);

  // Let's update counters and flags of the music mode data fetchers
  if(wholeClocks > 0)
    for(int x = 0; x <= 2; ++x)
      myMusicCounters[x] += myMusicFrequencies[x] * wholeClocks;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void CartridgeBUS::callFunction(uInt8 value)
{
  switch (value)
  {
    // Call user written ARM code (will most likely be C compiled for ARM)
    case 254: // call with IRQ driven audio, no special handling needed at this
              // time for Stella as ARM code "runs in zero 6507 cycles".
    case 255: // call without IRQ driven audio
      try {
        Int32 cycles = Int32(mySystem->cycles() - myARMCycles);
        myARMCycles = mySystem->cycles();

        myThumbEmulator->run(cycles);
      }
      catch(const runtime_error& e) {
        if(!mySystem->autodetectMode())
        {
          FatalEmulationError::raise(e.what());
        }
      }
      break;
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeBUS::peek(uInt16 address)
{
  if(!(address & 0x1000))                      // Hotspots below 0x1000
  {
    // Check for RAM or TIA mirroring
    uInt16 lowAddress = address & 0x3ff;
    if(lowAddress & 0x80)
      return mySystem->m6532().peek(address);
    else if(!(lowAddress & 0x200))
      return mySystem->tia().peek(address);
  }
  else
  {
    address &= 0x0FFF;

    uInt8 peekvalue = myProgramImage[myBankOffset + address];

    // In debugger/bank-locked mode, we ignore all hotspots and in general
    // anything that can change the internal state of the cart
    if(bankLocked())
      return peekvalue;

    // implement JMP FASTJMP which fetches the destination address from stream 17
    if (myFastJumpActive
        && myJMPoperandAddress == address)
    {
      uInt32 pointer;
      uInt8 value;

      --myFastJumpActive;
      ++myJMPoperandAddress;

      pointer = getDatastreamPointer(JUMPSTREAM);
      value = myDisplayImage[ pointer >> 20 ];
      pointer += 0x100000;  // always increment by 1
      setDatastreamPointer(JUMPSTREAM, pointer);

      return value;
    }

    // test for JMP FASTJUMP where FASTJUMP = $0000
    if (BUS_STUFF_ON
        && peekvalue == 0x4C
        && myProgramImage[myBankOffset + address+1] == 0
        && myProgramImage[myBankOffset + address+2] == 0)
    {
      myFastJumpActive = 2; // return next two peeks from datastream 17
      myJMPoperandAddress = address + 1;
      return peekvalue;
    }

    myJMPoperandAddress = 0;

    // save the STY's zero page address
    if (BUS_STUFF_ON && mySTYZeroPageAddress == address)
      myBusOverdriveAddress =  peekvalue;

    mySTYZeroPageAddress = 0;

    switch(address)
    {
      case 0xFEE: // AMPLITUDE
        // Update the music data fetchers (counter & flag)
        updateMusicModeDataFetchers();

        if DIGITAL_AUDIO_ON
        {
          // retrieve packed sample (max size is 2K, or 4K of unpacked data)
          uInt32 sampleaddress = getSample() + (myMusicCounters[0] >> 21);

          // get sample value from ROM or RAM
          if (sampleaddress < 0x8000)
            peekvalue = myImage[sampleaddress];
          else if (sampleaddress >= 0x40000000 && sampleaddress < 0x40002000) // check for RAM
            peekvalue = myRAM[sampleaddress - 0x40000000];
          else
            peekvalue = 0;

          // make sure current volume value is in the lower nybble
          if ((myMusicCounters[0] & (1<<20)) == 0)
            peekvalue >>= 4;
          peekvalue &= 0x0f;
        }
        else
        {
          // using myDisplayImage[] instead of myProgramImage[] because waveforms
          // can be modified during runtime.
          uInt32 i = myDisplayImage[(getWaveform(0) ) + (myMusicCounters[0] >> myMusicWaveformSize[0])] +
                     myDisplayImage[(getWaveform(1) ) + (myMusicCounters[1] >> myMusicWaveformSize[1])] +
                     myDisplayImage[(getWaveform(2) ) + (myMusicCounters[2] >> myMusicWaveformSize[2])];

          peekvalue = uInt8(i);
        }
        break;

      case 0xFEF: // DSREAD
        peekvalue = readFromDatastream(COMMSTREAM);
        break;

      case 0xFF0: // DSWRITE
      case 0xFF1: // DSPTR
      case 0xFF2: // SETMODE
      case 0xFF3: // CALLFN
        // these are write-only
        break;

      case 0xFF5:
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
        // Set the current bank to the last 4k bank
        bank(6);
        break;

      default:
        break;
    }

    // this might not work right for STY $84
    if (BUS_STUFF_ON && peekvalue == 0x84)
      mySTYZeroPageAddress = address + 1;

    return peekvalue;
  }

  return 0;  // make compiler happy
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBUS::poke(uInt16 address, uInt8 value)
{
  if (!(address & 0x1000))
  {
    value &= busOverdrive(address);

    // Check for RAM or TIA mirroring
    uInt16 lowAddress = address & 0x3ff;
    if(lowAddress & 0x80)
      mySystem->m6532().poke(address, value);
    else if(!(lowAddress & 0x200))
      mySystem->tia().poke(address, value);
  }
  else
  {
    uInt32 pointer;

    address &= 0x0FFF;

    switch(address)
    {
      case 0xFEE: // AMPLITUDE
      case 0xFEF: // DSREAD
        // these are read-only
        break;

      case 0xFF0: // DSWRITE
        pointer = getDatastreamPointer(COMMSTREAM);
        myDisplayImage[ pointer >> 20 ] = value;
        pointer += 0x100000;  // always increment by 1 when writing
        setDatastreamPointer(COMMSTREAM, pointer);
        break;

      case 0xFF1: // DSPTR
        pointer = getDatastreamPointer(COMMSTREAM);
        pointer <<=8;
        pointer &= 0xf0000000;
        pointer |= (value << 20);
        setDatastreamPointer(COMMSTREAM, pointer);
        break;

      case 0xFF2: // SETMODE
        myMode = value;
        break;

      case 0xFF3: // CALLFN
        callFunction(value);
        break;

      case 0xFF5:
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
        // Set the current bank to the last 4k bank
        bank(6);
        break;

      default:
        break;
    }
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBUS::bank(uInt16 bank, uInt16)
{
  if(bankLocked()) return false;

  // Remember what bank we're in
  myBankOffset = bank << 12;

  // Setup the page access methods for the current bank
  System::PageAccess access(this, System::PageAccessType::READ);

  // Map Program ROM image into the system
  for(uInt16 addr = 0x1040; addr < 0x2000; addr += System::PAGE_SIZE)
  {
    access.romAccessBase = &myRomAccessBase[myBankOffset + (addr & 0x0FFF)];
    access.romPeekCounter = &myRomAccessCounter[myBankOffset + (addr & 0x0FFF)];
    access.romPokeCounter = &myRomAccessCounter[myBankOffset + (addr & 0x0FFF) + 28_KB];
    mySystem->setPageAccess(addr, access);
  }
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeBUS::getBank(uInt16) const
{
  return myBankOffset >> 12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeBUS::romBankCount() const
{
  return 7;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBUS::patch(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  // For now, we ignore attempts to patch the BUS address space
  if(address >= 0x0040)
  {
    myProgramImage[myBankOffset + (address & 0x0FFF)] = value;
    return myBankChanged = true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteBuffer& CartridgeBUS::getImage(size_t& size) const
{
  size = 32_KB;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeBUS::busOverdrive(uInt16 address)
{
  uInt8 overdrive = 0xff;

  // only overdrive if the address matches
  if (address == myBusOverdriveAddress)
  {
    uInt8 map = address & 0x7f;
    if (map <= 0x24) // map TIA registers VSYNC thru HMBL inclusive
    {
      uInt32 alldatastreams = getAddressMap(map);
      uInt8 datastream = alldatastreams & 0x0f;  // lowest nybble has the current datastream to use
      overdrive = readFromDatastream(datastream);

      // rotate map nybbles for next time
      alldatastreams >>= 4;
      alldatastreams |= (datastream << 28);
      setAddressMap(map, alldatastreams);
    }
  }

  myBusOverdriveAddress = 0xff; // turns off overdrive for next poke event

  return overdrive;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeBUS::thumbCallback(uInt8 function, uInt32 value1, uInt32 value2)
{
  switch (function)
  {
    case 0:
      // _SetNote - set the note/frequency
      myMusicFrequencies[value1] = value2;
      break;

      // _ResetWave - reset counter,
      // used to make sure digital samples start from the beginning
    case 1:
      myMusicCounters[value1] = 0;
      break;

      // _GetWavePtr - return the counter
    case 2:
      return myMusicCounters[value1];

      // _SetWaveSize - set size of waveform buffer
    case 3:
      myMusicWaveformSize[value1] = value2;
      break;

    default:
      break;
  }

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeBUS::internalRamGetValue(uInt16 addr) const
{
  if(addr < internalRamSize())
    return myRAM[addr];
  else
    return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBUS::save(Serializer& out) const
{
  try
  {
    // Indicates which bank is currently active
    out.putShort(myBankOffset);

    // Harmony RAM
    out.putByteArray(myRAM.data(), myRAM.size());

    // Addresses for bus override logic
    out.putShort(myBusOverdriveAddress);
    out.putShort(mySTYZeroPageAddress);
    out.putShort(myJMPoperandAddress);

    // Save cycles and clocks
    out.putLong(myAudioCycles);
    out.putDouble(myFractionalClocks);
    out.putLong(myARMCycles);

    // Audio info
    out.putIntArray(myMusicCounters.data(), myMusicCounters.size());
    out.putIntArray(myMusicFrequencies.data(), myMusicFrequencies.size());
    out.putByteArray(myMusicWaveformSize.data(), myMusicWaveformSize.size());

    // Indicates current mode
    out.putByte(myMode);

    // Indicates if in the middle of a fast jump
    out.putByte(myFastJumpActive);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeBUS::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBUS::load(Serializer& in)
{
  try
  {
    // Indicates which bank is currently active
    myBankOffset = in.getShort();

    // Harmony RAM
    in.getByteArray(myRAM.data(), myRAM.size());

    // Addresses for bus override logic
    myBusOverdriveAddress = in.getShort();
    mySTYZeroPageAddress = in.getShort();
    myJMPoperandAddress = in.getShort();

    // Get system cycles and fractional clocks
    myAudioCycles = in.getLong();
    myFractionalClocks = in.getDouble();
    myARMCycles = in.getLong();

    // Audio info
    in.getIntArray(myMusicCounters.data(), myMusicCounters.size());
    in.getIntArray(myMusicFrequencies.data(), myMusicFrequencies.size());
    in.getByteArray(myMusicWaveformSize.data(), myMusicWaveformSize.size());

    // Indicates current mode
    myMode = in.getByte();

    // Indicates if in the middle of a fast jump
    myFastJumpActive = in.getByte();
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeBUS::load" << endl;
    return false;
  }

  // Now, go to the current bank
  bank(myBankOffset >> 12);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeBUS::getDatastreamPointer(uInt8 index) const
{
//  index &= 0x0f;

  return myRAM[DSxPTR + index*4 + 0]        +  // low byte
        (myRAM[DSxPTR + index*4 + 1] << 8)  +
        (myRAM[DSxPTR + index*4 + 2] << 16) +
        (myRAM[DSxPTR + index*4 + 3] << 24) ;  // high byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::setDatastreamPointer(uInt8 index, uInt32 value)
{
//  index &= 0x0f;
  myRAM[DSxPTR + index*4 + 0] = value & 0xff;          // low byte
  myRAM[DSxPTR + index*4 + 1] = (value >> 8) & 0xff;
  myRAM[DSxPTR + index*4 + 2] = (value >> 16) & 0xff;
  myRAM[DSxPTR + index*4 + 3] = (value >> 24) & 0xff;  // high byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeBUS::getDatastreamIncrement(uInt8 index) const
{
//  index &= 0x0f;
  return myRAM[DSxINC + index*4 + 0]        +   // low byte
        (myRAM[DSxINC + index*4 + 1] << 8)  +
        (myRAM[DSxINC + index*4 + 2] << 16) +
        (myRAM[DSxINC + index*4 + 3] << 24) ;   // high byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeBUS::getAddressMap(uInt8 index) const
{
  //  index &= 0x0f;
  return myRAM[DSMAPS + index*4 + 0]        +   // low byte
        (myRAM[DSMAPS + index*4 + 1] << 8)  +
        (myRAM[DSMAPS + index*4 + 2] << 16) +
        (myRAM[DSMAPS + index*4 + 3] << 24) ;   // high byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeBUS::getWaveform(uInt8 index) const
{
  // instead of 0, 1, 2, etc. this returned
  // 0x40000800 for 0
  // 0x40000820 for 1
  // 0x40000840 for 2
  // ...

//  return myBUSRAM[WAVEFORM + index*4 + 0]        +   // low byte
//        (myBUSRAM[WAVEFORM + index*4 + 1] << 8)  +
//        (myBUSRAM[WAVEFORM + index*4 + 2] << 16) +
//        (myBUSRAM[WAVEFORM + index*4 + 3] << 24) -   // high byte
//         0x40000800;

  uInt32 result;

  result = myRAM[WAVEFORM + index*4 + 0]        +  // low byte
          (myRAM[WAVEFORM + index*4 + 1] << 8)  +
          (myRAM[WAVEFORM + index*4 + 2] << 16) +
          (myRAM[WAVEFORM + index*4 + 3] << 24);   // high byte

  result -= 0x40000800;

  if (result >= 4096)
    result = 0;

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeBUS::getSample()
{
  uInt32 result;

  result = myRAM[WAVEFORM + 0]        +  // low byte
          (myRAM[WAVEFORM + 1] << 8)  +
          (myRAM[WAVEFORM + 2] << 16) +
          (myRAM[WAVEFORM + 3] << 24);   // high byte

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeBUS::getWaveformSize(uInt8 index) const
{
  return myMusicWaveformSize[index];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::setAddressMap(uInt8 index, uInt32 value)
{
  //  index &= 0x0f;
  myRAM[DSMAPS + index*4 + 0] = value & 0xff;          // low byte
  myRAM[DSMAPS + index*4 + 1] = (value >> 8) & 0xff;
  myRAM[DSMAPS + index*4 + 2] = (value >> 16) & 0xff;
  myRAM[DSMAPS + index*4 + 3] = (value >> 24) & 0xff;  // high byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeBUS::readFromDatastream(uInt8 index)
{
  // Pointers are stored as:
  // PPPFF---
  //
  // Increments are stored as
  // ----IIFF
  //
  // P = Pointer
  // I = Increment
  // F = Fractional

  uInt32 pointer = getDatastreamPointer(index);
  uInt16 increment = getDatastreamIncrement(index);
  uInt8 value = myDisplayImage[ pointer >> 20 ];
  pointer += (increment << 12);
  setDatastreamPointer(index, pointer);
  return value;
}
