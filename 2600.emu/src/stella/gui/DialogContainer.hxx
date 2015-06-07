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
// $Id: DialogContainer.hxx 3131 2015-01-01 03:49:32Z stephena $
//============================================================================

#ifndef DIALOG_CONTAINER_HXX
#define DIALOG_CONTAINER_HXX

class Dialog;
class OSystem;

#include "EventHandler.hxx"
#include "Stack.hxx"
#include "bspf.hxx"


/**
  The base class for groups of dialog boxes.  Each dialog box has a
  parent.  In most cases, the parent is itself a dialog box, but in the
  case of the lower-most dialog box, this class is its parent.
  
  This class keeps track of its children (dialog boxes), organizes them into
  a stack, and handles their events.

  @author  Stephen Anthony
  @version $Id: DialogContainer.hxx 3131 2015-01-01 03:49:32Z stephena $
*/
class DialogContainer
{
  friend class EventHandler;
  friend class Dialog;

  public:
    /**
      Create a new DialogContainer stack
    */
    DialogContainer(OSystem& osystem);

    /**
      Destructor
    */
    virtual ~DialogContainer();

  public:
    /**
      Update the dialog container with the current time.
      This is useful if we want to trigger events at some specified time.

      @param time  The current time in microseconds
    */
    void updateTime(uInt64 time);

    /**
      Handle a keyboard Unicode text event.

      @param text   Unicode character string
    */
    void handleTextEvent(char text);

    /**
      Handle a keyboard single-key event.

      @param key    Actual key symbol
      @param mod    Modifiers
      @param state  Pressed (true) or released (false)
    */
    void handleKeyEvent(StellaKey key, StellaMod mod, bool state);

    /**
      Handle a mouse motion event.

      @param x      The x location
      @param y      The y location
      @param button The currently pressed button
    */
    void handleMouseMotionEvent(int x, int y, int button);

    /**
      Handle a mouse button event.

      @param b     The mouse button
      @param x     The x location
      @param y     The y location
    */
    void handleMouseButtonEvent(MouseButton b, int x, int y);

    /**
      Handle a joystick button event.

      @param stick   The joystick number
      @param button  The joystick button
      @param state   The state (pressed or released)
    */
    void handleJoyEvent(int stick, int button, uInt8 state);

    /**
      Handle a joystick axis event.

      @param stick  The joystick number
      @param axis   The joystick axis
      @param value  Value associated with given axis
    */
    void handleJoyAxisEvent(int stick, int axis, int value);

    /**
      Handle a joystick hat event.

      @param stick  The joystick number
      @param axis   The joystick hat
      @param value  Value associated with given hat
    */
    void handleJoyHatEvent(int stick, int hat, JoyHat value);

    /**
      Draw the stack of menus (full indicates to redraw all items).
    */
    void draw(bool full = false);

    /**
      Reset dialog stack to the main configuration menu.
    */
    void reStack();

    /**
      Return the bottom-most dialog of this container.
    */
    const Dialog* baseDialog() const { return myBaseDialog; }

  private:
    void reset();

    /**
      Add a dialog box to the stack.
    */
    void addDialog(Dialog* d);

    /**
      Remove the topmost dialog box from the stack.
    */
    void removeDialog();

  protected:
    OSystem& myOSystem;
    Dialog*  myBaseDialog;
    Common::FixedStack<Dialog*> myDialogStack;

  private:
    enum {
      kDoubleClickDelay   = 500,
      kRepeatInitialDelay = 400,
      kRepeatSustainDelay = 50
    };

    // Indicates the most current time (in milliseconds) as set by updateTime()
    uInt64 myTime;

    // For continuous 'key down' events
    struct {
      StellaKey keycode;
      StellaMod flags;
    } myCurrentKeyDown;
    uInt64 myKeyRepeatTime;

    // For continuous 'mouse down' events
    struct {
      int x;
      int y;
      int button;
    } myCurrentMouseDown;
    uInt64 myClickRepeatTime;
	
    // For continuous 'joy button down' events
    struct {
      int stick;
      int button;
    } myCurrentButtonDown;
    uInt64 myButtonRepeatTime;

    // For continuous 'joy axis down' events
    struct {
      int stick;
      int axis;
      int value;
    } myCurrentAxisDown;
    uInt64 myAxisRepeatTime;

    // For continuous 'joy hat' events
    struct {
      int stick;
      int hat;
      int value;
    } myCurrentHatDown;
    uInt64 myHatRepeatTime;

    // Position and time of last mouse click (used to detect double clicks)
    struct {
      int x, y;    // Position of mouse when the click occurred
      int count;   // How often was it already pressed?
      uInt64 time; // Time
    } myLastClick;
};

#endif
