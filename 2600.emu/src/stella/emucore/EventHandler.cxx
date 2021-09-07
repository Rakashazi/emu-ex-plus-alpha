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

#include <sstream>
#include <map>

#include "Logger.hxx"

#include "Base.hxx"
#include "Console.hxx"
#include "PaletteHandler.hxx"
#include "FrameBuffer.hxx"
#include "FSNode.hxx"
#include "OSystem.hxx"
#include "Joystick.hxx"
#include "Paddles.hxx"
#include "Lightgun.hxx"
#include "PointingDevice.hxx"
#include "Driving.hxx"
#include "PropsSet.hxx"
#include "Settings.hxx"
#include "Sound.hxx"
#include "StateManager.hxx"
#include "RewindManager.hxx"
#include "TimerManager.hxx"
#ifdef GUI_SUPPORT
#include "HighScoresManager.hxx"
#endif
#include "Switches.hxx"
#include "M6532.hxx"
#include "MouseControl.hxx"
#include "PNGLibrary.hxx"
#include "TIASurface.hxx"

#include "EventHandler.hxx"

#ifdef CHEATCODE_SUPPORT
  #include "Cheat.hxx"
  #include "CheatManager.hxx"
#endif
#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif
#ifdef GUI_SUPPORT
  #include "Menu.hxx"
  #include "CommandMenu.hxx"
  #include "HighScoresMenu.hxx"
  #include "MessageMenu.hxx"
  #include "DialogContainer.hxx"
  #include "Launcher.hxx"
  #include "TimeMachine.hxx"
  #include "FileListWidget.hxx"
  #include "ScrollBarWidget.hxx"
#endif

