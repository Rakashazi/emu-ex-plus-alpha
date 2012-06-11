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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: MindLink.cxx 2366 2012-01-22 21:01:13Z stephena $
//============================================================================

#include "Event.hxx"
#include "MindLink.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MindLink::MindLink(Jack jack, const Event& event, const System& system)
  : Controller(jack, event, system, Controller::MindLink),
    myIOPort(0xff),
    myMindlinkPos(0x2800),
    myMindlinkPos1(0x2800),
    myMindlinkPos2(0x1000),
    myMindlinkShift(1)
{
  // Analog pins are never used by the MindLink
  myAnalogPinValue[Five] = myAnalogPinValue[Nine] = maximumResistance;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MindLink::~MindLink()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MindLink::update()
{
  myIOPort |= 0xf0;
  if(0)//MPdirection & 0x01)
    myMindlinkPos = myMindlinkPos2 + 0x1800;
  else
    myMindlinkPos = myMindlinkPos1;
	
  myMindlinkPos = (myMindlinkPos & 0x3fffffff) +
                  (myEvent.get(Event::MouseAxisXValue) << 3);
  if(myMindlinkPos < 0x2800)
    myMindlinkPos = 0x2800;
  if(myMindlinkPos >= 0x3800)
    myMindlinkPos = 0x3800;
	

  if(0)//MPdirection & 0x01)
  {
    myMindlinkPos2 = myMindlinkPos - 0x1800;
    myMindlinkPos = myMindlinkPos2;
  }
  else
    myMindlinkPos1 = myMindlinkPos;

  myMindlinkShift = 1;
  nextMindlinkBit();

  if(myEvent.get(Event::MouseButtonLeftValue) ||
     myEvent.get(Event::MouseButtonRightValue))
    myMindlinkPos |= 0x4000; /* this bit starts a game */

//cerr << HEX4 << (int)myMindlinkPos << " : " << HEX2 << (int)myIOPort << endl;

  // Convert IOPort values back to booleans
  myDigitalPinState[One]   = myIOPort & 0x10;
  myDigitalPinState[Two]   = myIOPort & 0x20;
  myDigitalPinState[Three] = myIOPort & 0x40;
  myDigitalPinState[Four]  = myIOPort & 0x80;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MindLink::nextMindlinkBit()
{
  if(myIOPort & 0x10)
  {
    myIOPort &= 0x3f;
    if(myMindlinkPos & myMindlinkShift)
      myIOPort |= 0x80;
    myMindlinkShift <<= 1;

  myDigitalPinState[One]   = myIOPort & 0x10;
  myDigitalPinState[Two]   = myIOPort & 0x20;
  myDigitalPinState[Three] = myIOPort & 0x40;
  myDigitalPinState[Four]  = myIOPort & 0x80;


cerr << dec << (int)myMindlinkShift << " : " << HEX2 << (int)myIOPort << endl;

	}
}
