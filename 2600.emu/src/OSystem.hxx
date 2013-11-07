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

#ifndef OSYSTEM_HXX
#define OSYSTEM_HXX

class Cartridge;
class CheatManager;
class CommandMenu;
class Console;
class Debugger;
class Launcher;
class Menu;
class Properties;
class PropertiesSet;
class SerialPort;
class Settings;
class Sound;
class StateManager;
class VideoDialog;

#include <stella/common/Array.hxx>
#include <stella/emucore/FSNode.hxx>
#include "FrameBuffer.hxx"
#include "bspf.hxx"

class OSystem
{
  friend class EventHandler;
  friend class VideoDialog;

  public:
    /**
      Create a new OSystem abstract class
    */
    OSystem();

  public:
    /**
      Adds the specified settings object to the system.

      @param settings The settings object to add 
    */
    void attach(Settings* settings) { mySettings = settings; }

    /**
      Get the event handler of the system

      @return The event handler
    */
    EventHandler& eventHandler() const { return *myEventHandler; }

    /**
      Get the frame buffer of the system

      @return The frame buffer
    */
    FrameBuffer& frameBuffer() const { return *myFrameBuffer; }

    /**
      Get the sound object of the system

      @return The sound object
    */
    Sound& sound() const { return *mySound; }

    /**
      Get the settings object of the system

      @return The settings object
    */
    Settings& settings() const { return *mySettings; }

    /**
      Get the set of game properties for the system

      @return The properties set object
    */
    PropertiesSet& propSet() const { return *myPropSet; }

    /**
      Get the console of the system.

      @return The console object
    */
    Console& console() const { return *myConsole; }

    /**
      Get the serial port of the system.

      @return The serial port object
    */
    SerialPort& serialPort() const { return *mySerialPort; }

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

    /**
      Set the framerate for the video system.  It's placed in this class since
      the mainLoop() method is defined here.

      @param framerate  The video framerate to use
    */
    void setFramerate(float framerate);

    /**
      Get the maximum dimensions of a window for the video hardware.
    */
    //uInt32 desktopWidth() const  { return myDesktopWidth; }
    uInt32 desktopHeight() const { return 256; }

    /**
      Return the full/complete directory name for storing state files.
    */
    const string& stateDir() const { static const string dir("."); return dir; }

    /**
      Return the full/complete directory name for storing nvram
      (flash/EEPROM) files.
    */
    const string& nvramDir() const;// { return myNVRamDir; }

    /**
      This method should be called to get the full path of the
      (optional) palette file.

      @return String representing the full path of the properties filename.
    */
    const string& paletteFile() const { return myPaletteFile; }

    /**
      Append a message to the internal log.

      @param message  The message to be appended
      @param level    If 0, always output the message, only append when
                      level is less than or equal to that in 'loglevel'
    */
	#ifdef NDEBUG
    void logMessage(const string& message, uInt8 level) { }
	#else
    void logMessage(const string& message, uInt8 level);
	#endif

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

    /**
      This method determines the default mapping of joystick actions to
      Stella events for a specific system/platform.

      @param event  The event which to (re)set (Event::NoType resets all)
      @param mode   The mode for which the defaults are set
    */
    void setDefaultJoymap(Event::Type event, EventMode mode);

    // Pointer to the (currently defined) Console object
    Console* myConsole;

  protected:
    // Pointer to the EventHandler object
    EventHandler* myEventHandler;

    // Pointer to the FrameBuffer object
    FrameBuffer* myFrameBuffer;

    // Pointer to the Sound object
    Sound* mySound;

    // Pointer to the Settings object
    Settings* mySettings;

    // Pointer to the PropertiesSet object
    PropertiesSet* myPropSet;

    // Pointer to the serial port object
    SerialPort* mySerialPort;

  private:
    string myPaletteFile;

  private:

    // Copy constructor isn't supported by this class so make it private
    OSystem(const OSystem&) = delete;

    // Assignment operator isn't supported by this class so make it private
    OSystem& operator= (const OSystem&) = delete;
};

#endif
