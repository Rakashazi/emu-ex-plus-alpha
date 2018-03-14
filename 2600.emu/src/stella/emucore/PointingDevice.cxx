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

#include <climits>

#include "Control.hxx"
#include "Event.hxx"
#include "System.hxx"
#include "TIA.hxx"

#include "PointingDevice.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PointingDevice::PointingDevice(Jack jack, const Event& event,
                               const System& system, Controller::Type type,
                               float sensitivity)
  : Controller(jack, event, system, type),
    mySensitivity(sensitivity),
    myHCounterRemainder(0.0), myVCounterRemainder(0.0),
    myTrackBallLinesH(1), myTrackBallLinesV(1),
    myTrackBallLeft(false), myTrackBallDown(false),
    myCountH(0), myCountV(0),
    myScanCountH(0), myScanCountV(0),
    myFirstScanOffsetH(0), myFirstScanOffsetV(0),
    myMouseEnabled(false)
{
  // The code in ::read() is set up to always return IOPortA values in
  // the lower 4 bits data value
  // As such, the jack type (left or right) isn't necessary here
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 PointingDevice::read()
{
  int scanline = mySystem.tia().scanlines();

  // Loop over all missed changes
  while(myScanCountH < scanline)
  {
    if(myTrackBallLeft) myCountH--;
    else                myCountH++;

    // Define scanline of next change
    myScanCountH += myTrackBallLinesH;
  }

  // Loop over all missed changes
  while(myScanCountV < scanline)
  {
    if(myTrackBallDown) myCountV++;
    else                myCountV--;

    // Define scanline of next change
    myScanCountV += myTrackBallLinesV;
  }

  myCountH &= 0b11;
  myCountV &= 0b11;

  uInt8 portA = ioPortA(myCountH, myCountV, myTrackBallLeft, myTrackBallDown);

  myDigitalPinState[One]   = portA & 0b0001;
  myDigitalPinState[Two]   = portA & 0b0010;
  myDigitalPinState[Three] = portA & 0b0100;
  myDigitalPinState[Four]  = portA & 0b1000;

  return portA;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PointingDevice::update()
{
  if(!myMouseEnabled)
    return;

  // Update horizontal direction
  updateDirection( myEvent.get(Event::MouseAxisXValue), myHCounterRemainder,
      myTrackBallLeft, myTrackBallLinesH, myScanCountH, myFirstScanOffsetH);

  // Update vertical direction
  updateDirection(-myEvent.get(Event::MouseAxisYValue), myVCounterRemainder,
      myTrackBallDown, myTrackBallLinesV, myScanCountV, myFirstScanOffsetV);

  // Get mouse button state
  myDigitalPinState[Six] = (myEvent.get(Event::MouseButtonLeftValue)  == 0) &&
                           (myEvent.get(Event::MouseButtonRightValue) == 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PointingDevice::setMouseControl(
    Controller::Type xtype, int xid, Controller::Type ytype, int yid)
{
  // Currently, the various trakball controllers take full control of the
  // mouse, and use both mouse buttons for the single fire button
  // As well, there's no separate setting for x and y axis, so any
  // combination of Controller and id is valid
  myMouseEnabled = (xtype == myType || ytype == myType) &&
                   (xid != -1 || yid != -1);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PointingDevice::setSensitivity(int sensitivity)
{
  BSPF::clamp(sensitivity, 1, 20, 10);
  TB_SENSITIVITY = sensitivity / 10.0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PointingDevice::updateDirection(int counter, float& counterRemainder,
    bool& trackBallDir, int& trackBallLines, int& scanCount, int& firstScanOffset)
{
  // Apply sensitivity and calculate remainder
  float fTrackBallCount = counter * mySensitivity * TB_SENSITIVITY + counterRemainder;
  int trackBallCount = int(std::lround(fTrackBallCount));
  counterRemainder = fTrackBallCount - trackBallCount;

  if(trackBallCount)
  {
    trackBallDir = (trackBallCount > 0);
    trackBallCount = abs(trackBallCount);

    // Calculate lines to wait between sending new horz/vert values
    trackBallLines = mySystem.tia().scanlinesLastFrame() / trackBallCount;

    // Set lower limit in case of (unrealistic) ultra fast mouse movements
    if (trackBallLines == 0) trackBallLines = 1;

    // Define scanline of first change
    scanCount = (trackBallLines * firstScanOffset) >> 12;
  }
  else
  {
    // Prevent any change
    scanCount = INT_MAX;

    // Define offset factor for first change, move randomly forward by up to 1/8th
    firstScanOffset = (((firstScanOffset << 3) + rand() %
                      (1 << 12)) >> 3) & ((1 << 12) - 1);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float PointingDevice::TB_SENSITIVITY = 1.0;
