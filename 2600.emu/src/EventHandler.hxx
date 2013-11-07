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
// Copyright (c) 1995-2013 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: EventHandler.hxx 2579 2013-01-04 19:49:01Z stephena $
//============================================================================

#ifndef EVENTHANDLER_HXX
#define EVENTHANDLER_HXX

#include <stella/emucore/Event.hxx>

class OSystem;

enum EventMode {
  kEmulationMode = 0,  // make sure these are set correctly,
  kMenuMode      = 1,  // since they'll be used as array indices
  kNumModes      = 2
};

class EventHandler
{
  public:

	EventHandler(OSystem* osystem);

    enum State {
      S_NONE,
      S_EMULATE,
      S_PAUSE,
      S_LAUNCHER,
      S_MENU,
      S_CMDMENU,
      S_DEBUGGER
    };

    Event& event() { return myEvent; }

    void allowAllDirections(bool allow) { myAllowAllDirectionsFlag = allow; }

    Event myEvent;

    bool myAllowAllDirectionsFlag;
};

#endif
