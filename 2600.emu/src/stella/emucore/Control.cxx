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

#include <cassert>

#include "System.hxx"
#include "Control.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Controller::Controller(Jack jack, const Event& event, const System& system,
                       Type type)
  : myJack{jack},
    myEvent{event},
    mySystem{system},
    myType{type}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Controller::read()
{
  uInt8 ioport = 0b0000;
  if(read(DigitalPin::One))   ioport |= 0b0001;
  if(read(DigitalPin::Two))   ioport |= 0b0010;
  if(read(DigitalPin::Three)) ioport |= 0b0100;
  if(read(DigitalPin::Four))  ioport |= 0b1000;
  return ioport;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Controller::read(DigitalPin pin)
{
  return getPin(pin);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnalogReadout::Connection Controller::read(AnalogPin pin)
{
  return getPin(pin);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Controller::save(Serializer& out) const
{
  try
  {
    // Output the digital pins
    out.putBool(getPin(DigitalPin::One));
    out.putBool(getPin(DigitalPin::Two));
    out.putBool(getPin(DigitalPin::Three));
    out.putBool(getPin(DigitalPin::Four));
    out.putBool(getPin(DigitalPin::Six));

    // Output the analog pins
    getPin(AnalogPin::Five).save(out);
    getPin(AnalogPin::Nine).save(out);
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
    setPin(DigitalPin::One,   in.getBool());
    setPin(DigitalPin::Two,   in.getBool());
    setPin(DigitalPin::Three, in.getBool());
    setPin(DigitalPin::Four,  in.getBool());
    setPin(DigitalPin::Six,   in.getBool());

    // Input the analog pins
    getPin(AnalogPin::Five).load(in);
    getPin(AnalogPin::Nine).load(in);
  }
  catch(...)
  {
    cerr << "ERROR: Controller::load() exception\n";
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Controller::getName(const Type type)
{
  static const std::array<string, int(Controller::Type::LastType)> NAMES =
  {
    "Unknown",
    "AmigaMouse", "AtariMouse", "AtariVox", "BoosterGrip", "CompuMate",
    "Driving", "Sega Genesis", "Joystick", "Keyboard", "KidVid", "MindLink",
    "Paddles", "Paddles_IAxis", "Paddles_IAxDr", "SaveKey", "TrakBall",
    "Lightgun", "QuadTari"
  };

  return NAMES[int(type)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Controller::getPropName(const Type type)
{
  static const std::array<string, int(Controller::Type::LastType)> PROP_NAMES =
  {
    "AUTO",
    "AMIGAMOUSE", "ATARIMOUSE", "ATARIVOX", "BOOSTERGRIP", "COMPUMATE",
    "DRIVING", "GENESIS", "JOYSTICK", "KEYBOARD", "KIDVID", "MINDLINK",
    "PADDLES", "PADDLES_IAXIS", "PADDLES_IAXDR", "SAVEKEY", "TRAKBALL",
    "LIGHTGUN", "QUADTARI"
  };

  return PROP_NAMES[int(type)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Controller::Type Controller::getType(const string& propName)
{
  for(int i = 0; i < static_cast<int>(Type::LastType); ++i)
  {
    if(BSPF::equalsIgnoreCase(propName, getPropName(Type(i))))
    {
      return Type(i);
    }
  }
  // special case
  if(BSPF::equalsIgnoreCase(propName, "KEYPAD"))
    return Type::Keyboard;

  return Type::Unknown;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Controller::setAutoFireRate(int rate, bool isNTSC)
{
  rate = BSPF::clamp(rate, 0, isNTSC ? 30 : 25);
  AUTO_FIRE_RATE = 32 * 1024 * rate / (isNTSC ? 60 : 50);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Controller::AUTO_FIRE_RATE = 0;

