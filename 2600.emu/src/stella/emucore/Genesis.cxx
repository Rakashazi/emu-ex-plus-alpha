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
#include "Genesis.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Genesis::Genesis(Jack jack, const Event& event, const System& system)
  : Controller(jack, event, system, Controller::Type::Genesis)
{
  if(myJack == Jack::Left)
  {
    myUpEvent      = Event::JoystickZeroUp;
    myDownEvent    = Event::JoystickZeroDown;
    myLeftEvent    = Event::JoystickZeroLeft;
    myRightEvent   = Event::JoystickZeroRight;
    myFire1Event   = Event::JoystickZeroFire;
    myFire2Event   = Event::JoystickZeroFire5;
  }
  else
  {
    myUpEvent      = Event::JoystickOneUp;
    myDownEvent    = Event::JoystickOneDown;
    myLeftEvent    = Event::JoystickOneLeft;
    myRightEvent   = Event::JoystickOneRight;
    myFire1Event   = Event::JoystickOneFire;
    myFire2Event   = Event::JoystickOneFire5;
  }

  setPin(AnalogPin::Five, MIN_RESISTANCE);
  setPin(AnalogPin::Nine, MIN_RESISTANCE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Genesis::update()
{
  // Digital events (from keyboard or joystick hats & buttons)
  setPin(DigitalPin::One, myEvent.get(myUpEvent) == 0);
  setPin(DigitalPin::Two, myEvent.get(myDownEvent) == 0);
  setPin(DigitalPin::Three, myEvent.get(myLeftEvent) == 0);
  setPin(DigitalPin::Four, myEvent.get(myRightEvent) == 0);
  setPin(DigitalPin::Six, myEvent.get(myFire1Event) == 0);

  // The Genesis has one more button (C) that can be read by the 2600
  // However, it seems to work opposite to the BoosterGrip controller,
  // in that the logic is inverted
  setPin(AnalogPin::Five,
    (myEvent.get(myFire2Event) == 0) ? MIN_RESISTANCE : MAX_RESISTANCE
  );

  // Mouse motion and button events
  if(myControlID > -1)
  {
    // The following code was taken from z26
    #define MJ_Threshold 2
    int mousex = myEvent.get(Event::MouseAxisXMove),
        mousey = myEvent.get(Event::MouseAxisYMove);
    if(mousex || mousey)
    {
      if((!(abs(mousey) > abs(mousex) << 1)) && (abs(mousex) >= MJ_Threshold))
      {
        if(mousex < 0)
          setPin(DigitalPin::Three, false);
        else if(mousex > 0)
          setPin(DigitalPin::Four, false);
      }

      if((!(abs(mousex) > abs(mousey) << 1)) && (abs(mousey) >= MJ_Threshold))
      {
        if(mousey < 0)
          setPin(DigitalPin::One, false);
        else if(mousey > 0)
          setPin(DigitalPin::Two, false);
      }
    }
    // Get mouse button state
    if(myEvent.get(Event::MouseButtonLeftValue))
      setPin(DigitalPin::Six, false);
    if(myEvent.get(Event::MouseButtonRightValue))
      setPin(AnalogPin::Five, MAX_RESISTANCE);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Genesis::setMouseControl(
    Controller::Type xtype, int xid, Controller::Type ytype, int yid)
{
  // Currently, the Genesis controller takes full control of the mouse, using
  // both axes for its two degrees of movement, and the left/right buttons for
  // 'B' and 'C', respectively
  if(xtype == Controller::Type::Genesis && ytype == Controller::Type::Genesis && xid == yid)
  {
    myControlID = ((myJack == Jack::Left && xid == 0) ||
                   (myJack == Jack::Right && xid == 1)
                  ) ? xid : -1;
  }
  else
    myControlID = -1;

  return true;
}
