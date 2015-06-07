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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: StateManager.hxx 3131 2015-01-01 03:49:32Z stephena $
//============================================================================

#ifndef STATE_MANAGER_HXX
#define STATE_MANAGER_HXX

class OSystem;

#include "Serializer.hxx"

/**
  This class provides an interface to all things related to emulation state.
  States can be loaded or saved here, as well as recorded, rewound, and later
  played back.

  @author  Stephen Anthony
  @version $Id: StateManager.hxx 3131 2015-01-01 03:49:32Z stephena $
*/
class StateManager
{
  public:
    /**
      Create a new statemananger class
    */
    StateManager(OSystem& osystem);

  public:
    /**
      Answers whether the manager is in record or playback mode
    */
    bool isActive() const { return myActiveMode != kOffMode; }

    bool toggleRecordMode();
    bool toggleRewindMode();

    /**
      Updates the state of the system based on the currently active mode
    */
    void update();

    /**
      Load a state into the current system

      @param slot  The state 'slot' to load state from
    */
    void loadState(int slot = -1);

    /**
      Save the current state from the system

      @param slot  The state 'slot' to save into
    */
    void saveState(int slot = -1);

    /**
      Switches to the next higher state slot (circular queue style)
    */
    void changeState();

    /**
      Load a state into the current system from the given Serializer.
      No messages are printed to the screen.

      @param in  The Serializer object to use

      @return  False on any load errors, else true
    */
    bool loadState(Serializer& in);

    /**
      Save the current state from the system into the given Serializer.
      No messages are printed to the screen.

      @param out  The Serializer object to use

      @return  False on any save errors, else true
    */
    bool saveState(Serializer& out);

    /**
      Resets manager to defaults
    */
    void reset();

  private:
    // Copy constructor and assignment operator not supported
    StateManager(const StateManager&);
    StateManager& operator = (const StateManager&);

  private:
    enum Mode {
      kOffMode,
      kMoviePlaybackMode,
      kMovieRecordMode,
      kRewindPlaybackMode,
      kRewindRecordMode
    };

    enum {
      kVersion = 001
    };

    // The parent OSystem object
    OSystem& myOSystem;

    // The current slot for load/save states
    int myCurrentSlot;

    // Whether the manager is in record or playback mode
    Mode myActiveMode;

    // MD5 of the currently active ROM (either in movie or rewind mode)
    string myMD5;

    // Serializer classes used to save/load the eventstream
    Serializer myMovieWriter;
    Serializer myMovieReader;
};

#endif
