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
#include "MindLink.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MindLink::MindLink(Jack jack, const Event& event, const System& system)
  : Controller(jack, event, system, Controller::Type::MindLink)
{
  setPin(DigitalPin::One, true);
  setPin(DigitalPin::Two, true);
  setPin(DigitalPin::Three, true);
  setPin(DigitalPin::Four, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MindLink::update()
{
  setPin(DigitalPin::One, true);
  setPin(DigitalPin::Two, true);
  setPin(DigitalPin::Three, true);
  setPin(DigitalPin::Four, true);

  if(!myMouseEnabled)
    return;

  myMindlinkPos = (myMindlinkPos & 0x3fffffff) +
                  (myEvent.get(Event::MouseAxisXMove) << 3);
  if(myMindlinkPos < 0x2800)
    myMindlinkPos = 0x2800;
  if(myMindlinkPos >= 0x3800)
    myMindlinkPos = 0x3800;

  myMindlinkShift = 1;
  nextMindlinkBit();

  if(myEvent.get(Event::MouseButtonLeftValue) ||
     myEvent.get(Event::MouseButtonRightValue))
    myMindlinkPos |= 0x4000;  // this bit starts a game
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MindLink::nextMindlinkBit()
{
  if(getPin(DigitalPin::One))
  {
    setPin(DigitalPin::Three, false);
    setPin(DigitalPin::Four, false);
    if(myMindlinkPos & myMindlinkShift)
      setPin(DigitalPin::Four, true);
    myMindlinkShift <<= 1;
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MindLink::setMouseControl(
    Controller::Type xtype, int xid, Controller::Type ytype, int yid)
{
  // Currently, the mindlink takes full control of the mouse, but only ever
  // uses the x-axis, and both mouse buttons for the single mindlink button
  // As well, there's no separate setting for x and y axis, so any
  // combination of Controller and id is valid
  myMouseEnabled = (xtype == myType || ytype == myType) &&
                   (xid != -1 || yid != -1);
  return true;
}
