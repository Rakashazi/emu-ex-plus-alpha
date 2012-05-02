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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: EventHandler.cxx 2251 2011-06-09 20:50:19Z stephena $
//============================================================================

#include <sstream>
#include <SDL.h>

#include "bspf.hxx"

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
#include "Snapshot.hxx"
#include "Sound.hxx"
#include "StateManager.hxx"
#include "Switches.hxx"

#include "EventHandler.hxx"

#ifdef CHEATCODE_SUPPORT
  #include "Cheat.hxx"
  #include "CheatManager.hxx"
#endif
#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::EventHandler(OSystem* osystem)
  : myOSystem(osystem),
    myEvent(NULL),
    myOverlay(NULL),
    myState(S_NONE),
    myAllowAllDirectionsFlag(false),
    myFryingFlag(false),
    mySkipMouseMotion(true)
{
  // Create the event object which will be used for this handler
  myEvent = new Event();

  // Erase the key mapping array
  for(int i = 0; i < SDLK_LAST; ++i)
  {
    ourSDLMapping[i] = "";
    for(int m = 0; m < kNumModes; ++m)
      myKeyTable[i][m] = Event::NoType;
  }

#ifdef JOYSTICK_SUPPORT
  // Erase the joystick button mapping array
  for(int m = 0; m < kNumModes; ++m)
    for(int i = 0; i < kNumJoysticks; ++i)
      for(int j = 0; j < kNumJoyButtons; ++j)
        myJoyTable[i][j][m] = Event::NoType;

  // Erase the joystick axis mapping array and last axis value
  for(int i = 0; i < kNumJoysticks; ++i)
    for(int j = 0; j < kNumJoyAxis; ++j)
    {
      myAxisLastValue[i][j] = 0;
      for(int m = 0; m < kNumModes; ++m)
        myJoyAxisTable[i][j][0][m] = myJoyAxisTable[i][j][1][m] = Event::NoType;
    }

  // Erase the joystick hat mapping array
  for(int m = 0; m < kNumModes; ++m)
    for(int i = 0; i < kNumJoysticks; ++i)
      for(int j = 0; j < kNumJoyHats; ++j)
        for(int k = 0; k < 4; ++k)
          myJoyHatTable[i][j][k][m] = Event::NoType;
#endif

  // Erase the 'combo' array
  for(int i = 0; i < kComboSize; ++i)
    for(int j = 0; j < kEventsPerCombo; ++j)
      myComboTable[i][j] = Event::NoType;

  // Erase the Message array
  for(int i = 0; i < Event::LastType; ++i)
    ourMessageTable[i] = "";

  // Set unchanging messages
  ourMessageTable[Event::ConsoleColor]      = "Color Mode";
  ourMessageTable[Event::ConsoleBlackWhite] = "BW Mode";
  ourMessageTable[Event::ConsoleLeftDiffA]  = "Left Difficulty A";
  ourMessageTable[Event::ConsoleLeftDiffB]  = "Left Difficulty B";
  ourMessageTable[Event::ConsoleRightDiffA] = "Right Difficulty A";
  ourMessageTable[Event::ConsoleRightDiffB] = "Right Difficulty B";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::~EventHandler()
{
  // Free strings created with strdup
  for(uInt32 i = 0; i < kEmulActionListSize; ++i)
    if(ourEmulActionList[i].key)
      free(ourEmulActionList[i].key);
  for(uInt32 i = 0; i < kMenuActionListSize; ++i)
    if(ourMenuActionList[i].key)
      free(ourMenuActionList[i].key);

  delete myEvent;

#ifdef JOYSTICK_SUPPORT
  if(SDL_WasInit(SDL_INIT_JOYSTICK) & SDL_INIT_JOYSTICK)
  {
    for(uInt32 i = 0; i < kNumJoysticks; i++)
    {
      if(SDL_JoystickOpened(i))
        SDL_JoystickClose(ourJoysticks[i].stick);
    }
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::initialize()
{
  // Make sure the event/action mappings are correctly set,
  // and fill the ActionList structure with valid values
  setSDLMappings();
  setKeymap();
  setJoymap();
  setJoyAxisMap();
  setJoyHatMap();
  setComboMap();
  setActionMappings(kEmulationMode);
  setActionMappings(kMenuMode);

  myUseCtrlKeyFlag = myOSystem->settings().getBool("ctrlcombo");

  Joystick::setDeadZone(myOSystem->settings().getInt("joydeadzone"));
  Paddles::setDigitalSensitivity(myOSystem->settings().getInt("dsense"));
  Paddles::setMouseSensitivity(myOSystem->settings().getInt("msense"));
  setMouseControllerMode(myOSystem->settings().getBool("usemouse") ? 0 : -1, false);

  // Set quick select delay when typing characters in listwidgets
  ListWidget::setQuickSelectDelay(myOSystem->settings().getInt("listdelay"));

  // Set number of lines a mousewheel will scroll
  ScrollBarWidget::setWheelLines(myOSystem->settings().getInt("mwheel"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::reset(State state)
{
  setEventState(state);
  myOSystem->state().reset();

  setContinuousSnapshots(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setupJoysticks()
{
#ifdef JOYSTICK_SUPPORT
  // Keep track of how many Stelladaptors we've found
  int saCount = 0;

  // First clear the joystick array, closing previously opened sticks
  if((SDL_WasInit(SDL_INIT_JOYSTICK) & SDL_INIT_JOYSTICK) > 0)
  {
    for(int i = 0; i < kNumJoysticks; i++)
    {
      if(ourJoysticks[i].stick && SDL_JoystickOpened(i))
        SDL_JoystickClose(ourJoysticks[i].stick);

      ourJoysticks[i].stick = (SDL_Joystick*) NULL;
      ourJoysticks[i].type  = JT_NONE;
    }
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
  }

  // (Re)-Initialize the joystick subsystem
  myOSystem->logMessage("Joystick devices found:\n", 1);
  if((SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1) || (SDL_NumJoysticks() <= 0))
  {
    myOSystem->logMessage("No joysticks present.\n\n", 1);
    return;
  }

  // Open up to 6 regular joysticks and 2 Stelladaptor devices
  uInt32 limit = SDL_NumJoysticks() <= kNumJoysticks ?
                 SDL_NumJoysticks() : kNumJoysticks;
  for(uInt32 i = 0; i < limit; i++)
  {
    string name = SDL_JoystickName(i);
    ourJoysticks[i].stick = SDL_JoystickOpen(i);

    // Skip if we couldn't open it for any reason
    if(ourJoysticks[i].stick == NULL)
    {
      ourJoysticks[i].type = JT_NONE;
      ourJoysticks[i].name = "None";
      continue;
    }

    // Figure out what type of joystick this is
    ostringstream buf;
    if(name.find("2600-daptor", 0) != string::npos)
    {
      saCount++;
      if(saCount > 2)  // Ignore more than 2 Stelladaptors
        continue;
      else if(saCount == 1)  // Type will be set by mapStelladaptors()
        ourJoysticks[i].name = "2600-daptor 1";
      else if(saCount == 2)
        ourJoysticks[i].name = "2600-daptor 2";

      buf << "  " << i << ": " << ourJoysticks[i].name << endl;
      myOSystem->logMessage(buf.str(), 1);
    }
    else if(name.find("Stelladaptor", 0) != string::npos)
    {
      saCount++;
      if(saCount > 2)  // Ignore more than 2 Stelladaptors
        continue;
      else if(saCount == 1)  // Type will be set by mapStelladaptors()
        ourJoysticks[i].name = "Stelladaptor 1";
      else if(saCount == 2)
        ourJoysticks[i].name = "Stelladaptor 2";

      buf << "  " << i << ": " << ourJoysticks[i].name << endl;
      myOSystem->logMessage(buf.str(), 1);
    }
    else
    {
      ourJoysticks[i].type = JT_REGULAR;
      ourJoysticks[i].name = SDL_JoystickName(i);

      buf << "  " << i << ": " << ourJoysticks[i].name << " with "
          << SDL_JoystickNumAxes(ourJoysticks[i].stick) << " axes, "
          << SDL_JoystickNumHats(ourJoysticks[i].stick) << " hats, "
          << SDL_JoystickNumBalls(ourJoysticks[i].stick) << " balls, "
          << SDL_JoystickNumButtons(ourJoysticks[i].stick) << " buttons"
          << endl;
      myOSystem->logMessage(buf.str(), 1);
    }
  }
  myOSystem->logMessage("\n", 1);

  // Map the stelladaptors we've found according to the specified ports
  const string& sa1 = myOSystem->settings().getString("sa1");
  const string& sa2 = myOSystem->settings().getString("sa2");
  mapStelladaptors(sa1, sa2);
#else
  myOSystem->logMessage("No joysticks present.\n", 1);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::mapStelladaptors(const string& sa1, const string& sa2)
{
#ifdef JOYSTICK_SUPPORT
  for(int i = 0; i < kNumJoysticks; i++)
  {
    if(ourJoysticks[i].name == "Stelladaptor 1")
    {
      if(sa1 == "left")
      {
        ourJoysticks[i].type = JT_STELLADAPTOR_LEFT;
        myOSystem->logMessage("  Stelladaptor 1 emulates left joystick port\n", 1);
      }
      else if(sa1 == "right")
      {
        ourJoysticks[i].type = JT_STELLADAPTOR_RIGHT;
        myOSystem->logMessage("  Stelladaptor 1 emulates right joystick port\n", 1);
      }
    }
    else if(ourJoysticks[i].name == "Stelladaptor 2")
    {
      if(sa2 == "left")
      {
        ourJoysticks[i].type = JT_STELLADAPTOR_LEFT;
        myOSystem->logMessage("  Stelladaptor 2 emulates left joystick port\n", 1);
      }
      else if(sa2 == "right")
      {
        ourJoysticks[i].type = JT_STELLADAPTOR_RIGHT;
        myOSystem->logMessage("  Stelladaptor 2 emulates right joystick port\n", 1);
      }
    }
    else if(ourJoysticks[i].name == "2600-daptor 1")
    {
      if(sa1 == "left")
      {
        ourJoysticks[i].type = JT_2600DAPTOR_LEFT;
        myOSystem->logMessage("  2600-daptor 1 emulates left joystick port\n", 1);
      }
      else if(sa1 == "right")
      {
        ourJoysticks[i].type = JT_2600DAPTOR_RIGHT;
        myOSystem->logMessage("  2600-daptor 1 emulates right joystick port\n", 1);
      }
    }
    else if(ourJoysticks[i].name == "2600-daptor 2")
    {
      if(sa2 == "left")
      {
        ourJoysticks[i].type = JT_2600DAPTOR_LEFT;
        myOSystem->logMessage("  2600-daptor 2 emulates left joystick port\n", 1);
      }
      else if(sa2 == "right")
      {
        ourJoysticks[i].type = JT_2600DAPTOR_RIGHT;
        myOSystem->logMessage("  2600-daptor 2 emulates right joystick port\n", 1);
      }
    }
  }
  myOSystem->logMessage("\n", 1);

  myOSystem->settings().setString("sa1", sa1);
  myOSystem->settings().setString("sa2", sa2);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::poll(uInt64 time)
{
  // Synthesize events for platform-specific hardware
  myOSystem->pollEvent();

  // Check for an event
  SDL_Event event;
  while(SDL_PollEvent(&event))
  {
    switch(event.type)
    {
      // keyboard events
      case SDL_KEYUP:
      case SDL_KEYDOWN:
      {
        int ascii   = event.key.keysym.unicode;
        SDLKey key  = event.key.keysym.sym;
        SDLMod mod  = event.key.keysym.mod;
        uInt8 state = event.key.type == SDL_KEYDOWN ? 1 : 0;
        bool handled = true;

        // An attempt to speed up event processing
        // All SDL-specific event actions are accessed by either
        // Control or Alt/Cmd keys.  So we quickly check for those.
        if(kbdAlt(mod) && state)
        {
      #ifdef MAC_OSX
          // These keys work in all states
          if(key == SDLK_q)
          {
            handleEvent(Event::Quit, 1);
          }
          else
      #endif
          if(key == SDLK_RETURN)
          {
            myOSystem->frameBuffer().toggleFullscreen();
          }
          // These only work when in emulation mode
          else if(myState == S_EMULATE)
          {
            switch(int(key))
            {
              case SDLK_EQUALS:
                myOSystem->frameBuffer().changeVidMode(+1);
                break;

              case SDLK_MINUS:
                myOSystem->frameBuffer().changeVidMode(-1);
                break;

              case SDLK_LEFTBRACKET:
                myOSystem->sound().adjustVolume(-1);
                break;

              case SDLK_RIGHTBRACKET:
                myOSystem->sound().adjustVolume(+1);
                break;

              case SDLK_PAGEUP:    // Alt-PageUp increases YStart
                myOSystem->console().changeYStart(+1);
                break;

              case SDLK_PAGEDOWN:  // Alt-PageDown decreases YStart
                myOSystem->console().changeYStart(-1);
                break;

              case SDLK_z:
                if(mod & KMOD_SHIFT)
                  myOSystem->console().toggleP0Collision();
                else
                  myOSystem->console().toggleP0Bit();
                break;

              case SDLK_x:
                if(mod & KMOD_SHIFT)
                  myOSystem->console().toggleP1Collision();
                else
                  myOSystem->console().toggleP1Bit();
                break;

              case SDLK_c:
                if(mod & KMOD_SHIFT)
                  myOSystem->console().toggleM0Collision();
                else
                  myOSystem->console().toggleM0Bit();
                break;

              case SDLK_v:
                if(mod & KMOD_SHIFT)
                  myOSystem->console().toggleM1Collision();
                else
                  myOSystem->console().toggleM1Bit();
                break;

              case SDLK_b:
                if(mod & KMOD_SHIFT)
                  myOSystem->console().toggleBLCollision();
                else
                  myOSystem->console().toggleBLBit();
                break;

              case SDLK_n:
                if(mod & KMOD_SHIFT)
                  myOSystem->console().togglePFCollision();
                else
                  myOSystem->console().togglePFBit();
                break;

              case SDLK_m:
                myOSystem->console().toggleHMOVE();
                break;

              case SDLK_COMMA:
                myOSystem->console().toggleFixedColors();
                break;

              case SDLK_PERIOD:
                if(mod & KMOD_SHIFT)
                  myOSystem->console().enableCollisions(false);
                else
                  myOSystem->console().enableBits(false);
                break;

              case SDLK_SLASH:
                if(mod & KMOD_SHIFT)
                  myOSystem->console().enableCollisions(true);
                else
                  myOSystem->console().enableBits(true);
                break;

              case SDLK_p:  // Alt-p toggles phosphor effect
                myOSystem->console().togglePhosphor();
                break;

              case SDLK_l:
                myOSystem->frameBuffer().toggleFrameStats();
                break;

              case SDLK_s:  // TODO - make this remappable
                if(myContSnapshotInterval == 0)
                {
                  ostringstream buf;
                  uInt32 interval = myOSystem->settings().getInt("ssinterval");
                  buf << "Enabling shotshots in " << interval << " second intervals";
                  myOSystem->frameBuffer().showMessage(buf.str());
                  setContinuousSnapshots(interval);
                }
                else
                {
                  ostringstream buf;
                  buf << "Disabling snapshots, generated "
                      << (myContSnapshotCounter / myContSnapshotInterval)
                      << " files";
                  myOSystem->frameBuffer().showMessage(buf.str());
                  setContinuousSnapshots(0);
                }
                break;
#if 0
// FIXME - these will be removed when a UI is added for event recording
              case SDLK_e:  // Alt-e starts/stops event recording
                if(myOSystem->state().toggleRecordMode())
                  myOSystem->frameBuffer().showMessage("Recording started");
                else
                  myOSystem->frameBuffer().showMessage("Recording stopped");
                break;

              case SDLK_r:  // Alt-r starts/stops rewind mode
                if(myOSystem->state().toggleRewindMode())
                  myOSystem->frameBuffer().showMessage("Rewind mode started");
                else
                  myOSystem->frameBuffer().showMessage("Rewind mode stopped");
                break;
/*
              case SDLK_l:  // Alt-l loads a recording
                if(myEventStreamer->loadRecording())
                  myOSystem->frameBuffer().showMessage("Playing recording");
                else
                  myOSystem->frameBuffer().showMessage("Playing recording error");
                return;
                break;
*/
////////////////////////////////////////////////////////////////////////
#endif
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
          if(key == SDLK_q)
          {
            handleEvent(Event::Quit, 1);
          }
          // These only work when in emulation mode
          else if(myState == S_EMULATE)
          {
            switch(int(key))
            {
              case SDLK_0:  // Ctrl-0 sets the mouse to controller 0
                setMouseControllerMode(0, true);
                break;

              case SDLK_1:  // Ctrl-1 sets the mouse to controller 1
                setMouseControllerMode(1, true);
                break;

              case SDLK_2:  // Ctrl-2 sets the mouse to controller 2
                setMouseControllerMode(2, true);
                break;

              case SDLK_3:  // Ctrl-3 sets the mouse to controller 3
                setMouseControllerMode(3, true);
                  break;

              case SDLK_f:  // Ctrl-f toggles NTSC/PAL mode
                myOSystem->console().toggleFormat();
                break;

              case SDLK_g:  // Ctrl-g (un)grabs mouse
                if(!myOSystem->frameBuffer().fullScreen())
                  myOSystem->frameBuffer().toggleGrabMouse();
                break;

              case SDLK_l:  // Ctrl-l toggles PAL color-loss effect
                myOSystem->console().toggleColorLoss();
                break;

              case SDLK_p:  // Ctrl-p toggles different palettes
                myOSystem->console().togglePalette();
                break;

              case SDLK_r:  // Ctrl-r reloads the currently loaded ROM
                myOSystem->deleteConsole();
                myOSystem->createConsole();
                break;

              case SDLK_PAGEUP:    // Ctrl-PageUp increases Height
                myOSystem->console().changeHeight(+1);
                break;

              case SDLK_PAGEDOWN:  // Ctrl-PageDown decreases Height
                myOSystem->console().changeHeight(-1);
                break;

              case SDLK_s:         // Ctrl-s saves properties to a file
              {
                string filename = myOSystem->baseDir() +
                    myOSystem->console().properties().get(Cartridge_Name) + ".pro";
                ofstream out(filename.c_str(), ios::out);
                if(out)
                {
                  myOSystem->console().properties().save(out);
                  out.close();
                  myOSystem->frameBuffer().showMessage("Properties saved");
                }
                else
                  myOSystem->frameBuffer().showMessage("Error saving properties");
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
        if(handled) break;

        // Handle keys which switch eventhandler state
        // Arrange the logic to take advantage of short-circuit evaluation
        if(!(kbdControl(mod) || kbdShift(mod) || kbdAlt(mod)) &&
            state && eventStateChange(myKeyTable[key][kEmulationMode]))
          return;

        // Otherwise, let the event handler deal with it
        if(myState == S_EMULATE)
          handleEvent(myKeyTable[key][kEmulationMode], state);
        else if(myOverlay != NULL)
        {
          // Assign ascii field if it doesn't exist
          // Make sure 'state change' keys (Shift, Ctrl, etc) are excluded
          if(!ascii || ascii >= SDLK_LAST ||
              key == SDLK_BACKSPACE || key == SDLK_DELETE) ascii = key;
          if(key > SDLK_F15 && key < SDLK_HELP) ascii = 0;

          myOverlay->handleKeyEvent(ascii, key, mod, state);
        }

        break;  // SDL_KEYUP, SDL_KEYDOWN
      }

      case SDL_MOUSEMOTION:
        // Determine which mode we're in, then send the event to the appropriate place
        if(myState == S_EMULATE)
        {
          if(myMouseEnabled)
          {
            if(!mySkipMouseMotion)
            {
              myEvent->set(Event::MouseAxisXValue, event.motion.xrel);
              myEvent->set(Event::MouseAxisYValue, event.motion.yrel);
            }
            mySkipMouseMotion = false;
          }
        }
        else if(myOverlay)
          myOverlay->handleMouseMotionEvent(event.motion.x, event.motion.y, 0);

        break;  // SDL_MOUSEMOTION

      case SDL_MOUSEBUTTONUP:
      case SDL_MOUSEBUTTONDOWN:
      {
        uInt8 state = event.button.type == SDL_MOUSEBUTTONDOWN ? 1 : 0;

        // Determine which mode we're in, then send the event to the appropriate place
        if(myState == S_EMULATE)
        {
          if(myMouseEnabled)
            myEvent->set(Event::MouseButtonValue, state);
        }
        else if(myOverlay)
        {
          // Take window zooming into account
          Int32 x = event.button.x, y = event.button.y;

          switch(event.button.button)
          {
            case SDL_BUTTON_LEFT:
              myOverlay->handleMouseButtonEvent(
                  state ? EVENT_LBUTTONDOWN : EVENT_LBUTTONUP, x, y, state);
              break;
            case SDL_BUTTON_RIGHT:
              myOverlay->handleMouseButtonEvent(
                  state ? EVENT_RBUTTONDOWN : EVENT_RBUTTONUP, x, y, state);
              break;
            case SDL_BUTTON_WHEELDOWN:
              if(state)
                myOverlay->handleMouseButtonEvent(EVENT_WHEELDOWN, x, y, 1);
              break;
            case SDL_BUTTON_WHEELUP:
              if(state)
                myOverlay->handleMouseButtonEvent(EVENT_WHEELUP, x, y, 1);
              break;
            default:
              break;
          }
        }
        break;  // SDL_MOUSEBUTTONUP, SDL_MOUSEBUTTONDOWN
      }

      case SDL_ACTIVEEVENT:
        if((event.active.state & SDL_APPACTIVE) && (event.active.gain == 0))
          if(myState == S_EMULATE) enterMenuMode(S_MENU);
        break; // SDL_ACTIVEEVENT

      case SDL_QUIT:
        handleEvent(Event::Quit, 1);
        break;  // SDL_QUIT

      case SDL_VIDEOEXPOSE:
        myOSystem->frameBuffer().refresh();
        break;  // SDL_VIDEOEXPOSE

#ifdef JOYSTICK_SUPPORT
      case SDL_JOYBUTTONUP:
      case SDL_JOYBUTTONDOWN:
      {
        if(event.jbutton.which >= kNumJoysticks)
          break;

        // Stelladaptors handle buttons differently than regular joysticks
        int type = ourJoysticks[event.jbutton.which].type;
        int stick  = event.jbutton.which;
        int button = event.jbutton.button;
        int state  = event.jbutton.state == SDL_PRESSED ? 1 : 0;

        switch(type)
        {
          case JT_REGULAR:
            if(event.jbutton.button >= kNumJoyButtons)
              return;

            // Filter out buttons handled by OSystem
            if(!myOSystem->joyButtonHandled(button))
            {
              // Handle buttons which switch eventhandler state
              if(state && eventStateChange(myJoyTable[stick][button][kEmulationMode]))
                return;

              // Determine which mode we're in, then send the event to the appropriate place
              if(myState == S_EMULATE)
                handleEvent(myJoyTable[stick][button][kEmulationMode], state);
              else if(myOverlay != NULL)
                myOverlay->handleJoyEvent(stick, button, state);
            }
            break;  // Regular button

          // These events don't have to pass through handleEvent, since
          // they can never be remapped
          case JT_STELLADAPTOR_LEFT:
          case JT_STELLADAPTOR_RIGHT:
            // The 'type-2' here refers to the fact that 'JT_STELLADAPTOR_LEFT'
            // and 'JT_STELLADAPTOR_RIGHT' are at index 2 and 3 in the JoyType
            // enum; subtracting two gives us Controller 0 and 1
            if(button < 2) myEvent->set(SA_Button[type-2][button], state);
            break;  // Stelladaptor button
          case JT_2600DAPTOR_LEFT:
          case JT_2600DAPTOR_RIGHT:
            // The 'type-4' here refers to the fact that 'JT_2600DAPTOR_LEFT'
            // and 'JT_2600DAPTOR_RIGHT' are at index 4 and 5 in the JoyType
            // enum; subtracting four gives us Controller 0 and 1
            if(button < 2) myEvent->set(SA_Button[type-4][button], state);
            break;  // 2600DAPTOR button
        }
        break;  // SDL_JOYBUTTONUP, SDL_JOYBUTTONDOWN
      }  

      case SDL_JOYAXISMOTION:
      {
        if(event.jaxis.which >= kNumJoysticks)
          break;

        // Stelladaptors handle axis differently than regular joysticks
        int type = ourJoysticks[event.jbutton.which].type;
        int stick = event.jaxis.which;
        int axis  = event.jaxis.axis;
        int value = event.jaxis.value;

        switch(type)
        {
          case JT_REGULAR:
            if(myState == S_EMULATE)
            {
              // Every axis event has two associated values, negative and positive
              Event::Type eventAxisNeg = myJoyAxisTable[stick][axis][0][kEmulationMode];
              Event::Type eventAxisPos = myJoyAxisTable[stick][axis][1][kEmulationMode];

              // Check for analog events, which are handled differently
              // We'll pass them off as Stelladaptor events, and let the controllers
              // handle it
              switch((int)eventAxisNeg)
              {
                case Event::PaddleZeroAnalog:
                  myEvent->set(Event::SALeftAxis0Value, value);
                  break;
                case Event::PaddleOneAnalog:
                  myEvent->set(Event::SALeftAxis1Value, value);
                  break;
                case Event::PaddleTwoAnalog:
                  myEvent->set(Event::SARightAxis0Value, value);
                  break;
                case Event::PaddleThreeAnalog:
                  myEvent->set(Event::SARightAxis1Value, value);
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
                    if(myAxisLastValue[stick][axis] != value)
                    {
                      // Turn off both events, since we don't know exactly which one
                      // was previously activated.
                      handleEvent(eventAxisNeg, 0);
                      handleEvent(eventAxisPos, 0);
                    }
                  }
                  myAxisLastValue[stick][axis] = value;
                  break;
                }
              }
            }
            else if(myOverlay != NULL)
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
              if(value != myAxisLastValue[stick][axis])
              {
                myOverlay->handleJoyAxisEvent(stick, axis, value);
                myAxisLastValue[stick][axis] = value;
              }
            }
            break;  // Regular joystick axis

          // Since the various controller classes deal with Stelladaptor
          // devices differently, we send the raw X and Y axis data directly,
          // and let the controller handle it
          // These events don't have to pass through handleEvent, since
          // they can never be remapped
          case JT_STELLADAPTOR_LEFT:
          case JT_STELLADAPTOR_RIGHT:
            // The 'type-2' here refers to the fact that 'JT_STELLADAPTOR_LEFT'
            // and 'JT_STELLADAPTOR_RIGHT' are at index 2 and 3 in the JoyType
            // enum; subtracting two gives us Controller 0 and 1
            if(axis < 2)
              myEvent->set(SA_Axis[type-2][axis], value);
            break;  // Stelladaptor axis
          case JT_2600DAPTOR_LEFT:
          case JT_2600DAPTOR_RIGHT:
            // The 'type-4' here refers to the fact that 'JT_2600DAPTOR_LEFT'
            // and 'JT_2600DAPTOR_RIGHT' are at index 4 and 5 in the JoyType
            // enum; subtracting four gives us Controller 0 and 1
            if(axis < 2)
              myEvent->set(SA_Axis[type-4][axis], value);
            break;  // 26000daptor axis
        }
        break;  // SDL_JOYAXISMOTION
      }

      case SDL_JOYHATMOTION:
      {
        int stick = event.jhat.which;
        int hat   = event.jhat.hat;
        int value = event.jhat.value;

        if(stick >= kNumJoysticks || hat >= kNumJoyHats)
          break;

        // Preprocess all hat events, converting to Stella JoyHat type
        // Generate multiple equivalent hat events representing combined direction
        // when we get a diagonal hat event
        if(myState == S_EMULATE)
        {
          handleEvent(myJoyHatTable[stick][hat][EVENT_HATUP][kEmulationMode],
                      value & SDL_HAT_UP);
          handleEvent(myJoyHatTable[stick][hat][EVENT_HATRIGHT][kEmulationMode],
                      value & SDL_HAT_RIGHT);
          handleEvent(myJoyHatTable[stick][hat][EVENT_HATDOWN][kEmulationMode],
                      value & SDL_HAT_DOWN);
          handleEvent(myJoyHatTable[stick][hat][EVENT_HATLEFT][kEmulationMode],
                      value & SDL_HAT_LEFT);
        }
        else if(myOverlay != NULL)
        {
          if(value == SDL_HAT_CENTERED)
            myOverlay->handleJoyHatEvent(stick, hat, EVENT_HATCENTER);
          else
          {
            if(value & SDL_HAT_UP)
              myOverlay->handleJoyHatEvent(stick, hat, EVENT_HATUP);
            if(value & SDL_HAT_RIGHT)
              myOverlay->handleJoyHatEvent(stick, hat, EVENT_HATRIGHT); 
            if(value & SDL_HAT_DOWN)
              myOverlay->handleJoyHatEvent(stick, hat, EVENT_HATDOWN);
            if(value & SDL_HAT_LEFT)
              myOverlay->handleJoyHatEvent(stick, hat, EVENT_HATLEFT);
          }
        }
        break;  // SDL_JOYHATMOTION
      }
#endif  // JOYSTICK_SUPPORT
    }
  }

  // Update controllers and console switches, and in general all other things
  // related to emulation
  if(myState == S_EMULATE)
  {
    myOSystem->console().controller(Controller::Left).update();
    myOSystem->console().controller(Controller::Right).update();
    myOSystem->console().switches().update();

#if 0
    // Now check if the StateManager should be saving or loading state
    // Per-frame cheats are disabled if the StateManager is active, since
    // it would interfere with proper playback
    if(myOSystem->state().isActive())
    {
      myOSystem->state().update();
    }
    else
#endif
    {
    #ifdef CHEATCODE_SUPPORT
      const CheatList& cheats = myOSystem->cheat().perFrame();
      for(uInt32 i = 0; i < cheats.size(); i++)
        cheats[i]->evaluate();
    #endif

      // Handle continuous snapshots
      if(myContSnapshotInterval > 0 &&
        (++myContSnapshotCounter % myContSnapshotInterval == 0))
        takeSnapshot(time >> 10);  // not quite milliseconds, but close enough
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
  myEvent->set(Event::MouseAxisXValue, 0);
  myEvent->set(Event::MouseAxisYValue, 0);
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
        myEvent->set(Event::JoystickZeroDown, 0);
      break;

    case Event::JoystickZeroDown:
      if(!myAllowAllDirectionsFlag && state)
        myEvent->set(Event::JoystickZeroUp, 0);
      break;

    case Event::JoystickZeroLeft:
      if(!myAllowAllDirectionsFlag && state)
        myEvent->set(Event::JoystickZeroRight, 0);
      break;

    case Event::JoystickZeroRight:
      if(!myAllowAllDirectionsFlag && state)
        myEvent->set(Event::JoystickZeroLeft, 0);
      break;

    case Event::JoystickOneUp:
      if(!myAllowAllDirectionsFlag && state)
        myEvent->set(Event::JoystickOneDown, 0);
      break;

    case Event::JoystickOneDown:
      if(!myAllowAllDirectionsFlag && state)
        myEvent->set(Event::JoystickOneUp, 0);
      break;

    case Event::JoystickOneLeft:
      if(!myAllowAllDirectionsFlag && state)
        myEvent->set(Event::JoystickOneRight, 0);
      break;

    case Event::JoystickOneRight:
      if(!myAllowAllDirectionsFlag && state)
        myEvent->set(Event::JoystickOneLeft, 0);
      break;
    ////////////////////////////////////////////////////////////////////////

    case Event::Fry:
      myFryingFlag = bool(state);
      return;

    case Event::VolumeDecrease:
      if(state) myOSystem->sound().adjustVolume(-1);
      return;

    case Event::VolumeIncrease:
      if(state) myOSystem->sound().adjustVolume(+1);
      return;

    case Event::SaveState:
      if(state) myOSystem->state().saveState();
      return;

    case Event::ChangeState:
      if(state) myOSystem->state().changeState();
      return;

    case Event::LoadState:
      if(state) myOSystem->state().loadState();
      return;

    case Event::TakeSnapshot:
      if(state) takeSnapshot();
      return;

    case Event::LauncherMode:
      if((myState == S_EMULATE || myState == S_CMDMENU ||
          myState == S_DEBUGGER) && state)
      {
        myOSystem->settings().saveConfig();

        // Go back to the launcher, or immediately quit
        if(myOSystem->settings().getBool("uselauncher"))
        {
          myOSystem->deleteConsole();
          myOSystem->createLauncher();
        }
        else
          myOSystem->quit();
      }
      return;

    case Event::Quit:
      if(state)
      {
        myOSystem->settings().saveConfig();
        myOSystem->quit();
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

    case Event::NoType:  // Ignore unmapped events
      return;

    default:
      break;
  }

  if(state && ourMessageTable[event] != "")
    myOSystem->frameBuffer().showMessage(ourMessageTable[event]);

  // Otherwise, pass it to the emulation core
  myEvent->set(event, state);
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
  ActionList* list = NULL;

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
      break;
  }

  ostringstream buf;

  // Fill the ActionList with the current key and joystick mappings
  for(int i = 0; i < listsize; ++i)
  {
    Event::Type event = list[i].event;
    free(list[i].key);  list[i].key = NULL;
    list[i].key = strdup("None");
    string key = "";
    for(int j = 0; j < SDLK_LAST; ++j)   // key mapping
    {
      if(myKeyTable[j][mode] == event)
      {
        if(key == "")
          key = key + ourSDLMapping[j];
        else
          key = key + ", " + ourSDLMapping[j];
      }
    }
#ifdef JOYSTICK_SUPPORT
    int stick, button, axis, hat, dir;

    // Joystick button mapping/labeling
    for(stick = 0; stick < kNumJoysticks; ++stick)
    {
      for(button = 0; button < kNumJoyButtons; ++button)
      {
        if(myJoyTable[stick][button][mode] == event)
        {
          buf.str("");
          buf << "J" << stick << "/B" << button;
          if(key == "")
            key = key + buf.str();
          else
            key = key + ", " + buf.str();
        }
      }
    }
    // Joystick axis mapping/labeling
    for(int stick = 0; stick < kNumJoysticks; ++stick)
    {
      for(axis = 0; axis < kNumJoyAxis; ++axis)
      {
        for(dir = 0; dir < 2; ++dir)
        {
          if(myJoyAxisTable[stick][axis][dir][mode] == event)
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
    }
    // Joystick hat mapping/labeling
    for(stick = 0; stick < kNumJoysticks; ++stick)
    {
      for(hat = 0; hat < kNumJoyHats; ++hat)
      {
        for(dir = 0; dir < 4; ++dir)
        {
          if(myJoyHatTable[stick][hat][dir][mode] == event)
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
#ifndef MAC_OSX
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
    {
      free(list[i].key);  list[i].key = NULL;
      list[i].key = strdup(key.c_str());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setKeymap()
{
  string list = myOSystem->settings().getString("keymap");
  IntArray map;

  if(isValidList(list, map, SDLK_LAST * kNumModes))
  {
    // Fill the keymap table with events
    IntArray::const_iterator event = map.begin();
    for(int mode = 0; mode < kNumModes; ++mode)
      for(int i = 0; i < SDLK_LAST; ++i)
        myKeyTable[i][mode] = (Event::Type) *event++;
  }
  else
  {
    setDefaultKeymap(Event::NoType, kEmulationMode);
    setDefaultKeymap(Event::NoType, kMenuMode);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setJoymap()
{
#ifdef JOYSTICK_SUPPORT
  string list = myOSystem->settings().getString("joymap");
  IntArray map;

  if(isValidList(list, map, kNumJoysticks*kNumJoyButtons*kNumModes))
  {
    // Fill the joymap table with events
    IntArray::const_iterator event = map.begin();
    for(int mode = 0; mode < kNumModes; ++mode)
      for(int i = 0; i < kNumJoysticks; ++i)
        for(int j = 0; j < kNumJoyButtons; ++j)
          myJoyTable[i][j][mode] = (Event::Type) *event++;
  }
  else
  {
    setDefaultJoymap(Event::NoType, kEmulationMode);
    setDefaultJoymap(Event::NoType, kMenuMode);
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setJoyAxisMap()
{
#ifdef JOYSTICK_SUPPORT
  string list = myOSystem->settings().getString("joyaxismap");
  IntArray map;

  if(isValidList(list, map, kNumJoysticks*kNumJoyAxis*2*kNumModes))
  {
    // Fill the joyaxismap table with events
    IntArray::const_iterator event = map.begin();
    for(int mode = 0; mode < kNumModes; ++mode)
      for(int i = 0; i < kNumJoysticks; ++i)
        for(int j = 0; j < kNumJoyAxis; ++j)
          for(int k = 0; k < 2; ++k)
            myJoyAxisTable[i][j][k][mode] = (Event::Type) *event++;
  }
  else
  {
    setDefaultJoyAxisMap(Event::NoType, kEmulationMode);
    setDefaultJoyAxisMap(Event::NoType, kMenuMode);
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setJoyHatMap()
{
#ifdef JOYSTICK_SUPPORT
  string list = myOSystem->settings().getString("joyhatmap");
  IntArray map;

  if(isValidList(list, map, kNumJoysticks*kNumJoyHats*4*kNumModes))
  {
    // Fill the joyhatmap table with events
    IntArray::const_iterator event = map.begin();
    for(int mode = 0; mode < kNumModes; ++mode)
      for(int i = 0; i < kNumJoysticks; ++i)
        for(int j = 0; j < kNumJoyHats; ++j)
          for(int k = 0; k < 4; ++k)
            myJoyHatTable[i][j][k][mode] = (Event::Type) *event++;
  }
  else
  {
    setDefaultJoyHatMap(Event::NoType, kEmulationMode);
    setDefaultJoyHatMap(Event::NoType, kMenuMode);
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setComboMap()
{
  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  string list = myOSystem->settings().getString("combomap");
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
        myComboTable[combocount][eventcount] = (Event::Type) atoi(key.c_str());
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
bool EventHandler::addKeyMapping(Event::Type event, EventMode mode, int key)
{
  // These keys cannot be remapped
  if(key == SDLK_TAB || eventIsAnalog(event))
    return false;
  else
  {
    myKeyTable[key][mode] = event;
    saveKeyMapping();

    setActionMappings(mode);
    return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoyMapping(Event::Type event, EventMode mode,
                                        int stick, int button)
{
  if(stick >= 0 && stick < kNumJoysticks &&
     button >= 0 && button < kNumJoyButtons &&
     event >= 0 && event < Event::LastType)
    myJoyTable[stick][button][mode] = event;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addJoyMapping(Event::Type event, EventMode mode,
                                 int stick, int button)
{
  if(!eventIsAnalog(event))
  {
    setDefaultJoyMapping(event, mode, stick, button);

    saveJoyMapping();
    setActionMappings(mode);
    return true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoyAxisMapping(Event::Type event, EventMode mode,
                                            int stick, int axis, int value)
{
  if(stick >= 0 && stick < kNumJoysticks &&
     axis >= 0 && axis < kNumJoyAxis &&
     event >= 0 && event < Event::LastType)
  {
    // This confusing code is because each axis has two associated values,
    // but analog events only affect one of the axis.
    if(eventIsAnalog(event))
      myJoyAxisTable[stick][axis][0][mode] =
        myJoyAxisTable[stick][axis][1][mode] = event;
    else
    {
      // Otherwise, turn off the analog event(s) for this axis
      if(eventIsAnalog(myJoyAxisTable[stick][axis][0][mode]))
        myJoyAxisTable[stick][axis][0][mode] = Event::NoType;
      if(eventIsAnalog(myJoyAxisTable[stick][axis][1][mode]))
        myJoyAxisTable[stick][axis][1][mode] = Event::NoType;
    
      myJoyAxisTable[stick][axis][(value > 0)][mode] = event;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addJoyAxisMapping(Event::Type event, EventMode mode,
                                     int stick, int axis, int value)
{
  setDefaultJoyAxisMapping(event, mode, stick, axis, value);

  saveJoyAxisMapping();
  setActionMappings(mode);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoyHatMapping(Event::Type event, EventMode mode,
                                           int stick, int hat, int value)
{
  if(stick >= 0 && stick < kNumJoysticks &&
     hat >= 0 && hat < kNumJoyHats &&
     event >= 0 && event < Event::LastType &&
     value != EVENT_HATCENTER)
  {
    myJoyHatTable[stick][hat][value][mode] = event;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addJoyHatMapping(Event::Type event, EventMode mode,
                                    int stick, int hat, int value)
{
  if(!eventIsAnalog(event))
  {
    setDefaultJoyHatMapping(event, mode, stick, hat, value);

    saveJoyHatMapping();
    setActionMappings(mode);
    return true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::eraseMapping(Event::Type event, EventMode mode)
{
  // Erase the KeyEvent arrays
  for(int i = 0; i < SDLK_LAST; ++i)
    if(myKeyTable[i][mode] == event && i != SDLK_TAB)
      myKeyTable[i][mode] = Event::NoType;
  saveKeyMapping();

#ifdef JOYSTICK_SUPPORT
  // Erase the JoyEvent array
  for(int i = 0; i < kNumJoysticks; ++i)
    for(int j = 0; j < kNumJoyButtons; ++j)
      if(myJoyTable[i][j][mode] == event)
        myJoyTable[i][j][mode] = Event::NoType;
  saveJoyMapping();

  // Erase the JoyAxisEvent array
  for(int i = 0; i < kNumJoysticks; ++i)
    for(int j = 0; j < kNumJoyAxis; ++j)
      for(int k = 0; k < 2; ++k)
        if(myJoyAxisTable[i][j][k][mode] == event)
          myJoyAxisTable[i][j][k][mode] = Event::NoType;
  saveJoyAxisMapping();

  // Erase the JoyHatEvent array
  for(int i = 0; i < kNumJoysticks; ++i)
    for(int j = 0; j < kNumJoyHats; ++j)
      for(int k = 0; k < 4; ++k)
        if(myJoyHatTable[i][j][k][mode] == event)
          myJoyHatTable[i][j][k][mode] = Event::NoType;
  saveJoyHatMapping();
#endif

  setActionMappings(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultMapping(Event::Type event, EventMode mode)
{
  setDefaultKeymap(event, mode);
  setDefaultJoymap(event, mode);
  setDefaultJoyAxisMap(event, mode);
  setDefaultJoyHatMap(event, mode);

  setActionMappings(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultKeymap(Event::Type event, EventMode mode)
{
#define SET_DEFAULT_KEY(sdk_key, sdk_mode, sdk_event, sdk_cmp_event) \
  if(eraseAll || sdk_cmp_event == sdk_event)  \
    myKeyTable[sdk_key][sdk_mode] = sdk_event;

  // If event is 'NoType', erase and reset all mappings
  // Otherwise, only reset the given event
  bool eraseAll = (event == Event::NoType);
  if(eraseAll)
  {
    // Erase all mappings
    for(int i = 0; i < SDLK_LAST; ++i)
      myKeyTable[i][mode] = Event::NoType;
  }

  switch(mode)
  {
    case kEmulationMode:
      SET_DEFAULT_KEY(SDLK_1,         mode, Event::KeyboardZero1,     event);
      SET_DEFAULT_KEY(SDLK_2,         mode, Event::KeyboardZero2,     event);
      SET_DEFAULT_KEY(SDLK_3,         mode, Event::KeyboardZero3,     event);
      SET_DEFAULT_KEY(SDLK_q,         mode, Event::KeyboardZero4,     event);
      SET_DEFAULT_KEY(SDLK_w,         mode, Event::KeyboardZero5,     event);
      SET_DEFAULT_KEY(SDLK_e,         mode, Event::KeyboardZero6,     event);
      SET_DEFAULT_KEY(SDLK_a,         mode, Event::KeyboardZero7,     event);
      SET_DEFAULT_KEY(SDLK_s,         mode, Event::KeyboardZero8,     event);
      SET_DEFAULT_KEY(SDLK_d,         mode, Event::KeyboardZero9,     event);
      SET_DEFAULT_KEY(SDLK_z,         mode, Event::KeyboardZeroStar,  event);
      SET_DEFAULT_KEY(SDLK_x,         mode, Event::KeyboardZero0,     event);
      SET_DEFAULT_KEY(SDLK_c,         mode, Event::KeyboardZeroPound, event);

      SET_DEFAULT_KEY(SDLK_8,         mode, Event::KeyboardOne1,      event);
      SET_DEFAULT_KEY(SDLK_9,         mode, Event::KeyboardOne2,      event);
      SET_DEFAULT_KEY(SDLK_0,         mode, Event::KeyboardOne3,      event);
      SET_DEFAULT_KEY(SDLK_i,         mode, Event::KeyboardOne4,      event);
      SET_DEFAULT_KEY(SDLK_o,         mode, Event::KeyboardOne5,      event);
      SET_DEFAULT_KEY(SDLK_p,         mode, Event::KeyboardOne6,      event);
      SET_DEFAULT_KEY(SDLK_k,         mode, Event::KeyboardOne7,      event);
      SET_DEFAULT_KEY(SDLK_l,         mode, Event::KeyboardOne8,      event);
      SET_DEFAULT_KEY(SDLK_SEMICOLON, mode, Event::KeyboardOne9,      event);
      SET_DEFAULT_KEY(SDLK_COMMA,     mode, Event::KeyboardOneStar,   event);
      SET_DEFAULT_KEY(SDLK_PERIOD,    mode, Event::KeyboardOne0,      event);
      SET_DEFAULT_KEY(SDLK_SLASH,     mode, Event::KeyboardOnePound,  event);

      SET_DEFAULT_KEY(SDLK_UP,        mode, Event::JoystickZeroUp,    event);
      SET_DEFAULT_KEY(SDLK_DOWN,      mode, Event::JoystickZeroDown,  event);
      SET_DEFAULT_KEY(SDLK_LEFT,      mode, Event::JoystickZeroLeft,  event);
      SET_DEFAULT_KEY(SDLK_RIGHT,     mode, Event::JoystickZeroRight, event);
      SET_DEFAULT_KEY(SDLK_SPACE,     mode, Event::JoystickZeroFire1, event);
      SET_DEFAULT_KEY(SDLK_LCTRL,     mode, Event::JoystickZeroFire1, event);
      SET_DEFAULT_KEY(SDLK_4,         mode, Event::JoystickZeroFire2, event);
      SET_DEFAULT_KEY(SDLK_5,         mode, Event::JoystickZeroFire3, event);

      SET_DEFAULT_KEY(SDLK_y,         mode, Event::JoystickOneUp,     event);
      SET_DEFAULT_KEY(SDLK_h,         mode, Event::JoystickOneDown,   event);
      SET_DEFAULT_KEY(SDLK_g,         mode, Event::JoystickOneLeft,   event);
      SET_DEFAULT_KEY(SDLK_j,         mode, Event::JoystickOneRight,  event);
      SET_DEFAULT_KEY(SDLK_f,         mode, Event::JoystickOneFire1,  event);
      SET_DEFAULT_KEY(SDLK_6,         mode, Event::JoystickOneFire2,  event);
      SET_DEFAULT_KEY(SDLK_7,         mode, Event::JoystickOneFire3,  event);


      SET_DEFAULT_KEY(SDLK_F1,        mode, Event::ConsoleSelect,     event);
      SET_DEFAULT_KEY(SDLK_F2,        mode, Event::ConsoleReset,      event);
      SET_DEFAULT_KEY(SDLK_F3,        mode, Event::ConsoleColor,      event);
      SET_DEFAULT_KEY(SDLK_F4,        mode, Event::ConsoleBlackWhite, event);
      SET_DEFAULT_KEY(SDLK_F5,        mode, Event::ConsoleLeftDiffA,  event);
      SET_DEFAULT_KEY(SDLK_F6,        mode, Event::ConsoleLeftDiffB,  event);
      SET_DEFAULT_KEY(SDLK_F7,        mode, Event::ConsoleRightDiffA, event);
      SET_DEFAULT_KEY(SDLK_F8,        mode, Event::ConsoleRightDiffB, event);
      SET_DEFAULT_KEY(SDLK_F9,        mode, Event::SaveState,         event);
      SET_DEFAULT_KEY(SDLK_F10,       mode, Event::ChangeState,       event);
      SET_DEFAULT_KEY(SDLK_F11,       mode, Event::LoadState,         event);
      SET_DEFAULT_KEY(SDLK_F12,       mode, Event::TakeSnapshot,      event);
      SET_DEFAULT_KEY(SDLK_BACKSPACE, mode, Event::Fry,               event);
      SET_DEFAULT_KEY(SDLK_PAUSE,     mode, Event::PauseMode,         event);
      SET_DEFAULT_KEY(SDLK_TAB,       mode, Event::MenuMode,          event);
      SET_DEFAULT_KEY(SDLK_BACKSLASH, mode, Event::CmdMenuMode,       event);
      SET_DEFAULT_KEY(SDLK_BACKQUOTE, mode, Event::DebuggerMode,      event);
      SET_DEFAULT_KEY(SDLK_ESCAPE,    mode, Event::LauncherMode,      event);
      break;

    case kMenuMode:
      SET_DEFAULT_KEY(SDLK_UP,        mode, Event::UIUp,      event);
      SET_DEFAULT_KEY(SDLK_DOWN,      mode, Event::UIDown,    event);
      SET_DEFAULT_KEY(SDLK_LEFT,      mode, Event::UILeft,    event);
      SET_DEFAULT_KEY(SDLK_RIGHT,     mode, Event::UIRight,   event);

      SET_DEFAULT_KEY(SDLK_HOME,      mode, Event::UIHome,    event);
      SET_DEFAULT_KEY(SDLK_END,       mode, Event::UIEnd,     event);
      SET_DEFAULT_KEY(SDLK_PAGEUP,    mode, Event::UIPgUp,    event);
      SET_DEFAULT_KEY(SDLK_PAGEDOWN,  mode, Event::UIPgDown,  event);

      SET_DEFAULT_KEY(SDLK_RETURN,    mode, Event::UISelect,  event);
      SET_DEFAULT_KEY(SDLK_ESCAPE,    mode, Event::UICancel,  event);

      SET_DEFAULT_KEY(SDLK_BACKSPACE, mode, Event::UIPrevDir, event);
      break;

    default:
      return;
      break;
  }

  saveKeyMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoymap(Event::Type event, EventMode mode)
{
  // If event is 'NoType', erase and reset all mappings
  // Otherwise, only reset the given event
  if(event == Event::NoType)
  {
    // Erase all mappings
    for(int i = 0; i < kNumJoysticks; ++i)
      for(int j = 0; j < kNumJoyButtons; ++j)
        myJoyTable[i][j][mode] = Event::NoType;
  }

  myOSystem->setDefaultJoymap(event, mode);
  saveJoyMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoyAxisMap(Event::Type event, EventMode mode)
{
  // If event is 'NoType', erase and reset all mappings
  // Otherwise, only reset the given event
  if(event == Event::NoType)
  {
    // Erase all mappings
    for(int i = 0; i < kNumJoysticks; ++i)
      for(int j = 0; j < kNumJoyAxis; ++j)
        for(int k = 0; k < 2; ++k)
          myJoyAxisTable[i][j][k][mode] = Event::NoType;
  }

  myOSystem->setDefaultJoyAxisMap(event, mode);
  saveJoyAxisMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoyHatMap(Event::Type event, EventMode mode)
{
  // If event is 'NoType', erase and reset all mappings
  // Otherwise, only reset the given event
  if(event == Event::NoType)
  {
    // Erase all mappings
    for(int i = 0; i < kNumJoysticks; ++i)
      for(int j = 0; j < kNumJoyHats; ++j)
        for(int k = 0; k < 4; ++k)
          myJoyHatTable[i][j][k][mode] = Event::NoType;
  }

  myOSystem->setDefaultJoyHatMap(event, mode);
  saveJoyHatMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveKeyMapping()
{
  // Iterate through the keymap table and create a colon-separated list
  // Prepend the event count, so we can check it on next load
  ostringstream keybuf;
  keybuf << Event::LastType;
  for(int mode = 0; mode < kNumModes; ++mode)
    for(int i = 0; i < SDLK_LAST; ++i)
      keybuf << ":" << myKeyTable[i][mode];

  myOSystem->settings().setString("keymap", keybuf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveJoyMapping()
{
  // Iterate through the joymap table and create a colon-separated list
  // Prepend the event count, so we can check it on next load
  ostringstream buf;
  buf << Event::LastType;
  for(int mode = 0; mode < kNumModes; ++mode)
    for(int i = 0; i < kNumJoysticks; ++i)
      for(int j = 0; j < kNumJoyButtons; ++j)
        buf << ":" << myJoyTable[i][j][mode];

  myOSystem->settings().setString("joymap", buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveJoyAxisMapping()
{
  // Iterate through the joyaxismap table and create a colon-separated list
  // Prepend the event count, so we can check it on next load
  ostringstream buf;
  buf << Event::LastType;
  for(int mode = 0; mode < kNumModes; ++mode)
    for(int i = 0; i < kNumJoysticks; ++i)
      for(int j = 0; j < kNumJoyAxis; ++j)
        for(int k = 0; k < 2; ++k)
          buf << ":" << myJoyAxisTable[i][j][k][mode];

  myOSystem->settings().setString("joyaxismap", buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveJoyHatMapping()
{
  // Iterate through the joyhatmap table and create a colon-separated list
  // Prepend the event count, so we can check it on next load
  ostringstream buf;
  buf << Event::LastType;
  for(int mode = 0; mode < kNumModes; ++mode)
    for(int i = 0; i < kNumJoysticks; ++i)
      for(int j = 0; j < kNumJoyHats; ++j)
        for(int k = 0; k < 4; ++k)
          buf << ":" << myJoyHatTable[i][j][k][mode];

  myOSystem->settings().setString("joyhatmap", buf.str());
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
  myOSystem->settings().setString("combomap", buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::isValidList(string& list, IntArray& map, uInt32 length) const
{
  string key;
  Event::Type event;

  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  replace(list.begin(), list.end(), ':', ' ');
  istringstream buf(list);

  // Get event count, which should be the first int in the list
  buf >> key;
  event = (Event::Type) atoi(key.c_str());
  if(event == Event::LastType)
    while(buf >> key)
      map.push_back(atoi(key.c_str()));

  return (event == Event::LastType && map.size() == length);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool EventHandler::eventIsAnalog(Event::Type event) const
{
  switch((int)event)
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
StringMap EventHandler::getComboList(EventMode) const
{
  // For now, this only works in emulation mode

  StringMap l;
  ostringstream buf;

  l.push_back("None", "-1");
  for(uInt32 i = 0; i < kEmulActionListSize; ++i)
    if(EventHandler::ourEmulActionList[i].allow_combo)
    {
      buf << i;
      l.push_back(EventHandler::ourEmulActionList[i].action, buf.str());
      buf.str("");
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
      break;
    case kMenuMode:
      if(idx < 0 || idx >= kMenuActionListSize)
        return Event::NoType;
      else
        return ourMenuActionList[idx].event;
      break;
    default:
      return Event::NoType;
      break;
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
      break;
    case kMenuMode:
      if(idx < 0 || idx >= kMenuActionListSize)
        return EmptyString;
      else
        return ourMenuActionList[idx].action;
      break;
    default:
      return EmptyString;
      break;
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
      break;
    case kMenuMode:
      if(idx < 0 || idx >= kMenuActionListSize)
        return EmptyString;
      else
        return ourMenuActionList[idx].key;
      break;
    default:
      return EmptyString;
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::takeSnapshot(uInt32 number)
{
  // Figure out the correct snapshot name
  string filename;
  bool showmessage = number == 0;
  string sspath = myOSystem->snapshotDir() +
      myOSystem->console().properties().get(Cartridge_Name);

  // Check whether we want multiple snapshots created
  if(number > 0)
  {
    ostringstream buf;
    buf << sspath << "_" << hex << setw(8) << setfill('0') << number << ".png";
    filename = buf.str();
  }
  else if(!myOSystem->settings().getBool("sssingle"))
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

  // Now create a PNG snapshot
  if(myOSystem->settings().getBool("ss1x"))
  {
    string msg = Snapshot::savePNG(myOSystem->frameBuffer(),
                   myOSystem->console().tia(),
                   myOSystem->console().properties(), filename);
    if(showmessage)
      myOSystem->frameBuffer().showMessage(msg);
  }
  else
  {
    // Make sure we have a 'clean' image, with no onscreen messages
    myOSystem->frameBuffer().enableMessages(false);

    string msg = Snapshot::savePNG(myOSystem->frameBuffer(),
                   myOSystem->console().properties(), filename);

    // Re-enable old messages
    myOSystem->frameBuffer().enableMessages(true);
    if(showmessage)
      myOSystem->frameBuffer().showMessage(msg);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setMouseControllerMode(int num, bool showmessage)
{
  if(num >= 0 && num <= 3)
  {
    myMouseEnabled = true;
    Controller::setMouseIsController(num);
    if(showmessage)
    {
      ostringstream buf;
      buf << "Mouse is controller " << num;
      myOSystem->frameBuffer().showMessage(buf.str());
    }
  }
  else
    myMouseEnabled = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setContinuousSnapshots(uInt32 interval)
{
  myContSnapshotInterval = (uInt32) myOSystem->frameRate() * interval;
  myContSnapshotCounter = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::enterMenuMode(State state)
{
  setEventState(state);
  myOverlay->reStack();
  myOSystem->sound().mute(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::leaveMenuMode()
{
  setEventState(S_EMULATE);
  myOSystem->sound().mute(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::enterDebugMode()
{
#ifdef DEBUGGER_SUPPORT
  if(myState == S_DEBUGGER || !(&myOSystem->console()))
    return false;

  // Make sure debugger starts in a consistent state
  // This absolutely *has* to come before we actually change to debugger
  // mode, since it takes care of locking the debugger state, which will
  // probably be modified below
  myOSystem->debugger().setStartState();
  setEventState(S_DEBUGGER);

  FBInitStatus fbstatus = myOSystem->createFrameBuffer();
  if(fbstatus != kSuccess)
  {
    myOSystem->debugger().setQuitState();
    setEventState(S_EMULATE);
    if(fbstatus == kFailTooLarge)
      myOSystem->frameBuffer().showMessage("Debugger window too large for screen",
                                           kBottomCenter, true);
    return false;
  }
  myOverlay->reStack();
  myOSystem->sound().mute(true);
#else
  myOSystem->frameBuffer().showMessage("Debugger support not included",
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
  myOSystem->debugger().setQuitState();

  setEventState(S_EMULATE);
  myOSystem->createFrameBuffer();
  myOSystem->sound().mute(false);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setEventState(State state)
{
  myState = state;

  switch(myState)
  {
    case S_EMULATE:
      myOverlay = NULL;
      myOSystem->sound().mute(false);
      break;

    case S_PAUSE:
      myOSystem->sound().mute(true);
      break;

    case S_MENU:
      myOverlay = &myOSystem->menu();
      break;

    case S_CMDMENU:
      myOverlay = &myOSystem->commandMenu();
      break;

    case S_LAUNCHER:
      myOverlay = &myOSystem->launcher();
      break;

#ifdef DEBUGGER_SUPPORT
    case S_DEBUGGER:
      myOverlay = &myOSystem->debugger();
      break;
#endif

    default:
      myOverlay = NULL;
      break;
  }

  // Inform various subsystems about the new state
  myOSystem->stateChanged(myState);
  if(&myOSystem->frameBuffer())
  {
    myOSystem->frameBuffer().stateChanged(myState);
    myOSystem->frameBuffer().setCursorState();
  }

  // Always clear any pending events when changing states
  myEvent->clear();

  // Sometimes an extraneous mouse motion event is generated
  // after a state change, which should be supressed
  mySkipMouseMotion = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setSDLMappings()
{
  ourSDLMapping[ SDLK_BACKSPACE ]    = "BACKSPACE";
  ourSDLMapping[ SDLK_TAB ]          = "TAB";
  ourSDLMapping[ SDLK_CLEAR ]        = "CLEAR";
  ourSDLMapping[ SDLK_RETURN ]       = "RETURN";
  ourSDLMapping[ SDLK_PAUSE ]        = "PAUSE";
  ourSDLMapping[ SDLK_ESCAPE ]       = "ESCAPE";
  ourSDLMapping[ SDLK_SPACE ]        = "SPACE";
  ourSDLMapping[ SDLK_EXCLAIM ]      = "!";
  ourSDLMapping[ SDLK_QUOTEDBL ]     = "\"";
  ourSDLMapping[ SDLK_HASH ]         = "#";
  ourSDLMapping[ SDLK_DOLLAR ]       = "$";
  ourSDLMapping[ SDLK_AMPERSAND ]    = "&";
  ourSDLMapping[ SDLK_QUOTE ]        = "\'";
  ourSDLMapping[ SDLK_LEFTPAREN ]    = "(";
  ourSDLMapping[ SDLK_RIGHTPAREN ]   = ")";
  ourSDLMapping[ SDLK_ASTERISK ]     = "*";
  ourSDLMapping[ SDLK_PLUS ]         = "+";
  ourSDLMapping[ SDLK_COMMA ]        = "COMMA";
  ourSDLMapping[ SDLK_MINUS ]        = "-";
  ourSDLMapping[ SDLK_PERIOD ]       = ".";
  ourSDLMapping[ SDLK_SLASH ]        = "/";
  ourSDLMapping[ SDLK_0 ]            = "0";
  ourSDLMapping[ SDLK_1 ]            = "1";
  ourSDLMapping[ SDLK_2 ]            = "2";
  ourSDLMapping[ SDLK_3 ]            = "3";
  ourSDLMapping[ SDLK_4 ]            = "4";
  ourSDLMapping[ SDLK_5 ]            = "5";
  ourSDLMapping[ SDLK_6 ]            = "6";
  ourSDLMapping[ SDLK_7 ]            = "7";
  ourSDLMapping[ SDLK_8 ]            = "8";
  ourSDLMapping[ SDLK_9 ]            = "9";
  ourSDLMapping[ SDLK_COLON ]        = ":";
  ourSDLMapping[ SDLK_SEMICOLON ]    = ";";
  ourSDLMapping[ SDLK_LESS ]         = "<";
  ourSDLMapping[ SDLK_EQUALS ]       = "=";
  ourSDLMapping[ SDLK_GREATER ]      = ">";
  ourSDLMapping[ SDLK_QUESTION ]     = "?";
  ourSDLMapping[ SDLK_AT ]           = "@";
  ourSDLMapping[ SDLK_LEFTBRACKET ]  = "[";
  ourSDLMapping[ SDLK_BACKSLASH ]    = "\\";
  ourSDLMapping[ SDLK_RIGHTBRACKET ] = "]";
  ourSDLMapping[ SDLK_CARET ]        = "^";
  ourSDLMapping[ SDLK_UNDERSCORE ]   = "_";
  ourSDLMapping[ SDLK_BACKQUOTE ]    = "`";
  ourSDLMapping[ SDLK_a ]            = "A";
  ourSDLMapping[ SDLK_b ]            = "B";
  ourSDLMapping[ SDLK_c ]            = "C";
  ourSDLMapping[ SDLK_d ]            = "D";
  ourSDLMapping[ SDLK_e ]            = "E";
  ourSDLMapping[ SDLK_f ]            = "F";
  ourSDLMapping[ SDLK_g ]            = "G";
  ourSDLMapping[ SDLK_h ]            = "H";
  ourSDLMapping[ SDLK_i ]            = "I";
  ourSDLMapping[ SDLK_j ]            = "J";
  ourSDLMapping[ SDLK_k ]            = "K";
  ourSDLMapping[ SDLK_l ]            = "L";
  ourSDLMapping[ SDLK_m ]            = "M";
  ourSDLMapping[ SDLK_n ]            = "N";
  ourSDLMapping[ SDLK_o ]            = "O";
  ourSDLMapping[ SDLK_p ]            = "P";
  ourSDLMapping[ SDLK_q ]            = "Q";
  ourSDLMapping[ SDLK_r ]            = "R";
  ourSDLMapping[ SDLK_s ]            = "S";
  ourSDLMapping[ SDLK_t ]            = "T";
  ourSDLMapping[ SDLK_u ]            = "U";
  ourSDLMapping[ SDLK_v ]            = "V";
  ourSDLMapping[ SDLK_w ]            = "W";
  ourSDLMapping[ SDLK_x ]            = "X";
  ourSDLMapping[ SDLK_y ]            = "Y";
  ourSDLMapping[ SDLK_z ]            = "Z";
  ourSDLMapping[ SDLK_DELETE ]       = "DELETE";
  ourSDLMapping[ SDLK_WORLD_0 ]      = "WORLD_0";
  ourSDLMapping[ SDLK_WORLD_1 ]      = "WORLD_1";
  ourSDLMapping[ SDLK_WORLD_2 ]      = "WORLD_2";
  ourSDLMapping[ SDLK_WORLD_3 ]      = "WORLD_3";
  ourSDLMapping[ SDLK_WORLD_4 ]      = "WORLD_4";
  ourSDLMapping[ SDLK_WORLD_5 ]      = "WORLD_5";
  ourSDLMapping[ SDLK_WORLD_6 ]      = "WORLD_6";
  ourSDLMapping[ SDLK_WORLD_7 ]      = "WORLD_7";
  ourSDLMapping[ SDLK_WORLD_8 ]      = "WORLD_8";
  ourSDLMapping[ SDLK_WORLD_9 ]      = "WORLD_9";
  ourSDLMapping[ SDLK_WORLD_10 ]     = "WORLD_10";
  ourSDLMapping[ SDLK_WORLD_11 ]     = "WORLD_11";
  ourSDLMapping[ SDLK_WORLD_12 ]     = "WORLD_12";
  ourSDLMapping[ SDLK_WORLD_13 ]     = "WORLD_13";
  ourSDLMapping[ SDLK_WORLD_14 ]     = "WORLD_14";
  ourSDLMapping[ SDLK_WORLD_15 ]     = "WORLD_15";
  ourSDLMapping[ SDLK_WORLD_16 ]     = "WORLD_16";
  ourSDLMapping[ SDLK_WORLD_17 ]     = "WORLD_17";
  ourSDLMapping[ SDLK_WORLD_18 ]     = "WORLD_18";
  ourSDLMapping[ SDLK_WORLD_19 ]     = "WORLD_19";
  ourSDLMapping[ SDLK_WORLD_20 ]     = "WORLD_20";
  ourSDLMapping[ SDLK_WORLD_21 ]     = "WORLD_21";
  ourSDLMapping[ SDLK_WORLD_22 ]     = "WORLD_22";
  ourSDLMapping[ SDLK_WORLD_23 ]     = "WORLD_23";
  ourSDLMapping[ SDLK_WORLD_24 ]     = "WORLD_24";
  ourSDLMapping[ SDLK_WORLD_25 ]     = "WORLD_25";
  ourSDLMapping[ SDLK_WORLD_26 ]     = "WORLD_26";
  ourSDLMapping[ SDLK_WORLD_27 ]     = "WORLD_27";
  ourSDLMapping[ SDLK_WORLD_28 ]     = "WORLD_28";
  ourSDLMapping[ SDLK_WORLD_29 ]     = "WORLD_29";
  ourSDLMapping[ SDLK_WORLD_30 ]     = "WORLD_30";
  ourSDLMapping[ SDLK_WORLD_31 ]     = "WORLD_31";
  ourSDLMapping[ SDLK_WORLD_32 ]     = "WORLD_32";
  ourSDLMapping[ SDLK_WORLD_33 ]     = "WORLD_33";
  ourSDLMapping[ SDLK_WORLD_34 ]     = "WORLD_34";
  ourSDLMapping[ SDLK_WORLD_35 ]     = "WORLD_35";
  ourSDLMapping[ SDLK_WORLD_36 ]     = "WORLD_36";
  ourSDLMapping[ SDLK_WORLD_37 ]     = "WORLD_37";
  ourSDLMapping[ SDLK_WORLD_38 ]     = "WORLD_38";
  ourSDLMapping[ SDLK_WORLD_39 ]     = "WORLD_39";
  ourSDLMapping[ SDLK_WORLD_40 ]     = "WORLD_40";
  ourSDLMapping[ SDLK_WORLD_41 ]     = "WORLD_41";
  ourSDLMapping[ SDLK_WORLD_42 ]     = "WORLD_42";
  ourSDLMapping[ SDLK_WORLD_43 ]     = "WORLD_43";
  ourSDLMapping[ SDLK_WORLD_44 ]     = "WORLD_44";
  ourSDLMapping[ SDLK_WORLD_45 ]     = "WORLD_45";
  ourSDLMapping[ SDLK_WORLD_46 ]     = "WORLD_46";
  ourSDLMapping[ SDLK_WORLD_47 ]     = "WORLD_47";
  ourSDLMapping[ SDLK_WORLD_48 ]     = "WORLD_48";
  ourSDLMapping[ SDLK_WORLD_49 ]     = "WORLD_49";
  ourSDLMapping[ SDLK_WORLD_50 ]     = "WORLD_50";
  ourSDLMapping[ SDLK_WORLD_51 ]     = "WORLD_51";
  ourSDLMapping[ SDLK_WORLD_52 ]     = "WORLD_52";
  ourSDLMapping[ SDLK_WORLD_53 ]     = "WORLD_53";
  ourSDLMapping[ SDLK_WORLD_54 ]     = "WORLD_54";
  ourSDLMapping[ SDLK_WORLD_55 ]     = "WORLD_55";
  ourSDLMapping[ SDLK_WORLD_56 ]     = "WORLD_56";
  ourSDLMapping[ SDLK_WORLD_57 ]     = "WORLD_57";
  ourSDLMapping[ SDLK_WORLD_58 ]     = "WORLD_58";
  ourSDLMapping[ SDLK_WORLD_59 ]     = "WORLD_59";
  ourSDLMapping[ SDLK_WORLD_60 ]     = "WORLD_60";
  ourSDLMapping[ SDLK_WORLD_61 ]     = "WORLD_61";
  ourSDLMapping[ SDLK_WORLD_62 ]     = "WORLD_62";
  ourSDLMapping[ SDLK_WORLD_63 ]     = "WORLD_63";
  ourSDLMapping[ SDLK_WORLD_64 ]     = "WORLD_64";
  ourSDLMapping[ SDLK_WORLD_65 ]     = "WORLD_65";
  ourSDLMapping[ SDLK_WORLD_66 ]     = "WORLD_66";
  ourSDLMapping[ SDLK_WORLD_67 ]     = "WORLD_67";
  ourSDLMapping[ SDLK_WORLD_68 ]     = "WORLD_68";
  ourSDLMapping[ SDLK_WORLD_69 ]     = "WORLD_69";
  ourSDLMapping[ SDLK_WORLD_70 ]     = "WORLD_70";
  ourSDLMapping[ SDLK_WORLD_71 ]     = "WORLD_71";
  ourSDLMapping[ SDLK_WORLD_72 ]     = "WORLD_72";
  ourSDLMapping[ SDLK_WORLD_73 ]     = "WORLD_73";
  ourSDLMapping[ SDLK_WORLD_74 ]     = "WORLD_74";
  ourSDLMapping[ SDLK_WORLD_75 ]     = "WORLD_75";
  ourSDLMapping[ SDLK_WORLD_76 ]     = "WORLD_76";
  ourSDLMapping[ SDLK_WORLD_77 ]     = "WORLD_77";
  ourSDLMapping[ SDLK_WORLD_78 ]     = "WORLD_78";
  ourSDLMapping[ SDLK_WORLD_79 ]     = "WORLD_79";
  ourSDLMapping[ SDLK_WORLD_80 ]     = "WORLD_80";
  ourSDLMapping[ SDLK_WORLD_81 ]     = "WORLD_81";
  ourSDLMapping[ SDLK_WORLD_82 ]     = "WORLD_82";
  ourSDLMapping[ SDLK_WORLD_83 ]     = "WORLD_83";
  ourSDLMapping[ SDLK_WORLD_84 ]     = "WORLD_84";
  ourSDLMapping[ SDLK_WORLD_85 ]     = "WORLD_85";
  ourSDLMapping[ SDLK_WORLD_86 ]     = "WORLD_86";
  ourSDLMapping[ SDLK_WORLD_87 ]     = "WORLD_87";
  ourSDLMapping[ SDLK_WORLD_88 ]     = "WORLD_88";
  ourSDLMapping[ SDLK_WORLD_89 ]     = "WORLD_89";
  ourSDLMapping[ SDLK_WORLD_90 ]     = "WORLD_90";
  ourSDLMapping[ SDLK_WORLD_91 ]     = "WORLD_91";
  ourSDLMapping[ SDLK_WORLD_92 ]     = "WORLD_92";
  ourSDLMapping[ SDLK_WORLD_93 ]     = "WORLD_93";
  ourSDLMapping[ SDLK_WORLD_94 ]     = "WORLD_94";
  ourSDLMapping[ SDLK_WORLD_95 ]     = "WORLD_95";
  ourSDLMapping[ SDLK_KP0 ]          = "KP0";
  ourSDLMapping[ SDLK_KP1 ]          = "KP1";
  ourSDLMapping[ SDLK_KP2 ]          = "KP2";
  ourSDLMapping[ SDLK_KP3 ]          = "KP3";
  ourSDLMapping[ SDLK_KP4 ]          = "KP4";
  ourSDLMapping[ SDLK_KP5 ]          = "KP5";
  ourSDLMapping[ SDLK_KP6 ]          = "KP6";
  ourSDLMapping[ SDLK_KP7 ]          = "KP7";
  ourSDLMapping[ SDLK_KP8 ]          = "KP8";
  ourSDLMapping[ SDLK_KP9 ]          = "KP9";
  ourSDLMapping[ SDLK_KP_PERIOD ]    = "KP .";
  ourSDLMapping[ SDLK_KP_DIVIDE ]    = "KP /";
  ourSDLMapping[ SDLK_KP_MULTIPLY ]  = "KP *";
  ourSDLMapping[ SDLK_KP_MINUS ]     = "KP -";
  ourSDLMapping[ SDLK_KP_PLUS ]      = "KP +";
  ourSDLMapping[ SDLK_KP_ENTER ]     = "KP ENTER";
  ourSDLMapping[ SDLK_KP_EQUALS ]    = "KP =";
  ourSDLMapping[ SDLK_UP ]           = "UP";
  ourSDLMapping[ SDLK_DOWN ]         = "DOWN";
  ourSDLMapping[ SDLK_RIGHT ]        = "RIGHT";
  ourSDLMapping[ SDLK_LEFT ]         = "LEFT";
  ourSDLMapping[ SDLK_INSERT ]       = "INS";
  ourSDLMapping[ SDLK_HOME ]         = "HOME";
  ourSDLMapping[ SDLK_END ]          = "END";
  ourSDLMapping[ SDLK_PAGEUP ]       = "PGUP";
  ourSDLMapping[ SDLK_PAGEDOWN ]     = "PGDN";
  ourSDLMapping[ SDLK_F1 ]           = "F1";
  ourSDLMapping[ SDLK_F2 ]           = "F2";
  ourSDLMapping[ SDLK_F3 ]           = "F3";
  ourSDLMapping[ SDLK_F4 ]           = "F4";
  ourSDLMapping[ SDLK_F5 ]           = "F5";
  ourSDLMapping[ SDLK_F6 ]           = "F6";
  ourSDLMapping[ SDLK_F7 ]           = "F7";
  ourSDLMapping[ SDLK_F8 ]           = "F8";
  ourSDLMapping[ SDLK_F9 ]           = "F9";
  ourSDLMapping[ SDLK_F10 ]          = "F10";
  ourSDLMapping[ SDLK_F11 ]          = "F11";
  ourSDLMapping[ SDLK_F12 ]          = "F12";
  ourSDLMapping[ SDLK_F13 ]          = "F13";
  ourSDLMapping[ SDLK_F14 ]          = "F14";
  ourSDLMapping[ SDLK_F15 ]          = "F15";
  ourSDLMapping[ SDLK_NUMLOCK ]      = "NUMLOCK";
  ourSDLMapping[ SDLK_CAPSLOCK ]     = "CAPSLOCK";
  ourSDLMapping[ SDLK_SCROLLOCK ]    = "SCROLLOCK";
  ourSDLMapping[ SDLK_RSHIFT ]       = "RSHIFT";
  ourSDLMapping[ SDLK_LSHIFT ]       = "LSHIFT";
  ourSDLMapping[ SDLK_RCTRL ]        = "RCTRL";
  ourSDLMapping[ SDLK_LCTRL ]        = "LCTRL";
  ourSDLMapping[ SDLK_RALT ]         = "RALT";
  ourSDLMapping[ SDLK_LALT ]         = "LALT";
  ourSDLMapping[ SDLK_RMETA ]        = "RMETA";
  ourSDLMapping[ SDLK_LMETA ]        = "LMETA";
  ourSDLMapping[ SDLK_LSUPER ]       = "LSUPER";
  ourSDLMapping[ SDLK_RSUPER ]       = "RSUPER";
  ourSDLMapping[ SDLK_MODE ]         = "MODE";
  ourSDLMapping[ SDLK_COMPOSE ]      = "COMPOSE";
  ourSDLMapping[ SDLK_HELP ]         = "HELP";
  ourSDLMapping[ SDLK_PRINT ]        = "PRINT";
  ourSDLMapping[ SDLK_SYSREQ ]       = "SYSREQ";
  ourSDLMapping[ SDLK_BREAK ]        = "BREAK";
  ourSDLMapping[ SDLK_MENU ]         = "MENU";
  ourSDLMapping[ SDLK_POWER ]        = "POWER";
  ourSDLMapping[ SDLK_EURO ]         = "EURO";
  ourSDLMapping[ SDLK_UNDO ]         = "UNDO";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::ActionList EventHandler::ourEmulActionList[kEmulActionListSize] = {
  { Event::ConsoleSelect,       "Select",                      0, true  },
  { Event::ConsoleReset,        "Reset",                       0, true  },
  { Event::ConsoleColor,        "Color TV",                    0, true  },
  { Event::ConsoleBlackWhite,   "Black & White TV",            0, true  },
  { Event::ConsoleLeftDiffA,    "P0 Difficulty A",             0, true  },
  { Event::ConsoleLeftDiffB,    "P0 Difficulty B",             0, true  },
  { Event::ConsoleRightDiffA,   "P1 Difficulty A",             0, true  },
  { Event::ConsoleRightDiffB,   "P1 Difficulty B",             0, true  },
  { Event::SaveState,           "Save State",                  0, false },
  { Event::ChangeState,         "Change State",                0, false },
  { Event::LoadState,           "Load State",                  0, false },
  { Event::TakeSnapshot,        "Snapshot",                    0, false },
  { Event::Fry,                 "Fry cartridge",               0, false },
  { Event::VolumeDecrease,      "Decrease volume",             0, false },
  { Event::VolumeIncrease,      "Increase volume",             0, false },
  { Event::PauseMode,           "Pause",                       0, false },
  { Event::MenuMode,            "Enter options menu mode",     0, false },
  { Event::CmdMenuMode,         "Toggle command menu mode",    0, false },
  { Event::DebuggerMode,        "Toggle debugger mode",        0, false },
  { Event::LauncherMode,        "Enter ROM launcher",          0, false },
  { Event::Quit,                "Quit",                        0, false },

  { Event::JoystickZeroUp,      "P0 Joystick Up",              0, true  },
  { Event::JoystickZeroDown,    "P0 Joystick Down",            0, true  },
  { Event::JoystickZeroLeft,    "P0 Joystick Left",            0, true  },
  { Event::JoystickZeroRight,   "P0 Joystick Right",           0, true  },
  { Event::JoystickZeroFire1,   "P0 Joystick Fire",            0, true  },
  { Event::JoystickZeroFire2,   "P0 BoosterGrip Trigger",      0, true  },
  { Event::JoystickZeroFire3,   "P0 BoosterGrip Booster",      0, true  },

  { Event::JoystickOneUp,       "P1 Joystick Up",              0, true  },
  { Event::JoystickOneDown,     "P1 Joystick Down",            0, true  },
  { Event::JoystickOneLeft,     "P1 Joystick Left",            0, true  },
  { Event::JoystickOneRight,    "P1 Joystick Right",           0, true  },
  { Event::JoystickOneFire1,    "P1 Joystick Fire",            0, true  },
  { Event::JoystickOneFire2,    "P1 BoosterGrip Trigger",      0, true  },
  { Event::JoystickOneFire3,    "P1 BoosterGrip Booster",      0, true  },

  { Event::PaddleZeroAnalog,    "Paddle 0 Analog",             0, true  },
  { Event::PaddleZeroDecrease,  "Paddle 0 Decrease",           0, true  },
  { Event::PaddleZeroIncrease,  "Paddle 0 Increase",           0, true  },
  { Event::PaddleZeroFire,      "Paddle 0 Fire",               0, true  },

  { Event::PaddleOneAnalog,     "Paddle 1 Analog",             0, true  },
  { Event::PaddleOneDecrease,   "Paddle 1 Decrease",           0, true  },
  { Event::PaddleOneIncrease,   "Paddle 1 Increase",           0, true  },
  { Event::PaddleOneFire,       "Paddle 1 Fire",               0, true  },

  { Event::PaddleTwoAnalog,     "Paddle 2 Analog",             0, true  },
  { Event::PaddleTwoDecrease,   "Paddle 2 Decrease",           0, true  },
  { Event::PaddleTwoIncrease,   "Paddle 2 Increase",           0, true  },
  { Event::PaddleTwoFire,       "Paddle 2 Fire",               0, true  },

  { Event::PaddleThreeAnalog,   "Paddle 3 Analog",             0, true  },
  { Event::PaddleThreeDecrease, "Paddle 3 Decrease",           0, true  },
  { Event::PaddleThreeIncrease, "Paddle 3 Increase",           0, true  },
  { Event::PaddleThreeFire,     "Paddle 3 Fire",               0, true  },

  { Event::KeyboardZero1,       "P0 Keyboard 1",               0, true  },
  { Event::KeyboardZero2,       "P0 Keyboard 2",               0, true  },
  { Event::KeyboardZero3,       "P0 Keyboard 3",               0, true  },
  { Event::KeyboardZero4,       "P0 Keyboard 4",               0, true  },
  { Event::KeyboardZero5,       "P0 Keyboard 5",               0, true  },
  { Event::KeyboardZero6,       "P0 Keyboard 6",               0, true  },
  { Event::KeyboardZero7,       "P0 Keyboard 7",               0, true  },
  { Event::KeyboardZero8,       "P0 Keyboard 8",               0, true  },
  { Event::KeyboardZero9,       "P0 Keyboard 9",               0, true  },
  { Event::KeyboardZeroStar,    "P0 Keyboard *",               0, true  },
  { Event::KeyboardZero0,       "P0 Keyboard 0",               0, true  },
  { Event::KeyboardZeroPound,   "P0 Keyboard #",               0, true  },

  { Event::KeyboardOne1,        "P1 Keyboard 1",               0, true  },
  { Event::KeyboardOne2,        "P1 Keyboard 2",               0, true  },
  { Event::KeyboardOne3,        "P1 Keyboard 3",               0, true  },
  { Event::KeyboardOne4,        "P1 Keyboard 4",               0, true  },
  { Event::KeyboardOne5,        "P1 Keyboard 5",               0, true  },
  { Event::KeyboardOne6,        "P1 Keyboard 6",               0, true  },
  { Event::KeyboardOne7,        "P1 Keyboard 7",               0, true  },
  { Event::KeyboardOne8,        "P1 Keyboard 8",               0, true  },
  { Event::KeyboardOne9,        "P1 Keyboard 9",               0, true  },
  { Event::KeyboardOneStar,     "P1 Keyboard *",               0, true  },
  { Event::KeyboardOne0,        "P1 Keyboard 0",               0, true  },
  { Event::KeyboardOnePound,    "P1 Keyboard #",               0, true  },

  { Event::Combo1,              "Combo 1",                     0, false },
  { Event::Combo2,              "Combo 2",                     0, false },
  { Event::Combo3,              "Combo 3",                     0, false },
  { Event::Combo4,              "Combo 4",                     0, false },
  { Event::Combo5,              "Combo 5",                     0, false },
  { Event::Combo6,              "Combo 6",                     0, false },
  { Event::Combo7,              "Combo 7",                     0, false },
  { Event::Combo8,              "Combo 8",                     0, false },
  { Event::Combo9,              "Combo 9",                     0, false },
  { Event::Combo10,             "Combo 10",                    0, false },
  { Event::Combo11,             "Combo 11",                    0, false },
  { Event::Combo12,             "Combo 12",                    0, false },
  { Event::Combo13,             "Combo 13",                    0, false },
  { Event::Combo14,             "Combo 14",                    0, false },
  { Event::Combo15,             "Combo 15",                    0, false },
  { Event::Combo16,             "Combo 16",                    0, false }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::ActionList EventHandler::ourMenuActionList[kMenuActionListSize] = {
  { Event::UIUp,        "Move Up",              0, false },
  { Event::UIDown,      "Move Down",            0, false },
  { Event::UILeft,      "Move Left",            0, false },
  { Event::UIRight,     "Move Right",           0, false },

  { Event::UIHome,      "Home",                 0, false },
  { Event::UIEnd,       "End",                  0, false },
  { Event::UIPgUp,      "Page Up",              0, false },
  { Event::UIPgDown,    "Page Down",            0, false },

  { Event::UIOK,        "OK",                   0, false },
  { Event::UICancel,    "Cancel",               0, false },
  { Event::UISelect,    "Select item",          0, false },

  { Event::UINavPrev,   "Previous object",      0, false },
  { Event::UINavNext,   "Next object",          0, false },

  { Event::UIPrevDir,   "Parent directory",     0, false }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used by the Stelladaptor to send absolute axis values
const Event::Type EventHandler::SA_Axis[2][2] = {
  { Event::SALeftAxis0Value,  Event::SALeftAxis1Value  },
  { Event::SARightAxis0Value, Event::SARightAxis1Value }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used by the Stelladaptor to map button presses to joystick or paddles
//  (driving controllers are considered the same as joysticks)
const Event::Type EventHandler::SA_Button[2][2] = {
  { Event::JoystickZeroFire1, Event::JoystickZeroFire3 },
  { Event::JoystickOneFire1,  Event::JoystickOneFire3  }
};
