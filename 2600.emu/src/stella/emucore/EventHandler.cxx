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
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: EventHandler.cxx 3308 2016-05-24 16:55:45Z stephena $
//============================================================================

#include <sstream>
#include <map>

#include "bspf.hxx"

#include "Base.hxx"
#include "CommandMenu.hxx"
#include "Console.hxx"
#include "DialogContainer.hxx"
#include "Event.hxx"
#include "FrameBuffer.hxx"
#include "FSNode.hxx"
#include "Launcher.hxx"
#include "Menu.hxx"
#include "OSystem.hxx"
#include "Joystick.hxx"
#include "Paddles.hxx"
#include "PropsSet.hxx"
#include "ListWidget.hxx"
#include "ScrollBarWidget.hxx"
#include "Settings.hxx"
#include "Sound.hxx"
#include "StateManager.hxx"
#include "Switches.hxx"
#include "M6532.hxx"
#include "MouseControl.hxx"
#include "Version.hxx"

#include "EventHandler.hxx"

#ifdef CHEATCODE_SUPPORT
  #include "Cheat.hxx"
  #include "CheatManager.hxx"
#endif
#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::EventHandler(OSystem& osystem)
  : myOSystem(osystem),
    myOverlay(nullptr),
    myState(S_NONE),
    myAllowAllDirectionsFlag(false),
    myFryingFlag(false),
    myUseCtrlKeyFlag(true),
    mySkipMouseMotion(true),
    myContSnapshotInterval(0),
    myContSnapshotCounter(0)
{
  // Erase the key mapping array
  for(int i = 0; i < KBDK_LAST; ++i)
    for(int m = 0; m < kNumModes; ++m)
      myKeyTable[i][m] = Event::NoType;

  // Erase the 'combo' array
  for(int i = 0; i < kComboSize; ++i)
    for(int j = 0; j < kEventsPerCombo; ++j)
      myComboTable[i][j] = Event::NoType;

  // Create joystick handler (to handle all joystick functionality)
  myJoyHandler = make_ptr<JoystickHandler>(osystem);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::initialize()
{
  // Make sure the event/action mappings are correctly set,
  // and fill the ActionList structure with valid values
  setKeymap();
  setComboMap();
  setActionMappings(kEmulationMode);
  setActionMappings(kMenuMode);

  myUseCtrlKeyFlag = myOSystem.settings().getBool("ctrlcombo");

  Joystick::setDeadZone(myOSystem.settings().getInt("joydeadzone"));
  Paddles::setDigitalSensitivity(myOSystem.settings().getInt("dsense"));
  Paddles::setMouseSensitivity(myOSystem.settings().getInt("msense"));

  // Set quick select delay when typing characters in listwidgets
  ListWidget::setQuickSelectDelay(myOSystem.settings().getInt("listdelay"));

  // Set number of lines a mousewheel will scroll
  ScrollBarWidget::setWheelLines(myOSystem.settings().getInt("mwheel"));

  // Integer to string conversions (for HEX) use upper or lower-case
  Common::Base::setHexUppercase(myOSystem.settings().getBool("dbg.uhex"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::reset(State state)
{
  setEventState(state);
  myOSystem.state().reset();

  setContinuousSnapshots(0);

  // Reset events almost immediately after starting emulation mode
  // We wait a little while, since 'hold' events may be present, and we want
  // time for the ROM to process them
  if(state == S_EMULATE)
    SDL_AddTimer(500, resetEventsCallback, static_cast<void*>(this));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::addJoystick(StellaJoystick* stick)
{
#ifdef JOYSTICK_SUPPORT
  if(!myJoyHandler->add(stick))
    return;

  setActionMappings(kEmulationMode);
  setActionMappings(kMenuMode);

  ostringstream buf;
  buf << "Added joystick " << stick->ID << ":" << endl
      << "  " << stick->about() << endl;
  myOSystem.logMessage(buf.str(), 1);

  // We're potentially swapping out an input device behind the back of
  // the Event system, so we make sure all Stelladaptor-generated events
  // are reset
  for(int i = 0; i < 2; ++i)
  {
    for(int j = 0; j < 2; ++j)
      myEvent.set(SA_Axis[i][j], 0);
    for(int j = 0; j < 4; ++j)
      myEvent.set(SA_Button[i][j], 0);
    for(int j = 0; j < 12; ++j)
      myEvent.set(SA_Key[i][j], 0);
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::removeJoystick(int id)
{
#ifdef JOYSTICK_SUPPORT
  myJoyHandler->remove(id);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::mapStelladaptors(const string& saport)
{
#ifdef JOYSTICK_SUPPORT
  myJoyHandler->mapStelladaptors(saport);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::toggleSAPortOrder()
{
#ifdef JOYSTICK_SUPPORT
  const string& saport = myOSystem.settings().getString("saport");
  if(saport == "lr")
  {
    mapStelladaptors("rl");
    myOSystem.frameBuffer().showMessage("Stelladaptor ports right/left");
  }
  else
  {
    mapStelladaptors("lr");
    myOSystem.frameBuffer().showMessage("Stelladaptor ports left/right");
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::poll(uInt64 time)
{
  // Process events from the underlying hardware
  pollEvent();

  // Update controllers and console switches, and in general all other things
  // related to emulation
  if(myState == S_EMULATE)
  {
    myOSystem.console().riot().update();

#if 0
    // Now check if the StateManager should be saving or loading state
    // Per-frame cheats are disabled if the StateManager is active, since
    // it would interfere with proper playback
    if(myOSystem.state().isActive())
    {
      myOSystem.state().update();
    }
    else
#endif
    {
    #ifdef CHEATCODE_SUPPORT
      for(auto& cheat: myOSystem.cheat().perFrame())
        cheat->evaluate();
    #endif

      // Handle continuous snapshots
      if(myContSnapshotInterval > 0 &&
        (++myContSnapshotCounter % myContSnapshotInterval == 0))
        takeSnapshot(uInt32(time) >> 10);  // not quite milliseconds, but close enough
    }
  }
  else if(myOverlay)
  {
    // Update the current dialog container at regular intervals
    // Used to implement continuous events
    myOverlay->updateTime(time);
  }

  // Turn off all mouse-related items; if they haven't been taken care of
  // in the previous ::update() methods, they're now invalid
  myEvent.set(Event::MouseAxisXValue, 0);
  myEvent.set(Event::MouseAxisYValue, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleTextEvent(char text)
{
  // Text events are only used in GUI mode
  if(myOverlay)
    myOverlay->handleTextEvent(text);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleKeyEvent(StellaKey key, StellaMod mod, bool state)
{
  bool handled = true;

  // Immediately store the key state
  myEvent.setKey(key, state);

  // An attempt to speed up event processing; we quickly check for
  // Control or Alt/Cmd combos first
  if(kbdAlt(mod) && state)
  {
#ifdef BSPF_MAC_OSX
    // These keys work in all states
    if(key == KBDK_Q)
    {
      handleEvent(Event::Quit, 1);
    }
    else
#endif
    if(key == KBDK_RETURN)
    {
      myOSystem.frameBuffer().toggleFullscreen();
    }
    // These only work when in emulation mode
    else if(myState == S_EMULATE)
    {
      switch(key)
      {
        case KBDK_EQUALS:
          myOSystem.frameBuffer().changeWindowedVidMode(+1);
          break;

        case KBDK_MINUS:
          myOSystem.frameBuffer().changeWindowedVidMode(-1);
          break;

        case KBDK_LEFTBRACKET:
          myOSystem.sound().adjustVolume(-1);
          break;

        case KBDK_RIGHTBRACKET:
          myOSystem.sound().adjustVolume(+1);
          break;

        case KBDK_PAGEUP:    // Alt-PageUp increases YStart
          myOSystem.console().changeYStart(+1);
          break;

        case KBDK_PAGEDOWN:  // Alt-PageDown decreases YStart
          myOSystem.console().changeYStart(-1);
          break;

        case KBDK_1:  // Alt-1 turns off NTSC filtering
          myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::PRESET_OFF);
          break;

        case KBDK_2:  // Alt-2 turns on 'composite' NTSC filtering
          myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::PRESET_COMPOSITE);
          break;

        case KBDK_3:  // Alt-3 turns on 'svideo' NTSC filtering
          myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::PRESET_SVIDEO);
          break;

        case KBDK_4:  // Alt-4 turns on 'rgb' NTSC filtering
          myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::PRESET_RGB);
          break;

        case KBDK_5:  // Alt-5 turns on 'bad' NTSC filtering
          myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::PRESET_BAD);
          break;

        case KBDK_6:  // Alt-6 turns on 'custom' NTSC filtering
          myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::PRESET_CUSTOM);
          break;

        case KBDK_7:  // Alt-7 changes scanline intensity for NTSC filtering
          if(mod & KBDM_SHIFT)
            myOSystem.frameBuffer().tiaSurface().setScanlineIntensity(-5);
          else
            myOSystem.frameBuffer().tiaSurface().setScanlineIntensity(+5);
          break;

        case KBDK_8:  // Alt-8 turns toggles scanline interpolation
          myOSystem.frameBuffer().tiaSurface().toggleScanlineInterpolation();
          break;

        case KBDK_9:  // Alt-9 selects various custom adjustables for NTSC filtering
          if(myOSystem.frameBuffer().tiaSurface().ntscEnabled())
          {
            if(mod & KBDM_SHIFT)
              myOSystem.frameBuffer().showMessage(
                  myOSystem.frameBuffer().tiaSurface().ntsc().setPreviousAdjustable());
            else
              myOSystem.frameBuffer().showMessage(
                  myOSystem.frameBuffer().tiaSurface().ntsc().setNextAdjustable());
          }
          break;

        case KBDK_0:  // Alt-0 changes custom adjustables for NTSC filtering
          if(myOSystem.frameBuffer().tiaSurface().ntscEnabled())
          {
            if(mod & KBDM_SHIFT)
              myOSystem.frameBuffer().showMessage(
                  myOSystem.frameBuffer().tiaSurface().ntsc().decreaseAdjustable());
            else
              myOSystem.frameBuffer().showMessage(
                  myOSystem.frameBuffer().tiaSurface().ntsc().increaseAdjustable());
          }
          break;

        case KBDK_Z:
          if(mod & KBDM_SHIFT)
            myOSystem.console().toggleP0Collision();
          else
            myOSystem.console().toggleP0Bit();
          break;

        case KBDK_X:
          if(mod & KBDM_SHIFT)
            myOSystem.console().toggleP1Collision();
          else
              myOSystem.console().toggleP1Bit();
          break;

        case KBDK_C:
          if(mod & KBDM_SHIFT)
            myOSystem.console().toggleM0Collision();
          else
            myOSystem.console().toggleM0Bit();
          break;

        case KBDK_V:
          if(mod & KBDM_SHIFT)
            myOSystem.console().toggleM1Collision();
          else
            myOSystem.console().toggleM1Bit();
          break;

        case KBDK_B:
          if(mod & KBDM_SHIFT)
            myOSystem.console().toggleBLCollision();
          else
            myOSystem.console().toggleBLBit();
          break;

        case KBDK_N:
          if(mod & KBDM_SHIFT)
            myOSystem.console().togglePFCollision();
          else
            myOSystem.console().togglePFBit();
          break;

        case KBDK_M:
          myOSystem.console().toggleHMOVE();
          break;

        case KBDK_COMMA:
          myOSystem.console().toggleFixedColors();
          break;

        case KBDK_PERIOD:
          if(mod & KBDM_SHIFT)
            myOSystem.console().toggleCollisions();
          else
            myOSystem.console().toggleBits();
          break;

        case KBDK_P:  // Alt-p toggles phosphor effect
          myOSystem.console().togglePhosphor();
          break;

        case KBDK_J:  // Alt-j toggles scanline jitter
          myOSystem.console().toggleJitter();
          break;

        case KBDK_L:
          myOSystem.frameBuffer().toggleFrameStats();
          break;

        case KBDK_S:
          if(myContSnapshotInterval == 0)
          {
            ostringstream buf;
            uInt32 interval = myOSystem.settings().getInt("ssinterval");
            if(mod & KBDM_SHIFT)
            {
              buf << "Enabling shotshots every frame";
              interval = 1;
            }
            else
            {
              buf << "Enabling shotshots in " << interval << " second intervals";
              interval *= uInt32(myOSystem.frameRate());
            }
            myOSystem.frameBuffer().showMessage(buf.str());
            setContinuousSnapshots(interval);
          }
          else
          {
            ostringstream buf;
            buf << "Disabling snapshots, generated "
                << (myContSnapshotCounter / myContSnapshotInterval)
                << " files";
            myOSystem.frameBuffer().showMessage(buf.str());
            setContinuousSnapshots(0);
          }
          break;

        default:
          handled = false;
          break;
      }
    }
    else
      handled = false;
  }
  else if(kbdControl(mod) && state && myUseCtrlKeyFlag)
  {
    // These keys work in all states
    if(key == KBDK_Q)
    {
      handleEvent(Event::Quit, 1);
    }
    // These only work when in emulation mode
    else if(myState == S_EMULATE)
    {
      switch(key)
      {
        case KBDK_0:  // Ctrl-0 switches between mouse control modes
          if(myMouseControl)
            myOSystem.frameBuffer().showMessage(myMouseControl->next());
          break;

        case KBDK_1:  // Ctrl-1 swaps Stelladaptor/2600-daptor ports
          toggleSAPortOrder();
          break;

        case KBDK_F:  // (Shift) Ctrl-f toggles NTSC/PAL/SECAM mode
          myOSystem.console().toggleFormat(mod & KBDM_SHIFT ? -1 : 1);
          break;

        case KBDK_G:  // Ctrl-g (un)grabs mouse
          if(!myOSystem.frameBuffer().fullScreen())
            myOSystem.frameBuffer().toggleGrabMouse();
          break;

        case KBDK_L:  // Ctrl-l toggles PAL color-loss effect
          myOSystem.console().toggleColorLoss();
          break;

        case KBDK_P:  // Ctrl-p toggles different palettes
          myOSystem.console().togglePalette();
          break;

        case KBDK_R:  // Ctrl-r reloads the currently loaded ROM
          myOSystem.reloadConsole();
          break;

        case KBDK_PAGEUP:    // Ctrl-PageUp increases Height
          myOSystem.console().changeHeight(+1);
          break;

        case KBDK_PAGEDOWN:  // Ctrl-PageDown decreases Height
          myOSystem.console().changeHeight(-1);
          break;

        case KBDK_S:         // Ctrl-s saves properties to a file
        {
          string filename = myOSystem.baseDir() +
              myOSystem.console().properties().get(Cartridge_Name) + ".pro";
          ofstream out(filename);
          if(out)
          {
            out << myOSystem.console().properties();
            myOSystem.frameBuffer().showMessage("Properties saved");
          }
          else
            myOSystem.frameBuffer().showMessage("Error saving properties");
          break;
        }

        default:
          handled = false;
          break;
      }
    }
    else
      handled = false;
  }
  else
    handled = false;

  // Don't pass the key on if we've already taken care of it
  if(handled) return;

  // Handle keys which switch eventhandler state
  // Arrange the logic to take advantage of short-circuit evaluation
  if(!(kbdControl(mod) || kbdShift(mod) || kbdAlt(mod)) &&
      !state && eventStateChange(myKeyTable[key][kEmulationMode]))
    return;

  // Otherwise, let the event handler deal with it
  switch(myState)
  {
    case S_EMULATE:
      handleEvent(myKeyTable[key][kEmulationMode], state);
      break;
    case S_PAUSE:
      if(myKeyTable[key][kEmulationMode] == Event::TakeSnapshot)
        handleEvent(myKeyTable[key][kEmulationMode], state);
      break;
    default:
      if(myOverlay)
        myOverlay->handleKeyEvent(key, mod, state);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleMouseMotionEvent(int x, int y, int xrel, int yrel, int button)
{
  // Determine which mode we're in, then send the event to the appropriate place
  if(myState == S_EMULATE)
  {
    if(!mySkipMouseMotion)
    {
      myEvent.set(Event::MouseAxisXValue, xrel);
      myEvent.set(Event::MouseAxisYValue, yrel);
    }
    mySkipMouseMotion = false;
  }
  else if(myOverlay)
    myOverlay->handleMouseMotionEvent(x, y, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleMouseButtonEvent(MouseButton b, int x, int y)
{
  // Determine which mode we're in, then send the event to the appropriate place
  if(myState == S_EMULATE)
  {
    switch(b)
    {
      case EVENT_LBUTTONDOWN:
        myEvent.set(Event::MouseButtonLeftValue, 1);
        break;
      case EVENT_LBUTTONUP:
        myEvent.set(Event::MouseButtonLeftValue, 0);
        break;
      case EVENT_RBUTTONDOWN:
        myEvent.set(Event::MouseButtonRightValue, 1);
        break;
      case EVENT_RBUTTONUP:
        myEvent.set(Event::MouseButtonRightValue, 0);
        break;
      default:
        return;
    }
  }
  else if(myOverlay)
    myOverlay->handleMouseButtonEvent(b, x, y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleJoyEvent(int stick, int button, uInt8 state)
{
  const StellaJoystick* joy = myJoyHandler->joy(stick);
  if(!joy)  return;

  // Stelladaptors handle buttons differently than regular joysticks
  switch(joy->type)
  {
    case StellaJoystick::JT_REGULAR:
      // Handle buttons which switch eventhandler state
      if(state && eventStateChange(joy->btnTable[button][kEmulationMode]))
        return;

      // Determine which mode we're in, then send the event to the appropriate place
      if(myState == S_EMULATE)
        handleEvent(joy->btnTable[button][kEmulationMode], state);
      else if(myOverlay)
        myOverlay->handleJoyEvent(stick, button, state);
      break;  // Regular button

    // These events don't have to pass through handleEvent, since
    // they can never be remapped
    case StellaJoystick::JT_STELLADAPTOR_LEFT:
    case StellaJoystick::JT_STELLADAPTOR_RIGHT:
      // The 'type-2' here refers to the fact that 'StellaJoystick::JT_STELLADAPTOR_LEFT'
      // and 'StellaJoystick::JT_STELLADAPTOR_RIGHT' are at index 2 and 3 in the JoyType
      // enum; subtracting two gives us Controller 0 and 1
      if(button < 2) myEvent.set(SA_Button[joy->type-2][button], state);
      break;  // Stelladaptor button
    case StellaJoystick::JT_2600DAPTOR_LEFT:
    case StellaJoystick::JT_2600DAPTOR_RIGHT:
      // The 'type-4' here refers to the fact that 'StellaJoystick::JT_2600DAPTOR_LEFT'
      // and 'StellaJoystick::JT_2600DAPTOR_RIGHT' are at index 4 and 5 in the JoyType
      // enum; subtracting four gives us Controller 0 and 1
      if(myState == S_EMULATE)
      {
        switch(myOSystem.console().leftController().type())
        {
          case Controller::Keyboard:
            if(button < 12) myEvent.set(SA_Key[joy->type-4][button], state);
            break;
          default:
            if(button < 4) myEvent.set(SA_Button[joy->type-4][button], state);
        }
        switch(myOSystem.console().rightController().type())
        {
          case Controller::Keyboard:
            if(button < 12) myEvent.set(SA_Key[joy->type-4][button], state);
            break;
          default:
            if(button < 4) myEvent.set(SA_Button[joy->type-4][button], state);
        }
      }
      break;  // 2600DAPTOR button
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleJoyAxisEvent(int stick, int axis, int value)
{
  const StellaJoystick* joy = myJoyHandler->joy(stick);
  if(!joy)  return;

  // Stelladaptors handle axis differently than regular joysticks
  switch(joy->type)
  {
    case StellaJoystick::JT_REGULAR:
      if(myState == S_EMULATE)
      {
        // Every axis event has two associated values, negative and positive
        Event::Type eventAxisNeg = joy->axisTable[axis][0][kEmulationMode];
        Event::Type eventAxisPos = joy->axisTable[axis][1][kEmulationMode];

        // Check for analog events, which are handled differently
        // We'll pass them off as Stelladaptor events, and let the controllers
        // handle it
        switch(int(eventAxisNeg))
        {
          case Event::PaddleZeroAnalog:
            myEvent.set(Event::SALeftAxis0Value, value);
            break;
          case Event::PaddleOneAnalog:
            myEvent.set(Event::SALeftAxis1Value, value);
            break;
          case Event::PaddleTwoAnalog:
            myEvent.set(Event::SARightAxis0Value, value);
            break;
          case Event::PaddleThreeAnalog:
            myEvent.set(Event::SARightAxis1Value, value);
            break;
          default:
          {
            // Otherwise, we know the event is digital
            if(value > Joystick::deadzone())
              handleEvent(eventAxisPos, 1);
            else if(value < -Joystick::deadzone())
              handleEvent(eventAxisNeg, 1);
            else
            {
              // Treat any deadzone value as zero
              value = 0;

              // Now filter out consecutive, similar values
              // (only pass on the event if the state has changed)
              if(joy->axisLastValue[axis] != value)
              {
                // Turn off both events, since we don't know exactly which one
                // was previously activated.
                handleEvent(eventAxisNeg, 0);
                handleEvent(eventAxisPos, 0);
              }
            }
            joy->axisLastValue[axis] = value;
            break;
          }
        }
      }
      else if(myOverlay)
      {
        // First, clamp the values to simulate digital input
        // (the only thing that the underlying code understands)
        if(value > Joystick::deadzone())
          value = 32000;
        else if(value < -Joystick::deadzone())
          value = -32000;
        else
          value = 0;

        // Now filter out consecutive, similar values
        // (only pass on the event if the state has changed)
        if(value != joy->axisLastValue[axis])
        {
          myOverlay->handleJoyAxisEvent(stick, axis, value);
          joy->axisLastValue[axis] = value;
        }
      }
      break;  // Regular joystick axis

    // Since the various controller classes deal with Stelladaptor
    // devices differently, we send the raw X and Y axis data directly,
    // and let the controller handle it
    // These events don't have to pass through handleEvent, since
    // they can never be remapped
    case StellaJoystick::JT_STELLADAPTOR_LEFT:
    case StellaJoystick::JT_STELLADAPTOR_RIGHT:
      // The 'type-2' here refers to the fact that 'StellaJoystick::JT_STELLADAPTOR_LEFT'
      // and 'StellaJoystick::JT_STELLADAPTOR_RIGHT' are at index 2 and 3 in the JoyType
      // enum; subtracting two gives us Controller 0 and 1
      if(axis < 2)
        myEvent.set(SA_Axis[joy->type-2][axis], value);
      break;  // Stelladaptor axis
    case StellaJoystick::JT_2600DAPTOR_LEFT:
    case StellaJoystick::JT_2600DAPTOR_RIGHT:
      // The 'type-4' here refers to the fact that 'StellaJoystick::JT_2600DAPTOR_LEFT'
      // and 'StellaJoystick::JT_2600DAPTOR_RIGHT' are at index 4 and 5 in the JoyType
      // enum; subtracting four gives us Controller 0 and 1
      if(axis < 2)
        myEvent.set(SA_Axis[joy->type-4][axis], value);
      break;  // 2600-daptor axis
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleJoyHatEvent(int stick, int hat, int value)
{
  const StellaJoystick* joy = myJoyHandler->joy(stick);
  if(!joy)  return;

  // Preprocess all hat events, converting to Stella JoyHat type
  // Generate multiple equivalent hat events representing combined direction
  // when we get a diagonal hat event
  if(myState == S_EMULATE)
  {
    handleEvent(joy->hatTable[hat][EVENT_HATUP][kEmulationMode],
                value & EVENT_HATUP_M);
    handleEvent(joy->hatTable[hat][EVENT_HATRIGHT][kEmulationMode],
                value & EVENT_HATRIGHT_M);
    handleEvent(joy->hatTable[hat][EVENT_HATDOWN][kEmulationMode],
                value & EVENT_HATDOWN_M);
    handleEvent(joy->hatTable[hat][EVENT_HATLEFT][kEmulationMode],
                value & EVENT_HATLEFT_M);
  }
  else if(myOverlay)
  {
    if(value == EVENT_HATCENTER_M)
      myOverlay->handleJoyHatEvent(stick, hat, EVENT_HATCENTER);
    else
    {
      if(value & EVENT_HATUP_M)
        myOverlay->handleJoyHatEvent(stick, hat, EVENT_HATUP);
      if(value & EVENT_HATRIGHT_M)
        myOverlay->handleJoyHatEvent(stick, hat, EVENT_HATRIGHT); 
      if(value & EVENT_HATDOWN_M)
        myOverlay->handleJoyHatEvent(stick, hat, EVENT_HATDOWN);
      if(value & EVENT_HATLEFT_M)
        myOverlay->handleJoyHatEvent(stick, hat, EVENT_HATLEFT);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleSystemEvent(SystemEvent e, int, int)
{
  switch(e)
  {
    case EVENT_WINDOW_EXPOSED:
        myOSystem.frameBuffer().update();
        break;
#if 0
    case EVENT_WINDOW_MINIMIZED:
        if(myState == S_EMULATE) enterMenuMode(S_MENU);
        break;
#endif
    default:  // handle other events as testing requires
      // cerr << "handleSystemEvent: " << e << endl;
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleEvent(Event::Type event, int state)
{
  // Take care of special events that aren't part of the emulation core
  // or need to be preprocessed before passing them on
  switch(event)
  {
    ////////////////////////////////////////////////////////////////////////
    // If enabled, make sure 'impossible' joystick directions aren't allowed
    case Event::JoystickZeroUp:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickZeroDown, 0);
      break;

    case Event::JoystickZeroDown:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickZeroUp, 0);
      break;

    case Event::JoystickZeroLeft:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickZeroRight, 0);
      break;

    case Event::JoystickZeroRight:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickZeroLeft, 0);
      break;

    case Event::JoystickOneUp:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickOneDown, 0);
      break;

    case Event::JoystickOneDown:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickOneUp, 0);
      break;

    case Event::JoystickOneLeft:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickOneRight, 0);
      break;

    case Event::JoystickOneRight:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickOneLeft, 0);
      break;
    ////////////////////////////////////////////////////////////////////////

    case Event::Fry:
      if(myUseCtrlKeyFlag) myFryingFlag = bool(state);
      return;

    case Event::VolumeDecrease:
      if(state) myOSystem.sound().adjustVolume(-1);
      return;

    case Event::VolumeIncrease:
      if(state) myOSystem.sound().adjustVolume(+1);
      return;

    case Event::SaveState:
      if(state) myOSystem.state().saveState();
      return;

    case Event::ChangeState:
      if(state) myOSystem.state().changeState();
      return;

    case Event::LoadState:
      if(state) myOSystem.state().loadState();
      return;

    case Event::TakeSnapshot:
      if(state) takeSnapshot();
      return;

    case Event::LauncherMode:
      if((myState == S_EMULATE || myState == S_CMDMENU ||
          myState == S_DEBUGGER) && state)
      {
        // Go back to the launcher, or immediately quit
        if(myOSystem.settings().getBool("exitlauncher") ||
           myOSystem.launcherUsed())
          myOSystem.createLauncher();
        else
          handleEvent(Event::Quit, 1);
      }
      return;

    case Event::Quit:
      if(state)
      {
        saveKeyMapping();
        saveJoyMapping();
        myOSystem.quit();
      }
      return;

    ////////////////////////////////////////////////////////////////////////
    // A combo event is simply multiple calls to handleEvent, once for
    // each event it contains
    case Event::Combo1:
    case Event::Combo2:
    case Event::Combo3:
    case Event::Combo4:
    case Event::Combo5:
    case Event::Combo6:
    case Event::Combo7:
    case Event::Combo8:
    case Event::Combo9:
    case Event::Combo10:
    case Event::Combo11:
    case Event::Combo12:
    case Event::Combo13:
    case Event::Combo14:
    case Event::Combo15:
    case Event::Combo16:
      for(int i = 0, combo = event - Event::Combo1; i < kEventsPerCombo; ++i)
        if(myComboTable[combo][i] != Event::NoType)
          handleEvent(myComboTable[combo][i], state);
      return;
    ////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////
    // Events which relate to switches()
    case Event::ConsoleColor:
      if(state)
      {
        myEvent.set(Event::ConsoleBlackWhite, 0);
        myOSystem.frameBuffer().showMessage("Color Mode");
      }
      break;
    case Event::ConsoleBlackWhite:
      if(state)
      {
        myEvent.set(Event::ConsoleColor, 0);
        myOSystem.frameBuffer().showMessage("BW Mode");
      }
      break;
    case Event::ConsoleColorToggle:
      if(state)
      {
        if(myOSystem.console().switches().tvColor())
        {
          myEvent.set(Event::ConsoleBlackWhite, 1);
          myEvent.set(Event::ConsoleColor, 0);
          myOSystem.frameBuffer().showMessage("BW Mode");
        }
        else
        {
          myEvent.set(Event::ConsoleBlackWhite, 0);
          myEvent.set(Event::ConsoleColor, 1);
          myOSystem.frameBuffer().showMessage("Color Mode");
        }
        myOSystem.console().switches().update();
      }
      return;

    case Event::ConsoleLeftDiffA:
      if(state)
      {
        myEvent.set(Event::ConsoleLeftDiffB, 0);
        myOSystem.frameBuffer().showMessage("Left Difficulty A");
      }
      break;
    case Event::ConsoleLeftDiffB:
      if(state)
      {
        myEvent.set(Event::ConsoleLeftDiffA, 0);
        myOSystem.frameBuffer().showMessage("Left Difficulty B");
      }
      break;
    case Event::ConsoleLeftDiffToggle:
      if(state)
      {
        if(myOSystem.console().switches().leftDifficultyA())
        {
          myEvent.set(Event::ConsoleLeftDiffA, 0);
          myEvent.set(Event::ConsoleLeftDiffB, 1);
          myOSystem.frameBuffer().showMessage("Left Difficulty B");
        }
        else
        {
          myEvent.set(Event::ConsoleLeftDiffA, 1);
          myEvent.set(Event::ConsoleLeftDiffB, 0);
          myOSystem.frameBuffer().showMessage("Left Difficulty A");
        }
        myOSystem.console().switches().update();
      }
      return;

    case Event::ConsoleRightDiffA:
      if(state)
      {
        myEvent.set(Event::ConsoleRightDiffB, 0);
        myOSystem.frameBuffer().showMessage("Right Difficulty A");
      }
      break;
    case Event::ConsoleRightDiffB:
      if(state)
      {
        myEvent.set(Event::ConsoleRightDiffA, 0);
        myOSystem.frameBuffer().showMessage("Right Difficulty B");
      }
      break;
    case Event::ConsoleRightDiffToggle:
      if(state)
      {
        if(myOSystem.console().switches().rightDifficultyA())
        {
          myEvent.set(Event::ConsoleRightDiffA, 0);
          myEvent.set(Event::ConsoleRightDiffB, 1);
          myOSystem.frameBuffer().showMessage("Right Difficulty B");
        }
        else
        {
          myEvent.set(Event::ConsoleRightDiffA, 1);
          myEvent.set(Event::ConsoleRightDiffB, 0);
          myOSystem.frameBuffer().showMessage("Right Difficulty A");
        }
        myOSystem.console().switches().update();
      }
      return;
    ////////////////////////////////////////////////////////////////////////

    case Event::NoType:  // Ignore unmapped events
      return;

    default:
      break;
  }

  // Otherwise, pass it to the emulation core
  myEvent.set(event, state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::eventStateChange(Event::Type type)
{
  bool handled = true;

  switch(type)
  {
    case Event::PauseMode:
      if(myState == S_EMULATE)
        setEventState(S_PAUSE);
      else if(myState == S_PAUSE)
        setEventState(S_EMULATE);
      else
        handled = false;
      break;

    case Event::MenuMode:
      if(myState == S_EMULATE)
        enterMenuMode(S_MENU);
      else
        handled = false;
      break;

    case Event::CmdMenuMode:
      if(myState == S_EMULATE)
        enterMenuMode(S_CMDMENU);
      else if(myState == S_CMDMENU)
        leaveMenuMode();
      else
        handled = false;
      break;

    case Event::DebuggerMode:
      if(myState == S_EMULATE)
        enterDebugMode();
      else if(myState == S_DEBUGGER)
        leaveDebugMode();
      else
        handled = false;
      break;

    default:
      handled = false;
  }

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setActionMappings(EventMode mode)
{
  int listsize = 0;
  ActionList* list = nullptr;

  switch(mode)
  {
    case kEmulationMode:
      listsize = kEmulActionListSize;
      list     = ourEmulActionList;
      break;
    case kMenuMode:
      listsize = kMenuActionListSize;
      list     = ourMenuActionList;
      break;
    default:
      return;
  }

  ostringstream buf;

  // Fill the ActionList with the current key and joystick mappings
  for(int i = 0; i < listsize; ++i)
  {
    Event::Type event = list[i].event;
    list[i].key = "None";
    string key = "";
    for(int j = 0; j < KBDK_LAST; ++j)   // key mapping
    {
      if(myKeyTable[j][mode] == event)
      {
        if(key == "")
          key = key + nameForKey(StellaKey(j));
        else
          key = key + ", " + nameForKey(StellaKey(j));
      }
    }

#ifdef JOYSTICK_SUPPORT
    for(const auto& st: myJoyHandler->sticks())
    {
      uInt32 stick = st.first;
      const StellaJoystick* joy = st.second;
      if(!joy)  continue;

      // Joystick button mapping/labeling
      for(int button = 0; button < joy->numButtons; ++button)
      {
        if(joy->btnTable[button][mode] == event)
        {
          buf.str("");
          buf << "J" << stick << "/B" << button;
          if(key == "")
            key = key + buf.str();
          else
            key = key + ", " + buf.str();
        }
      }

      // Joystick axis mapping/labeling
      for(int axis = 0; axis < joy->numAxes; ++axis)
      {
        for(int dir = 0; dir < 2; ++dir)
        {
          if(joy->axisTable[axis][dir][mode] == event)
          {
            buf.str("");
            buf << "J" << stick << "/A" << axis;
            if(eventIsAnalog(event))
            {
              dir = 2;  // Immediately exit the inner loop after this iteration
              buf << "/+|-";
            }
            else if(dir == 0)
              buf << "/-";
            else
              buf << "/+";

            if(key == "")
              key = key + buf.str();
            else
              key = key + ", " + buf.str();
          }
        }
      }

      // Joystick hat mapping/labeling
      for(int hat = 0; hat < joy->numHats; ++hat)
      {
        for(int dir = 0; dir < 4; ++dir)
        {
          if(joy->hatTable[hat][dir][mode] == event)
          {
            buf.str("");
            buf << "J" << stick << "/H" << hat;
            switch(dir)
            {
              case EVENT_HATUP:    buf << "/up";    break;
              case EVENT_HATDOWN:  buf << "/down";  break;
              case EVENT_HATLEFT:  buf << "/left";  break;
              case EVENT_HATRIGHT: buf << "/right"; break;
            }
            if(key == "")
              key = key + buf.str();
            else
              key = key + ", " + buf.str();
          }
        }
      }
    }
#endif

    // There are some keys which are hardcoded.  These should be represented too.
    string prepend = "";
    if(event == Event::Quit)
#ifndef BSPF_MAC_OSX
      prepend = "Ctrl Q";
#else
      prepend = "Cmd Q";
#endif
    else if(event == Event::UINavNext)
      prepend = "TAB";
    else if(event == Event::UINavPrev)
      prepend = "Shift-TAB";
    // else if ...

    if(key == "")
      key = prepend;
    else if(prepend != "")
      key = prepend + ", " + key;

    if(key != "")
      list[i].key = key;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setKeymap()
{
  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  string list = myOSystem.settings().getString("keymap");
  replace(list.begin(), list.end(), ':', ' ');
  istringstream buf(list);

  IntArray map;
  int value;
  Event::Type event;

  // Get event count, which should be the first int in the list
  buf >> value;
  event = Event::Type(value);
  if(event == Event::LastType)
    while(buf >> value)
      map.push_back(value);

  // Only fill the key mapping array if the data is valid
  if(event == Event::LastType && map.size() == KBDK_LAST * kNumModes)
  {
    // Fill the keymap table with events
    auto e = map.begin();
    for(int mode = 0; mode < kNumModes; ++mode)
      for(int i = 0; i < KBDK_LAST; ++i)
        myKeyTable[i][mode] = Event::Type(*e++);
  }
  else
  {
    setDefaultKeymap(Event::NoType, kEmulationMode);
    setDefaultKeymap(Event::NoType, kMenuMode);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setComboMap()
{
  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  string list = myOSystem.settings().getString("combomap");
  replace(list.begin(), list.end(), ':', ' ');
  istringstream buf(list);

  // Get combo count, which should be the first int in the list
  // If it isn't, then we treat the entire list as invalid
  string key;
  buf >> key;
  if(atoi(key.c_str()) == kComboSize)
  {
    // Fill the combomap table with events for as long as they exist
    int combocount = 0;
    while(buf >> key && combocount < kComboSize)
    {
      // Each event in a comboevent is separated by a comma
      replace(key.begin(), key.end(), ',', ' ');
      istringstream buf2(key);

      int eventcount = 0;
      while(buf2 >> key && eventcount < kEventsPerCombo)
      {
        myComboTable[combocount][eventcount] = Event::Type(atoi(key.c_str()));
        ++eventcount;
      }
      ++combocount;
    }
  }
  else
  {
    // Erase the 'combo' array
    for(int i = 0; i < kComboSize; ++i)
      for(int j = 0; j < kEventsPerCombo; ++j)
        myComboTable[i][j] = Event::NoType;
  }

  saveComboMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VariantList EventHandler::joystickDatabase() const
{
  VariantList db;
  for(const auto& i: myJoyHandler->database())
    VarList::push_back(db, i.first, i.second.joy ? i.second.joy->ID : -1);

  return db;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::removeJoystickFromDatabase(const string& name)
{
  myJoyHandler->remove(name);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addKeyMapping(Event::Type event, EventMode mode, StellaKey key)
{
  // These keys cannot be remapped
  if(key == KBDK_TAB || eventIsAnalog(event))
    return false;
  else
  {
    myKeyTable[key][mode] = event;
    setActionMappings(mode);

    return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addJoyAxisMapping(Event::Type event, EventMode mode,
                                     int stick, int axis, int value,
                                     bool updateMenus)
{
#ifdef JOYSTICK_SUPPORT
  const StellaJoystick* joy = myJoyHandler->joy(stick);
  if(joy)
  {
    if(axis >= 0 && axis < joy->numAxes &&
       event >= 0 && event < Event::LastType)
    {
      // This confusing code is because each axis has two associated values,
      // but analog events only affect one of the axis.
      if(eventIsAnalog(event))
        joy->axisTable[axis][0][mode] =
          joy->axisTable[axis][1][mode] = event;
      else
      {
        // Otherwise, turn off the analog event(s) for this axis
        if(eventIsAnalog(joy->axisTable[axis][0][mode]))
          joy->axisTable[axis][0][mode] = Event::NoType;
        if(eventIsAnalog(joy->axisTable[axis][1][mode]))
          joy->axisTable[axis][1][mode] = Event::NoType;
    
        joy->axisTable[axis][(value > 0)][mode] = event;
      }
      if(updateMenus)
        setActionMappings(mode);
      return true;
    }
  }
#endif
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addJoyButtonMapping(Event::Type event, EventMode mode,
                                       int stick, int button,
                                       bool updateMenus)
{
#ifdef JOYSTICK_SUPPORT
  const StellaJoystick* joy = myJoyHandler->joy(stick);
  if(joy)
  {
    if(button >= 0 && button < joy->numButtons &&
       event >= 0 && event < Event::LastType)
    {
      joy->btnTable[button][mode] = event;
      if(updateMenus)
        setActionMappings(mode);
      return true;
    }
  }
#endif
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addJoyHatMapping(Event::Type event, EventMode mode,
                                    int stick, int hat, int value,
                                    bool updateMenus)
{
#ifdef JOYSTICK_SUPPORT
  const StellaJoystick* joy = myJoyHandler->joy(stick);
  if(joy)
  {
    if(hat >= 0 && hat < joy->numHats &&
       event >= 0 && event < Event::LastType && value != EVENT_HATCENTER)
    {
      joy->hatTable[hat][value][mode] = event;
      if(updateMenus)
        setActionMappings(mode);
      return true;
    }
  }
#endif
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::eraseMapping(Event::Type event, EventMode mode)
{
  // Erase the KeyEvent arrays
  for(int i = 0; i < KBDK_LAST; ++i)
    if(myKeyTable[i][mode] == event && i != KBDK_TAB)
      myKeyTable[i][mode] = Event::NoType;

#ifdef JOYSTICK_SUPPORT
  // Erase the joystick mapping arrays
  myJoyHandler->eraseMapping(event, mode);
#endif

  setActionMappings(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultMapping(Event::Type event, EventMode mode)
{
  setDefaultKeymap(event, mode);
  setDefaultJoymap(event, mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultKeymap(Event::Type event, EventMode mode)
{
  // If event is 'NoType', erase and reset all mappings
  // Otherwise, only reset the given event
  bool eraseAll = (event == Event::NoType);
  if(eraseAll)
  {
    // Erase all mappings
    for(int i = 0; i < KBDK_LAST; ++i)
      myKeyTable[i][mode] = Event::NoType;
  }

  auto setDefaultKey = [&](StellaKey key, Event::Type k_event)
  {
    if(eraseAll || k_event == event)
      myKeyTable[key][mode] = k_event;
  };

  switch(mode)
  {
    case kEmulationMode:
      setDefaultKey( KBDK_1,         Event::KeyboardZero1     );
      setDefaultKey( KBDK_2,         Event::KeyboardZero2     );
      setDefaultKey( KBDK_3,         Event::KeyboardZero3     );
      setDefaultKey( KBDK_Q,         Event::KeyboardZero4     );
      setDefaultKey( KBDK_W,         Event::KeyboardZero5     );
      setDefaultKey( KBDK_E,         Event::KeyboardZero6     );
      setDefaultKey( KBDK_A,         Event::KeyboardZero7     );
      setDefaultKey( KBDK_S,         Event::KeyboardZero8     );
      setDefaultKey( KBDK_D,         Event::KeyboardZero9     );
      setDefaultKey( KBDK_Z,         Event::KeyboardZeroStar  );
      setDefaultKey( KBDK_X,         Event::KeyboardZero0     );
      setDefaultKey( KBDK_C,         Event::KeyboardZeroPound );

      setDefaultKey( KBDK_8,         Event::KeyboardOne1      );
      setDefaultKey( KBDK_9,         Event::KeyboardOne2      );
      setDefaultKey( KBDK_0,         Event::KeyboardOne3      );
      setDefaultKey( KBDK_I,         Event::KeyboardOne4      );
      setDefaultKey( KBDK_O,         Event::KeyboardOne5      );
      setDefaultKey( KBDK_P,         Event::KeyboardOne6      );
      setDefaultKey( KBDK_K,         Event::KeyboardOne7      );
      setDefaultKey( KBDK_L,         Event::KeyboardOne8      );
      setDefaultKey( KBDK_SEMICOLON, Event::KeyboardOne9      );
      setDefaultKey( KBDK_COMMA,     Event::KeyboardOneStar   );
      setDefaultKey( KBDK_PERIOD,    Event::KeyboardOne0      );
      setDefaultKey( KBDK_SLASH,     Event::KeyboardOnePound  );

      setDefaultKey( KBDK_UP,        Event::JoystickZeroUp    );
      setDefaultKey( KBDK_DOWN,      Event::JoystickZeroDown  );
      setDefaultKey( KBDK_LEFT,      Event::JoystickZeroLeft  );
      setDefaultKey( KBDK_RIGHT,     Event::JoystickZeroRight );
      setDefaultKey( KBDK_SPACE,     Event::JoystickZeroFire  );
      setDefaultKey( KBDK_LCTRL,     Event::JoystickZeroFire  );
      setDefaultKey( KBDK_4,         Event::JoystickZeroFire5 );
      setDefaultKey( KBDK_5,         Event::JoystickZeroFire9 );

      setDefaultKey( KBDK_Y,         Event::JoystickOneUp     );
      setDefaultKey( KBDK_H,         Event::JoystickOneDown   );
      setDefaultKey( KBDK_G,         Event::JoystickOneLeft   );
      setDefaultKey( KBDK_J,         Event::JoystickOneRight  );
      setDefaultKey( KBDK_F,         Event::JoystickOneFire   );
      setDefaultKey( KBDK_6,         Event::JoystickOneFire5  );
      setDefaultKey( KBDK_7,         Event::JoystickOneFire9  );


      setDefaultKey( KBDK_F1,        Event::ConsoleSelect     );
      setDefaultKey( KBDK_F2,        Event::ConsoleReset      );
      setDefaultKey( KBDK_F3,        Event::ConsoleColor      );
      setDefaultKey( KBDK_F4,        Event::ConsoleBlackWhite );
      setDefaultKey( KBDK_F5,        Event::ConsoleLeftDiffA  );
      setDefaultKey( KBDK_F6,        Event::ConsoleLeftDiffB  );
      setDefaultKey( KBDK_F7,        Event::ConsoleRightDiffA );
      setDefaultKey( KBDK_F8,        Event::ConsoleRightDiffB );
      setDefaultKey( KBDK_F9,        Event::SaveState         );
      setDefaultKey( KBDK_F10,       Event::ChangeState       );
      setDefaultKey( KBDK_F11,       Event::LoadState         );
      setDefaultKey( KBDK_F12,       Event::TakeSnapshot      );
      setDefaultKey( KBDK_BACKSPACE, Event::Fry               );
      setDefaultKey( KBDK_PAUSE,     Event::PauseMode         );
      setDefaultKey( KBDK_TAB,       Event::MenuMode          );
      setDefaultKey( KBDK_BACKSLASH, Event::CmdMenuMode       );
      setDefaultKey( KBDK_GRAVE,     Event::DebuggerMode      );
      setDefaultKey( KBDK_ESCAPE,    Event::LauncherMode      );
      break;

    case kMenuMode:
      setDefaultKey( KBDK_UP,        Event::UIUp      );
      setDefaultKey( KBDK_DOWN,      Event::UIDown    );
      setDefaultKey( KBDK_LEFT,      Event::UILeft    );
      setDefaultKey( KBDK_RIGHT,     Event::UIRight   );

      setDefaultKey( KBDK_HOME,      Event::UIHome    );
      setDefaultKey( KBDK_END,       Event::UIEnd     );
      setDefaultKey( KBDK_PAGEUP,    Event::UIPgUp    );
      setDefaultKey( KBDK_PAGEDOWN,  Event::UIPgDown  );

      setDefaultKey( KBDK_RETURN,    Event::UISelect  );
      setDefaultKey( KBDK_ESCAPE,    Event::UICancel  );

      setDefaultKey( KBDK_BACKSPACE, Event::UIPrevDir );
      break;

    default:
      return;
  }
  setActionMappings(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoymap(Event::Type event, EventMode mode)
{
  myJoyHandler->setDefaultMapping(event, mode);
  setActionMappings(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveKeyMapping()
{
  // Iterate through the keymap table and create a colon-separated list
  // Prepend the event count, so we can check it on next load
  ostringstream keybuf;
  keybuf << Event::LastType;
  for(int mode = 0; mode < kNumModes; ++mode)
    for(int i = 0; i < KBDK_LAST; ++i)
      keybuf << ":" << myKeyTable[i][mode];

  myOSystem.settings().setValue("keymap", keybuf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveJoyMapping()
{
  myJoyHandler->saveMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveComboMapping()
{
  // Iterate through the combomap table and create a colon-separated list
  // For each combo event, create a comma-separated list of its events
  // Prepend the event count, so we can check it on next load
  ostringstream buf;
  buf << kComboSize;
  for(int i = 0; i < kComboSize; ++i)
  {
    buf << ":" << myComboTable[i][0];
    for(int j = 1; j < kEventsPerCombo; ++j)
      buf << "," << myComboTable[i][j];
  }
  myOSystem.settings().setValue("combomap", buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool EventHandler::eventIsAnalog(Event::Type event) const
{
  switch(event)
  {
    case Event::PaddleZeroAnalog:
    case Event::PaddleOneAnalog:
    case Event::PaddleTwoAnalog:
    case Event::PaddleThreeAnalog:
      return true;
    default:
      return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StringList EventHandler::getActionList(EventMode mode) const
{
  StringList l;
  switch(mode)
  {
    case kEmulationMode:
      for(uInt32 i = 0; i < kEmulActionListSize; ++i)
        l.push_back(EventHandler::ourEmulActionList[i].action);
      break;
    case kMenuMode:
      for(uInt32 i = 0; i < kMenuActionListSize; ++i)
        l.push_back(EventHandler::ourMenuActionList[i].action);
      break;
    default:
      break;
  }
  return l;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VariantList EventHandler::getComboList(EventMode /**/) const
{
  // For now, this only works in emulation mode
  VariantList l;
  ostringstream buf;

  VarList::push_back(l, "None", "-1");
  for(uInt32 i = 0; i < kEmulActionListSize; ++i)
  {
    if(EventHandler::ourEmulActionList[i].allow_combo)
    {
      buf << i;
      VarList::push_back(l, EventHandler::ourEmulActionList[i].action, buf.str());
      buf.str("");
    }
  }
  return l;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StringList EventHandler::getComboListForEvent(Event::Type event) const
{
  StringList l;
  ostringstream buf;
  if(event >= Event::Combo1 && event <= Event::Combo16)
  {
    int combo = event - Event::Combo1;
    for(uInt32 i = 0; i < kEventsPerCombo; ++i)
    {
      Event::Type e = myComboTable[combo][i];
      for(uInt32 j = 0; j < kEmulActionListSize; ++j)
      {
        if(EventHandler::ourEmulActionList[j].event == e &&
           EventHandler::ourEmulActionList[j].allow_combo)
        {
          buf << j;
          l.push_back(buf.str());
          buf.str("");
        }
      }
      // Make sure entries are 1-to-1, using '-1' to indicate Event::NoType
      if(i == l.size())
        l.push_back("-1");
    }
  }
  return l;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setComboListForEvent(Event::Type event, const StringList& events)
{
  if(event >= Event::Combo1 && event <= Event::Combo16)
  {
    assert(events.size() == 8);
    int combo = event - Event::Combo1;
    for(int i = 0; i < 8; ++i)
    {
      int idx = atoi(events[i].c_str());
      if(idx >=0 && idx < kEmulActionListSize)
        myComboTable[combo][i] = EventHandler::ourEmulActionList[idx].event;
      else
        myComboTable[combo][i] = Event::NoType;
    }
    saveComboMapping();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type EventHandler::eventAtIndex(int idx, EventMode mode) const
{
  switch(mode)
  {
    case kEmulationMode:
      if(idx < 0 || idx >= kEmulActionListSize)
        return Event::NoType;
      else
        return ourEmulActionList[idx].event;
    case kMenuMode:
      if(idx < 0 || idx >= kMenuActionListSize)
        return Event::NoType;
      else
        return ourMenuActionList[idx].event;
    default:
      return Event::NoType;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EventHandler::actionAtIndex(int idx, EventMode mode) const
{
  switch(mode)
  {
    case kEmulationMode:
      if(idx < 0 || idx >= kEmulActionListSize)
        return EmptyString;
      else
        return ourEmulActionList[idx].action;
    case kMenuMode:
      if(idx < 0 || idx >= kMenuActionListSize)
        return EmptyString;
      else
        return ourMenuActionList[idx].action;
    default:
      return EmptyString;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EventHandler::keyAtIndex(int idx, EventMode mode) const
{
  switch(mode)
  {
    case kEmulationMode:
      if(idx < 0 || idx >= kEmulActionListSize)
        return EmptyString;
      else
        return ourEmulActionList[idx].key;
    case kMenuMode:
      if(idx < 0 || idx >= kMenuActionListSize)
        return EmptyString;
      else
        return ourMenuActionList[idx].key;
    default:
      return EmptyString;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::takeSnapshot(uInt32 number)
{
  // Figure out the correct snapshot name
  string filename;
  bool showmessage = number == 0;
  string sspath = myOSystem.snapshotSaveDir() +
      (myOSystem.settings().getString("snapname") != "int" ?
          myOSystem.romFile().getNameWithExt("")
        : myOSystem.console().properties().get(Cartridge_Name));

  // Check whether we want multiple snapshots created
  if(number > 0)
  {
    ostringstream buf;
    buf << sspath << "_" << std::hex << std::setw(8) << std::setfill('0')
        << number << ".png";
    filename = buf.str();
  }
  else if(!myOSystem.settings().getBool("sssingle"))
  {
    // Determine if the file already exists, checking each successive filename
    // until one doesn't exist
    filename = sspath + ".png";
    FilesystemNode node(filename);
    if(node.exists())
    {
      ostringstream buf;
      for(uInt32 i = 1; ;++i)
      {
        buf.str("");
        buf << sspath << "_" << i << ".png";
        FilesystemNode next(buf.str());
        if(!next.exists())
          break;
      }
      filename = buf.str();
    }
  }
  else
    filename = sspath + ".png";

  // Some text fields to add to the PNG snapshot
  VariantList comments;
  ostringstream version;
  version << "Stella " << STELLA_VERSION << " (Build " << STELLA_BUILD << ") ["
          << BSPF::ARCH << "]";
  VarList::push_back(comments, "Software", version.str());
  VarList::push_back(comments, "ROM Name", myOSystem.console().properties().get(Cartridge_Name));
  VarList::push_back(comments, "ROM MD5", myOSystem.console().properties().get(Cartridge_MD5));
  VarList::push_back(comments, "TV Effects", myOSystem.frameBuffer().tiaSurface().effectsInfo());

  // Now create a PNG snapshot
  if(myOSystem.settings().getBool("ss1x"))
  {
    string message = "Snapshot saved";
    try
    {
      GUI::Rect rect;
      const FBSurface& surface = myOSystem.frameBuffer().tiaSurface().baseSurface(rect);
      myOSystem.png().saveImage(filename, surface, rect, comments);
    }
    catch(const runtime_error& e)
    {
      message = e.what();
    }
    if(showmessage)
      myOSystem.frameBuffer().showMessage(message);
  }
  else
  {
    // Make sure we have a 'clean' image, with no onscreen messages
    myOSystem.frameBuffer().enableMessages(false);
    myOSystem.frameBuffer().tiaSurface().render();

    string message = "Snapshot saved";
    try
    {
      myOSystem.png().saveImage(filename, comments);
    }
    catch(const runtime_error& e)
    {
      message = e.what();
    }

    // Re-enable old messages
    myOSystem.frameBuffer().enableMessages(true);
    if(showmessage)
      myOSystem.frameBuffer().showMessage(message);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setMouseControllerMode(const string& enable)
{
  if(myOSystem.hasConsole())
  {
    bool usemouse = false;
    if(BSPF::equalsIgnoreCase(enable, "always"))
      usemouse = true;
    else if(BSPF::equalsIgnoreCase(enable, "never"))
      usemouse = false;
    else  // 'analog'
    {
      switch(myOSystem.console().leftController().type())
      {
        case Controller::Paddles:
        case Controller::Driving:
        case Controller::TrackBall22:
        case Controller::TrackBall80:
        case Controller::AmigaMouse:
        case Controller::MindLink:
          usemouse = true;
          break;
        default:
          break;
      }
      switch(myOSystem.console().rightController().type())
      {
        case Controller::Paddles:
        case Controller::Driving:
        case Controller::TrackBall22:
        case Controller::TrackBall80:
        case Controller::AmigaMouse:
        case Controller::MindLink:
          usemouse = true;
          break;
        default:
          break;
      }
    }

    const string& control = usemouse ?
      myOSystem.console().properties().get(Controller_MouseAxis) : "none";

    myMouseControl = make_ptr<MouseControl>(myOSystem.console(), control);
    myMouseControl->next();  // set first available mode
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setContinuousSnapshots(uInt32 interval)
{
  myContSnapshotInterval = interval;
  myContSnapshotCounter = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::enterMenuMode(State state)
{
  setEventState(state);
  myOverlay->reStack();
  myOSystem.sound().mute(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::leaveMenuMode()
{
  setEventState(S_EMULATE);
  myOSystem.sound().mute(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::enterDebugMode()
{
#ifdef DEBUGGER_SUPPORT
  if(myState == S_DEBUGGER || !myOSystem.hasConsole())
    return false;

  // Make sure debugger starts in a consistent state
  // This absolutely *has* to come before we actually change to debugger
  // mode, since it takes care of locking the debugger state, which will
  // probably be modified below
  myOSystem.debugger().setStartState();
  setEventState(S_DEBUGGER);

  FBInitStatus fbstatus = myOSystem.createFrameBuffer();
  if(fbstatus != kSuccess)
  {
    myOSystem.debugger().setQuitState();
    setEventState(S_EMULATE);
    if(fbstatus == kFailTooLarge)
      myOSystem.frameBuffer().showMessage("Debugger window too large for screen",
                                           kBottomCenter, true);
    return false;
  }
  myOverlay->reStack();
  myOSystem.sound().mute(true);
#else
  myOSystem.frameBuffer().showMessage("Debugger support not included",
                                       kBottomCenter, true);
#endif

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::leaveDebugMode()
{
#ifdef DEBUGGER_SUPPORT
  // paranoia: this should never happen:
  if(myState != S_DEBUGGER)
    return;

  // Make sure debugger quits in a consistent state
  myOSystem.debugger().setQuitState();

  setEventState(S_EMULATE);
  myOSystem.createFrameBuffer();
  myOSystem.sound().mute(false);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setEventState(State state)
{
  myState = state;

  // Normally, the usage of Control key is determined by 'ctrlcombo'
  // For certain ROMs it may be forced off, whatever the setting
  myUseCtrlKeyFlag = myOSystem.settings().getBool("ctrlcombo");

  // Only enable text input in GUI modes, since in emulation mode the
  // keyboard acts as one large joystick with many (single) buttons
  switch(myState)
  {
    case S_EMULATE:
      myOverlay = nullptr;
      myOSystem.sound().mute(false);
      enableTextEvents(false);
      if(myOSystem.console().leftController().type() == Controller::CompuMate)
        myUseCtrlKeyFlag = false;
      break;

    case S_PAUSE:
      myOverlay = nullptr;
      myOSystem.sound().mute(true);
      enableTextEvents(false);
      break;

    case S_MENU:
      myOverlay = &myOSystem.menu();
      enableTextEvents(true);
      break;

    case S_CMDMENU:
      myOverlay = &myOSystem.commandMenu();
      enableTextEvents(true);
      break;

    case S_LAUNCHER:
      myOverlay = &myOSystem.launcher();
      enableTextEvents(true);
      break;

#ifdef DEBUGGER_SUPPORT
    case S_DEBUGGER:
      myOverlay = &myOSystem.debugger();
      enableTextEvents(true);
      break;
#endif

    default:
      myOverlay = nullptr;
      break;
  }

  // Inform various subsystems about the new state
  myOSystem.stateChanged(myState);
  myOSystem.frameBuffer().stateChanged(myState);
  myOSystem.frameBuffer().setCursorState();
  if(myOSystem.hasConsole())
    myOSystem.console().stateChanged(myState);

  // Always clear any pending events when changing states
  myEvent.clear();

  // Sometimes an extraneous mouse motion event is generated
  // after a state change, which should be supressed
  mySkipMouseMotion = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 EventHandler::resetEventsCallback(uInt32 interval, void* param)
{
  (static_cast<EventHandler*>(param))->myEvent.clear();
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::ActionList EventHandler::ourEmulActionList[kEmulActionListSize] = {
  { Event::ConsoleSelect,          "Select",                   "", true  },
  { Event::ConsoleReset,           "Reset",                    "", true  },
  { Event::ConsoleColor,           "Color TV",                 "", true  },
  { Event::ConsoleBlackWhite,      "Black & White TV",         "", true  },
  { Event::ConsoleColorToggle,     "Swap Color / B&W TV",      "", true  },
  { Event::ConsoleLeftDiffA,       "P0 Difficulty A",          "", true  },
  { Event::ConsoleLeftDiffB,       "P0 Difficulty B",          "", true  },
  { Event::ConsoleLeftDiffToggle,  "P0 Swap Difficulty",       "", true  },
  { Event::ConsoleRightDiffA,      "P1 Difficulty A",          "", true  },
  { Event::ConsoleRightDiffB,      "P1 Difficulty B",          "", true  },
  { Event::ConsoleRightDiffToggle, "P1 Swap Difficulty",       "", true  },
  { Event::SaveState,              "Save State",               "", false },
  { Event::ChangeState,            "Change State",             "", false },
  { Event::LoadState,              "Load State",               "", false },
  { Event::TakeSnapshot,           "Snapshot",                 "", false },
  { Event::Fry,                    "Fry cartridge",            "", false },
  { Event::VolumeDecrease,         "Decrease volume",          "", false },
  { Event::VolumeIncrease,         "Increase volume",          "", false },
  { Event::PauseMode,              "Pause",                    "", false },
  { Event::MenuMode,               "Enter options menu mode",  "", false },
  { Event::CmdMenuMode,            "Toggle command menu mode", "", false },
  { Event::DebuggerMode,           "Toggle debugger mode",     "", false },
  { Event::LauncherMode,           "Enter ROM launcher",       "", false },
  { Event::Quit,                   "Quit",                     "", false },

  { Event::JoystickZeroUp,      "P0 Joystick Up",              "", true  },
  { Event::JoystickZeroDown,    "P0 Joystick Down",            "", true  },
  { Event::JoystickZeroLeft,    "P0 Joystick Left",            "", true  },
  { Event::JoystickZeroRight,   "P0 Joystick Right",           "", true  },
  { Event::JoystickZeroFire,    "P0 Joystick Fire",            "", true  },
  { Event::JoystickZeroFire5,   "P0 Booster Top Trigger",      "", true  },
  { Event::JoystickZeroFire9,   "P0 Booster Handle Grip",      "", true  },

  { Event::JoystickOneUp,       "P1 Joystick Up",              "", true  },
  { Event::JoystickOneDown,     "P1 Joystick Down",            "", true  },
  { Event::JoystickOneLeft,     "P1 Joystick Left",            "", true  },
  { Event::JoystickOneRight,    "P1 Joystick Right",           "", true  },
  { Event::JoystickOneFire,     "P1 Joystick Fire",            "", true  },
  { Event::JoystickOneFire5,    "P1 Booster Top Trigger",      "", true  },
  { Event::JoystickOneFire9,    "P1 Booster Handle Grip",      "", true  },

  { Event::PaddleZeroAnalog,    "Paddle 0 Analog",             "", true  },
  { Event::PaddleZeroDecrease,  "Paddle 0 Decrease",           "", true  },
  { Event::PaddleZeroIncrease,  "Paddle 0 Increase",           "", true  },
  { Event::PaddleZeroFire,      "Paddle 0 Fire",               "", true  },

  { Event::PaddleOneAnalog,     "Paddle 1 Analog",             "", true  },
  { Event::PaddleOneDecrease,   "Paddle 1 Decrease",           "", true  },
  { Event::PaddleOneIncrease,   "Paddle 1 Increase",           "", true  },
  { Event::PaddleOneFire,       "Paddle 1 Fire",               "", true  },

  { Event::PaddleTwoAnalog,     "Paddle 2 Analog",             "", true  },
  { Event::PaddleTwoDecrease,   "Paddle 2 Decrease",           "", true  },
  { Event::PaddleTwoIncrease,   "Paddle 2 Increase",           "", true  },
  { Event::PaddleTwoFire,       "Paddle 2 Fire",               "", true  },

  { Event::PaddleThreeAnalog,   "Paddle 3 Analog",             "", true  },
  { Event::PaddleThreeDecrease, "Paddle 3 Decrease",           "", true  },
  { Event::PaddleThreeIncrease, "Paddle 3 Increase",           "", true  },
  { Event::PaddleThreeFire,     "Paddle 3 Fire",               "", true  },

  { Event::KeyboardZero1,       "P0 Keyboard 1",               "", true  },
  { Event::KeyboardZero2,       "P0 Keyboard 2",               "", true  },
  { Event::KeyboardZero3,       "P0 Keyboard 3",               "", true  },
  { Event::KeyboardZero4,       "P0 Keyboard 4",               "", true  },
  { Event::KeyboardZero5,       "P0 Keyboard 5",               "", true  },
  { Event::KeyboardZero6,       "P0 Keyboard 6",               "", true  },
  { Event::KeyboardZero7,       "P0 Keyboard 7",               "", true  },
  { Event::KeyboardZero8,       "P0 Keyboard 8",               "", true  },
  { Event::KeyboardZero9,       "P0 Keyboard 9",               "", true  },
  { Event::KeyboardZeroStar,    "P0 Keyboard *",               "", true  },
  { Event::KeyboardZero0,       "P0 Keyboard 0",               "", true  },
  { Event::KeyboardZeroPound,   "P0 Keyboard #",               "", true  },

  { Event::KeyboardOne1,        "P1 Keyboard 1",               "", true  },
  { Event::KeyboardOne2,        "P1 Keyboard 2",               "", true  },
  { Event::KeyboardOne3,        "P1 Keyboard 3",               "", true  },
  { Event::KeyboardOne4,        "P1 Keyboard 4",               "", true  },
  { Event::KeyboardOne5,        "P1 Keyboard 5",               "", true  },
  { Event::KeyboardOne6,        "P1 Keyboard 6",               "", true  },
  { Event::KeyboardOne7,        "P1 Keyboard 7",               "", true  },
  { Event::KeyboardOne8,        "P1 Keyboard 8",               "", true  },
  { Event::KeyboardOne9,        "P1 Keyboard 9",               "", true  },
  { Event::KeyboardOneStar,     "P1 Keyboard *",               "", true  },
  { Event::KeyboardOne0,        "P1 Keyboard 0",               "", true  },
  { Event::KeyboardOnePound,    "P1 Keyboard #",               "", true  },

  { Event::Combo1,              "Combo 1",                     "", false },
  { Event::Combo2,              "Combo 2",                     "", false },
  { Event::Combo3,              "Combo 3",                     "", false },
  { Event::Combo4,              "Combo 4",                     "", false },
  { Event::Combo5,              "Combo 5",                     "", false },
  { Event::Combo6,              "Combo 6",                     "", false },
  { Event::Combo7,              "Combo 7",                     "", false },
  { Event::Combo8,              "Combo 8",                     "", false },
  { Event::Combo9,              "Combo 9",                     "", false },
  { Event::Combo10,             "Combo 10",                    "", false },
  { Event::Combo11,             "Combo 11",                    "", false },
  { Event::Combo12,             "Combo 12",                    "", false },
  { Event::Combo13,             "Combo 13",                    "", false },
  { Event::Combo14,             "Combo 14",                    "", false },
  { Event::Combo15,             "Combo 15",                    "", false },
  { Event::Combo16,             "Combo 16",                    "", false }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::ActionList EventHandler::ourMenuActionList[kMenuActionListSize] = {
  { Event::UIUp,        "Move Up",              "", false },
  { Event::UIDown,      "Move Down",            "", false },
  { Event::UILeft,      "Move Left",            "", false },
  { Event::UIRight,     "Move Right",           "", false },

  { Event::UIHome,      "Home",                 "", false },
  { Event::UIEnd,       "End",                  "", false },
  { Event::UIPgUp,      "Page Up",              "", false },
  { Event::UIPgDown,    "Page Down",            "", false },

  { Event::UIOK,        "OK",                   "", false },
  { Event::UICancel,    "Cancel",               "", false },
  { Event::UISelect,    "Select item",          "", false },

  { Event::UINavPrev,   "Previous object",      "", false },
  { Event::UINavNext,   "Next object",          "", false },

  { Event::UIPrevDir,   "Parent directory",     "", false }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used by the Stelladaptor to send absolute axis values
const Event::Type EventHandler::SA_Axis[2][2] = {
  { Event::SALeftAxis0Value,  Event::SALeftAxis1Value  },
  { Event::SARightAxis0Value, Event::SARightAxis1Value }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used by the Stelladaptor to map button presses to joystick or paddles
//  (driving controllers and boostergrip are considered the same as joysticks)
const Event::Type EventHandler::SA_Button[2][4] = {
  { Event::JoystickZeroFire,  Event::JoystickZeroFire9,
    Event::JoystickZeroFire5, Event::JoystickZeroFire9 },
  { Event::JoystickOneFire,   Event::JoystickOneFire9,
    Event::JoystickOneFire5,  Event::JoystickOneFire9  }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used by the 2600-daptor to map button presses to keypad keys
const Event::Type EventHandler::SA_Key[2][12] = {
  { Event::KeyboardZero1,    Event::KeyboardZero2,  Event::KeyboardZero3,
    Event::KeyboardZero4,    Event::KeyboardZero5,  Event::KeyboardZero6,
    Event::KeyboardZero7,    Event::KeyboardZero8,  Event::KeyboardZero9,
    Event::KeyboardZeroStar, Event::KeyboardZero0,  Event::KeyboardZeroPound },
  { Event::KeyboardOne1,     Event::KeyboardOne2,   Event::KeyboardOne3,
    Event::KeyboardOne4,     Event::KeyboardOne5,   Event::KeyboardOne6,
    Event::KeyboardOne7,     Event::KeyboardOne8,   Event::KeyboardOne9,
    Event::KeyboardOneStar,  Event::KeyboardOne0,   Event::KeyboardOnePound  }
};
