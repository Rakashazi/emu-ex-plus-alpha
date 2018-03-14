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

#include <cstring>

#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif

#include "System.hxx"
#include "Thumbulator.hxx"
#include "CartCDF.hxx"
#include "TIA.hxx"

// Location of data within the RAM copy of the CDF Driver.
//  Version                   0       1
const uInt16 DSxPTR[]   = {0x06E0, 0x00A0};
const uInt16 DSxINC[]   = {0x0768, 0x0128};
const uInt16 WAVEFORM[] = {0x07F0, 0x01B0};
#define DSRAM         0x0800

#define COMMSTREAM    0x20
#define JUMPSTREAM    0x21
#define AMPLITUDE     0x22

#define FAST_FETCH_ON ((myMode & 0x0F) == 0)
#define DIGITAL_AUDIO_ON ((myMode & 0xF0) == 0)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCDF::CartridgeCDF(const BytePtr& image, uInt32 size,
                           const Settings& settings)
  : Cartridge(settings),
    myAudioCycles(0),
    myARMCycles(0),
    myFractionalClocks(0.0)
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image.get(), std::min(32768u, size));

  // even though the ROM is 32K, only 28K is accessible to the 6507
  createCodeAccessBase(4096 * 7);

  // Pointer to the program ROM (28K @ 0 byte offset)
  // which starts after the 2K CDF Driver and 2K C Code
  myProgramImage = myImage + 4096;

  // Pointer to CDF driver in RAM
  myBusDriverImage = myCDFRAM;

  // Pointer to the display RAM
  myDisplayImage = myCDFRAM + DSRAM;

  setVersion();

  // Create Thumbulator ARM emulator
  myThumbEmulator = make_unique<Thumbulator>(
    reinterpret_cast<uInt16*>(myImage), reinterpret_cast<uInt16*>(myCDFRAM),
    settings.getBool("thumb.trapfatal"), myVersion ?
    Thumbulator::ConfigureFor::CDF1 : Thumbulator::ConfigureFor::CDF, this);

  setInitialState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::reset()
{
  initializeRAM(myCDFRAM+2048, 8192-2048);

  myAudioCycles = myARMCycles = 0;
  myFractionalClocks = 0.0;

  setInitialState();

  // Upon reset we switch to the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::setInitialState()
{
  // Copy initial CDF driver to Harmony RAM
  memcpy(myBusDriverImage, myImage, 0x0800);

  for (int i=0; i < 3; ++i)
    myMusicWaveformSize[i] = 27;

  // CDF always starts in bank 6
  myStartBank = 6;

  // Assuming mode starts out with Fast Fetch off and 3-Voice music,
  // need to confirm with Chris
  myMode = 0xFF;

  myFastJumpActive = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::consoleChanged(ConsoleTiming timing)
{
  myThumbEmulator->setConsoleTiming(timing);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::install(System& system)
{
  mySystem = &system;

  // Map all of the accesses to call peek and poke
  System::PageAccess access(this, System::PA_READ);
  for(uInt16 addr = 0x1000; addr < 0x1040; addr += System::PAGE_SIZE)
    mySystem->setPageAccess(addr, access);

  // Install pages for the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void CartridgeCDF::updateMusicModeDataFetchers()
{
  // Calculate the number of cycles since the last update
  uInt32 cycles = uInt32(mySystem->cycles() - myAudioCycles);
  myAudioCycles = mySystem->cycles();

  // Calculate the number of CDF OSC clocks since the last update
  double clocks = ((20000.0 * cycles) / 1193191.66666667) + myFractionalClocks;
  uInt32 wholeClocks = uInt32(clocks);
  myFractionalClocks = clocks - double(wholeClocks);

  // Let's update counters and flags of the music mode data fetchers
  if(wholeClocks > 0)
    for(int x = 0; x <= 2; ++x)
      myMusicCounters[x] += myMusicFrequencies[x] * wholeClocks;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void CartridgeCDF::callFunction(uInt8 value)
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
#ifdef DEBUGGER_SUPPORT
          Debugger::debugger().startWithFatalError(e.what());
#else
          cout << e.what() << endl;
#endif
        }
      }
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeCDF::peek(uInt16 address)
{
  address &= 0x0FFF;

  uInt8 peekvalue = myProgramImage[myBankOffset + address];

  // In debugger/bank-locked mode, we ignore all hotspots and in general
  // anything that can change the internal state of the cart
  if(bankLocked())
    return peekvalue;

  // implement JMP FASTJMP which fetches the destination address from stream 33
  if (myFastJumpActive
      && myJMPoperandAddress == address)
  {
    uInt32 pointer;
    uInt8 value;

    myFastJumpActive--;
    myJMPoperandAddress++;

    pointer = getDatastreamPointer(JUMPSTREAM);
    value = myDisplayImage[ pointer >> 20 ];
    pointer += 0x100000;  // always increment by 1
    setDatastreamPointer(JUMPSTREAM, pointer);

    return value;
  }

  // test for JMP FASTJUMP where FASTJUMP = $0000
  if (FAST_FETCH_ON
      && peekvalue == 0x4C
      && myProgramImage[myBankOffset + address+1] == 0
      && myProgramImage[myBankOffset + address+2] == 0)
  {
    myFastJumpActive = 2; // return next two peeks from datastream 31
    myJMPoperandAddress = address + 1;
    return peekvalue;
  }

  myJMPoperandAddress = 0;

  // Do a FAST FETCH LDA# if:
  //  1) in Fast Fetch mode
  //  2) peeking the operand of an LDA # instruction
  //  3) peek value is 0-34
  if(FAST_FETCH_ON
     && myLDAimmediateOperandAddress == address
     && peekvalue <= AMPLITUDE)
  {
    myLDAimmediateOperandAddress = 0;
    if (peekvalue == AMPLITUDE)
    {
      updateMusicModeDataFetchers();

      if DIGITAL_AUDIO_ON
      {
        // retrieve packed sample (max size is 2K, or 4K of unpacked data)
        uInt32 sampleaddress = getSample() + (myMusicCounters[0] >> 21);

        // get sample value from ROM or RAM
        if (sampleaddress < 0x8000)
          peekvalue = myImage[sampleaddress];
        else if (sampleaddress >= 0x40000000 && sampleaddress < 0x40002000) // check for RAM
          peekvalue = myCDFRAM[sampleaddress - 0x40000000];
        else
          peekvalue = 0;

        // make sure current volume value is in the lower nybble
        if ((myMusicCounters[0] & (1<<20)) == 0)
          peekvalue >>= 4;
        peekvalue &= 0x0f;
      }
      else
      {
        peekvalue = myDisplayImage[getWaveform(0) + (myMusicCounters[0] >> myMusicWaveformSize[0])]
                  + myDisplayImage[getWaveform(1) + (myMusicCounters[1] >> myMusicWaveformSize[1])]
                  + myDisplayImage[getWaveform(2) + (myMusicCounters[2] >> myMusicWaveformSize[2])];
      }
      return peekvalue;
    }
    else
    {
      return readFromDatastream(peekvalue);
    }
  }
  myLDAimmediateOperandAddress = 0;

  // Switch banks if necessary
  switch(address)
  {
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

  if(FAST_FETCH_ON && peekvalue == 0xA9)
    myLDAimmediateOperandAddress = address + 1;

  return peekvalue;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCDF::poke(uInt16 address, uInt8 value)
{
  uInt32 pointer;

  address &= 0x0FFF;

  switch(address)
  {
    case 0xFF0:   // DSWRITE
      pointer = getDatastreamPointer(COMMSTREAM);
      myDisplayImage[ pointer >> 20 ] = value;
      pointer += 0x100000;  // always increment by 1 when writing
      setDatastreamPointer(COMMSTREAM, pointer);
      break;

    case 0xFF1:   // DSPTR
      pointer = getDatastreamPointer(COMMSTREAM);
      pointer <<=8;
      pointer &= 0xf0000000;
      pointer |= (value << 20);
      setDatastreamPointer(COMMSTREAM, pointer);
      break;

    case 0xFF2:   // SETMODE
      myMode = value;
      break;

    case 0xFF3:   // CALLFN
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

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCDF::bank(uInt16 bank)
{
  if(bankLocked()) return false;

  // Remember what bank we're in
  myBankOffset = bank << 12;

  // Setup the page access methods for the current bank
  System::PageAccess access(this, System::PA_READ);

  // Map Program ROM image into the system
  for(uInt16 addr = 0x1040; addr < 0x2000; addr += System::PAGE_SIZE)
  {
    access.codeAccessBase = &myCodeAccessBase[myBankOffset + (addr & 0x0FFF)];
    mySystem->setPageAccess(addr, access);
  }
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeCDF::getBank() const
{
  return myBankOffset >> 12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeCDF::bankCount() const
{
  return 7;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCDF::patch(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  // For now, we ignore attempts to patch the CDF address space
  if(address >= 0x0040)
  {
    myProgramImage[myBankOffset + (address & 0x0FFF)] = value;
    return myBankChanged = true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeCDF::getImage(uInt32& size) const
{
  size = 32768;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uInt32 CartridgeCDF::thumbCallback(uInt8 function, uInt32 value1, uInt32 value2)
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
  }

  return 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCDF::save(Serializer& out) const
{
  try
  {
    out.putString(name());

    // Indicates which bank is currently active
    out.putShort(myBankOffset);

    // Indicates current mode
    out.putByte(myMode);

    // State of FastJump
    out.putByte(myFastJumpActive);

    // operand addresses
    out.putShort(myLDAimmediateOperandAddress);
    out.putShort(myJMPoperandAddress);

    // Harmony RAM
    out.putByteArray(myCDFRAM, 8192);

    // Audio info
    out.putIntArray(myMusicCounters, 3);
    out.putIntArray(myMusicFrequencies, 3);
    out.putByteArray(myMusicWaveformSize, 3);

    // Save cycles and clocks
    out.putLong(myAudioCycles);
    out.putDouble(myFractionalClocks);
    out.putLong(myARMCycles);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeCDF::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCDF::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    // Indicates which bank is currently active
    myBankOffset = in.getShort();

    // Indicates current mode
    myMode = in.getByte();

    // State of FastJump
    myFastJumpActive = in.getByte();

    // Address of LDA # operand
    myLDAimmediateOperandAddress = in.getShort();
    myJMPoperandAddress = in.getShort();

    // Harmony RAM
    in.getByteArray(myCDFRAM, 8192);

    // Audio info
    in.getIntArray(myMusicCounters, 3);
    in.getIntArray(myMusicFrequencies, 3);
    in.getByteArray(myMusicWaveformSize, 3);

    // Get cycles and clocks
    myAudioCycles = in.getLong();
    myFractionalClocks = in.getDouble();
    myARMCycles = in.getLong();
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeCDF::load" << endl;
    return false;
  }

  // Now, go to the current bank
  bank(myBankOffset >> 12);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeCDF::getDatastreamPointer(uInt8 index) const
{
  uInt16 address = DSxPTR[myVersion] + index * 4;

  return myCDFRAM[address + 0]        +  // low byte
        (myCDFRAM[address + 1] << 8)  +
        (myCDFRAM[address + 2] << 16) +
        (myCDFRAM[address + 3] << 24) ;  // high byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::setDatastreamPointer(uInt8 index, uInt32 value)
{
  uInt16 address = DSxPTR[myVersion] + index * 4;

  myCDFRAM[address + 0] = value & 0xff;          // low byte
  myCDFRAM[address + 1] = (value >> 8) & 0xff;
  myCDFRAM[address + 2] = (value >> 16) & 0xff;
  myCDFRAM[address + 3] = (value >> 24) & 0xff;  // high byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeCDF::getDatastreamIncrement(uInt8 index) const
{
  uInt16 address = DSxINC[myVersion] + index * 4;

  return myCDFRAM[address + 0]        +   // low byte
        (myCDFRAM[address + 1] << 8)  +
        (myCDFRAM[address + 2] << 16) +
        (myCDFRAM[address + 3] << 24) ;   // high byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeCDF::getWaveform(uInt8 index) const
{
  uInt32 result;
  uInt16 address = WAVEFORM[myVersion] + index * 4;

  result = myCDFRAM[address + 0]        +  // low byte
          (myCDFRAM[address + 1] << 8)  +
          (myCDFRAM[address + 2] << 16) +
          (myCDFRAM[address + 3] << 24);   // high byte

  result -= (0x40000000 + DSRAM);

  if (result >= 4096)
    result &= 4095;

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeCDF::getSample()
{
  uInt32 result;
  uInt16 address = WAVEFORM[myVersion];

  result = myCDFRAM[address + 0]        +  // low byte
          (myCDFRAM[address + 1] << 8)  +
          (myCDFRAM[address + 2] << 16) +
          (myCDFRAM[address + 3] << 24);   // high byte

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeCDF::getWaveformSize(uInt8 index) const
{
  return myMusicWaveformSize[index];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeCDF::readFromDatastream(uInt8 index)
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::setVersion()
{
  myVersion = 0;

  for(uInt32 i = 0; i < 2048; i += 4)
  {
    // CDF signature occurs 3 times in a row, i+3 (+7 or +11) is version
    if (    myImage[i+0] == 0x43 && myImage[i + 4] == 0x43 && myImage[i + 8] == 0x43) // C
      if (  myImage[i+1] == 0x44 && myImage[i + 5] == 0x44 && myImage[i + 9] == 0x44) // D
        if (myImage[i+2] == 0x46 && myImage[i + 6] == 0x46 && myImage[i +10] == 0x46) // F
        {
          myVersion = myImage[i+3];
          break;
        }
  }
}
