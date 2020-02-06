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

#include "Event.hxx"
#include "Keyboard.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Keyboard::Keyboard(Jack jack, const Event& event, const System& system)
  : Controller(jack, event, system, Controller::Type::Keyboard)
{
  if(myJack == Jack::Left)
  {
    myOneEvent   = Event::KeyboardZero1;
    myTwoEvent   = Event::KeyboardZero2;
    myThreeEvent = Event::KeyboardZero3;
    myFourEvent  = Event::KeyboardZero4;
    myFiveEvent  = Event::KeyboardZero5;
    mySixEvent   = Event::KeyboardZero6;
    mySevenEvent = Event::KeyboardZero7;
    myEightEvent = Event::KeyboardZero8;
    myNineEvent  = Event::KeyboardZero9;
    myStarEvent  = Event::KeyboardZeroStar;
    myZeroEvent  = Event::KeyboardZero0;
    myPoundEvent = Event::KeyboardZeroPound;
  }
  else
  {
    myOneEvent   = Event::KeyboardOne1;
    myTwoEvent   = Event::KeyboardOne2;
    myThreeEvent = Event::KeyboardOne3;
    myFourEvent  = Event::KeyboardOne4;
    myFiveEvent  = Event::KeyboardOne5;
    mySixEvent   = Event::KeyboardOne6;
    mySevenEvent = Event::KeyboardOne7;
    myEightEvent = Event::KeyboardOne8;
    myNineEvent  = Event::KeyboardOne9;
    myStarEvent  = Event::KeyboardOneStar;
    myZeroEvent  = Event::KeyboardOne0;
    myPoundEvent = Event::KeyboardOnePound;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Keyboard::write(DigitalPin pin, bool value)
{
  setPin(pin, value);

  // Set defaults
  setPin(DigitalPin::Six, true);
  Int32 resistanceFive = MIN_RESISTANCE;
  Int32 resistanceNine = MIN_RESISTANCE;

  // Now scan the rows and columns
  if(!getPin(DigitalPin::Four))
  {
    setPin(DigitalPin::Six, myEvent.get(myPoundEvent) == 0);
    if(myEvent.get(myZeroEvent) != 0) resistanceFive = MAX_RESISTANCE;
    if(myEvent.get(myStarEvent) != 0) resistanceNine = MAX_RESISTANCE;
  }
  if(!getPin(DigitalPin::Three))
  {
    setPin(DigitalPin::Six, myEvent.get(myNineEvent) == 0);
    if(myEvent.get(myEightEvent) != 0) resistanceFive = MAX_RESISTANCE;
    if(myEvent.get(mySevenEvent) != 0) resistanceNine = MAX_RESISTANCE;
  }
  if(!getPin(DigitalPin::Two))
  {
    setPin(DigitalPin::Six, myEvent.get(mySixEvent) == 0);
    if(myEvent.get(myFiveEvent) != 0) resistanceFive = MAX_RESISTANCE;
    if(myEvent.get(myFourEvent) != 0) resistanceNine = MAX_RESISTANCE;
  }
  if(!getPin(DigitalPin::One))
  {
    setPin(DigitalPin::Six, myEvent.get(myThreeEvent) == 0);
    if(myEvent.get(myTwoEvent) != 0) resistanceFive = MAX_RESISTANCE;
    if(myEvent.get(myOneEvent) != 0) resistanceNine = MAX_RESISTANCE;
  }

  if(resistanceFive != read(AnalogPin::Five))
    setPin(AnalogPin::Five, resistanceFive);
  if(resistanceNine != read(AnalogPin::Nine))
    setPin(AnalogPin::Nine, resistanceNine);
}
