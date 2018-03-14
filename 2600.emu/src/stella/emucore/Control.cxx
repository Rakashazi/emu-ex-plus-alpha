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

#include "System.hxx"
#include "Control.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Controller::Controller(Jack jack, const Event& event, const System& system,
                       Type type)
  : myJack(jack),
    myEvent(event),
    mySystem(system),
    myType(type),
    myOnAnalogPinUpdateCallback(nullptr)
{
  myDigitalPinState[One]   =
  myDigitalPinState[Two]   =
  myDigitalPinState[Three] =
  myDigitalPinState[Four]  =
  myDigitalPinState[Six]   = true;

  myAnalogPinValue[Five] =
  myAnalogPinValue[Nine] = maximumResistance;

  switch(myType)
  {
    case Joystick:
      myName = "Joystick";
      break;
    case Paddles:
      myName = "Paddles";
      break;
    case BoosterGrip:
      myName = "BoosterGrip";
      break;
    case Driving:
      myName = "Driving";
      break;
    case Keyboard:
      myName = "Keyboard";
      break;
    case AmigaMouse:
      myName = "AmigaMouse";
      break;
    case AtariMouse:
      myName = "AtariMouse";
      break;
    case TrakBall:
      myName = "TrakBall";
      break;
    case AtariVox:
      myName = "AtariVox";
      break;
    case SaveKey:
      myName = "SaveKey";
      break;
    case KidVid:
      myName = "KidVid";
      break;
    case Genesis:
      myName = "Genesis";
      break;
    case MindLink:
      myName = "MindLink";
      break;
    case CompuMate:
      myName = "CompuMate";
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Controller::read()
{
  uInt8 ioport = 0x00;
  if(read(One))   ioport |= 0x01;
  if(read(Two))   ioport |= 0x02;
  if(read(Three)) ioport |= 0x04;
  if(read(Four))  ioport |= 0x08;
  return ioport;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Controller::read(DigitalPin pin)
{
  return myDigitalPinState[pin];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 Controller::read(AnalogPin pin)
{
  return myAnalogPinValue[pin];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Controller::set(DigitalPin pin, bool value)
{
  myDigitalPinState[pin] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Controller::set(AnalogPin pin, Int32 value)
{
  updateAnalogPin(pin, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Controller::updateAnalogPin(AnalogPin pin, Int32 value)
{
  myAnalogPinValue[pin] = value;

  if (myOnAnalogPinUpdateCallback) {
    myOnAnalogPinUpdateCallback(pin);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Controller::save(Serializer& out) const
{
  try
  {
    // Output the digital pins
    out.putBool(myDigitalPinState[One]);
    out.putBool(myDigitalPinState[Two]);
    out.putBool(myDigitalPinState[Three]);
    out.putBool(myDigitalPinState[Four]);
    out.putBool(myDigitalPinState[Six]);

    // Output the analog pins
    out.putInt(myAnalogPinValue[Five]);
    out.putInt(myAnalogPinValue[Nine]);
  }
  catch(...)
  {
    cerr << "ERROR: Controller::save() exception\n";
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Controller::load(Serializer& in)
{
  try
  {
    // Input the digital pins
    myDigitalPinState[One]   = in.getBool();
    myDigitalPinState[Two]   = in.getBool();
    myDigitalPinState[Three] = in.getBool();
    myDigitalPinState[Four]  = in.getBool();
    myDigitalPinState[Six]   = in.getBool();

    // Input the analog pins
    myAnalogPinValue[Five] = in.getInt();
    myAnalogPinValue[Nine] = in.getInt();
  }
  catch(...)
  {
    cerr << "ERROR: Controller::load() exception\n";
    return false;
  }
  return true;
}