using namespace std::placeholders;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::EventHandler(OSystem& osystem)
  : myOSystem{osystem}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::~EventHandler()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::initialize()
{
  // Create keyboard handler (to handle all physical keyboard functionality)
  myPKeyHandler = make_unique<PhysicalKeyboardHandler>(myOSystem, *this);

  // Create joystick handler (to handle all physical joystick functionality)
  myPJoyHandler = make_unique<PhysicalJoystickHandler>(myOSystem, *this, myEvent);

  // Erase the 'combo' array
  for(int i = 0; i < COMBO_SIZE; ++i)
    for(int j = 0; j < EVENTS_PER_COMBO; ++j)
      myComboTable[i][j] = Event::NoType;

  // Make sure the event/action mappings are correctly set,
  // and fill the ActionList structure with valid values
  setComboMap();
  setActionMappings(EventMode::kEmulationMode);
  setActionMappings(EventMode::kMenuMode);

  Joystick::setDeadZone(myOSystem.settings().getInt("joydeadzone"));
  Paddles::setDejitterBase(myOSystem.settings().getInt("dejitter.base"));
  Paddles::setDejitterDiff(myOSystem.settings().getInt("dejitter.diff"));
  Paddles::setDigitalSensitivity(myOSystem.settings().getInt("dsense"));
  Paddles::setMouseSensitivity(myOSystem.settings().getInt("msense"));
  PointingDevice::setSensitivity(myOSystem.settings().getInt("tsense"));
  Driving::setSensitivity(myOSystem.settings().getInt("dcsense"));
  Controller::setAutoFireRate(myOSystem.settings().getInt("autofirerate"));

#ifdef GUI_SUPPORT
  // Set quick select delay when typing characters in listwidgets
  FileListWidget::setQuickSelectDelay(myOSystem.settings().getInt("listdelay"));

  // Set number of lines a mousewheel will scroll
  ScrollBarWidget::setWheelLines(myOSystem.settings().getInt("mwheel"));

  // Mouse double click
  DialogContainer::setDoubleClickDelay(myOSystem.settings().getInt("mdouble"));

  // Input delay
  DialogContainer::setControllerDelay(myOSystem.settings().getInt("inpDelay"));

  // Input rate
  DialogContainer::setControllerRate(myOSystem.settings().getInt("inpRate"));
#endif

  // Integer to string conversions (for HEX) use upper or lower-case
  Common::Base::setHexUppercase(myOSystem.settings().getBool("dbg.uhex"));

  // Default phosphor blend
  Properties::setDefault(PropType::Display_PPBlend,
                         myOSystem.settings().getString("tv.phosblend"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::reset(EventHandlerState state)
{
  setState(state);
  myOSystem.state().reset();
#ifdef PNG_SUPPORT
  myOSystem.png().setContinuousSnapInterval(0);
#endif
  myFryingFlag = false;

  // Reset events almost immediately after starting emulation mode
  // We wait a little while (0.5s), since 'hold' events may be present,
  // and we want time for the ROM to process them
  if(state == EventHandlerState::EMULATION)
    myOSystem.timer().setTimeout([&ev = myEvent]() { ev.clear(); }, 500);
  // Toggle 7800 mode
  set7800Mode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::addPhysicalJoystick(const PhysicalJoystickPtr& joy)
{
#ifdef JOYSTICK_SUPPORT
  int ID = myPJoyHandler->add(joy);
  if(ID < 0)
    return;

  setActionMappings(EventMode::kEmulationMode);
  setActionMappings(EventMode::kMenuMode);

  ostringstream buf;
  buf << "Added joystick " << ID << ":" << endl
      << "  " << joy->about() << endl;
  Logger::info(buf.str());
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::removePhysicalJoystick(int id)
{
#ifdef JOYSTICK_SUPPORT
  myPJoyHandler->remove(id);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::mapStelladaptors(const string& saport)
{
#ifdef JOYSTICK_SUPPORT
  myPJoyHandler->mapStelladaptors(saport);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::toggleAllow4JoyDirections(bool toggle)
{
  bool joyAllow4 = myOSystem.settings().getBool("joyallow4");

  if(toggle)
  {
    joyAllow4 = !joyAllow4;
    allowAllDirections(joyAllow4);
    myOSystem.settings().setValue("joyallow4", joyAllow4);
  }

  ostringstream ss;
  ss << "Allow all 4 joystick directions ";
  ss << (joyAllow4 ? "enabled" : "disabled");
  myOSystem.frameBuffer().showTextMessage(ss.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::toggleSAPortOrder(bool toggle)
{
#ifdef JOYSTICK_SUPPORT
  string saport = myOSystem.settings().getString("saport");

  if(toggle)
  {
    if(saport == "lr")
      saport = "rl";
    else
      saport = "lr";
    mapStelladaptors(saport);
  }

  if(saport == "lr")
    myOSystem.frameBuffer().showTextMessage("Stelladaptor ports left/right");
  else
    myOSystem.frameBuffer().showTextMessage("Stelladaptor ports right/left");
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::set7800Mode()
{
  if(myOSystem.hasConsole())
    myIs7800 = myOSystem.console().switches().check7800Mode(myOSystem.settings());
  else
    myIs7800 = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::changeMouseControl(int direction)
{
  if(myMouseControl)
    myOSystem.frameBuffer().showTextMessage(myMouseControl->change(direction));
  else
    myOSystem.frameBuffer().showTextMessage("Mouse input is disabled");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::hasMouseControl() const
{
  return myMouseControl && myMouseControl->hasMouseControl();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::poll(uInt64 time)
{
  // Process events from the underlying hardware
  pollEvent();

  // Update controllers and console switches, and in general all other things
  // related to emulation
  if(myState == EventHandlerState::EMULATION)
  {
    myOSystem.console().riot().update();

    // Now check if the StateManager should be saving or loading state
    // (for rewind and/or movies
    if(myOSystem.state().mode() != StateManager::Mode::Off)
      myOSystem.state().update();

  #ifdef CHEATCODE_SUPPORT
    for(auto& cheat: myOSystem.cheat().perFrame())
      cheat->evaluate();
  #endif

  #ifdef PNG_SUPPORT
    // Handle continuous snapshots
    if(myOSystem.png().continuousSnapEnabled())
      myOSystem.png().updateTime(time);
  #endif
  }
#ifdef GUI_SUPPORT
  else if(myOverlay)
  {
    // Update the current dialog container at regular intervals
    // Used to implement continuous events
    myOverlay->updateTime(time);
  }
#endif

  // Turn off all mouse-related items; if they haven't been taken care of
  // in the previous ::update() methods, they're now invalid
  myEvent.set(Event::MouseAxisXMove, 0);
  myEvent.set(Event::MouseAxisYMove, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleTextEvent(char text)
{
#ifdef GUI_SUPPORT
  // Text events are only used in GUI mode
  if(myOverlay)
    myOverlay->handleTextEvent(text);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleMouseMotionEvent(int x, int y, int xrel, int yrel)
{
  // Determine which mode we're in, then send the event to the appropriate place
  if(myState == EventHandlerState::EMULATION)
  {
    if(!mySkipMouseMotion)
    {
      myEvent.set(Event::MouseAxisXValue, x); // required for Lightgun controller
      myEvent.set(Event::MouseAxisYValue, y); // required for Lightgun controller
      myEvent.set(Event::MouseAxisXMove, xrel);
      myEvent.set(Event::MouseAxisYMove, yrel);
    }
    mySkipMouseMotion = false;
  }
#ifdef GUI_SUPPORT
  else if(myOverlay)
    myOverlay->handleMouseMotionEvent(x, y);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleMouseButtonEvent(MouseButton b, bool pressed,
                                          int x, int y)
{
  // Determine which mode we're in, then send the event to the appropriate place
  if(myState == EventHandlerState::EMULATION)
  {
    switch(b)
    {
      case MouseButton::LEFT:
        myEvent.set(Event::MouseButtonLeftValue, int(pressed));
        break;
      case MouseButton::RIGHT:
        myEvent.set(Event::MouseButtonRightValue, int(pressed));
        break;
      default:
        return;
    }
  }
#ifdef GUI_SUPPORT
  else if(myOverlay)
    myOverlay->handleMouseButtonEvent(b, pressed, x, y);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleSystemEvent(SystemEvent e, int, int)
{
  switch(e)
  {
    case SystemEvent::WINDOW_EXPOSED:
    case SystemEvent::WINDOW_RESIZED:
      // Force full render update
      myOSystem.frameBuffer().update(FrameBuffer::UpdateMode::RERENDER);
      break;
#ifdef BSPF_UNIX
    case SystemEvent::WINDOW_FOCUS_GAINED:
      // Used to handle Alt-x key combos; sometimes the key associated with
      // Alt gets 'stuck'  and is passed to the core for processing
      if(myPKeyHandler->altKeyCount() > 0)
        myPKeyHandler->altKeyCount() = 2;
      break;
#endif
#if 0
    case SystemEvent::WINDOW_MINIMIZED:
      if(myState == EventHandlerState::EMULATION)
        enterMenuMode(EventHandlerState::OPTIONSMENU);
      break;
#endif
    default:  // handle other events as testing requires
      // cerr << "handleSystemEvent: " << e << endl;
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::AdjustGroup EventHandler::getAdjustGroup()
{
  if (myAdjustSetting >= AdjustSetting::START_DEBUG_ADJ && myAdjustSetting <= AdjustSetting::END_DEBUG_ADJ)
    return AdjustGroup::DEBUG;
  if(myAdjustSetting >= AdjustSetting::START_INPUT_ADJ && myAdjustSetting <= AdjustSetting::END_INPUT_ADJ)
    return AdjustGroup::INPUT;

  return AdjustGroup::AV;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::isJoystick(const Controller& controller) const
{
  return controller.type() == Controller::Type::Joystick
    || controller.type() == Controller::Type::BoosterGrip
    || controller.type() == Controller::Type::Genesis;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::isPaddle(const Controller& controller) const
{
  return controller.type() == Controller::Type::Paddles
    || controller.type() == Controller::Type::PaddlesIAxDr
    || controller.type() == Controller::Type::PaddlesIAxis;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::isTrackball(const Controller& controller) const
{
  return controller.type() == Controller::Type::AmigaMouse
    || controller.type() == Controller::Type::AtariMouse
    || controller.type() == Controller::Type::TrakBall;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::skipAVSetting() const
{
  const bool isFullScreen = myOSystem.frameBuffer().fullScreen();
  const bool isCustomPalette =
    myOSystem.settings().getString("palette") == PaletteHandler::SETTING_CUSTOM;
  const bool isCustomFilter =
    myOSystem.settings().getInt("tv.filter") == int(NTSCFilter::Preset::CUSTOM);

  return (myAdjustSetting == AdjustSetting::OVERSCAN && !isFullScreen)
  #ifdef ADAPTABLE_REFRESH_SUPPORT
    || (myAdjustSetting == AdjustSetting::ADAPT_REFRESH && !isFullScreen)
  #endif
    || (myAdjustSetting >= AdjustSetting::PALETTE_PHASE
        && myAdjustSetting <= AdjustSetting::PALETTE_BLUE_SHIFT
        && !isCustomPalette)
    || (myAdjustSetting >= AdjustSetting::NTSC_SHARPNESS
        && myAdjustSetting <= AdjustSetting::NTSC_BLEEDING
        && !isCustomFilter);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::skipInputSetting() const
{
  const bool grabMouseAllowed = myOSystem.frameBuffer().grabMouseAllowed();
  const bool analog = myOSystem.console().leftController().isAnalog()
    || myOSystem.console().rightController().isAnalog();
  const bool joystick = isJoystick(myOSystem.console().leftController())
    || isJoystick(myOSystem.console().rightController());
  const bool paddle = isPaddle(myOSystem.console().leftController())
    || isPaddle(myOSystem.console().rightController());
  const bool trackball = isTrackball(myOSystem.console().leftController())
    || isTrackball(myOSystem.console().rightController());
  const bool driving =
    myOSystem.console().leftController().type() == Controller::Type::Driving
    || myOSystem.console().rightController().type() == Controller::Type::Driving;
  const bool useMouse =
    BSPF::equalsIgnoreCase("always", myOSystem.settings().getString("usemouse"))
    || (BSPF::equalsIgnoreCase("analog", myOSystem.settings().getString("usemouse"))
        && analog);
  const bool stelladapter = myPJoyHandler->hasStelladaptors();

  return (!grabMouseAllowed && myAdjustSetting == AdjustSetting::GRAB_MOUSE)
    || (!joystick
        && (myAdjustSetting == AdjustSetting::DEADZONE
        || myAdjustSetting == AdjustSetting::FOUR_DIRECTIONS))
    || (!paddle
        && (myAdjustSetting == AdjustSetting::ANALOG_SENSITIVITY
        || myAdjustSetting == AdjustSetting::DEJITTER_AVERAGING
        || myAdjustSetting == AdjustSetting::DEJITTER_REACTION
        || myAdjustSetting == AdjustSetting::DIGITAL_SENSITIVITY
        || myAdjustSetting == AdjustSetting::SWAP_PADDLES
        || myAdjustSetting == AdjustSetting::PADDLE_CENTER_X
        || myAdjustSetting == AdjustSetting::PADDLE_CENTER_Y))
    || ((!paddle || !useMouse)
        && myAdjustSetting == AdjustSetting::PADDLE_SENSITIVITY)
    || ((!trackball || !useMouse)
        && myAdjustSetting == AdjustSetting::TRACKBALL_SENSITIVITY)
    || (!driving
        && myAdjustSetting == AdjustSetting::DRIVING_SENSITIVITY) // also affects digital device input sensitivity
    || ((!hasMouseControl() || !useMouse)
        && myAdjustSetting == AdjustSetting::MOUSE_CONTROL)
    || ((!paddle || !useMouse)
        && myAdjustSetting == AdjustSetting::MOUSE_RANGE)
    || (!stelladapter
        && myAdjustSetting == AdjustSetting::SA_PORT_ORDER);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::skipDebugSetting() const
{
  const bool isPAL = myOSystem.console().timing() == ConsoleTiming::pal;

  return (myAdjustSetting == AdjustSetting::COLOR_LOSS && !isPAL);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AdjustFunction EventHandler::cycleAdjustSetting(int direction)
{
  bool skip = false;
  do
  {
    switch (getAdjustGroup())
    {
      case AdjustGroup::AV:
        myAdjustSetting =
          AdjustSetting(BSPF::clampw(int(myAdjustSetting) + direction,
                        int(AdjustSetting::START_AV_ADJ), int(AdjustSetting::END_AV_ADJ)));
        // skip currently non-relevant adjustments
        skip = skipAVSetting();
        break;

      case AdjustGroup::INPUT:
        myAdjustSetting =
          AdjustSetting(BSPF::clampw(int(myAdjustSetting) + direction,
                        int(AdjustSetting::START_INPUT_ADJ), int(AdjustSetting::END_INPUT_ADJ)));
        // skip currently non-relevant adjustments
        skip = skipInputSetting();
        break;

      case AdjustGroup::DEBUG:
        myAdjustSetting =
          AdjustSetting(BSPF::clampw(int(myAdjustSetting) + direction,
                        int(AdjustSetting::START_DEBUG_ADJ), int(AdjustSetting::END_DEBUG_ADJ)));
        // skip currently non-relevant adjustments
        skip = skipDebugSetting();
        break;

      default:
        break;
    }
    // avoid endless loop
    if(skip && !direction)
      direction = 1;
  } while(skip);

  return getAdjustSetting(myAdjustSetting);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AdjustFunction EventHandler::getAdjustSetting(AdjustSetting setting)
{
  // Notes:
  // - All methods MUST show a message
  // - This array MUST have the same order as AdjustSetting
  const AdjustFunction ADJUST_FUNCTIONS[int(AdjustSetting::NUM_ADJ)] =
  {
    // *** Audio & Video settings ***
    std::bind(&Sound::adjustVolume, &myOSystem.sound(), _1),
    std::bind(&FrameBuffer::switchVideoMode, &myOSystem.frameBuffer(), _1),
    std::bind(&FrameBuffer::toggleFullscreen, &myOSystem.frameBuffer(), _1),
  #ifdef ADAPTABLE_REFRESH_SUPPORT
    std::bind(&FrameBuffer::toggleAdaptRefresh, &myOSystem.frameBuffer(), _1),
  #endif
    std::bind(&FrameBuffer::changeOverscan, &myOSystem.frameBuffer(), _1),
    std::bind(&Console::selectFormat, &myOSystem.console(), _1),
    std::bind(&Console::changeVerticalCenter, &myOSystem.console(), _1),
    std::bind(&Console::toggleCorrectAspectRatio, &myOSystem.console(), _1),
    std::bind(&Console::changeVSizeAdjust, &myOSystem.console(), _1),
    // Palette adjustables
    std::bind(&PaletteHandler::cyclePalette, &myOSystem.frameBuffer().tiaSurface().paletteHandler(), _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::PHASE_SHIFT, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::RED_SCALE, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::RED_SHIFT, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::GREEN_SCALE, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::GREEN_SHIFT, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::BLUE_SCALE, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::BLUE_SHIFT, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::HUE, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::SATURATION, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::CONTRAST, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::BRIGHTNESS, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::GAMMA, _1),
    // NTSC filter adjustables
    std::bind(&TIASurface::changeNTSC, &myOSystem.frameBuffer().tiaSurface(), _1),
    std::bind(&TIASurface::changeNTSCAdjustable, &myOSystem.frameBuffer().tiaSurface(),
      int(NTSCFilter::Adjustables::SHARPNESS), _1),
    std::bind(&TIASurface::changeNTSCAdjustable, &myOSystem.frameBuffer().tiaSurface(),
      int(NTSCFilter::Adjustables::RESOLUTION), _1),
    std::bind(&TIASurface::changeNTSCAdjustable, &myOSystem.frameBuffer().tiaSurface(),
      int(NTSCFilter::Adjustables::ARTIFACTS), _1),
    std::bind(&TIASurface::changeNTSCAdjustable, &myOSystem.frameBuffer().tiaSurface(),
      int(NTSCFilter::Adjustables::FRINGING), _1),
    std::bind(&TIASurface::changeNTSCAdjustable, &myOSystem.frameBuffer().tiaSurface(),
      int(NTSCFilter::Adjustables::BLEEDING), _1),
    std::bind(&Console::changePhosphor, &myOSystem.console(), _1),
    std::bind(&TIASurface::setScanlineIntensity, &myOSystem.frameBuffer().tiaSurface(), _1),
    std::bind(&Console::toggleInter, &myOSystem.console(), _1),

    // *** Input settings ***
    std::bind(&PhysicalJoystickHandler::changeDeadzone, &joyHandler(), _1),
    std::bind(&PhysicalJoystickHandler::changeAnalogPaddleSensitivity, &joyHandler(), _1),
    std::bind(&PhysicalJoystickHandler::changePaddleDejitterAveraging, &joyHandler(), _1),
    std::bind(&PhysicalJoystickHandler::changePaddleDejitterReaction, &joyHandler(), _1),
    std::bind(&PhysicalJoystickHandler::changeDigitalPaddleSensitivity, &joyHandler(), _1),
    std::bind(&Console::changeAutoFireRate, &myOSystem.console(), _1),
    std::bind(&EventHandler::toggleAllow4JoyDirections, this, _1),
    std::bind(&PhysicalKeyboardHandler::toggleModKeys, &keyHandler(), _1),
    std::bind(&EventHandler::toggleSAPortOrder, this, _1),
    std::bind(&EventHandler::changeMouseControllerMode, this, _1),
    std::bind(&PhysicalJoystickHandler::changeMousePaddleSensitivity, &joyHandler(), _1),
    std::bind(&PhysicalJoystickHandler::changeMouseTrackballSensitivity, &joyHandler(), _1),
    std::bind(&PhysicalJoystickHandler::changeDrivingSensitivity, &joyHandler(), _1),
    std::bind(&EventHandler::changeMouseCursor, this, _1),
    std::bind(&FrameBuffer::toggleGrabMouse, &myOSystem.frameBuffer(), _1),
    // Game properties/Controllers
    std::bind(&Console::changeLeftController, &myOSystem.console(), _1),
    std::bind(&Console::changeRightController, &myOSystem.console(), _1),
    std::bind(&Console::toggleSwapPorts, &myOSystem.console(), _1),
    std::bind(&Console::toggleSwapPaddles, &myOSystem.console(), _1),
    std::bind(&Console::changePaddleCenterX, &myOSystem.console(), _1),
    std::bind(&Console::changePaddleCenterY, &myOSystem.console(), _1),
    std::bind(&EventHandler::changeMouseControl, this, _1),
    std::bind(&Console::changePaddleAxesRange, &myOSystem.console(), _1),

    // *** Debug settings ***
    std::bind(&FrameBuffer::toggleFrameStats, &myOSystem.frameBuffer(), _1),
    std::bind(&Console::toggleP0Bit, &myOSystem.console(), _1),
    std::bind(&Console::toggleP1Bit, &myOSystem.console(), _1),
    std::bind(&Console::toggleM0Bit, &myOSystem.console(), _1),
    std::bind(&Console::toggleM1Bit, &myOSystem.console(), _1),
    std::bind(&Console::toggleBLBit, &myOSystem.console(), _1),
    std::bind(&Console::togglePFBit, &myOSystem.console(), _1),
    std::bind(&Console::toggleBits, &myOSystem.console(), _1),
    std::bind(&Console::toggleP0Collision, &myOSystem.console(), _1),
    std::bind(&Console::toggleP1Collision, &myOSystem.console(), _1),
    std::bind(&Console::toggleM0Collision, &myOSystem.console(), _1),
    std::bind(&Console::toggleM1Collision, &myOSystem.console(), _1),
    std::bind(&Console::toggleBLCollision, &myOSystem.console(), _1),
    std::bind(&Console::togglePFCollision, &myOSystem.console(), _1),
    std::bind(&Console::toggleCollisions, &myOSystem.console(), _1),
    std::bind(&Console::toggleFixedColors, &myOSystem.console(), _1),
    std::bind(&Console::toggleColorLoss, &myOSystem.console(), _1),
    std::bind(&Console::toggleJitter, &myOSystem.console(), _1),

    // *** Following functions are not used when cycling settings but for "direct only" hotkeys ***
    std::bind(&StateManager::changeState, &myOSystem.state(), _1),
    std::bind(&PaletteHandler::changeCurrentAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(), _1),
    std::bind(&TIASurface::changeCurrentNTSCAdjustable, &myOSystem.frameBuffer().tiaSurface(), _1),
    std::bind(&Console::changeSpeed, &myOSystem.console(), _1),
  };

  return ADJUST_FUNCTIONS[int(setting)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleEvent(Event::Type event, Int32 value, bool repeated)
{
  // Take care of special events that aren't part of the emulation core
  // or need to be preprocessed before passing them on
  const bool pressed = (value != 0);

  // The global settings keys change settings or values as long as the setting
  //  message from the previous settings event is still displayed.
  // Therefore, do not change global settings/values or direct values if
  //  a) the setting message is no longer shown
  //  b) other keys have been pressed
  if(!myOSystem.frameBuffer().messageShown())
  {
    myAdjustActive = false;
    myAdjustDirect = AdjustSetting::NONE;
  }

  const bool adjustActive = myAdjustActive;
  const AdjustSetting adjustAVDirect = myAdjustDirect;

  if(pressed)
  {
    myAdjustActive = false;
    myAdjustDirect = AdjustSetting::NONE;
  }

  switch(event)
  {
    ////////////////////////////////////////////////////////////////////////
    // Allow adjusting several (mostly repeated) settings using the same six hotkeys
    case Event::PreviousSettingGroup:
    case Event::NextSettingGroup:
      if(pressed && !repeated)
      {
        const int direction = event == Event::PreviousSettingGroup ? -1 : +1;
        AdjustGroup adjustGroup = AdjustGroup(BSPF::clampw(int(getAdjustGroup()) + direction,
                                              0, int(AdjustGroup::NUM_GROUPS) - 1));
        string msg;

        switch(adjustGroup)
        {
          case AdjustGroup::AV:
            msg = "Audio & Video";
            myAdjustSetting = AdjustSetting::START_AV_ADJ;
            break;

          case AdjustGroup::INPUT:
            msg = "Input Devices & Ports";
            myAdjustSetting = AdjustSetting::START_INPUT_ADJ;
            break;

          case AdjustGroup::DEBUG:
            msg = "Debug";
            myAdjustSetting = AdjustSetting::START_DEBUG_ADJ;
            break;

          default:
            break;
        }
        myOSystem.frameBuffer().showTextMessage(msg + " settings");
        myAdjustActive = false;
      }
      break;

      // Allow adjusting several (mostly repeated) settings using the same four hotkeys
    case Event::PreviousSetting:
    case Event::NextSetting:
      if(pressed && !repeated)
      {
        const int direction = event == Event::PreviousSetting ? -1 : +1;

        // Get (and display) the previous|next adjustment function,
        //  but do not change its value
        cycleAdjustSetting(adjustActive ? direction : 0)(0);
        // Fallback message when no message is displayed by method
        //if(!myOSystem.frameBuffer().messageShown())
        //  myOSystem.frameBuffer().showMessage("Message " + std::to_string(int(myAdjustSetting)));
        myAdjustActive = true;
      }
      break;

    case Event::SettingDecrease:
    case Event::SettingIncrease:
      if(pressed)
      {
        const int direction = event == Event::SettingDecrease ? -1 : +1;

        // if a "direct only" hotkey was pressed last, use this one
        if(adjustAVDirect != AdjustSetting::NONE)
        {
          myAdjustDirect = adjustAVDirect;
          getAdjustSetting(myAdjustDirect)(direction);
        }
        else
        {
          // Get (and display) the current adjustment function,
          //  but only change its value if the function was already active before
          getAdjustSetting(myAdjustSetting)(adjustActive ? direction : 0);
          myAdjustActive = true;
        }
      }
      return;

    ////////////////////////////////////////////////////////////////////////
    // If enabled, make sure 'impossible' joystick directions aren't allowed
    case Event::JoystickZeroUp:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickZeroDown, 0);
      break;

    case Event::JoystickZeroDown:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickZeroUp, 0);
      break;

    case Event::JoystickZeroLeft:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickZeroRight, 0);
      break;

    case Event::JoystickZeroRight:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickZeroLeft, 0);
      break;

    case Event::JoystickOneUp:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickOneDown, 0);
      break;

    case Event::JoystickOneDown:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickOneUp, 0);
      break;

    case Event::JoystickOneLeft:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickOneRight, 0);
      break;

    case Event::JoystickOneRight:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickOneLeft, 0);
      break;

    case Event::JoystickTwoUp:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickTwoDown, 0);
      break;

    case Event::JoystickTwoDown:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickTwoUp, 0);
      break;

    case Event::JoystickTwoLeft:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickTwoRight, 0);
      break;

    case Event::JoystickTwoRight:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickTwoLeft, 0);
      break;

    case Event::JoystickThreeUp:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickThreeDown, 0);
      break;

    case Event::JoystickThreeDown:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickThreeUp, 0);
      break;

    case Event::JoystickThreeLeft:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickThreeRight, 0);
      break;

    case Event::JoystickThreeRight:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickThreeLeft, 0);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Audio & Video events (with global hotkeys)
    case Event::VolumeDecrease:
      if(pressed)
      {
        myOSystem.sound().adjustVolume(-1);
        myAdjustSetting = AdjustSetting::VOLUME;
        myAdjustActive = true;
      }
      return;

    case Event::VolumeIncrease:
      if(pressed)
      {
        myOSystem.sound().adjustVolume(+1);
        myAdjustSetting = AdjustSetting::VOLUME;
        myAdjustActive = true;
      }
      return;

    case Event::SoundToggle:
      if(pressed && !repeated)
      {
        myOSystem.sound().toggleMute();
        myAdjustSetting = AdjustSetting::VOLUME;
        myAdjustActive = true;
      }
      return;

    case Event::VidmodeDecrease:
      if(pressed)
      {
        myOSystem.frameBuffer().switchVideoMode(-1);
        myAdjustSetting = AdjustSetting::ZOOM;
        myAdjustActive = true;
      }
      return;

    case Event::VidmodeIncrease:
      if(pressed)
      {
        myOSystem.frameBuffer().switchVideoMode(+1);
        myAdjustSetting = AdjustSetting::ZOOM;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleFullScreen:
      if(pressed && !repeated)
      {
        myOSystem.frameBuffer().toggleFullscreen();
        myAdjustSetting = AdjustSetting::FULLSCREEN;
        myAdjustActive = true;
      }
      return;

    #ifdef ADAPTABLE_REFRESH_SUPPORT
    case Event::ToggleAdaptRefresh:
      if(pressed && !repeated)
      {
        myOSystem.frameBuffer().toggleAdaptRefresh();
        myAdjustSetting = AdjustSetting::ADAPT_REFRESH;
        myAdjustActive = true;
      }
      return;
    #endif

    case Event::OverscanDecrease:
      if(pressed)
      {
        myOSystem.frameBuffer().changeOverscan(-1);
        myAdjustSetting = AdjustSetting::OVERSCAN;
        myAdjustActive = true;
      }
      return;

    case Event::OverscanIncrease:
      if(pressed)
      {
        myOSystem.frameBuffer().changeOverscan(+1);
        myAdjustSetting = AdjustSetting::OVERSCAN;
        myAdjustActive = true;
      }
      return;

    case Event::FormatDecrease:
      if(pressed && !repeated)
      {
        myOSystem.console().selectFormat(-1);
        myAdjustSetting = AdjustSetting::TVFORMAT;
        myAdjustActive = true;
      }
      return;

    case Event::FormatIncrease:
      if(pressed && !repeated)
      {
        myOSystem.console().selectFormat(+1);
        myAdjustSetting = AdjustSetting::TVFORMAT;
        myAdjustActive = true;
      }
      return;

    case Event::VCenterDecrease:
      if(pressed)
      {
        myOSystem.console().changeVerticalCenter(-1);
        myAdjustSetting = AdjustSetting::VCENTER;
        myAdjustActive = true;
      }
      return;

    case Event::VCenterIncrease:
      if(pressed)
      {
        myOSystem.console().changeVerticalCenter(+1);
        myAdjustSetting = AdjustSetting::VCENTER;
        myAdjustActive = true;
      }
      return;
    case Event::VSizeAdjustDecrease:
      if(pressed)
      {
        myOSystem.console().changeVSizeAdjust(-1);
        myAdjustSetting = AdjustSetting::VSIZE;
        myAdjustActive = true;
      }
      return;

    case Event::VSizeAdjustIncrease:
      if(pressed)
      {
        myOSystem.console().changeVSizeAdjust(+1);
        myAdjustSetting = AdjustSetting::VSIZE;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleCorrectAspectRatio:
      if(pressed && !repeated)
      {
        myOSystem.console().toggleCorrectAspectRatio();
        myAdjustSetting = AdjustSetting::ASPECT_RATIO;
        myAdjustActive = true;
      }
      break;

    case Event::PaletteDecrease:
      if(pressed && !repeated)
      {
        myOSystem.frameBuffer().tiaSurface().paletteHandler().cyclePalette(-1);
        myAdjustSetting = AdjustSetting::PALETTE;
        myAdjustActive = true;
      }
      return;

    case Event::PaletteIncrease:
      if(pressed && !repeated)
      {
        myOSystem.frameBuffer().tiaSurface().paletteHandler().cyclePalette(+1);
        myAdjustSetting = AdjustSetting::PALETTE;
        myAdjustActive = true;
      }
      return;

    case Event::PreviousVideoMode:
      if(pressed && !repeated)
      {
        myOSystem.frameBuffer().tiaSurface().changeNTSC(-1);
        myAdjustSetting = AdjustSetting::NTSC_PRESET;
        myAdjustActive = true;
      }
      return;

    case Event::NextVideoMode:
      if(pressed && !repeated)
      {
        myOSystem.frameBuffer().tiaSurface().changeNTSC(+1);
        myAdjustSetting = AdjustSetting::NTSC_PRESET;
        myAdjustActive = true;
      }
      return;

    case Event::VidmodeStd:
      if(pressed && !repeated)
      {
        myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::Preset::OFF);
        myAdjustDirect = AdjustSetting::NTSC_PRESET;
        myAdjustActive = true;
      }
      return;

    case Event::VidmodeRGB:
      if(pressed && !repeated)
      {
        myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::Preset::RGB);
        myAdjustSetting = AdjustSetting::NTSC_PRESET;
        myAdjustActive = true;
      }
      return;

    case Event::VidmodeSVideo:
      if(pressed && !repeated)
      {
        myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::Preset::SVIDEO);
        myAdjustSetting = AdjustSetting::NTSC_PRESET;
        myAdjustActive = true;
      }
      return;

    case Event::VidModeComposite:
      if(pressed && !repeated)
      {
        myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::Preset::COMPOSITE);
        myAdjustSetting = AdjustSetting::NTSC_PRESET;
        myAdjustActive = true;
      }
      return;

    case Event::VidModeBad:
      if(pressed && !repeated)
      {
        myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::Preset::BAD);
        myAdjustSetting = AdjustSetting::NTSC_PRESET;
        myAdjustActive = true;
      }
      return;

    case Event::VidModeCustom:
      if(pressed && !repeated)
      {
        myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::Preset::CUSTOM);
        myAdjustSetting = AdjustSetting::NTSC_PRESET;
        myAdjustActive = true;
      }
      return;
    case Event::PhosphorDecrease:
      if(pressed)
      {
        myOSystem.console().changePhosphor(-1);
        myAdjustSetting = AdjustSetting::PHOSPHOR;
        myAdjustActive = true;
      }
      return;

    case Event::PhosphorIncrease:
      if(pressed)
      {
        myOSystem.console().changePhosphor(+1);
        myAdjustSetting = AdjustSetting::PHOSPHOR;
        myAdjustActive = true;
      }
      return;

    case Event::TogglePhosphor:
      if(pressed && !repeated)
      {
        myOSystem.console().togglePhosphor();
        myAdjustSetting = AdjustSetting::PHOSPHOR;
        myAdjustActive = true;
      }
      return;

    case Event::ScanlinesDecrease:
      if(pressed)
      {
        myOSystem.frameBuffer().tiaSurface().setScanlineIntensity(-1);
        myAdjustSetting = AdjustSetting::SCANLINES;
        myAdjustActive = true;
      }
      return;

    case Event::ScanlinesIncrease:
      if(pressed)
      {
        myOSystem.frameBuffer().tiaSurface().setScanlineIntensity(+1);
        myAdjustSetting = AdjustSetting::SCANLINES;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleInter:
      if(pressed && !repeated)
      {
        myOSystem.console().toggleInter();
        myAdjustSetting = AdjustSetting::INTERPOLATION;
        myAdjustActive = true;
      }
      return;

      ///////////////////////////////////////////////////////////////////////////
      // Direct key Audio & Video events
    case Event::PreviousPaletteAttribute:
      if(pressed)
      {
        myOSystem.frameBuffer().tiaSurface().paletteHandler().cycleAdjustable(-1);
        myAdjustDirect = AdjustSetting::PALETTE_CHANGE_ATTRIBUTE;
      }
      return;

    case Event::NextPaletteAttribute:
      if(pressed)
      {
        myOSystem.frameBuffer().tiaSurface().paletteHandler().cycleAdjustable(+1);
        myAdjustDirect = AdjustSetting::PALETTE_CHANGE_ATTRIBUTE;
      }
      return;

    case Event::PaletteAttributeDecrease:
      if(pressed)
      {
        myOSystem.frameBuffer().tiaSurface().paletteHandler().changeCurrentAdjustable(-1);
        myAdjustDirect = AdjustSetting::PALETTE_CHANGE_ATTRIBUTE;
      }
      return;

    case Event::PaletteAttributeIncrease:
      if(pressed)
      {
        myOSystem.frameBuffer().tiaSurface().paletteHandler().changeCurrentAdjustable(+1);
        myAdjustDirect = AdjustSetting::PALETTE_CHANGE_ATTRIBUTE;
      }
      return;

    case Event::PreviousAttribute:
      if(pressed)
      {
        myOSystem.frameBuffer().tiaSurface().setNTSCAdjustable(-1);
        myAdjustDirect = AdjustSetting::NTSC_CHANGE_ATTRIBUTE;
      }
      return;

    case Event::NextAttribute:
      if(pressed)
      {
        myOSystem.frameBuffer().tiaSurface().setNTSCAdjustable(+1);
        myAdjustDirect = AdjustSetting::NTSC_CHANGE_ATTRIBUTE;
      }
      return;

    case Event::DecreaseAttribute:
      if(pressed)
      {
        myOSystem.frameBuffer().tiaSurface().changeCurrentNTSCAdjustable(-1);
        myAdjustDirect = AdjustSetting::NTSC_CHANGE_ATTRIBUTE;
      }
      return;

    case Event::IncreaseAttribute:
      if(pressed)
      {
        myOSystem.frameBuffer().tiaSurface().changeCurrentNTSCAdjustable(+1);
        myAdjustDirect = AdjustSetting::NTSC_CHANGE_ATTRIBUTE;
      }
      return;

    ///////////////////////////////////////////////////////////////////////////
    // Debug events (with global hotkeys)
    case Event::ToggleFrameStats:
      if(pressed && !repeated)
      {
        myOSystem.frameBuffer().toggleFrameStats();
        myAdjustSetting = AdjustSetting::STATS;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleP0Collision:
      if(pressed && !repeated)
      {
        myOSystem.console().toggleP0Collision();
        myAdjustSetting = AdjustSetting::P0_CX;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleP0Bit:
      if(pressed && !repeated)
      {
        myOSystem.console().toggleP0Bit();
        myAdjustSetting = AdjustSetting::P0_ENAM;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleP1Collision:
      if(pressed && !repeated)
      {
        myOSystem.console().toggleP1Collision();
        myAdjustSetting = AdjustSetting::P1_CX;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleP1Bit:
      if(pressed && !repeated)
      {
        myOSystem.console().toggleP1Bit();
        myAdjustSetting = AdjustSetting::P1_ENAM;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleM0Collision:
      if(pressed && !repeated)
      {
        myOSystem.console().toggleM0Collision();
        myAdjustSetting = AdjustSetting::M0_CX;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleM0Bit:
      if(pressed && !repeated)
      {
        myOSystem.console().toggleM0Bit();
        myAdjustSetting = AdjustSetting::M0_ENAM;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleM1Collision:
      if(pressed && !repeated)
      {
        myOSystem.console().toggleM1Collision();
        myAdjustSetting = AdjustSetting::M1_CX;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleM1Bit:
      if(pressed && !repeated)
      {
        myOSystem.console().toggleM1Bit();
        myAdjustSetting = AdjustSetting::M1_ENAM;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleBLCollision:
      if(pressed && !repeated)
      {
        myOSystem.console().toggleBLCollision();
        myAdjustSetting = AdjustSetting::BL_CX;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleBLBit:
      if(pressed && !repeated)
      {
        myOSystem.console().toggleBLBit();
        myAdjustSetting = AdjustSetting::BL_ENAM;
        myAdjustActive = true;
      }
      return;

    case Event::TogglePFCollision:
      if(pressed && !repeated)
      {
        myOSystem.console().togglePFCollision();
        myAdjustSetting = AdjustSetting::PF_CX;
        myAdjustActive = true;
      }
      return;

    case Event::TogglePFBit:
      if(pressed && !repeated)
      {
        myOSystem.console().togglePFBit();
        myAdjustSetting = AdjustSetting::PF_ENAM;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleCollisions:
      if(pressed && !repeated)
      {
        myOSystem.console().toggleCollisions();
        myAdjustSetting = AdjustSetting::ALL_CX;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleBits:
      if(pressed && !repeated)
      {
        myOSystem.console().toggleBits();
        myAdjustSetting = AdjustSetting::ALL_ENAM;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleFixedColors:
      if(pressed && !repeated)
      {
        myOSystem.console().toggleFixedColors();
        myAdjustSetting = AdjustSetting::FIXED_COL;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleColorLoss:
      if(pressed && !repeated)
      {
        myOSystem.console().toggleColorLoss();
        myAdjustSetting = AdjustSetting::COLOR_LOSS;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleJitter:
      if(pressed && !repeated)
      {
        myOSystem.console().toggleJitter();
        myAdjustSetting = AdjustSetting::JITTER;
        myAdjustActive = true;
      }
      return;

    ///////////////////////////////////////////////////////////////////////////
    // Input events
    case Event::DecreaseDeadzone:
      if(pressed)
      {
        myPJoyHandler->changeDeadzone(-1);
        myAdjustSetting = AdjustSetting::DEADZONE;
        myAdjustActive = true;
      }
      return;

    case Event::IncreaseDeadzone:
      if(pressed)
      {
        myPJoyHandler->changeDeadzone(+1);
        myAdjustSetting = AdjustSetting::DEADZONE;
        myAdjustActive = true;
      }
      return;

    case Event::DecAnalogSense:
      if(pressed)
      {
        myPJoyHandler->changeAnalogPaddleSensitivity(-1);
        myAdjustSetting = AdjustSetting::PADDLE_SENSITIVITY;
        myAdjustActive = true;
      }
      return;

    case Event::IncAnalogSense:
      if(pressed)
      {
        myPJoyHandler->changeAnalogPaddleSensitivity(+1);
        myAdjustSetting = AdjustSetting::PADDLE_SENSITIVITY;
        myAdjustActive = true;
      }

      return;

    case Event::DecDejtterAveraging:
      if(pressed)
      {
        myPJoyHandler->changePaddleDejitterAveraging(-1);
        myAdjustSetting = AdjustSetting::DEJITTER_AVERAGING;
        myAdjustActive = true;
      }
      return;

    case Event::IncDejtterAveraging:
      if(pressed)
      {
        myPJoyHandler->changePaddleDejitterAveraging(+1);
        myAdjustSetting = AdjustSetting::DEJITTER_AVERAGING;
        myAdjustActive = true;
      }
      return;

    case Event::DecDejtterReaction:
      if(pressed)
      {
        myPJoyHandler->changePaddleDejitterReaction(-1);
        myAdjustSetting = AdjustSetting::DEJITTER_REACTION;
        myAdjustActive = true;
      }
      return;

    case Event::IncDejtterReaction:
      if(pressed)
      {
        myPJoyHandler->changePaddleDejitterReaction(+1);
        myAdjustSetting = AdjustSetting::DEJITTER_REACTION;
        myAdjustActive = true;
      }
      return;

    case Event::DecDigitalSense:
      if(pressed)
      {
        myPJoyHandler->changeDigitalPaddleSensitivity(-1);
        myAdjustSetting = AdjustSetting::DIGITAL_SENSITIVITY;
        myAdjustActive = true;
      }
      return;

    case Event::IncDigitalSense:
      if(pressed)
      {
        myPJoyHandler->changeDigitalPaddleSensitivity(+1);
        myAdjustSetting = AdjustSetting::DIGITAL_SENSITIVITY;
        myAdjustActive = true;
      }
      return;

    case Event::DecreaseAutoFire:
      if(pressed)
      {
        myOSystem.console().changeAutoFireRate(-1);
        myAdjustSetting = AdjustSetting::AUTO_FIRE;
        myAdjustActive = true;
      }
      return;

    case Event::IncreaseAutoFire:
      if(pressed)
      {
        myOSystem.console().changeAutoFireRate(+1);
        myAdjustSetting = AdjustSetting::AUTO_FIRE;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleFourDirections:
      if(pressed && !repeated)
      {
        toggleAllow4JoyDirections();
        myAdjustSetting = AdjustSetting::FOUR_DIRECTIONS;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleKeyCombos:
      if(pressed && !repeated)
      {
        myPKeyHandler->toggleModKeys();
        myAdjustSetting = AdjustSetting::MOD_KEY_COMBOS;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleSAPortOrder:
      if(pressed && !repeated)
      {
        toggleSAPortOrder();
        myAdjustSetting = AdjustSetting::SA_PORT_ORDER;
        myAdjustActive = true;
      }
      return;

    case Event::PrevMouseAsController:
      if(pressed && !repeated)
      {
        changeMouseControllerMode(-1);
        myAdjustSetting = AdjustSetting::USE_MOUSE;
        myAdjustActive = true;
      }
      return;

    case Event::NextMouseAsController:
      if(pressed && !repeated)
      {
        changeMouseControllerMode(+1);
        myAdjustSetting = AdjustSetting::USE_MOUSE;
        myAdjustActive = true;
      }
      return;

    case Event::DecMousePaddleSense:
      if(pressed)
      {
        myPJoyHandler->changeMousePaddleSensitivity(-1);
        myAdjustSetting = AdjustSetting::PADDLE_SENSITIVITY;
        myAdjustActive = true;
      }
      return;

    case Event::IncMousePaddleSense:
      if(pressed)
      {
        myPJoyHandler->changeMousePaddleSensitivity(+1);
        myAdjustSetting = AdjustSetting::PADDLE_SENSITIVITY;
        myAdjustActive = true;
      }
      return;

    case Event::DecMouseTrackballSense:
      if(pressed)
      {
        myPJoyHandler->changeMouseTrackballSensitivity(-1);
        myAdjustSetting = AdjustSetting::TRACKBALL_SENSITIVITY;
        myAdjustActive = true;
      }
      return;

    case Event::IncMouseTrackballSense:
      if(pressed)
      {
        myPJoyHandler->changeMouseTrackballSensitivity(+1);
        myAdjustSetting = AdjustSetting::TRACKBALL_SENSITIVITY;
        myAdjustActive = true;
      }
      return;

    case Event::DecreaseDrivingSense:
      if(pressed)
      {
        myPJoyHandler->changeDrivingSensitivity(-1);
        myAdjustSetting = AdjustSetting::DRIVING_SENSITIVITY;
        myAdjustActive = true;
      }
      return;

    case Event::IncreaseDrivingSense:
      if(pressed)
      {
        myPJoyHandler->changeDrivingSensitivity(+1);
        myAdjustSetting = AdjustSetting::DRIVING_SENSITIVITY;
        myAdjustActive = true;
      }
      return;

    case Event::PreviousCursorVisbility:
      if(pressed && !repeated)
      {
        changeMouseCursor(-1);
        myAdjustSetting = AdjustSetting::MOUSE_CURSOR;
        myAdjustActive = true;
      }
      return;

    case Event::NextCursorVisbility:
      if(pressed && !repeated)
      {
        changeMouseCursor(+1);
        myAdjustSetting = AdjustSetting::MOUSE_CURSOR;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleGrabMouse:
      if(pressed && !repeated && !myOSystem.frameBuffer().fullScreen())
      {
        myOSystem.frameBuffer().toggleGrabMouse();
        myAdjustSetting = AdjustSetting::GRAB_MOUSE;
        myAdjustActive = true;
      }
      return;

    case Event::PreviousLeftPort:
      if(pressed && !repeated)
      {
        myOSystem.console().changeLeftController(-1);
        myAdjustSetting = AdjustSetting::LEFT_PORT;
        myAdjustActive = true;
      }
      return;

    case Event::NextLeftPort:
      if(pressed && !repeated)
      {
        myOSystem.console().changeLeftController(+1);
        myAdjustSetting = AdjustSetting::LEFT_PORT;
        myAdjustActive = true;
      }

      return;

    case Event::PreviousRightPort:
      if(pressed && !repeated)
      {
        myOSystem.console().changeRightController(-1);
        myAdjustSetting = AdjustSetting::RIGHT_PORT;
        myAdjustActive = true;
      }
      return;

    case Event::NextRightPort:
      if(pressed && !repeated)
      {
        myOSystem.console().changeRightController(+1);
        myAdjustSetting = AdjustSetting::RIGHT_PORT;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleSwapPorts:
      if(pressed && !repeated)
      {
        myOSystem.console().toggleSwapPorts();
        myAdjustSetting = AdjustSetting::SWAP_PORTS;
        myAdjustActive = true;
      }
      return;

    case Event::ToggleSwapPaddles:
      if(pressed && !repeated)
      {
        myOSystem.console().toggleSwapPaddles();
        myAdjustSetting = AdjustSetting::SWAP_PADDLES;
        myAdjustActive = true;
      }
      return;

    case Event::DecreasePaddleCenterX:
      if(pressed)
      {
        myOSystem.console().changePaddleCenterX(-1);
        myAdjustSetting = AdjustSetting::PADDLE_CENTER_X;
        myAdjustActive = true;
      }
      return;

    case Event::IncreasePaddleCenterX:
      if(pressed)
      {
        myOSystem.console().changePaddleCenterX(+1);
        myAdjustSetting = AdjustSetting::PADDLE_CENTER_X;
        myAdjustActive = true;
      }
      return;

    case Event::DecreasePaddleCenterY:
      if(pressed)
      {
        myOSystem.console().changePaddleCenterY(-1);
        myAdjustSetting = AdjustSetting::PADDLE_CENTER_Y;
        myAdjustActive = true;
      }
      return;

    case Event::IncreasePaddleCenterY:
      if(pressed)
      {
        myOSystem.console().changePaddleCenterY(+1);
        myAdjustSetting = AdjustSetting::PADDLE_CENTER_Y;
        myAdjustActive = true;
      }
      return;

    case Event::PreviousMouseControl:
      if(pressed && !repeated) changeMouseControl(-1);
      return;

    case Event::NextMouseControl:
      if(pressed && !repeated) changeMouseControl(+1);
      return;


    ///////////////////////////////////////////////////////////////////////////
    // State events
    case Event::SaveState:
      if (pressed && !repeated)
      {
        myOSystem.state().saveState();
        myAdjustDirect = AdjustSetting::STATE;
      }
      return;

    case Event::SaveAllStates:
      if (pressed && !repeated)
        myOSystem.frameBuffer().showTextMessage(myOSystem.state().rewindManager().saveAllStates());
      return;

    case Event::PreviousState:
      if (pressed)
      {
        myOSystem.state().changeState(-1);
        myAdjustDirect = AdjustSetting::STATE;
      }
      return;

    case Event::NextState:
      if (pressed)
      {
        myOSystem.state().changeState(+1);
        myAdjustDirect = AdjustSetting::STATE;
      }
      return;

    case Event::ToggleAutoSlot:
      if (pressed) myOSystem.state().toggleAutoSlot();
      return;

    case Event::LoadState:
      if (pressed && !repeated)
      {
        myOSystem.state().loadState();
        myAdjustDirect = AdjustSetting::STATE;
      }
      return;

    case Event::LoadAllStates:
      if (pressed && !repeated)
        myOSystem.frameBuffer().showTextMessage(myOSystem.state().rewindManager().loadAllStates());
      return;

    case Event::RewindPause:
      if (pressed) myOSystem.state().rewindStates();
      if (myState == EventHandlerState::EMULATION)
        setState(EventHandlerState::PAUSE);
      return;

    case Event::UnwindPause:
      if (pressed) myOSystem.state().unwindStates();
      if (myState == EventHandlerState::EMULATION)
        setState(EventHandlerState::PAUSE);
      return;

    case Event::Rewind1Menu:
      if (pressed) enterTimeMachineMenuMode(1, false);
      return;

    case Event::Rewind10Menu:
      if (pressed) enterTimeMachineMenuMode(10, false);
      return;

    case Event::RewindAllMenu:
      if (pressed) enterTimeMachineMenuMode(1000, false);
      return;

    case Event::Unwind1Menu:
      if (pressed) enterTimeMachineMenuMode(1, true);
      return;

    case Event::Unwind10Menu:
      if (pressed) enterTimeMachineMenuMode(10, true);
      return;

    case Event::UnwindAllMenu:
      if (pressed) enterTimeMachineMenuMode(1000, true);
      return;

    ///////////////////////////////////////////////////////////////////////////
    // Misc events
    case Event::DecreaseSpeed:
      if(pressed)
      {
        myOSystem.console().changeSpeed(-1);
        myAdjustDirect = AdjustSetting::CHANGE_SPEED;
      }
      return;

    case Event::IncreaseSpeed:
      if(pressed)
      {
        myOSystem.console().changeSpeed(+1);
        myAdjustDirect = AdjustSetting::CHANGE_SPEED;
      }
      return;

    case Event::ToggleTurbo:
      if (pressed && !repeated) myOSystem.console().toggleTurbo();
      return;

    case Event::Fry:
      if (!repeated) myFryingFlag = pressed;
      return;

    case Event::ReloadConsole:
      if (pressed && !repeated) myOSystem.reloadConsole(true);
      return;

    case Event::PreviousMultiCartRom:
      if (pressed && !repeated) myOSystem.reloadConsole(false);
      return;

    case Event::ToggleTimeMachine:
      if (pressed && !repeated) myOSystem.state().toggleTimeMachine();
      return;

  #ifdef PNG_SUPPORT
    case Event::ToggleContSnapshots:
      if (pressed && !repeated) myOSystem.png().toggleContinuousSnapshots(false);
      return;

    case Event::ToggleContSnapshotsFrame:
      if (pressed && !repeated) myOSystem.png().toggleContinuousSnapshots(true);
      return;
  #endif

    case Event::TakeSnapshot:
      if(pressed && !repeated) myOSystem.frameBuffer().tiaSurface().saveSnapShot();
      return;

    case Event::ExitMode:
      // Special handling for Escape key
      // Basically, exit whichever mode we're currently in
      switch (myState)
      {
        case EventHandlerState::PAUSE:
          if (pressed && !repeated) changeStateByEvent(Event::TogglePauseMode);
          return;

        case EventHandlerState::CMDMENU:
          if (pressed && !repeated) changeStateByEvent(Event::CmdMenuMode);
          return;

        case EventHandlerState::TIMEMACHINE:
          if (pressed && !repeated) changeStateByEvent(Event::TimeMachineMode);
          return;

        case EventHandlerState::PLAYBACK:
          if (pressed && !repeated) changeStateByEvent(Event::TogglePlayBackMode);
          return;

        // this event is called when exiting a ROM from the debugger, so it acts like pressing ESC in emulation
        case EventHandlerState::EMULATION:
        case EventHandlerState::DEBUGGER:
          if (pressed && !repeated)
          {
            if (myState == EventHandlerState::EMULATION)
            {
#ifdef GUI_SUPPORT
              if (myOSystem.settings().getBool("confirmexit"))
              {
                StringList msg;
                const string saveOnExit = myOSystem.settings().getString("saveonexit");
                bool activeTM = myOSystem.settings().getBool(
                  myOSystem.settings().getBool("dev.settings") ? "dev.timemachine" : "plr.timemachine");


                msg.push_back("Do you really want to exit emulation?");
                if (saveOnExit != "all" || !activeTM)
                {
                  msg.push_back("");
                  msg.push_back("You will lose all your progress.");
                }
                myOSystem.messageMenu().setMessage("Exit Emulation", msg, true);
                enterMenuMode(EventHandlerState::MESSAGEMENU);
              }
              else
#endif
                exitEmulation(true);
            }
            else
              exitEmulation(true);
          }
          return;

#ifdef GUI_SUPPORT
        case EventHandlerState::MESSAGEMENU:
          if (pressed && !repeated)
          {
            leaveMenuMode();
            if (myOSystem.messageMenu().confirmed())
              exitEmulation(true);
          }
          return;
#endif
        default:
          return;
      }

    case Event::ExitGame:
      exitEmulation(true);
      return;

    case Event::Quit:
      if(pressed && !repeated)
      {
        saveKeyMapping();
        saveJoyMapping();
        if (myState != EventHandlerState::LAUNCHER)
          exitEmulation();
        myOSystem.quit();
      }
      return;

    case Event::StartPauseMode:
      if (pressed && !repeated && myState == EventHandlerState::EMULATION)
        setState(EventHandlerState::PAUSE);
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
      for(int i = 0, combo = event - Event::Combo1; i < EVENTS_PER_COMBO; ++i)
        if(myComboTable[combo][i] != Event::NoType)
          handleEvent(myComboTable[combo][i], pressed, repeated);
      return;

    ////////////////////////////////////////////////////////////////////////
    // Events which relate to switches()
    case Event::ConsoleColor:
      if(pressed && !repeated)
      {
        myEvent.set(Event::ConsoleBlackWhite, 0);
        myEvent.set(Event::ConsoleColor, 1);
        myOSystem.frameBuffer().showTextMessage(myIs7800 ? "Pause released" : "Color Mode");
        myOSystem.console().switches().update();
      }
      return;
    case Event::ConsoleBlackWhite:
      if(pressed && !repeated)
      {
        myEvent.set(Event::ConsoleBlackWhite, 1);
        myEvent.set(Event::ConsoleColor, 0);
        myOSystem.frameBuffer().showTextMessage(myIs7800 ? "Pause pushed" : "B/W Mode");
        myOSystem.console().switches().update();
      }
      return;
    case Event::ConsoleColorToggle:
      if(pressed && !repeated)
      {
        if(myOSystem.console().switches().tvColor())
        {
          myEvent.set(Event::ConsoleBlackWhite, 1);
          myEvent.set(Event::ConsoleColor, 0);
          myOSystem.frameBuffer().showTextMessage(myIs7800 ? "Pause pushed" : "B/W Mode");
        }
        else
        {
          myEvent.set(Event::ConsoleBlackWhite, 0);
          myEvent.set(Event::ConsoleColor, 1);
          myOSystem.frameBuffer().showTextMessage(myIs7800 ? "Pause released" : "Color Mode");
        }
        myOSystem.console().switches().update();
      }
      return;

    case Event::Console7800Pause:
      if(pressed && !repeated)
      {
        myEvent.set(Event::ConsoleBlackWhite, 0);
        myEvent.set(Event::ConsoleColor, 0);
        if (myIs7800)
          myOSystem.frameBuffer().showTextMessage("Pause pressed");
        myOSystem.console().switches().update();
      }
      return;

    case Event::ConsoleLeftDiffA:
      if(pressed && !repeated)
      {
        myEvent.set(Event::ConsoleLeftDiffA, 1);
        myEvent.set(Event::ConsoleLeftDiffB, 0);
        myOSystem.frameBuffer().showTextMessage(GUI::LEFT_DIFFICULTY + " A");
        myOSystem.console().switches().update();
      }
      return;
    case Event::ConsoleLeftDiffB:
      if(pressed && !repeated)
      {
        myEvent.set(Event::ConsoleLeftDiffA, 0);
        myEvent.set(Event::ConsoleLeftDiffB, 1);
        myOSystem.frameBuffer().showTextMessage(GUI::LEFT_DIFFICULTY + " B");
        myOSystem.console().switches().update();
      }
      return;
    case Event::ConsoleLeftDiffToggle:
      if(pressed && !repeated)
      {
        if(myOSystem.console().switches().leftDifficultyA())
        {
          myEvent.set(Event::ConsoleLeftDiffA, 0);
          myEvent.set(Event::ConsoleLeftDiffB, 1);
          myOSystem.frameBuffer().showTextMessage(GUI::LEFT_DIFFICULTY + " B");
        }
        else
        {
          myEvent.set(Event::ConsoleLeftDiffA, 1);
          myEvent.set(Event::ConsoleLeftDiffB, 0);
          myOSystem.frameBuffer().showTextMessage(GUI::LEFT_DIFFICULTY + " A");
        }
        myOSystem.console().switches().update();
      }
      return;

    case Event::ConsoleRightDiffA:
      if(pressed && !repeated)
      {
        myEvent.set(Event::ConsoleRightDiffA, 1);
        myEvent.set(Event::ConsoleRightDiffB, 0);
        myOSystem.frameBuffer().showTextMessage(GUI::RIGHT_DIFFICULTY + " A");
        myOSystem.console().switches().update();
      }
      return;
    case Event::ConsoleRightDiffB:
      if(pressed && !repeated)
      {
        myEvent.set(Event::ConsoleRightDiffA, 0);
        myEvent.set(Event::ConsoleRightDiffB, 1);
        myOSystem.frameBuffer().showTextMessage(GUI::RIGHT_DIFFICULTY + " B");
        myOSystem.console().switches().update();
      }
      return;
    case Event::ConsoleRightDiffToggle:
      if(pressed && !repeated)
      {
        if(myOSystem.console().switches().rightDifficultyA())
        {
          myEvent.set(Event::ConsoleRightDiffA, 0);
          myEvent.set(Event::ConsoleRightDiffB, 1);
          myOSystem.frameBuffer().showTextMessage(GUI::RIGHT_DIFFICULTY + " B");
        }
        else
        {
          myEvent.set(Event::ConsoleRightDiffA, 1);
          myEvent.set(Event::ConsoleRightDiffB, 0);
          myOSystem.frameBuffer().showTextMessage(GUI::RIGHT_DIFFICULTY + " A");
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
  if(!repeated)
    myEvent.set(event, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleConsoleStartupEvents()
{
  if(myOSystem.settings().getBool("holdreset"))
    handleEvent(Event::ConsoleReset);

  if(myOSystem.settings().getBool("holdselect"))
    handleEvent(Event::ConsoleSelect);

  const string& holdjoy0 = myOSystem.settings().getString("holdjoy0");

  if(BSPF::containsIgnoreCase(holdjoy0, "U"))
    handleEvent(Event::JoystickZeroUp);
  if(BSPF::containsIgnoreCase(holdjoy0, "D"))
    handleEvent(Event::JoystickZeroDown);
  if(BSPF::containsIgnoreCase(holdjoy0, "L"))
    handleEvent(Event::JoystickZeroLeft);
  if(BSPF::containsIgnoreCase(holdjoy0, "R"))
    handleEvent(Event::JoystickZeroRight);
  if(BSPF::containsIgnoreCase(holdjoy0, "F"))
    handleEvent(Event::JoystickZeroFire);

  const string& holdjoy1 = myOSystem.settings().getString("holdjoy1");
  if(BSPF::containsIgnoreCase(holdjoy1, "U"))
    handleEvent(Event::JoystickOneUp);
  if(BSPF::containsIgnoreCase(holdjoy1, "D"))
    handleEvent(Event::JoystickOneDown);
  if(BSPF::containsIgnoreCase(holdjoy1, "L"))
    handleEvent(Event::JoystickOneLeft);
  if(BSPF::containsIgnoreCase(holdjoy1, "R"))
    handleEvent(Event::JoystickOneRight);
  if(BSPF::containsIgnoreCase(holdjoy1, "F"))
    handleEvent(Event::JoystickOneFire);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::changeStateByEvent(Event::Type type)
{
  bool handled = true;

  switch(type)
  {
    case Event::TogglePauseMode:
      if(myState == EventHandlerState::EMULATION || myState == EventHandlerState::PLAYBACK)
        setState(EventHandlerState::PAUSE);
      else if(myState == EventHandlerState::PAUSE)
        setState(EventHandlerState::EMULATION);
      else
        handled = false;
      break;

    case Event::OptionsMenuMode:
      if (myState == EventHandlerState::EMULATION || myState == EventHandlerState::PAUSE
          || myState == EventHandlerState::TIMEMACHINE || myState == EventHandlerState::PLAYBACK)
        enterMenuMode(EventHandlerState::OPTIONSMENU);
      else
        handled = false;
      break;

    case Event::CmdMenuMode:
      if(myState == EventHandlerState::EMULATION || myState == EventHandlerState::PAUSE
         || myState == EventHandlerState::TIMEMACHINE || myState == EventHandlerState::PLAYBACK)
        enterMenuMode(EventHandlerState::CMDMENU);
      else if(myState == EventHandlerState::CMDMENU && !myOSystem.settings().getBool("minimal_ui"))
        // The extra check for "minimal_ui" allows mapping e.g. right joystick fire
        //  to open the command dialog and navigate there using that fire button
        leaveMenuMode();
      else
        handled = false;
      break;

#ifdef GUI_SUPPORT
    case Event::HighScoresMenuMode:
      if (myState == EventHandlerState::EMULATION || myState == EventHandlerState::PAUSE
          || myState == EventHandlerState::TIMEMACHINE)
      {
        if (myOSystem.highScores().enabled())
          enterMenuMode(EventHandlerState::HIGHSCORESMENU);
        else
          myOSystem.frameBuffer().showTextMessage("No high scores data defined");
      }
      else if(myState == EventHandlerState::HIGHSCORESMENU)
        leaveMenuMode();
      else
        handled = false;
      break;
#endif

    case Event::TimeMachineMode:
      if(myState == EventHandlerState::EMULATION || myState == EventHandlerState::PAUSE
         || myState == EventHandlerState::PLAYBACK)
        enterTimeMachineMenuMode(0, false);
      else if(myState == EventHandlerState::TIMEMACHINE)
        leaveMenuMode();
      else
        handled = false;
      break;

    case Event::TogglePlayBackMode:
      if (myState == EventHandlerState::EMULATION || myState == EventHandlerState::PAUSE
          || myState == EventHandlerState::TIMEMACHINE)
        enterPlayBackMode();
      else if (myState == EventHandlerState::PLAYBACK)
    #ifdef GUI_SUPPORT
        enterMenuMode(EventHandlerState::TIMEMACHINE);
    #else
        setState(EventHandlerState::PAUSE);
    #endif
      else
        handled = false;
      break;

    case Event::DebuggerMode:
  #ifdef DEBUGGER_SUPPORT
      if(myState == EventHandlerState::EMULATION || myState == EventHandlerState::PAUSE
         || myState == EventHandlerState::TIMEMACHINE || myState == EventHandlerState::PLAYBACK)
        enterDebugMode();
      else if(myState == EventHandlerState::DEBUGGER && myOSystem.debugger().canExit())
        leaveDebugMode();
      else
        handled = false;
  #endif
      break;

    default:
      handled = false;
  }

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setActionMappings(EventMode mode)
{
  switch(mode)
  {
    case EventMode::kEmulationMode:
      // Fill the EmulActionList with the current key and joystick mappings
      for(auto& item: ourEmulActionList)
      {
        const Event::Type event = item.event;
        item.key = "None";
        string key = myPKeyHandler->getMappingDesc(event, mode);

    #ifdef JOYSTICK_SUPPORT
        const string joydesc = myPJoyHandler->getMappingDesc(event, mode);
        if(joydesc != "")
        {
          if(key != "")
            key += ", ";
          key += joydesc;
        }
    #endif

        if(key != "")
          item.key = key;
      }
      break;
    case EventMode::kMenuMode:
      // Fill the MenuActionList with the current key and joystick mappings
      for(auto& item: ourMenuActionList)
      {
        const Event::Type event = item.event;
        item.key = "None";
        string key = myPKeyHandler->getMappingDesc(event, mode);

    #ifdef JOYSTICK_SUPPORT
        const string joydesc = myPJoyHandler->getMappingDesc(event, mode);
        if(joydesc != "")
        {
          if(key != "")
            key += ", ";
          key += joydesc;
        }
    #endif

        if(key != "")
          item.key = key;
      }
      break;
    default:
      return;
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
  const Int32 version = myOSystem.settings().getInt("event_ver");

  // Erase the 'combo' array
  auto ERASE_ALL = [&]() {
    for(int i = 0; i < COMBO_SIZE; ++i)
      for(int j = 0; j < EVENTS_PER_COMBO; ++j)
        myComboTable[i][j] = Event::NoType;
  };

  // Compare if event list version has changed so that combo maps became invalid
  if(version != Event::VERSION || !buf.good())
    ERASE_ALL();
  else
  {
    // Get combo count, which should be the first int in the list
    // If it isn't, then we treat the entire list as invalid
    try
    {
      string key;
      buf >> key;
      if(BSPF::stringToInt(key) == COMBO_SIZE)
      {
        // Fill the combomap table with events for as long as they exist
        int combocount = 0;
        while(buf >> key && combocount < COMBO_SIZE)
        {
          // Each event in a comboevent is separated by a comma
          replace(key.begin(), key.end(), ',', ' ');
          istringstream buf2(key);

          int eventcount = 0;
          while(buf2 >> key && eventcount < EVENTS_PER_COMBO)
          {
            myComboTable[combocount][eventcount] = Event::Type(BSPF::stringToInt(key));
            ++eventcount;
          }
          ++combocount;
        }
      }
      else
        ERASE_ALL();
    }
    catch(...)
    {
      ERASE_ALL();
    }
  }

  saveComboMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::removePhysicalJoystickFromDatabase(const string& name)
{
#ifdef JOYSTICK_SUPPORT
  myPJoyHandler->remove(name);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addKeyMapping(Event::Type event, EventMode mode, StellaKey key, StellaMod mod)
{
  const bool mapped = myPKeyHandler->addMapping(event, mode, key, mod);
  if(mapped)
    setActionMappings(mode);

  return mapped;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addJoyMapping(Event::Type event, EventMode mode,
                                 int stick, int button, JoyAxis axis, JoyDir adir,
                                 bool updateMenus)
{
#ifdef JOYSTICK_SUPPORT
  const bool mapped = myPJoyHandler->addJoyMapping(event, mode, stick, button, axis, adir);
  if (mapped && updateMenus)
    setActionMappings(mode);

  return mapped;
#else
  return false;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addJoyHatMapping(Event::Type event, EventMode mode,
                                    int stick, int button, int hat, JoyHatDir dir,
                                    bool updateMenus)
{
#ifdef JOYSTICK_SUPPORT
  bool mapped = myPJoyHandler->addJoyHatMapping(event, mode, stick, button, hat, dir);
  if (mapped && updateMenus)
    setActionMappings(mode);

  return mapped;
#else
  return false;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::eraseMapping(Event::Type event, EventMode mode)
{
  // Erase the KeyEvent array
  myPKeyHandler->eraseMapping(event, mode);

#ifdef JOYSTICK_SUPPORT
  // Erase the joystick mapping arrays
  myPJoyHandler->eraseMapping(event, mode);
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
  myPKeyHandler->setDefaultMapping(event, mode);
  setActionMappings(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoymap(Event::Type event, EventMode mode)
{
#ifdef JOYSTICK_SUPPORT
  myPJoyHandler->setDefaultMapping(event, mode);
  setActionMappings(mode);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveKeyMapping()
{
  myPKeyHandler->saveMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveJoyMapping()
{
#ifdef JOYSTICK_SUPPORT
  myPJoyHandler->saveMapping();
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveComboMapping()
{
  // Iterate through the combomap table and create a colon-separated list
  // For each combo event, create a comma-separated list of its events
  // Prepend the event count, so we can check it on next load
  ostringstream buf;
  buf << COMBO_SIZE;
  for(int i = 0; i < COMBO_SIZE; ++i)
  {
    buf << ":" << myComboTable[i][0];
    for(int j = 1; j < EVENTS_PER_COMBO; ++j)
      buf << "," << myComboTable[i][j];
  }
  myOSystem.settings().setValue("combomap", buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StringList EventHandler::getActionList(EventMode mode) const
{
  StringList l;
  switch(mode)
  {
    case EventMode::kEmulationMode:
      for(const auto& item: ourEmulActionList)
        l.push_back(item.action);
      break;
    case EventMode::kMenuMode:
      for(const auto& item: ourMenuActionList)
        l.push_back(item.action);
      break;
    default:
      break;
  }
  return l;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StringList EventHandler::getActionList(Event::Group group) const
{
  StringList l;

  switch(group)
  {
    case Event::Group::Menu:
      return getActionList(EventMode::kMenuMode);

    case Event::Group::Emulation:
      return getActionList(EventMode::kEmulationMode);

    case Event::Group::Misc:
      return getActionList(MiscEvents);

    case Event::Group::AudioVideo:
      return getActionList(AudioVideoEvents);

    case Event::Group::States:
      return getActionList(StateEvents);

    case Event::Group::Console:
      return getActionList(ConsoleEvents);

    case Event::Group::Joystick:
      return getActionList(JoystickEvents);

    case Event::Group::Paddles:
      return getActionList(PaddlesEvents);

    case Event::Group::Keyboard:
      return getActionList(KeyboardEvents);

    case Event::Group::Devices:
      return getActionList(DevicesEvents);

    case Event::Group::Debug:
      return getActionList(DebugEvents);

    case Event::Group::Combo:
      return getActionList(ComboEvents);

    default:
      return l; // ToDo
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StringList EventHandler::getActionList(const Event::EventSet& events, EventMode mode) const
{
  StringList l;

  switch(mode)
  {
    case EventMode::kMenuMode:
      for(const auto& item: ourMenuActionList)
        for(const auto& event : events)
          if(item.event == event)
          {
            l.push_back(item.action);
            break;
          }
      break;

    default:
      for(const auto& item: ourEmulActionList)
        for(const auto& event : events)
          if(item.event == event)
          {
            l.push_back(item.action);
            break;
          }
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
  for(uInt32 i = 0; i < ourEmulActionList.size(); ++i)
  {
    const Event::Type event = EventHandler::ourEmulActionList[i].event;
    // exclude combos events
    if(!(event >= Event::Combo1 && event <= Event::Combo16))
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
    for(uInt32 i = 0; i < EVENTS_PER_COMBO; ++i)
    {
      const Event::Type e = myComboTable[combo][i];
      for(uInt32 j = 0; j < ourEmulActionList.size(); ++j)
      {
        if(EventHandler::ourEmulActionList[j].event == e)
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
    const int combo = event - Event::Combo1;
    for(uInt32 i = 0; i < 8; ++i)
    {
      uInt32 idx = BSPF::stringToInt(events[i]);
      if(idx < ourEmulActionList.size())
        myComboTable[combo][i] = EventHandler::ourEmulActionList[idx].event;
      else
        myComboTable[combo][i] = Event::NoType;
    }
    saveComboMapping();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int EventHandler::getEmulActionListIndex(int idx, const Event::EventSet& events) const
{
  // idx = index into intersection set of 'events' and 'ourEmulActionList'
  //   ordered by 'ourEmulActionList'!
  Event::Type event = Event::NoType;

  for(uInt32 i = 0; i < ourEmulActionList.size(); ++i)
  {
    for(const auto& item : events)
      if(EventHandler::ourEmulActionList[i].event == item)
      {
        idx--;
        if(idx < 0)
          event = item;
        break;
      }
    if(idx < 0)
      break;
  }

  for(uInt32 i = 0; i < ourEmulActionList.size(); ++i)
    if(EventHandler::ourEmulActionList[i].event == event)
      return i;

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int EventHandler::getActionListIndex(int idx, Event::Group group) const
{
  switch(group)
  {
    case Event::Group::Menu:
      return idx;

    case Event::Group::Emulation:
      return idx;

    case Event::Group::Misc:
      return getEmulActionListIndex(idx, MiscEvents);

    case Event::Group::AudioVideo:
      return getEmulActionListIndex(idx, AudioVideoEvents);

    case Event::Group::States:
      return getEmulActionListIndex(idx, StateEvents);

    case Event::Group::Console:
      return getEmulActionListIndex(idx, ConsoleEvents);

    case Event::Group::Joystick:
      return getEmulActionListIndex(idx, JoystickEvents);

    case Event::Group::Paddles:
      return getEmulActionListIndex(idx, PaddlesEvents);

    case Event::Group::Keyboard:
      return getEmulActionListIndex(idx, KeyboardEvents);

    case Event::Group::Devices:
      return getEmulActionListIndex(idx, DevicesEvents);

    case Event::Group::Debug:
      return getEmulActionListIndex(idx, DebugEvents);

    case Event::Group::Combo:
      return getEmulActionListIndex(idx, ComboEvents);

    default:
      return -1;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type EventHandler::eventAtIndex(int idx, Event::Group group) const
{
  const int index = getActionListIndex(idx, group);

  if(group == Event::Group::Menu)
  {
    if(index < 0 || index >= int(ourMenuActionList.size()))
      return Event::NoType;
    else
      return ourMenuActionList[index].event;
  }
  else
  {
    if(index < 0 || index >= int(ourEmulActionList.size()))
      return Event::NoType;
    else
      return ourEmulActionList[index].event;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EventHandler::actionAtIndex(int idx, Event::Group group) const
{
  const int index = getActionListIndex(idx, group);

  if(group == Event::Group::Menu)
  {
    if(index < 0 || index >= int(ourMenuActionList.size()))
      return EmptyString;
    else
      return ourMenuActionList[index].action;
  }
  else
  {
    if(index < 0 || index >= int(ourEmulActionList.size()))
      return EmptyString;
    else
      return ourEmulActionList[index].action;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EventHandler::keyAtIndex(int idx, Event::Group group) const
{
  const int index = getActionListIndex(idx, group);

  if(group == Event::Group::Menu)
  {
    if(index < 0 || index >= int(ourMenuActionList.size()))
      return EmptyString;
    else
      return ourMenuActionList[index].key;
  }
  else
  {
    if(index < 0 || index >= int(ourEmulActionList.size()))
      return EmptyString;
    else
      return ourEmulActionList[index].key;
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
      usemouse = myOSystem.console().leftController().isAnalog() ||
                 myOSystem.console().rightController().isAnalog();
    }

    const string& control = usemouse ?
      myOSystem.console().properties().get(PropType::Controller_MouseAxis) : "none";

    myMouseControl = make_unique<MouseControl>(myOSystem.console(), control);
    myMouseControl->change(0);  // set first available mode
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::changeMouseControllerMode(int direction)
{
  const int NUM_MODES = 3;
  const string MODES[NUM_MODES] = {"always", "analog", "never"};
  const string MSG[NUM_MODES] = {"all", "analog", "no"};
  string usemouse = myOSystem.settings().getString("usemouse");

  int i = 0;
  for(auto& mode : MODES)
  {
    if(mode == usemouse)
    {
      i = BSPF::clampw(i + direction, 0, NUM_MODES - 1);
      usemouse = MODES[i];
      break;
    }
    ++i;
  }
  myOSystem.settings().setValue("usemouse", usemouse);
  setMouseControllerMode(usemouse);
  myOSystem.frameBuffer().setCursorState(); // if necessary change grab mouse

  ostringstream ss;
  ss << "Mouse controls " << MSG[i] << " devices";
  myOSystem.frameBuffer().showTextMessage(ss.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::changeMouseCursor(int direction)
{
  int cursor = BSPF::clampw(myOSystem.settings().getInt("cursor") + direction, 0, 3);

  myOSystem.settings().setValue("cursor", cursor);
  myOSystem.frameBuffer().setCursorState();

  ostringstream ss;
  ss << "Mouse cursor visibilility: "
    << ((cursor & 2) ? "+" : "-") << "UI, "
    << ((cursor & 1) ? "+" : "-") << "Emulation";
  myOSystem.frameBuffer().showTextMessage(ss.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::enterMenuMode(EventHandlerState state)
{
#ifdef GUI_SUPPORT
  setState(state);
  myOverlay->reStack();
  myOSystem.sound().mute(true);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::leaveMenuMode()
{
#ifdef GUI_SUPPORT
  myOverlay->removeDialog(); // remove the base dialog from dialog stack
  setState(EventHandlerState::EMULATION);
  myOSystem.sound().mute(false);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::enterDebugMode()
{
#ifdef DEBUGGER_SUPPORT
  if(myState == EventHandlerState::DEBUGGER || !myOSystem.hasConsole())
    return false;

  // Make sure debugger starts in a consistent state
  // This absolutely *has* to come before we actually change to debugger
  // mode, since it takes care of locking the debugger state, which will
  // probably be modified below
  myOSystem.debugger().setStartState();
  setState(EventHandlerState::DEBUGGER);

  FBInitStatus fbstatus = myOSystem.createFrameBuffer();
  if(fbstatus != FBInitStatus::Success)
  {
    myOSystem.debugger().setQuitState();
    setState(EventHandlerState::EMULATION);
    if(fbstatus == FBInitStatus::FailTooLarge)
      myOSystem.frameBuffer().showTextMessage("Debugger window too large for screen",
                                              MessagePosition::BottomCenter, true);
    return false;
  }
  myOverlay->reStack();
  myOSystem.sound().mute(true);

#else
  myOSystem.frameBuffer().showTextMessage("Debugger support not included",
                                          MessagePosition::BottomCenter, true);
#endif

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::leaveDebugMode()
{
#ifdef DEBUGGER_SUPPORT
  // paranoia: this should never happen:
  if(myState != EventHandlerState::DEBUGGER)
    return;

  // Make sure debugger quits in a consistent state
  myOSystem.debugger().setQuitState();

  setState(EventHandlerState::EMULATION);
  myOSystem.createFrameBuffer();
  myOSystem.sound().mute(false);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::enterTimeMachineMenuMode(uInt32 numWinds, bool unwind)
{
#ifdef GUI_SUPPORT
  // add one extra state if we are in Time Machine mode
  // TODO: maybe remove this state if we leave the menu at this new state
  myOSystem.state().addExtraState("enter Time Machine dialog"); // force new state

  if(numWinds)
    // hande winds and display wind message (numWinds != 0) in time machine dialog
    myOSystem.timeMachine().setEnterWinds(unwind ? numWinds : -numWinds);

  enterMenuMode(EventHandlerState::TIMEMACHINE);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::enterPlayBackMode()
{
#ifdef GUI_SUPPORT
  setState(EventHandlerState::PLAYBACK);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setState(EventHandlerState state)
{
  myState = state;

  // Normally, the usage of modifier keys is determined by 'modcombo'
  // For certain ROMs it may be forced off, whatever the setting
  myPKeyHandler->useModKeys() = myOSystem.settings().getBool("modcombo");

  // Only enable text input in GUI modes, since in emulation mode the
  // keyboard acts as one large joystick with many (single) buttons
  myOverlay = nullptr;
  switch(myState)
  {
    case EventHandlerState::EMULATION:
    case EventHandlerState::PLAYBACK:
      myOSystem.sound().mute(false);
      enableTextEvents(false);
      break;

    case EventHandlerState::PAUSE:
      myOSystem.sound().mute(true);
      enableTextEvents(false);
      break;

  #ifdef GUI_SUPPORT
    case EventHandlerState::OPTIONSMENU:
      myOverlay = &myOSystem.menu();
      enableTextEvents(true);
      break;

    case EventHandlerState::CMDMENU:
      myOverlay = &myOSystem.commandMenu();
      enableTextEvents(true);
      break;

    case EventHandlerState::HIGHSCORESMENU:
      myOverlay = &myOSystem.highscoresMenu();
      enableTextEvents(true);
      break;

    case EventHandlerState::MESSAGEMENU:
      myOverlay = &myOSystem.messageMenu();
      enableTextEvents(true);
      break;

    case EventHandlerState::TIMEMACHINE:
      myOSystem.timeMachine().requestResize();
      myOverlay = &myOSystem.timeMachine();
      enableTextEvents(true);
      break;

    case EventHandlerState::LAUNCHER:
      myOverlay = &myOSystem.launcher();
      enableTextEvents(true);
      break;
  #endif

  #ifdef DEBUGGER_SUPPORT
    case EventHandlerState::DEBUGGER:
      myOverlay = &myOSystem.debugger();
      enableTextEvents(true);
      break;
  #endif

    case EventHandlerState::NONE:
    default:
      break;
  }

  // Inform various subsystems about the new state
  myOSystem.stateChanged(myState);
  myOSystem.frameBuffer().stateChanged(myState);
  myOSystem.frameBuffer().setCursorState();
  if(myOSystem.hasConsole())
    myOSystem.console().stateChanged(myState);

  // Sometimes an extraneous mouse motion event is generated
  // after a state change, which should be supressed
  mySkipMouseMotion = true;

  // Erase any previously set events, since a state change implies
  // that old events are now invalid
  myEvent.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::exitEmulation(bool checkLauncher)
{
  const string saveOnExit = myOSystem.settings().getString("saveonexit");
  const bool activeTM = myOSystem.settings().getBool(
    myOSystem.settings().getBool("dev.settings") ? "dev.timemachine" : "plr.timemachine");


  if (saveOnExit == "all" && activeTM)
    handleEvent(Event::SaveAllStates);
  else if (saveOnExit == "current")
    handleEvent(Event::SaveState);

  if (checkLauncher)
  {
    // Go back to the launcher, or immediately quit
    if (myOSystem.settings().getBool("exitlauncher") ||
        myOSystem.launcherUsed())
      myOSystem.createLauncher();
    else
      handleEvent(Event::Quit);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::EmulActionList EventHandler::ourEmulActionList = { {
  { Event::Quit,                    "Quit",                                  "" },
  { Event::ReloadConsole,           "Reload current ROM/load next game",     "" },
  { Event::PreviousMultiCartRom,    "Load previous multicart game",          "" },
  { Event::ExitMode,                "Exit current Stella menu/mode",         "" },
  { Event::OptionsMenuMode,         "Enter Options menu UI",                 "" },
  { Event::CmdMenuMode,             "Toggle Commands menu UI",               "" },
  { Event::HighScoresMenuMode,      "Toggle High Scores UI",                 "" },
  { Event::TogglePauseMode,         "Toggle Pause mode",                     "" },
  { Event::StartPauseMode,          "Start Pause mode",                      "" },
  { Event::Fry,                     "Fry cartridge",                         "" },
  { Event::DecreaseSpeed,           "Decrease emulation speed",              "" },
  { Event::IncreaseSpeed,           "Increase emulation speed",              "" },
  { Event::ToggleTurbo,             "Toggle 'Turbo' mode",                   "" },
  { Event::DebuggerMode,            "Toggle Debugger mode",                  "" },

  { Event::ConsoleSelect,           "Select",                                "" },
  { Event::ConsoleReset,            "Reset",                                 "" },
  { Event::ConsoleColor,            "Color TV",                              "" },
  { Event::ConsoleBlackWhite,       "Black & White TV",                      "" },
  { Event::ConsoleColorToggle,      "Toggle Color / B&W TV",                 "" },
  { Event::Console7800Pause,        "7800 Pause Key",                        "" },
  { Event::ConsoleLeftDiffA,        "P0 Difficulty A",                       "" },
  { Event::ConsoleLeftDiffB,        "P0 Difficulty B",                       "" },
  { Event::ConsoleLeftDiffToggle,   "P0 Toggle Difficulty",                  "" },
  { Event::ConsoleRightDiffA,       "P1 Difficulty A",                       "" },
  { Event::ConsoleRightDiffB,       "P1 Difficulty B",                       "" },
  { Event::ConsoleRightDiffToggle,  "P1 Toggle Difficulty",                  "" },
  { Event::SaveState,               "Save state",                            "" },
  { Event::SaveAllStates,           "Save all TM states of current game",    "" },
  { Event::PreviousState,           "Change to previous state slot",         "" },
  { Event::NextState,               "Change to next state slot",             "" },
  { Event::ToggleAutoSlot,          "Toggle automatic state slot change",    "" },
  { Event::LoadState,               "Load state",                            "" },
  { Event::LoadAllStates,           "Load saved TM states for current game", "" },

#ifdef PNG_SUPPORT
  { Event::TakeSnapshot,            "Snapshot",                              "" },
  { Event::ToggleContSnapshots,     "Save continuous snapsh. (as defined)",  "" },
  { Event::ToggleContSnapshotsFrame,"Save continuous snapsh. (every frame)", "" },
#endif

  { Event::JoystickZeroUp,          "P0 Joystick Up",                        "" },
  { Event::JoystickZeroDown,        "P0 Joystick Down",                      "" },
  { Event::JoystickZeroLeft,        "P0 Joystick Left",                      "" },
  { Event::JoystickZeroRight,       "P0 Joystick Right",                     "" },
  { Event::JoystickZeroFire,        "P0 Joystick Fire",                      "" },
  { Event::JoystickZeroFire5,       "P0 Booster Top Booster Button",         "" },
  { Event::JoystickZeroFire9,       "P0 Booster Handle Grip Trigger",        "" },

  { Event::JoystickOneUp,           "P1 Joystick Up",                        "" },
  { Event::JoystickOneDown,         "P1 Joystick Down",                      "" },
  { Event::JoystickOneLeft,         "P1 Joystick Left",                      "" },
  { Event::JoystickOneRight,        "P1 Joystick Right",                     "" },
  { Event::JoystickOneFire,         "P1 Joystick Fire",                      "" },
  { Event::JoystickOneFire5,        "P1 Booster Top Booster Button",         "" },
  { Event::JoystickOneFire9,        "P1 Booster Handle Grip Trigger",        "" },

  { Event::JoystickTwoUp,           "P2 Joystick Up",                        "" },
  { Event::JoystickTwoDown,         "P2 Joystick Down",                      "" },
  { Event::JoystickTwoLeft,         "P2 Joystick Left",                      "" },
  { Event::JoystickTwoRight,        "P2 Joystick Right",                     "" },
  { Event::JoystickTwoFire,         "P2 Joystick Fire",                      "" },

  { Event::JoystickThreeUp,         "P3 Joystick Up",                        "" },
  { Event::JoystickThreeDown,       "P3 Joystick Down",                      "" },
  { Event::JoystickThreeLeft,       "P3 Joystick Left",                      "" },
  { Event::JoystickThreeRight,      "P3 Joystick Right",                     "" },
  { Event::JoystickThreeFire,       "P3 Joystick Fire",                      "" },

  { Event::PaddleZeroAnalog,        "Paddle 0 Analog",                       "" },
  { Event::PaddleZeroIncrease,      "Paddle 0 Turn Left",                    "" },
  { Event::PaddleZeroDecrease,      "Paddle 0 Turn Right",                   "" },
  { Event::PaddleZeroFire,          "Paddle 0 Fire",                         "" },

  { Event::PaddleOneAnalog,         "Paddle 1 Analog",                       "" },
  { Event::PaddleOneIncrease,       "Paddle 1 Turn Left",                    "" },
  { Event::PaddleOneDecrease,       "Paddle 1 Turn Right",                   "" },
  { Event::PaddleOneFire,           "Paddle 1 Fire",                         "" },

  { Event::PaddleTwoAnalog,         "Paddle 2 Analog",                       "" },
  { Event::PaddleTwoIncrease,       "Paddle 2 Turn Left",                    "" },
  { Event::PaddleTwoDecrease,       "Paddle 2 Turn Right",                   "" },
  { Event::PaddleTwoFire,           "Paddle 2 Fire",                         "" },

  { Event::PaddleThreeAnalog,       "Paddle 3 Analog",                       "" },
  { Event::PaddleThreeIncrease,     "Paddle 3 Turn Left",                    "" },
  { Event::PaddleThreeDecrease,     "Paddle 3 Turn Right",                   "" },
  { Event::PaddleThreeFire,         "Paddle 3 Fire",                         "" },

  { Event::PaddleFourFire,          "Paddle 4 Fire",                         "" },
  { Event::PaddleFiveFire,          "Paddle 5 Fire",                         "" },
  { Event::PaddleSixFire,           "Paddle 6 Fire",                         "" },
  { Event::PaddleSevenFire,         "Paddle 7 Fire",                         "" },

  { Event::KeyboardZero1,           "P0 Keyboard 1",                         "" },
  { Event::KeyboardZero2,           "P0 Keyboard 2",                         "" },
  { Event::KeyboardZero3,           "P0 Keyboard 3",                         "" },
  { Event::KeyboardZero4,           "P0 Keyboard 4",                         "" },
  { Event::KeyboardZero5,           "P0 Keyboard 5",                         "" },
  { Event::KeyboardZero6,           "P0 Keyboard 6",                         "" },
  { Event::KeyboardZero7,           "P0 Keyboard 7",                         "" },
  { Event::KeyboardZero8,           "P0 Keyboard 8",                         "" },
  { Event::KeyboardZero9,           "P0 Keyboard 9",                         "" },
  { Event::KeyboardZeroStar,        "P0 Keyboard *",                         "" },
  { Event::KeyboardZero0,           "P0 Keyboard 0",                         "" },
  { Event::KeyboardZeroPound,       "P0 Keyboard #",                         "" },

  { Event::KeyboardOne1,            "P1 Keyboard 1",                         "" },
  { Event::KeyboardOne2,            "P1 Keyboard 2",                         "" },
  { Event::KeyboardOne3,            "P1 Keyboard 3",                         "" },
  { Event::KeyboardOne4,            "P1 Keyboard 4",                         "" },
  { Event::KeyboardOne5,            "P1 Keyboard 5",                         "" },
  { Event::KeyboardOne6,            "P1 Keyboard 6",                         "" },
  { Event::KeyboardOne7,            "P1 Keyboard 7",                         "" },
  { Event::KeyboardOne8,            "P1 Keyboard 8",                         "" },
  { Event::KeyboardOne9,            "P1 Keyboard 9",                         "" },
  { Event::KeyboardOneStar,         "P1 Keyboard *",                         "" },
  { Event::KeyboardOne0,            "P1 Keyboard 0",                         "" },
  { Event::KeyboardOnePound,        "P1 Keyboard #",                         "" },
  // Video
  { Event::ToggleFullScreen,        "Toggle fullscreen",                     "" },
#ifdef ADAPTABLE_REFRESH_SUPPORT
  { Event::ToggleAdaptRefresh,      "Toggle fullscreen refresh rate adapt",  "" },
#endif
  { Event::OverscanDecrease,        "Decrease overscan in fullscreen mode",  "" },
  { Event::OverscanIncrease,        "Increase overscan in fullscreen mode",  "" },
  { Event::VidmodeDecrease,         "Previous zoom level",                   "" },
  { Event::VidmodeIncrease,         "Next zoom level",                       "" },
  { Event::ToggleCorrectAspectRatio,"Toggle aspect ratio correct scaling",   "" },
  { Event::VSizeAdjustDecrease,     "Decrease vertical display size",        "" },
  { Event::VSizeAdjustIncrease,     "Increase vertical display size",        "" },
  { Event::VCenterDecrease,         "Move display up",                       "" },
  { Event::VCenterIncrease,         "Move display down",                     "" },
  { Event::FormatDecrease,          "Decrease display format",               "" },
  { Event::FormatIncrease,          "Increase display format",               "" },
    // Palette settings
  { Event::PaletteDecrease,         "Switch to previous palette",            "" },
  { Event::PaletteIncrease,         "Switch to next palette",                "" },
  { Event::PreviousPaletteAttribute,"Select previous palette attribute",     "" },
  { Event::NextPaletteAttribute,    "Select next palette attribute",         "" },
  { Event::PaletteAttributeDecrease,"Decrease selected palette attribute",   "" },
  { Event::PaletteAttributeIncrease,"Increase selected palette attribute",   "" },
  { Event::ToggleInter,             "Toggle display interpolation",          "" },
  // Blargg TV effects:
  { Event::VidmodeStd,              "Disable TV effects",                    "" },
  { Event::VidmodeRGB,              "Select 'RGB' preset",                   "" },
  { Event::VidmodeSVideo,           "Select 'S-Video' preset",               "" },
  { Event::VidModeComposite,        "Select 'Composite' preset",             "" },
  { Event::VidModeBad,              "Select 'Badly adjusted' preset",        "" },
  { Event::VidModeCustom,           "Select 'Custom' preset",                "" },
  { Event::PreviousVideoMode,       "Select previous TV effect mode preset", "" },
  { Event::NextVideoMode,           "Select next TV effect mode preset",     "" },
  { Event::PreviousAttribute,       "Select previous 'Custom' attribute",    "" },
  { Event::NextAttribute,           "Select next 'Custom' attribute",        "" },
  { Event::DecreaseAttribute,       "Decrease selected 'Custom' attribute",  "" },
  { Event::IncreaseAttribute,       "Increase selected 'Custom' attribute",  "" },
  // Other TV effects
  { Event::TogglePhosphor,          "Toggle 'phosphor' effect",              "" },
  { Event::PhosphorDecrease,        "Decrease 'phosphor' blend",             "" },
  { Event::PhosphorIncrease,        "Increase 'phosphor' blend",             "" },
  { Event::ScanlinesDecrease,       "Decrease scanlines",                    "" },
  { Event::ScanlinesIncrease,       "Increase scanlines",                    "" },

  { Event::PreviousSettingGroup,    "Select previous setting group",         "" },
  { Event::NextSettingGroup,        "Select next setting group",             "" },
  { Event::PreviousSetting,         "Select previous setting",               "" },
  { Event::NextSetting,             "Select next setting",                   "" },
  { Event::SettingDecrease,         "Decrease current setting",              "" },
  { Event::SettingIncrease,         "Increase current setting",              "" },

  // Developer keys:
  { Event::ToggleFrameStats,        "Toggle frame stats",                    "" },
  { Event::ToggleP0Bit,             "Toggle TIA Player0 object",             "" },
  { Event::ToggleP0Collision,       "Toggle TIA Player0 collisions",         "" },
  { Event::ToggleP1Bit,             "Toggle TIA Player1 object",             "" },
  { Event::ToggleP1Collision,       "Toggle TIA Player1 collisions",         "" },
  { Event::ToggleM0Bit,             "Toggle TIA Missile0 object",            "" },
  { Event::ToggleM0Collision,       "Toggle TIA Missile0 collisions",        "" },
  { Event::ToggleM1Bit,             "Toggle TIA Missile1 object",            "" },
  { Event::ToggleM1Collision,       "Toggle TIA Missile1 collisions",        "" },
  { Event::ToggleBLBit,             "Toggle TIA Ball object",                "" },
  { Event::ToggleBLCollision,       "Toggle TIA Ball collisions",            "" },
  { Event::TogglePFBit,             "Toggle TIA Playfield object",           "" },
  { Event::TogglePFCollision,       "Toggle TIA Playfield collisions",       "" },
  { Event::ToggleBits,              "Toggle all TIA objects",                "" },
  { Event::ToggleCollisions,        "Toggle all TIA collisions",             "" },
  { Event::ToggleFixedColors,       "Toggle TIA 'Fixed Debug Colors' mode",  "" },
  { Event::ToggleColorLoss,         "Toggle PAL color-loss effect",          "" },
  { Event::ToggleJitter,            "Toggle TV 'Jitter' effect",             "" },
  // Other keys:
  { Event::SoundToggle,             "Toggle sound",                          "" },
  { Event::VolumeDecrease,          "Decrease volume",                       "" },
  { Event::VolumeIncrease,          "Increase volume",                       "" },


  { Event::DecreaseDeadzone,        "Decrease joystick deadzone",            "" },
  { Event::IncreaseDeadzone,        "Increase joystick deadzone",            "" },
  { Event::DecAnalogSense,          "Decrease analog paddle sensitivity",    "" },
  { Event::IncAnalogSense,          "Increase analog paddle sensitivity",    "" },
  { Event::DecDejtterAveraging,     "Decrease paddle dejitter averaging",    "" },
  { Event::IncDejtterAveraging,     "Increase paddle dejitter averaging",    "" },
  { Event::DecDejtterReaction,      "Decrease paddle dejitter reaction",     "" },
  { Event::IncDejtterReaction,      "Increase paddle dejitter reaction",     "" },
  { Event::DecDigitalSense,         "Decrease digital paddle sensitivity",   "" },
  { Event::IncDigitalSense,         "Increase digital paddle sensitivity",   "" },
  { Event::DecreaseAutoFire,        "Decrease auto fire speed",              "" },
  { Event::IncreaseAutoFire,        "Increase auto fire speed",              "" },
  { Event::ToggleFourDirections,    "Toggle allow four joystick directions", "" },
  { Event::ToggleKeyCombos,         "Toggle use of modifier key combos",     "" },
  { Event::ToggleSAPortOrder,       "Swap Stelladaptor port ordering",       "" },
  { Event::PrevMouseAsController,   "Select previous mouse controls",        "" },
  { Event::NextMouseAsController,   "Select next mouse controls",            "" },
  { Event::DecMousePaddleSense,     "Decrease mouse paddle sensitivity",     "" },
  { Event::IncMousePaddleSense,     "Increase mouse paddle sensitivity",     "" },
  { Event::DecMouseTrackballSense,  "Decrease mouse trackball sensitivity",  "" },
  { Event::IncMouseTrackballSense,  "Increase mouse trackball sensitivity",  "" },
  { Event::DecreaseDrivingSense,    "Decrease driving sensitivity",          "" },
  { Event::IncreaseDrivingSense,    "Increase driving sensitivity",          "" },
  { Event::PreviousCursorVisbility, "Select prev. cursor visibility mode",   "" },
  { Event::NextCursorVisbility,     "Select next cursor visibility mode"    ,"" },
  { Event::ToggleGrabMouse,         "Toggle grab mouse",                     "" },
  { Event::PreviousLeftPort,        "Select previous left controller",       "" },
  { Event::NextLeftPort,            "Select next left controller",           "" },
  { Event::PreviousRightPort,       "Select previous right controller",      "" },
  { Event::NextRightPort,           "Select next right controller",          "" },
  { Event::ToggleSwapPorts,         "Toggle swap ports",                     "" },
  { Event::ToggleSwapPaddles,       "Toggle swap paddles",                   "" },
  { Event::PreviousMouseControl,    "Select previous mouse emulation mode",  "" },
  { Event::NextMouseControl,        "Select next mouse emulation mode",      "" },
  { Event::DecreaseMouseAxesRange,  "Decrease mouse axes range",             "" },
  { Event::IncreaseMouseAxesRange,  "Increase mouse axes range",             "" },

  { Event::ToggleTimeMachine,       "Toggle 'Time Machine' mode",            "" },
  { Event::TimeMachineMode,         "Toggle 'Time Machine' UI",              "" },
  { Event::RewindPause,             "Rewind one state & enter Pause mode",   "" },
  { Event::Rewind1Menu,             "Rewind one state & enter TM UI",        "" },
  { Event::Rewind10Menu,            "Rewind 10 states & enter TM UI",        "" },
  { Event::RewindAllMenu,           "Rewind all states & enter TM UI",       "" },
  { Event::UnwindPause,             "Unwind one state & enter Pause mode",   "" },
  { Event::Unwind1Menu,             "Unwind one state & enter TM UI",        "" },
  { Event::Unwind10Menu,            "Unwind 10 states & enter TM UI",        "" },
  { Event::UnwindAllMenu,           "Unwind all states & enter TM UI",       "" },
  { Event::TogglePlayBackMode,      "Toggle 'Time Machine' playback mode",   "" },

  { Event::Combo1,                  "Combo 1",                               "" },
  { Event::Combo2,                  "Combo 2",                               "" },
  { Event::Combo3,                  "Combo 3",                               "" },
  { Event::Combo4,                  "Combo 4",                               "" },
  { Event::Combo5,                  "Combo 5",                               "" },
  { Event::Combo6,                  "Combo 6",                               "" },
  { Event::Combo7,                  "Combo 7",                               "" },
  { Event::Combo8,                  "Combo 8",                               "" },
  { Event::Combo9,                  "Combo 9",                               "" },
  { Event::Combo10,                 "Combo 10",                              "" },
  { Event::Combo11,                 "Combo 11",                              "" },
  { Event::Combo12,                 "Combo 12",                              "" },
  { Event::Combo13,                 "Combo 13",                              "" },
  { Event::Combo14,                 "Combo 14",                              "" },
  { Event::Combo15,                 "Combo 15",                              "" },
  { Event::Combo16,                 "Combo 16",                              "" },
} };

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::MenuActionList EventHandler::ourMenuActionList = { {
  { Event::UIHelp,            "Open context-sensitive help",  "" },

  { Event::UIUp,              "Move Up",                      "" },
  { Event::UIDown,            "Move Down",                    "" },
  { Event::UILeft,            "Move Left",                    "" },
  { Event::UIRight,           "Move Right",                   "" },

  { Event::UIHome,            "Home",                         "" },
  { Event::UIEnd,             "End",                          "" },
  { Event::UIPgUp,            "Page Up",                      "" },
  { Event::UIPgDown,          "Page Down",                    "" },

  { Event::UIOK,              "OK",                           "" },
  { Event::UICancel,          "Cancel",                       "" },
  { Event::UISelect,          "Select item",                  "" },

  { Event::UINavPrev,         "Previous object",              "" },
  { Event::UINavNext,         "Next object",                  "" },
  { Event::UITabPrev,         "Previous tab",                 "" },
  { Event::UITabNext,         "Next tab",                     "" },

  { Event::UIPrevDir,         "Parent directory",             "" },
  { Event::ToggleFullScreen,  "Toggle fullscreen",            "" },
  { Event::Quit,              "Quit",                         "" }
} };

// Event groups
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Event::EventSet EventHandler::MiscEvents = {
  Event::Quit, Event::ReloadConsole, Event::Fry, Event::StartPauseMode,
  Event::TogglePauseMode, Event::OptionsMenuMode, Event::CmdMenuMode, Event::ExitMode,
  Event::ToggleTurbo, Event::DecreaseSpeed, Event::IncreaseSpeed,
  Event::TakeSnapshot, Event::ToggleContSnapshots, Event::ToggleContSnapshotsFrame,
  // Event::MouseAxisXMove, Event::MouseAxisYMove,
  // Event::MouseButtonLeftValue, Event::MouseButtonRightValue,
  Event::HighScoresMenuMode,
  Event::PreviousMultiCartRom,
  Event::PreviousSettingGroup, Event::NextSettingGroup,
  Event::PreviousSetting, Event::NextSetting,
  Event::SettingDecrease, Event::SettingIncrease,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Event::EventSet EventHandler::AudioVideoEvents = {
  Event::VolumeDecrease, Event::VolumeIncrease, Event::SoundToggle,
  Event::VidmodeDecrease, Event::VidmodeIncrease,
  Event::ToggleFullScreen, Event::ToggleAdaptRefresh,
  Event::OverscanDecrease, Event::OverscanIncrease,
  Event::FormatDecrease, Event::FormatIncrease,
  Event::VCenterDecrease, Event::VCenterIncrease,
  Event::VSizeAdjustDecrease, Event::VSizeAdjustIncrease, Event::ToggleCorrectAspectRatio,
  Event::PaletteDecrease, Event::PaletteIncrease,
  Event::PreviousPaletteAttribute, Event::NextPaletteAttribute,
  Event::PaletteAttributeDecrease, Event::PaletteAttributeIncrease,
  Event::VidmodeStd, Event::VidmodeRGB, Event::VidmodeSVideo, Event::VidModeComposite, Event::VidModeBad, Event::VidModeCustom,
  Event::PreviousVideoMode, Event::NextVideoMode,
  Event::PreviousAttribute, Event::NextAttribute, Event::DecreaseAttribute, Event::IncreaseAttribute,
  Event::PhosphorDecrease, Event::PhosphorIncrease, Event::TogglePhosphor,
  Event::ScanlinesDecrease, Event::ScanlinesIncrease,
  Event::ToggleInter,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Event::EventSet EventHandler::StateEvents = {
  Event::NextState, Event::PreviousState, Event::LoadState, Event::SaveState,
  Event::TimeMachineMode, Event::RewindPause, Event::UnwindPause, Event::ToggleTimeMachine,
  Event::Rewind1Menu, Event::Rewind10Menu, Event::RewindAllMenu,
  Event::Unwind1Menu, Event::Unwind10Menu, Event::UnwindAllMenu,
  Event::TogglePlayBackMode,
  Event::SaveAllStates, Event::LoadAllStates, Event::ToggleAutoSlot,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Event::EventSet EventHandler::ConsoleEvents = {
  Event::ConsoleColor, Event::ConsoleBlackWhite,
  Event::ConsoleLeftDiffA, Event::ConsoleLeftDiffB,
  Event::ConsoleRightDiffA, Event::ConsoleRightDiffB,
  Event::ConsoleSelect, Event::ConsoleReset,
  Event::ConsoleLeftDiffToggle, Event::ConsoleRightDiffToggle, Event::ConsoleColorToggle,
  Event::Console7800Pause,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Event::EventSet EventHandler::JoystickEvents = {
  Event::JoystickZeroUp, Event::JoystickZeroDown, Event::JoystickZeroLeft, Event::JoystickZeroRight,
  Event::JoystickZeroFire, Event::JoystickZeroFire5, Event::JoystickZeroFire9,
  Event::JoystickOneUp, Event::JoystickOneDown, Event::JoystickOneLeft, Event::JoystickOneRight,
  Event::JoystickOneFire, Event::JoystickOneFire5, Event::JoystickOneFire9,
  Event::JoystickTwoUp, Event::JoystickTwoDown, Event::JoystickTwoLeft, Event::JoystickTwoRight,
  Event::JoystickTwoFire,
  Event::JoystickThreeUp, Event::JoystickThreeDown, Event::JoystickThreeLeft, Event::JoystickThreeRight,
  Event::JoystickThreeFire,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Event::EventSet EventHandler::PaddlesEvents = {
  Event::PaddleZeroDecrease, Event::PaddleZeroIncrease, Event::PaddleZeroAnalog, Event::PaddleZeroFire,
  Event::PaddleOneDecrease, Event::PaddleOneIncrease, Event::PaddleOneAnalog, Event::PaddleOneFire,
  Event::PaddleTwoDecrease, Event::PaddleTwoIncrease, Event::PaddleTwoAnalog, Event::PaddleTwoFire,
  Event::PaddleThreeDecrease, Event::PaddleThreeIncrease, Event::PaddleThreeAnalog, Event::PaddleThreeFire,
  Event::PaddleFourFire, Event::PaddleFiveFire,Event::PaddleSixFire,Event::PaddleSevenFire,
};

const Event::EventSet EventHandler::KeyboardEvents = {
  Event::KeyboardZero1, Event::KeyboardZero2, Event::KeyboardZero3,
  Event::KeyboardZero4, Event::KeyboardZero5, Event::KeyboardZero6,
  Event::KeyboardZero7, Event::KeyboardZero8, Event::KeyboardZero9,
  Event::KeyboardZeroStar, Event::KeyboardZero0, Event::KeyboardZeroPound,

  Event::KeyboardOne1, Event::KeyboardOne2, Event::KeyboardOne3,
  Event::KeyboardOne4, Event::KeyboardOne5, Event::KeyboardOne6,
  Event::KeyboardOne7, Event::KeyboardOne8, Event::KeyboardOne9,
  Event::KeyboardOneStar, Event::KeyboardOne0, Event::KeyboardOnePound,
};

const Event::EventSet EventHandler::DevicesEvents = {
  Event::DecreaseDeadzone, Event::IncreaseDeadzone,
  Event::DecAnalogSense, Event::IncAnalogSense,
  Event::DecDejtterAveraging, Event::IncDejtterAveraging,
  Event::DecDejtterReaction, Event::IncDejtterReaction,
  Event::DecDigitalSense, Event::IncDigitalSense,
  Event::DecreaseAutoFire, Event::IncreaseAutoFire,
  Event::ToggleFourDirections, Event::ToggleKeyCombos, Event::ToggleSAPortOrder,
  Event::PrevMouseAsController, Event::NextMouseAsController,
  Event::DecMousePaddleSense, Event::IncMousePaddleSense,
  Event::DecMouseTrackballSense, Event::IncMouseTrackballSense,
  Event::DecreaseDrivingSense, Event::IncreaseDrivingSense,
  Event::PreviousCursorVisbility, Event::NextCursorVisbility,
  Event::ToggleGrabMouse,
  Event::PreviousLeftPort, Event::NextLeftPort,
  Event::PreviousRightPort, Event::NextRightPort,
  Event::ToggleSwapPorts, Event::ToggleSwapPaddles,
  Event::DecreasePaddleCenterX, Event::IncreasePaddleCenterX,
  Event::DecreasePaddleCenterY, Event::IncreasePaddleCenterY,
  Event::PreviousMouseControl, Event::NextMouseControl,
  Event::DecreaseMouseAxesRange, Event::IncreaseMouseAxesRange,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Event::EventSet EventHandler::ComboEvents = {
  Event::Combo1, Event::Combo2, Event::Combo3, Event::Combo4,
  Event::Combo5, Event::Combo6, Event::Combo7, Event::Combo8,
  Event::Combo9, Event::Combo10, Event::Combo11, Event::Combo12,
  Event::Combo13, Event::Combo14, Event::Combo15, Event::Combo16,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Event::EventSet EventHandler::DebugEvents = {
  Event::DebuggerMode,
  Event::ToggleFrameStats,
  Event::ToggleP0Collision, Event::ToggleP0Bit, Event::ToggleP1Collision, Event::ToggleP1Bit,
  Event::ToggleM0Collision, Event::ToggleM0Bit, Event::ToggleM1Collision, Event::ToggleM1Bit,
  Event::ToggleBLCollision, Event::ToggleBLBit, Event::TogglePFCollision, Event::TogglePFBit,
  Event::ToggleCollisions, Event::ToggleBits, Event::ToggleFixedColors,
  Event::ToggleColorLoss,
  Event::ToggleJitter,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Event::EventSet EventHandler::EditEvents = {

};
