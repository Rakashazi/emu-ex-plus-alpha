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
// $Id: OSystem.hxx 2471 2012-05-13 18:06:56Z stephena $
//============================================================================

#pragma once

class Cartridge;
class CheatManager;
class CommandMenu;
class Console;
class Debugger;
class EventHandler;
class FrameBuffer;
class Launcher;
class Menu;
class SoundGeneric;
class Properties;
class PropertiesSet;
class Random;
class SerialPort;
class Settings;
class Sound;
class StateManager;
class VideoDialog;

#include "bspf.hxx"
#undef HAVE_UNISTD_H
#include "Console.hxx"

class OSystem
{
  friend class EventHandler;

  public:
    OSystem() {}

    /**
      Get the event handler of the system

      @return The event handler
    */
    EventHandler& eventHandler() const;

    /**
      Get the frame buffer of the system

      @return The frame buffer
    */
    FrameBuffer& frameBuffer() const;

    /**
      Get the sound object of the system

      @return The sound object
    */
    Sound& sound() const;

    SoundGeneric& soundGeneric() const;

    /**
      Get the settings object of the system

      @return The settings object
    */
    Settings& settings() const;

    /**
      Get the random object of the system.

      @return The random object
    */
    Random& random() const;

    /**
      Get the set of game properties for the system

      @return The properties set object
    */
    PropertiesSet& propSet() const;

    /**
      Get the console of the system.

      @return The console object
    */
    Console& console() const { return *myConsole; }
    bool hasConsole() const { return myConsole != nullptr; }

    void makeConsole(unique_ptr<Cartridge>& cart, const Properties& props);
    void deleteConsole();

    /**
      Get the serial port of the system.

      @return The serial port object
    */
    SerialPort& serialPort() const;

#ifdef DEBUGGER_SUPPORT
    /**
      Create all child objects which belong to this OSystem
    */
    void createDebugger(Console& console);

    /**
      Get the ROM debugger of the system.

      @return The debugger object
    */
    Debugger& debugger() const { return *myDebugger; }
#endif

#ifdef CHEATCODE_SUPPORT
    /**
      Get the cheat manager of the system.

      @return The cheatmanager object
    */
    CheatManager& cheat() const { return *myCheatManager; }
#endif

    // no-op
    void setFramerate(float framerate) {}

    /**
      Return the full/complete directory name for storing state files.
    */
    string stateDir() const;

    /**
      Return the full/complete directory name for storing nvram
      (flash/EEPROM) files.
    */
    string nvramDir() const;

    /**
      This method should be called to get the full path of the config file.

      @return String representing the full path of the config filename.
    */
    string configFile() const { return ""; }

    /**
      This method should be called to get the full path of the
      (optional) palette file.

      @return String representing the full path of the properties filename.
    */
    string paletteFile() const { return ""; }

    /**
      Append a message to the internal log.

      @param message  The message to be appended
      @param level    If 0, always output the message, only append when
                      level is less than or equal to that in 'loglevel'
    */
	void logMessage(const string& message, uInt8 level);

  public:
    //////////////////////////////////////////////////////////////////////
    // The following methods are system-specific and can be overrided in
    // derived classes.  Otherwise, the base methods will be used.
    //////////////////////////////////////////////////////////////////////
    /**
      This method returns number of ticks in microseconds since some
      pre-defined time in the past.  *NOTE*: it is necessary that this
      pre-defined time exists between runs of the application, and must
      be (relatively) unique.  For example, the time since the system
      started running is not a good choice, since it can be duplicated.
      The current implementation uses time since the UNIX epoch.

      @return Current time in microseconds.
    */
    uInt64 getTicks() const;

  protected:
    // Pointer to the (currently defined) Console object
    std::unique_ptr<Console> myConsole{};
};
