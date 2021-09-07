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

#include "Joystick.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Joystick::Joystick(Jack jack, const Event& event, const System& system, bool altmap)
  : Joystick(jack, event, system, Controller::Type::Joystick, altmap)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Joystick::Joystick(Jack jack, const Event& event, const System& system,
                   Controller::Type type, bool altmap)
  : Controller(jack, event, system, type)
{
  if(myJack == Jack::Left)
  {
    if(!altmap)
    {
      myUpEvent = Event::JoystickZeroUp;
      myDownEvent = Event::JoystickZeroDown;
      myLeftEvent = Event::JoystickZeroLeft;
      myRightEvent = Event::JoystickZeroRight;
      myFireEvent = Event::JoystickZeroFire;
    }
    else
    {
      myUpEvent = Event::JoystickTwoUp;
      myDownEvent = Event::JoystickTwoDown;
      myLeftEvent = Event::JoystickTwoLeft;
      myRightEvent = Event::JoystickTwoRight;
      myFireEvent = Event::JoystickTwoFire;
    }
  }
  else
  {
    if(!altmap)
    {
      myUpEvent = Event::JoystickOneUp;
      myDownEvent = Event::JoystickOneDown;
      myLeftEvent = Event::JoystickOneLeft;
      myRightEvent = Event::JoystickOneRight;
      myFireEvent = Event::JoystickOneFire;
    }
    else
    {
      myUpEvent = Event::JoystickThreeUp;
      myDownEvent = Event::JoystickThreeDown;
      myLeftEvent = Event::JoystickThreeLeft;
      myRightEvent = Event::JoystickThreeRight;
      myFireEvent = Event::JoystickThreeFire;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Joystick::update()
{
  updateButtons();

  updateDigitalAxes();
  updateMouseAxes();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Joystick::updateButtons()
{
  bool firePressed = myEvent.get(myFireEvent) != 0;

  // The joystick uses both mouse buttons for the single joystick button
  updateMouseButtons(firePressed, firePressed);

  setPin(DigitalPin::Six, !getAutoFireState(firePressed));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Joystick::updateMouseButtons(bool& pressedLeft, bool& pressedRight)
{
  if(myControlID > -1)
  {
    pressedLeft |= (myEvent.get(Event::MouseButtonLeftValue) != 0);
    pressedRight |= (pressedRight || myEvent.get(Event::MouseButtonRightValue) != 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Joystick::updateDigitalAxes()
{
  // Digital events (from keyboard or joystick hats & buttons)
  setPin(DigitalPin::One, myEvent.get(myUpEvent) == 0);
  setPin(DigitalPin::Two, myEvent.get(myDownEvent) == 0);
  setPin(DigitalPin::Three, myEvent.get(myLeftEvent) == 0);
  setPin(DigitalPin::Four, myEvent.get(myRightEvent) == 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Joystick::updateMouseAxes()
{
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
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Joystick::setMouseControl(
    Controller::Type xtype, int xid, Controller::Type ytype, int yid)
{
  // Currently, the joystick takes full control of the mouse, using both
  // axes for its two degrees of movement
  if(xtype == myType && ytype == myType && xid == yid)
  {
    myControlID = ((myJack == Jack::Left && xid == 0) ||
                   (myJack == Jack::Right && xid == 1)
                  ) ? xid : -1;
  }
  else
    myControlID = -1;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Joystick::setDeadZone(int deadzone)
{
  _DEAD_ZONE = deadZoneValue(deadzone);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Joystick::deadZoneValue(int deadzone)
{
  deadzone = BSPF::clamp(deadzone, DEAD_ZONE_MIN, DEAD_ZONE_MAX);

  return 3200 + deadzone * 1000;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Joystick::_DEAD_ZONE = 3200;
