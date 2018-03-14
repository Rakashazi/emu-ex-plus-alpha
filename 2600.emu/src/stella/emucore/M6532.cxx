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

#include <cassert>

#include "Console.hxx"
#include "Settings.hxx"
#include "Switches.hxx"
#include "System.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "CartDebug.hxx"
#endif

#include "M6532.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6532::M6532(const Console& console, const Settings& settings)
  : myConsole(console),
    mySettings(settings),
    myTimer(0), mySubTimer(0), myDivider(1),
    myTimerWrapped(false), myWrappedThisCycle(false),
    mySetTimerCycle(0), myLastCycle(0),
    myDDRA(0), myDDRB(0), myOutA(0), myOutB(0),
    myInterruptFlag(false),
    myEdgeDetectPositive(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::reset()
{
  static constexpr uInt8 RAM_7800[128] = {
    0xA9, 0x00, 0xAA, 0x85, 0x01, 0x95, 0x03, 0xE8, 0xE0, 0x2A, 0xD0, 0xF9, 0x85, 0x02, 0xA9, 0x04,
    0xEA, 0x30, 0x23, 0xA2, 0x04, 0xCA, 0x10, 0xFD, 0x9A, 0x8D, 0x10, 0x01, 0x20, 0xCB, 0x04, 0x20,
    0xCB, 0x04, 0x85, 0x11, 0x85, 0x1B, 0x85, 0x1C, 0x85, 0x0F, 0xEA, 0x85, 0x02, 0xA9, 0x00, 0xEA,
    0x30, 0x04, 0x24, 0x03, 0x30, 0x09, 0xA9, 0x02, 0x85, 0x09, 0x8D, 0x12, 0xF1, 0xD0, 0x1E, 0x24,
    0x02, 0x30, 0x0C, 0xA9, 0x02, 0x85, 0x06, 0x8D, 0x18, 0xF1, 0x8D, 0x60, 0xF4, 0xD0, 0x0E, 0x85,
    0x2C, 0xA9, 0x08, 0x85, 0x1B, 0x20, 0xCB, 0x04, 0xEA, 0x24, 0x02, 0x30, 0xD9, 0xA9, 0xFD, 0x85,
    0x08, 0x6C, 0xFC, 0xFF, 0xEA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
  };

  // Initialize the 128 bytes of memory
  bool devSettings = mySettings.getBool("dev.settings");
  if(mySettings.getString(devSettings ? "dev.console" : "plr.console") == "7800")
    for(uInt32 t = 0; t < 128; ++t)
      myRAM[t] = RAM_7800[t];
  else if(mySettings.getBool(devSettings ? "dev.ramrandom" : "plr.ramrandom"))
    for(uInt32 t = 0; t < 128; ++t)
      myRAM[t] = mySystem->randGenerator().next();
  else
    memset(myRAM, 0, 128);

  myTimer = mySystem->randGenerator().next() & 0xff;
  myDivider = 1024;
  mySubTimer = 0;
  myTimerWrapped = false;
  myWrappedThisCycle = false;

  mySetTimerCycle = myLastCycle = 0;

  // Zero the I/O registers
  myDDRA = myDDRB = myOutA = myOutB = 0x00;

  // Zero the timer registers
  myOutTimer[0] = myOutTimer[1] = myOutTimer[2] = myOutTimer[3] = 0x00;

  // Zero the interrupt flag register and mark D7 as invalid
  myInterruptFlag = 0x00;

  // Edge-detect set to negative (high to low)
  myEdgeDetectPositive = false;

  // Let the controllers know about the reset
  myConsole.leftController().reset();
  myConsole.rightController().reset();

#ifdef DEBUGGER_SUPPORT
  createAccessBases();
#endif // DEBUGGER_SUPPORT
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::update()
{
  Controller& port0 = myConsole.leftController();
  Controller& port1 = myConsole.rightController();

  // Get current PA7 state
  bool prevPA7 = port0.myDigitalPinState[Controller::Four];

  // Update entire port state
  port0.update();
  port1.update();
  myConsole.switches().update();

  // Get new PA7 state
  bool currPA7 = port0.myDigitalPinState[Controller::Four];

  // PA7 Flag is set on active transition in appropriate direction
  if((!myEdgeDetectPositive && prevPA7 && !currPA7) ||
     (myEdgeDetectPositive && !prevPA7 && currPA7))
    myInterruptFlag |= PA7Bit;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::updateEmulation()
{
  uInt32 cycles = uInt32(mySystem->cycles() - myLastCycle);
  uInt32 subTimer = mySubTimer;

  // Guard against further state changes if the debugger alread forwarded emulation
  // state (in particular myWrappedThisCycle)
  if (cycles == 0) return;

  myWrappedThisCycle = false;
  mySubTimer = (cycles + mySubTimer) % myDivider;

  if(!myTimerWrapped)
  {
    uInt32 timerTicks = (cycles + subTimer) / myDivider;

    if(timerTicks > myTimer)
    {
      cycles -= ((myTimer + 1) * myDivider - subTimer);
      myWrappedThisCycle = cycles == 0;
      myTimer = 0xFF;
      myTimerWrapped = true;
      myInterruptFlag |= TimerBit;
    }
    else
    {
      myTimer -= timerTicks;
      cycles = 0;
    }
  }

  if(myTimerWrapped)
    myTimer = (myTimer - cycles) & 0xFF;

  myLastCycle = mySystem->cycles();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::install(System& system)
{
  installDelegate(system, *this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::installDelegate(System& system, Device& device)
{
  // Remember which system I'm installed in
  mySystem = &system;

  // All accesses are to the given device
  System::PageAccess access(&device, System::PA_READWRITE);

  // Map all peek/poke to mirrors of RIOT address space to this class
  // That is, all mirrors of ZP RAM ($80 - $FF) and IO ($280 - $29F) in the
  // lower 4K of the 2600 address space are mapped here
  // The two types of addresses are differentiated in peek/poke as follows:
  //    (addr & 0x0200) == 0x0200 is IO     (A9 is 1)
  //    (addr & 0x0300) == 0x0100 is Stack  (A8 is 1, A9 is 0)
  //    (addr & 0x0300) == 0x0000 is ZP RAM (A8 is 0, A9 is 0)
  for (uInt16 addr = 0; addr < 0x1000; addr += System::PAGE_SIZE)
    if ((addr & 0x0080) == 0x0080) {
      mySystem->setPageAccess(addr, access);
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 M6532::peek(uInt16 addr)
{
  updateEmulation();

  // A9 distinguishes I/O registers from ZP RAM
  // A9 = 1 is read from I/O
  // A9 = 0 is read from RAM
  if((addr & 0x0200) == 0x0000)
    return myRAM[addr & 0x007f];

  switch(addr & 0x07)
  {
    case 0x00:    // SWCHA - Port A I/O Register (Joystick)
    {
      uInt8 value = (myConsole.leftController().read() << 4) |
                     myConsole.rightController().read();

      // Each pin is high (1) by default and will only go low (0) if either
      //  (a) External device drives the pin low
      //  (b) Corresponding bit in SWACNT = 1 and SWCHA = 0
      // Thanks to A. Herbert for this info
      return (myOutA | ~myDDRA) & value;
    }

    case 0x01:    // SWACNT - Port A Data Direction Register
    {
      return myDDRA;
    }

    case 0x02:    // SWCHB - Port B I/O Register (Console switches)
    {
      return (myOutB | ~myDDRB) & (myConsole.switches().read() | myDDRB);
    }

    case 0x03:    // SWBCNT - Port B Data Direction Register
    {
      return myDDRB;
    }

    case 0x04:    // INTIM - Timer Output
    case 0x06:
    {
      // Timer Flag is always cleared when accessing INTIM
      if (!myWrappedThisCycle) myInterruptFlag &= ~TimerBit;
      myTimerWrapped = false;
      return myTimer;
    }

    case 0x05:    // TIMINT/INSTAT - Interrupt Flag
    case 0x07:
    {
      // PA7 Flag is always cleared after accessing TIMINT
      uInt8 result = myInterruptFlag;
      myInterruptFlag &= ~PA7Bit;
      return result;
    }

    default:
    {
#ifdef DEBUG_ACCESSES
      cerr << "BAD M6532 Peek: " << hex << addr << endl;
#endif
      return 0;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool M6532::poke(uInt16 addr, uInt8 value)
{
  updateEmulation();

  // A9 distinguishes I/O registers from ZP RAM
  // A9 = 1 is write to I/O
  // A9 = 0 is write to RAM
  if((addr & 0x0200) == 0x0000)
  {
    myRAM[addr & 0x007f] = value;
    return true;
  }

  // A2 distinguishes I/O registers from the timer
  // A2 = 1 is write to timer
  // A2 = 0 is write to I/O
  if((addr & 0x04) != 0)
  {
    // A4 = 1 is write to TIMxT (x = 1, 8, 64, 1024)
    // A4 = 0 is write to edge detect control
    if((addr & 0x10) != 0)
      setTimerRegister(value, addr & 0x03);  // A1A0 determines interval
    else
      myEdgeDetectPositive = addr & 0x01;    // A0 determines direction
  }
  else
  {
    switch(addr & 0x03)
    {
      case 0:     // SWCHA - Port A I/O Register (Joystick)
      {
        myOutA = value;
        setPinState(true);
        break;
      }

      case 1:     // SWACNT - Port A Data Direction Register
      {
        myDDRA = value;
        setPinState(false);
        break;
      }

      case 2:     // SWCHB - Port B I/O Register (Console switches)
      {
        myOutB = value;
        break;
      }

      case 3:     // SWBCNT - Port B Data Direction Register
      {
        myDDRB = value;
        break;
      }
    }
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::setTimerRegister(uInt8 value, uInt8 interval)
{
  static constexpr uInt32 divider[] = { 1, 8, 64, 1024 };

  myDivider = divider[interval];
  myOutTimer[interval] = value;

  myTimer = value;
  mySubTimer = myDivider - 1;
  myTimerWrapped = false;

  // Interrupt timer flag is cleared (and invalid) when writing to the timer
  myInterruptFlag &= ~TimerBit;

  mySetTimerCycle = mySystem->cycles();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::setPinState(bool swcha)
{
  /*
    When a bit in the DDR is set as input, +5V is placed on its output
    pin.  When it's set as output, either +5V or 0V (depending on the
    contents of SWCHA) will be placed on the output pin.
    The standard macros for the AtariVox and SaveKey use this fact to
    send data to the port.  This is represented by the following algorithm:

      if(DDR bit is input)       set output as 1
      else if(DDR bit is output) set output as bit in ORA
  */
  Controller& port0 = myConsole.leftController();
  Controller& port1 = myConsole.rightController();

  uInt8 ioport = myOutA | ~myDDRA;

  port0.write(Controller::One,   ioport & 0x10);
  port0.write(Controller::Two,   ioport & 0x20);
  port0.write(Controller::Three, ioport & 0x40);
  port0.write(Controller::Four,  ioport & 0x80);
  port1.write(Controller::One,   ioport & 0x01);
  port1.write(Controller::Two,   ioport & 0x02);
  port1.write(Controller::Three, ioport & 0x04);
  port1.write(Controller::Four,  ioport & 0x08);

  if(swcha)
  {
    port0.controlWrite(ioport);
    port1.controlWrite(ioport);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool M6532::save(Serializer& out) const
{
  try
  {
    out.putString(name());

    out.putByteArray(myRAM, 128);

    out.putInt(myTimer);
    out.putInt(mySubTimer);
    out.putInt(myDivider);
    out.putBool(myTimerWrapped);
    out.putBool(myWrappedThisCycle);
    out.putLong(myLastCycle);
    out.putLong(mySetTimerCycle);

    out.putByte(myDDRA);
    out.putByte(myDDRB);
    out.putByte(myOutA);
    out.putByte(myOutB);

    out.putByte(myInterruptFlag);
    out.putBool(myEdgeDetectPositive);
    out.putByteArray(myOutTimer, 4);
  }
  catch(...)
  {
    cerr << "ERROR: M6532::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool M6532::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    in.getByteArray(myRAM, 128);

    myTimer = in.getInt();
    mySubTimer = in.getInt();
    myDivider = in.getInt();
    myTimerWrapped = in.getBool();
    myWrappedThisCycle = in.getBool();
    myLastCycle = in.getLong();
    mySetTimerCycle = in.getLong();

    myDDRA = in.getByte();
    myDDRB = in.getByte();
    myOutA = in.getByte();
    myOutB = in.getByte();

    myInterruptFlag = in.getByte();
    myEdgeDetectPositive = in.getBool();
    in.getByteArray(myOutTimer, 4);
  }
  catch(...)
  {
    cerr << "ERROR: M6532::load" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 M6532::intim()
{
  updateEmulation();

  return myTimer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 M6532::timint()
{
  updateEmulation();

  return myInterruptFlag;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 M6532::intimClocks()
{
  updateEmulation();

  // This method is similar to intim(), except instead of giving the actual
  // INTIM value, it will give the current number of clocks between one
  // INTIM value and the next

  return myTimerWrapped ? 1 : (myDivider - mySubTimer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 M6532::timerClocks() const
{
  return uInt32(mySystem->cycles() - mySetTimerCycle);
}

#ifdef DEBUGGER_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::createAccessBases()
{
  myRAMAccessBase = make_unique<uInt8[]>(RAM_SIZE);
  memset(myRAMAccessBase.get(), CartDebug::NONE, RAM_SIZE);
  myStackAccessBase = make_unique<uInt8[]>(STACK_SIZE);
  memset(myStackAccessBase.get(), CartDebug::NONE, STACK_SIZE);
  myIOAccessBase = make_unique<uInt8[]>(IO_SIZE);
  memset(myIOAccessBase.get(), CartDebug::NONE, IO_SIZE);

  myZPAccessDelay = make_unique<uInt8[]>(RAM_SIZE);
  memset(myZPAccessDelay.get(), ZP_DELAY, RAM_SIZE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 M6532::getAccessFlags(uInt16 address) const
{
  if (address & IO_BIT)
    return myIOAccessBase[address & IO_MASK];
  else if (address & STACK_BIT)
    return myStackAccessBase[address & STACK_MASK];
  else
    return myRAMAccessBase[address & RAM_MASK];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::setAccessFlags(uInt16 address, uInt8 flags)
{
  // ignore none flag
  if (flags != CartDebug::NONE) {
    if (address & IO_BIT)
      myIOAccessBase[address & IO_MASK] |= flags;
    else {
      // the first access, either by direct RAM or stack access is assumed as initialization
      if (myZPAccessDelay[address & RAM_MASK])
        myZPAccessDelay[address & RAM_MASK]--;
      else if (address & STACK_BIT)
        myStackAccessBase[address & STACK_MASK] |= flags;
      else
        myRAMAccessBase[address & RAM_MASK] |= flags;
    }
  }
}
#endif // DEBUGGER_SUPPORT
