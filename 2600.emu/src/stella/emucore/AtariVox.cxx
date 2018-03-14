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

#include "SerialPort.hxx"
#include "System.hxx"
#include "AtariVox.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AtariVox::AtariVox(Jack jack, const Event& event, const System& system,
                   const SerialPort& port, const string& portname,
                   const string& eepromfile)
  : SaveKey(jack, event, system, eepromfile, Controller::AtariVox),
    mySerialPort(const_cast<SerialPort&>(port)),
    myShiftCount(0),
    myShiftRegister(0),
    myLastDataWriteCycle(0)
{
  if(mySerialPort.openPort(portname))
    myAboutString = " (using serial port \'" + portname + "\')";
  else
    myAboutString = " (invalid serial port \'" + portname + "\')";

  myDigitalPinState[Three] = myDigitalPinState[Four] = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AtariVox::read(DigitalPin pin)
{
  // We need to override the Controller::read() method, since the timing
  // of the actual read is important for the EEPROM (we can't just read
  // 60 times per second in the ::update() method)
  switch(pin)
  {
    // Pin 2: SpeakJet READY
    case Two:
      // For now, we just assume the device is always ready
      return myDigitalPinState[Two] = true;

    default:
      return SaveKey::read(pin);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariVox::write(DigitalPin pin, bool value)
{
  // Change the pin state based on value
  switch(pin)
  {
    // Pin 1: SpeakJet DATA
    //        output serial data to the speakjet
    case One:
      myDigitalPinState[One] = value;
      clockDataIn(value);
      break;

    default:
      SaveKey::write(pin, value);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariVox::clockDataIn(bool value)
{
  if(value && (myShiftCount == 0))
    return;

  // If this is the first write this frame, or if it's been a long time
  // since the last write, start a new data byte.
  uInt64 cycle = mySystem.cycles();
  if((cycle < myLastDataWriteCycle) || (cycle > myLastDataWriteCycle + 1000))
  {
    myShiftRegister = 0;
    myShiftCount = 0;
  }

  // If this is the first write this frame, or if it's been 62 cycles
  // since the last write, shift this bit into the current byte.
  if((cycle < myLastDataWriteCycle) || (cycle >= myLastDataWriteCycle + 62))
  {
    myShiftRegister >>= 1;
    myShiftRegister |= (value << 15);
    if(++myShiftCount == 10)
    {
      myShiftCount = 0;
      myShiftRegister >>= 6;
      if(!(myShiftRegister & (1<<9)))
        cerr << "AtariVox: bad start bit" << endl;
      else if((myShiftRegister & 1))
        cerr << "AtariVox: bad stop bit" << endl;
      else
      {
        uInt8 data = ((myShiftRegister >> 1) & 0xff);
        mySerialPort.writeByte(&data);
      }
      myShiftRegister = 0;
    }
  }

  myLastDataWriteCycle = cycle;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariVox::reset()
{
  myLastDataWriteCycle = 0;
  SaveKey::reset();
}
