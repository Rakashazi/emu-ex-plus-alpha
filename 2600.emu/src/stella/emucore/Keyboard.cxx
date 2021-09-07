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

Keyboard::ColumnState Keyboard::processColumn(const Event::Type buttons[]) {
  constexpr DigitalPin signals[] =
    {DigitalPin::One, DigitalPin::Two, DigitalPin::Three, DigitalPin::Four};

  for (uInt8 i = 0; i < 4; i++)
    if (myEvent.get(buttons[i]) && !getPin(signals[i])) return ColumnState::gnd;

  for (uInt8 i = 0; i < 4; i++)
    if (myEvent.get(buttons[i]) && getPin(signals[i])) return ColumnState::vcc;

  return ColumnState::notConnected;
}

AnalogReadout::Connection Keyboard::columnStateToAnalogSignal(ColumnState state) const {
  switch (state) {
    case ColumnState::gnd:
      return AnalogReadout::connectToGround();

    case ColumnState::vcc:
       return AnalogReadout::connectToVcc();

    case ColumnState::notConnected:
      return AnalogReadout::connectToVcc(INTERNAL_RESISTANCE);

    default:
      throw runtime_error("unreachable");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Keyboard::write(DigitalPin pin, bool value)
{
  setPin(pin, value);

  const Event::Type col0[] = {myOneEvent, myFourEvent, mySevenEvent, myStarEvent};
  const Event::Type col1[] = {myTwoEvent, myFiveEvent, myEightEvent, myZeroEvent};
  const Event::Type col2[] = {myThreeEvent, mySixEvent, myNineEvent, myPoundEvent};

  ColumnState stateCol0 = processColumn(col0);
  ColumnState stateCol1 = processColumn(col1);
  ColumnState stateCol2 = processColumn(col2);

  setPin(DigitalPin::Six, stateCol2 == ColumnState::gnd ? 0 : 1);
  setPin(AnalogPin::Five, columnStateToAnalogSignal(stateCol1));
  setPin(AnalogPin::Nine, columnStateToAnalogSignal(stateCol0));
}
