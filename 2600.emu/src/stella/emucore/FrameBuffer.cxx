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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "bspf.hxx"
#include "Logger.hxx"

#include "Console.hxx"
#include "EventHandler.hxx"
#include "Event.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "TIA.hxx"
#include "Sound.hxx"

#include "FBSurface.hxx"
#include "TIASurface.hxx"
#include "FrameBuffer.hxx"

#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif
#ifdef GUI_SUPPORT
  #include "Font.hxx"
  #include "StellaFont.hxx"
  #include "StellaMediumFont.hxx"
  #include "StellaLargeFont.hxx"
  #include "ConsoleFont.hxx"
  #include "Launcher.hxx"
  #include "Menu.hxx"
  #include "CommandMenu.hxx"
  #include "MessageMenu.hxx"
  #include "TimeMachine.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::FrameBuffer(OSystem& osystem)
  : myOSystem(osystem)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::~FrameBuffer()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::initialize()
{
  // Get desktop resolution and supported renderers
  vector<Common::Size> windowedDisplays;
  queryHardware(myFullscreenDisplays, windowedDisplays, myRenderers);
  uInt32 query_w = windowedDisplays[0].w, query_h = windowedDisplays[0].h;

  // Check the 'maxres' setting, which is an undocumented developer feature
  // that specifies the desktop size (not normally set)
  const Common::Size& s = myOSystem.settings().getSize("maxres");
  if(s.valid())
  {
    query_w = s.w;
    query_h = s.h;
  }
  // Various parts of the codebase assume a minimum screen size
  myAbsDesktopSize.w = std::max(query_w, FBMinimum::Width);
  myAbsDesktopSize.h = std::max(query_h, FBMinimum::Height);
  myDesktopSize = myAbsDesktopSize;

  // Check for HiDPI mode (is it activated, and can we use it?)
  myHiDPIAllowed = ((myAbsDesktopSize.w / 2) >= FBMinimum::Width) &&
    ((myAbsDesktopSize.h / 2) >= FBMinimum::Height);
  myHiDPIEnabled = myHiDPIAllowed && myOSystem.settings().getBool("hidpi");

  // In HiDPI mode, the desktop resolution is essentially halved
  // Later, the output is scaled and rendered in 2x mode
  if(hidpiEnabled())
  {
    myDesktopSize.w = myAbsDesktopSize.w / hidpiScaleFactor();
    myDesktopSize.h = myAbsDesktopSize.h / hidpiScaleFactor();
  }

#ifdef GUI_SUPPORT
  ////////////////////////////////////////////////////////////////////
  // Create fonts to draw text
  // NOTE: the logic determining appropriate font sizes is done here,
  //       so that the UI classes can just use the font they expect,
  //       and not worry about it
  //       This logic should also take into account the size of the
  //       framebuffer, and try to be intelligent about font sizes
  //       We can probably add ifdefs to take care of corner cases,
  //       but that means we've failed to abstract it enough ...
  ////////////////////////////////////////////////////////////////////

  // This font is used in a variety of situations when a really small
  // font is needed; we let the specific widget/dialog decide when to
  // use it
  mySmallFont = make_unique<GUI::Font>(GUI::stellaDesc);

  // The general font used in all UI elements
  // This is determined by the size of the framebuffer
  if(myOSystem.settings().getBool("minimal_ui"))
    myFont = make_unique<GUI::Font>(GUI::stellaLargeDesc);
  else
    myFont = make_unique<GUI::Font>(GUI::stellaMediumDesc);

  // The info font used in all UI elements
  // This is determined by the size of the framebuffer
  myInfoFont = make_unique<GUI::Font>(GUI::consoleDesc);

  // The font used by the ROM launcher
  const string& lf = myOSystem.settings().getString("launcherfont");
  if(lf == "small")
    myLauncherFont = make_unique<GUI::Font>(GUI::consoleDesc);
  else if(lf == "medium")
    myLauncherFont = make_unique<GUI::Font>(GUI::stellaMediumDesc);
  else
    myLauncherFont = make_unique<GUI::Font>(GUI::stellaLargeDesc);
#endif

  // Determine possible TIA windowed zoom levels
  myTIAMaxZoom = maxZoomForScreen(
      TIAConstants::viewableWidth, TIAConstants::viewableHeight,
      myAbsDesktopSize.w, myAbsDesktopSize.h);

  setUIPalette();

  myGrabMouse = myOSystem.settings().getBool("grabmouse");

  // Create a TIA surface; we need it for rendering TIA images
  myTIASurface = make_unique<TIASurface>(myOSystem);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBInitStatus FrameBuffer::createDisplay(const string& title,
                                        uInt32 width, uInt32 height,
                                        bool honourHiDPI)
{
  ++myInitializedCount;
  myScreenTitle = title;

  // In HiDPI mode, all created displays must be scaled by 2x
  if(honourHiDPI && hidpiEnabled())
  {
    width  *= hidpiScaleFactor();
    height *= hidpiScaleFactor();
  }

  // A 'windowed' system is defined as one where the window size can be
  // larger than the screen size, as there's some sort of window manager
  // that takes care of it (all current desktop systems fall in this category)
  // However, some systems have no concept of windowing, and have hard limits
  // on how large a window can be (ie, the size of the 'desktop' is the
  // absolute upper limit on window size)
  //
  // If the WINDOWED_SUPPORT macro is defined, we treat the system as the
  // former type; if not, as the latter type

  bool useFullscreen = false;
#ifdef WINDOWED_SUPPORT
  // We assume that a desktop of at least minimum acceptable size means that
  // we're running on a 'large' system, and the window size requirements
  // can be relaxed
  // Otherwise, we treat the system as if WINDOWED_SUPPORT is not defined
  if(myDesktopSize.w < FBMinimum::Width && myDesktopSize.h < FBMinimum::Height &&
    (myDesktopSize.w < width || myDesktopSize.h < height))
    return FBInitStatus::FailTooLarge;

  useFullscreen = myOSystem.settings().getBool("fullscreen");
#else
  // Make sure this mode is even possible
  // We only really need to worry about it in non-windowed environments,
  // where requesting a window that's too large will probably cause a crash
  if(myDesktopSize.w < width || myDesktopSize.h < height)
    return FBInitStatus::FailTooLarge;

  useFullscreen = true;
#endif

  // Set the available video modes for this framebuffer
  setAvailableVidModes(width, height);

  // Initialize video subsystem (make sure we get a valid mode)
  string pre_about = about();
  const FrameBuffer::VideoMode& mode = getSavedVidMode(useFullscreen);
  if(width <= mode.screen.w && height <= mode.screen.h)
  {
    // Changing the video mode can take some time, during which the last
    // sound played may get 'stuck'
    // So we mute the sound until the operation completes
    bool oldMuteState = myOSystem.sound().mute(true);
    if(setVideoMode(myScreenTitle, mode))
    {
      myImageRect = mode.image;
      myScreenSize = mode.screen;
      myScreenRect = Common::Rect(mode.screen);

      // Inform TIA surface about new mode
      if(myOSystem.eventHandler().state() != EventHandlerState::LAUNCHER &&
         myOSystem.eventHandler().state() != EventHandlerState::DEBUGGER)
        myTIASurface->initialize(myOSystem.console(), mode);

      // Did we get the requested fullscreen state?
      myOSystem.settings().setValue("fullscreen", fullScreen());
      resetSurfaces();
      setCursorState();

      myOSystem.sound().mute(oldMuteState);
    }
    else
    {
      Logger::error("ERROR: Couldn't initialize video subsystem");
      return FBInitStatus::FailNotSupported;
    }
  }
  else
    return FBInitStatus::FailTooLarge;

#ifdef GUI_SUPPORT
  // Erase any messages from a previous run
  myMsg.counter = 0;

  // Create surfaces for TIA statistics and general messages
  const GUI::Font& f = hidpiEnabled() ? infoFont() : font();
  myStatsMsg.color = kColorInfo;
  myStatsMsg.w = f.getMaxCharWidth() * 40 + 3;
  myStatsMsg.h = (f.getFontHeight() + 2) * 3;

  if(!myStatsMsg.surface)
  {
    myStatsMsg.surface = allocateSurface(myStatsMsg.w, myStatsMsg.h);
    myStatsMsg.surface->attributes().blending = true;
    myStatsMsg.surface->attributes().blendalpha = 92; //aligned with TimeMachineDialog
    myStatsMsg.surface->applyAttributes();
  }

  if(!myMsg.surface)
    myMsg.surface = allocateSurface(FBMinimum::Width, font().getFontHeight()+10);
#endif

  // Print initial usage message, but only print it later if the status has changed
  if(myInitializedCount == 1)
  {
    Logger::info(about());
  }
  else
  {
    string post_about = about();
    if(post_about != pre_about)
      Logger::info(post_about);
  }

  return FBInitStatus::Success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::update(bool force)
{
  // Onscreen messages are a special case and require different handling than
  // other objects; they aren't UI dialogs in the normal sense nor are they
  // TIA images, and they need to be rendered on top of everything
  // The logic is split in two pieces:
  //  - at the top of ::update(), to determine whether underlying dialogs
  //    need to be force-redrawn
  //  - at the bottom of ::update(), to actually draw them (this must come
  //    last, since they are always drawn on top of everything else).

  // Full rendering is required when messages are enabled
  force = force || myMsg.counter >= 0;

  // Detect when a message has been turned off; one last redraw is required
  // in this case, to draw over the area that the message occupied
  if(myMsg.counter == 0)
    myMsg.counter = -1;

  switch(myOSystem.eventHandler().state())
  {
    case EventHandlerState::NONE:
    case EventHandlerState::EMULATION:
      // Do nothing; emulation mode is handled separately (see below)
      return;

    case EventHandlerState::PAUSE:
    {
      // Show a pause message immediately and then every 7 seconds
      if(myPausedCount-- <= 0)
      {
        myPausedCount = uInt32(7 * myOSystem.frameRate());
        showMessage("Paused", MessagePosition::MiddleCenter);
      }
      if(force)
        myTIASurface->render();

      break;  // EventHandlerState::PAUSE
    }

  #ifdef GUI_SUPPORT
    case EventHandlerState::OPTIONSMENU:
    {
      force = force || myOSystem.menu().needsRedraw();
      if(force)
      {
        clear();
        myTIASurface->render();
        myOSystem.menu().draw(force);
      }
      break;  // EventHandlerState::OPTIONSMENU
    }

    case EventHandlerState::CMDMENU:
    {
      force = force || myOSystem.commandMenu().needsRedraw();
      if(force)
      {
        clear();
        myTIASurface->render();
        myOSystem.commandMenu().draw(force);
      }
      break;  // EventHandlerState::CMDMENU
    }

    case EventHandlerState::MESSAGEMENU:
    {
      force = force || myOSystem.messageMenu().needsRedraw();
      if (force)
      {
        clear();
        myTIASurface->render();
        myOSystem.messageMenu().draw(force);
      }
      break;  // EventHandlerState::MESSAGEMENU
    }

    case EventHandlerState::TIMEMACHINE:
    {
      force = force || myOSystem.timeMachine().needsRedraw();
      if(force)
      {
        clear();
        myTIASurface->render();
        myOSystem.timeMachine().draw(force);
      }
      break;  // EventHandlerState::TIMEMACHINE
    }

    case EventHandlerState::LAUNCHER:
    {
      force = force || myOSystem.launcher().needsRedraw();
      if(force)
      {
        clear();
        myOSystem.launcher().draw(force);
      }
      break;  // EventHandlerState::LAUNCHER
    }
  #endif

  #ifdef DEBUGGER_SUPPORT
    case EventHandlerState::DEBUGGER:
    {
      force = force || myOSystem.debugger().needsRedraw();
      if(force)
      {
        clear();
        myOSystem.debugger().draw(force);
      }
      break;  // EventHandlerState::DEBUGGER
    }
  #endif
    default:
      break;
  }

  // Draw any pending messages
  // The logic here determines whether to draw the message
  // If the message is to be disabled, logic inside the draw method
  // indicates that, and then the code at the top of this method sees
  // the change and redraws everything
  if(myMsg.enabled)
    drawMessage();

  // Push buffers to screen only when necessary
  if(force)
    renderToScreen();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::updateInEmulationMode(float framesPerSecond)
{
  // Update method that is specifically tailored to emulation mode
  // Typically called from a thread, so it needs to be separate from
  // the normal update() method
  //
  // We don't worry about selective rendering here; the rendering
  // always happens at the full framerate

  clear();  // TODO - test this: it may cause slowdowns on older systems
  myTIASurface->render();

  // Show frame statistics
  if(myStatsMsg.enabled)
    drawFrameStats(framesPerSecond);

  myLastScanlines = myOSystem.console().tia().frameBufferScanlinesLastFrame();
  myPausedCount = 0;

  // Draw any pending messages
  if(myMsg.enabled)
    drawMessage();

  // Push buffers to screen
  renderToScreen();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showMessage(const string& message, MessagePosition position,
                              bool force)
{
#ifdef GUI_SUPPORT
  // Only show messages if they've been enabled
  if(myMsg.surface == nullptr || !(force || myOSystem.settings().getBool("uimessages")))
    return;

  // Precompute the message coordinates
  myMsg.text    = message;
  myMsg.counter = uInt32(myOSystem.frameRate()) << 1; // Show message for 2 seconds
  if(myMsg.counter == 0)  myMsg.counter = 60;
  myMsg.color   = kBtnTextColor;

  myMsg.w = font().getStringWidth(myMsg.text) + 10;
  myMsg.h = font().getFontHeight() + 8;
  myMsg.surface->setSrcSize(myMsg.w, myMsg.h);
  myMsg.surface->setDstSize(myMsg.w * hidpiScaleFactor(), myMsg.h * hidpiScaleFactor());
  myMsg.position = position;
  myMsg.enabled = true;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::drawFrameStats(float framesPerSecond)
{
#ifdef GUI_SUPPORT
  const ConsoleInfo& info = myOSystem.console().about();
  int xPos = 2, yPos = 0;
  const GUI::Font& f = hidpiEnabled() ? infoFont() : font();
  const int dy = f.getFontHeight() + 2;

  ostringstream ss;

  myStatsMsg.surface->invalidate();

  // draw scanlines
  ColorId color = myOSystem.console().tia().frameBufferScanlinesLastFrame() !=
    myLastScanlines ? kDbgColorRed : myStatsMsg.color;

  ss
    << myOSystem.console().tia().frameBufferScanlinesLastFrame()
    << " / "
    << std::fixed << std::setprecision(1) << myOSystem.console().getFramerate()
    << "Hz => "
    << info.DisplayFormat;

  myStatsMsg.surface->drawString(f, ss.str(), xPos, yPos,
                                 myStatsMsg.w, color, TextAlign::Left, 0, true, kBGColor);

  yPos += dy;
  ss.str("");

  ss
    << std::fixed << std::setprecision(1) << framesPerSecond
    << "fps @ "
    << std::fixed << std::setprecision(0) << 100 * myOSystem.settings().getFloat("speed")
    << "% speed";

  myStatsMsg.surface->drawString(f, ss.str(), xPos, yPos,
      myStatsMsg.w, myStatsMsg.color, TextAlign::Left, 0, true, kBGColor);

  yPos += dy;
  ss.str("");

  ss << info.BankSwitch;
  if (myOSystem.settings().getBool("dev.settings")) ss << "| Developer";

  myStatsMsg.surface->drawString(f, ss.str(), xPos, yPos,
      myStatsMsg.w, myStatsMsg.color, TextAlign::Left, 0, true, kBGColor);

  myStatsMsg.surface->setDstPos(myImageRect.x() + 10, myImageRect.y() + 8);
  myStatsMsg.surface->setDstSize(myStatsMsg.w * hidpiScaleFactor(),
                                 myStatsMsg.h * hidpiScaleFactor());
  myStatsMsg.surface->render();
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleFrameStats()
{
  showFrameStats(!myStatsEnabled);
  myOSystem.settings().setValue(
    myOSystem.settings().getBool("dev.settings") ? "dev.stats" : "plr.stats", myStatsEnabled);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showFrameStats(bool enable)
{
  myStatsEnabled = myStatsMsg.enabled = enable;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::enableMessages(bool enable)
{
  if(enable)
  {
    // Only re-enable frame stats if they were already enabled before
    myStatsMsg.enabled = myStatsEnabled;
  }
  else
  {
    // Temporarily disable frame stats
    myStatsMsg.enabled = false;

    // Erase old messages on the screen
    myMsg.enabled = false;
    myMsg.counter = 0;
    update(true);  // Force update immediately
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool FrameBuffer::drawMessage()
{
#ifdef GUI_SUPPORT
  // Either erase the entire message (when time is reached),
  // or show again this frame
  if(myMsg.counter == 0)
  {
    myMsg.enabled = false;
    return true;
  }
  else if(myMsg.counter < 0)
  {
    myMsg.enabled = false;
    return false;
  }

  // Draw the bounded box and text
  const Common::Rect& dst = myMsg.surface->dstRect();

  switch(myMsg.position)
  {
    case MessagePosition::TopLeft:
      myMsg.x = 5;
      myMsg.y = 5;
      break;

    case MessagePosition::TopCenter:
      myMsg.x = (myImageRect.w() - dst.w()) >> 1;
      myMsg.y = 5;
      break;

    case MessagePosition::TopRight:
      myMsg.x = myImageRect.w() - dst.w() - 5;
      myMsg.y = 5;
      break;

    case MessagePosition::MiddleLeft:
      myMsg.x = 5;
      myMsg.y = (myImageRect.h() - dst.h()) >> 1;
      break;

    case MessagePosition::MiddleCenter:
      myMsg.x = (myImageRect.w() - dst.w()) >> 1;
      myMsg.y = (myImageRect.h() - dst.h()) >> 1;
      break;

    case MessagePosition::MiddleRight:
      myMsg.x = myImageRect.w() - dst.w() - 5;
      myMsg.y = (myImageRect.h() - dst.h()) >> 1;
      break;

    case MessagePosition::BottomLeft:
      myMsg.x = 5;
      myMsg.y = myImageRect.h() - dst.h() - 5;
      break;

    case MessagePosition::BottomCenter:
      myMsg.x = (myImageRect.w() - dst.w()) >> 1;
      myMsg.y = myImageRect.h() - dst.h() - 5;
      break;

    case MessagePosition::BottomRight:
      myMsg.x = myImageRect.w() - dst.w() - 5;
      myMsg.y = myImageRect.h() - dst.h() - 5;
      break;
  }

  myMsg.surface->setDstPos(myMsg.x + myImageRect.x(), myMsg.y + myImageRect.y());
  myMsg.surface->fillRect(1, 1, myMsg.w-2, myMsg.h-2, kBtnColor);
  myMsg.surface->frameRect(0, 0, myMsg.w, myMsg.h, kColor);
  myMsg.surface->drawString(font(), myMsg.text, 5, 4,
                            myMsg.w, myMsg.color, TextAlign::Left);
  myMsg.surface->render();
  myMsg.counter--;
#endif

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setPauseDelay()
{
  myPausedCount = uInt32(2 * myOSystem.frameRate());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
shared_ptr<FBSurface> FrameBuffer::allocateSurface(int w, int h, ScalingInterpolation interpolation, const uInt32* data)
{
  // Add new surface to the list
  mySurfaceList.push_back(createSurface(w, h, interpolation, data));

  // And return a pointer to it (pointer should be treated read-only)
  return mySurfaceList.at(mySurfaceList.size() - 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::freeSurfaces()
{
  for(auto& s: mySurfaceList)
    s->free();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::reloadSurfaces()
{
  for(auto& s: mySurfaceList)
    s->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::resetSurfaces()
{
  // Free all resources for each surface, then reload them
  // Due to possible timing and/or synchronization issues, all free()'s
  // are done first, then all reload()'s
  // Any derived FrameBuffer classes that call this method should be
  // aware of these restrictions, and act accordingly

  freeSurfaces();
  reloadSurfaces();

  update(true); // force full update
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setTIAPalette(const PaletteArray& rgb_palette)
{
  // Create a TIA palette from the raw RGB data
  PaletteArray tia_palette;
  for(int i = 0; i < 256; ++i)
  {
    uInt8 r = (rgb_palette[i] >> 16) & 0xff;
    uInt8 g = (rgb_palette[i] >> 8) & 0xff;
    uInt8 b = rgb_palette[i] & 0xff;

    tia_palette[i] = mapRGB(r, g, b);
  }

  // Remember the TIA palette; place it at the beginning of the full palette
  std::copy_n(tia_palette.begin(), tia_palette.size(), myFullPalette.begin());

  // Let the TIA surface know about the new palette
  myTIASurface->setPalette(tia_palette, rgb_palette);

  // Since the UI palette shares the TIA palette, we need to update it too
  setUIPalette();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setUIPalette()
{
  // Set palette for UI (upper area of full palette)
  const UIPaletteArray& ui_palette =
     (myOSystem.settings().getString("uipalette") == "classic") ? ourClassicUIPalette :
     (myOSystem.settings().getString("uipalette") == "light")   ? ourLightUIPalette :
      ourStandardUIPalette;

  for(size_t i = 0, j = myFullPalette.size() - ui_palette.size();
      i < ui_palette.size(); ++i, ++j)
  {
    const uInt8 r = (ui_palette[i] >> 16) & 0xff,
                g = (ui_palette[i] >> 8) & 0xff,
                b =  ui_palette[i] & 0xff;

    myFullPalette[j] = mapRGB(r, g, b);
  }
  FBSurface::setPalette(myFullPalette);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::stateChanged(EventHandlerState state)
{
  // Make sure any onscreen messages are removed
  myMsg.enabled = false;
  myMsg.counter = 0;

  update(true); // force full update
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setFullscreen(bool enable)
{
#ifdef WINDOWED_SUPPORT
  // Switching between fullscreen and windowed modes will invariably mean
  // that the 'window' resolution changes.  Currently, dialogs are not
  // able to resize themselves when they are actively being shown
  // (they would have to be closed and then re-opened, etc).
  // For now, we simply disallow screen switches in such modes
  switch(myOSystem.eventHandler().state())
  {
    case EventHandlerState::EMULATION:
    case EventHandlerState::PAUSE:
      break; // continue with processing (aka, allow a mode switch)
    case EventHandlerState::DEBUGGER:
    case EventHandlerState::LAUNCHER:
      if(myOSystem.eventHandler().overlay().baseDialogIsActive())
        break; // allow a mode switch when there is only one dialog
      [[fallthrough]];
    default:
      return;
  }

  // Changing the video mode can take some time, during which the last
  // sound played may get 'stuck'
  // So we mute the sound until the operation completes
  bool oldMuteState = myOSystem.sound().mute(true);

  const VideoMode& mode = getSavedVidMode(enable);
  if(setVideoMode(myScreenTitle, mode))
  {
    myImageRect = mode.image;
    myScreenSize = mode.screen;
    myScreenRect = Common::Rect(mode.screen);

    // Inform TIA surface about new mode
    if(myOSystem.eventHandler().state() != EventHandlerState::LAUNCHER &&
       myOSystem.eventHandler().state() != EventHandlerState::DEBUGGER)
      myTIASurface->initialize(myOSystem.console(), mode);

    // Did we get the requested fullscreen state?
    myOSystem.settings().setValue("fullscreen", fullScreen());
    resetSurfaces();
    setCursorState();
  }
  myOSystem.sound().mute(oldMuteState);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleFullscreen()
{
  setFullscreen(!fullScreen());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::changeOverscan(int direction)
{
  if (fullScreen())
  {
    int oldOverscan = myOSystem.settings().getInt("tia.fs_overscan");
    int overscan = BSPF::clamp(oldOverscan + direction, 0, 10);

    if (overscan != oldOverscan)
    {
      myOSystem.settings().setValue("tia.fs_overscan", overscan);

      // issue a complete framebuffer re-initialization
      myOSystem.createFrameBuffer();
    }
    ostringstream msg;
    msg << "Overscan at " << overscan << "%";
    showMessage(msg.str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::changeVidMode(int direction)
{
  EventHandlerState state = myOSystem.eventHandler().state();
  bool tiaMode = (state != EventHandlerState::DEBUGGER &&
                  state != EventHandlerState::LAUNCHER);

  // Only applicable when in TIA/emulation mode
  if(!tiaMode)
    return false;

  if(direction == +1)
    myCurrentModeList->next();
  else if(direction == -1)
    myCurrentModeList->previous();
  else
    return false;

  // Changing the video mode can take some time, during which the last
  // sound played may get 'stuck'
  // So we mute the sound until the operation completes
  bool oldMuteState = myOSystem.sound().mute(true);

  const VideoMode& mode = myCurrentModeList->current();
  if(setVideoMode(myScreenTitle, mode))
  {
    myImageRect = mode.image;
    myScreenSize = mode.screen;
    myScreenRect = Common::Rect(mode.screen);

    // Inform TIA surface about new mode
    myTIASurface->initialize(myOSystem.console(), mode);

    resetSurfaces();
    showMessage(mode.description);
    myOSystem.sound().mute(oldMuteState);

    if(fullScreen())
      myOSystem.settings().setValue("tia.fs_stretch",
          mode.stretch == VideoMode::Stretch::Fill);
    else
      myOSystem.settings().setValue("tia.zoom", mode.zoom);

    return true;
  }
  myOSystem.sound().mute(oldMuteState);

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setCursorState()
{
  // Always grab mouse in emulation (if enabled) and emulating a controller
  // that always uses the mouse
  bool emulation =
      myOSystem.eventHandler().state() == EventHandlerState::EMULATION;
  bool analog = myOSystem.hasConsole() ?
      (myOSystem.console().leftController().isAnalog() ||
       myOSystem.console().rightController().isAnalog()) : false;
  bool usesLightgun = emulation && myOSystem.hasConsole() ?
    myOSystem.console().leftController().type() == Controller::Type::Lightgun ||
    myOSystem.console().rightController().type() == Controller::Type::Lightgun : false;
  bool alwaysUseMouse = BSPF::equalsIgnoreCase("always", myOSystem.settings().getString("usemouse"));

  // Show/hide cursor in UI/emulation mode based on 'cursor' setting
  int cursor = myOSystem.settings().getInt("cursor");
  // always enable cursor in lightgun games
  if (usesLightgun)
    cursor |= 1;

  switch(cursor)
  {
    case 0:
      showCursor(false);
      break;
    case 1:
      showCursor(emulation);
      myGrabMouse = false; // disable grab while cursor is shown in emulation
      break;
    case 2:
      showCursor(!emulation);
      break;
    case 3:
      showCursor(true);
      myGrabMouse = false; // disable grab while cursor is shown in emulation
      break;
  }

  grabMouse(emulation && (analog || alwaysUseMouse) && myGrabMouse);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::enableGrabMouse(bool enable)
{
  myGrabMouse = enable;
  setCursorState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleGrabMouse()
{
  myGrabMouse = !myGrabMouse;
  setCursorState();
  myOSystem.settings().setValue("grabmouse", myGrabMouse);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float FrameBuffer::maxZoomForScreen(uInt32 baseWidth, uInt32 baseHeight,
                                    uInt32 screenWidth, uInt32 screenHeight) const
{
  float multiplier = 1;
  for(;;)
  {
    // Figure out the zoomed size of the window
    uInt32 width  = baseWidth * multiplier;
    uInt32 height = baseHeight * multiplier;

    if((width > screenWidth) || (height > screenHeight))
      break;

    multiplier += ZOOM_STEPS;
  }
  return multiplier > 1 ? multiplier - ZOOM_STEPS : 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setAvailableVidModes(uInt32 baseWidth, uInt32 baseHeight)
{
  myWindowedModeList.clear();

  for(auto& mode: myFullscreenModeLists)
    mode.clear();
  for(size_t i = myFullscreenModeLists.size(); i < myFullscreenDisplays.size(); ++i)
    myFullscreenModeLists.emplace_back(VideoModeList());

  // Check if zooming is allowed for this state (currently only allowed
  // for TIA screens)
  EventHandlerState state = myOSystem.eventHandler().state();
  bool tiaMode = (state != EventHandlerState::DEBUGGER &&
                  state != EventHandlerState::LAUNCHER);
  float overscan = 1 - myOSystem.settings().getInt("tia.fs_overscan") / 100.0;

  // TIA mode allows zooming at integral factors in windowed modes,
  // and also non-integral factors in fullscreen mode
  if(tiaMode)
  {
    // TIA windowed modes
    uInt32 minZoom = supportedTIAMinZoom();
    myTIAMaxZoom = maxZoomForScreen(baseWidth, baseHeight,
                     myAbsDesktopSize.w, myAbsDesktopSize.h);
    // Determine all zoom levels
    for(float zoom = minZoom; zoom <= myTIAMaxZoom; zoom += ZOOM_STEPS)
    {
      ostringstream desc;
      desc << "Zoom " << zoom << "x";

      VideoMode mode(baseWidth*zoom, baseHeight*zoom, baseWidth*zoom, baseHeight*zoom,
                     VideoMode::Stretch::Fill, 1.0, desc.str(), zoom);
      myWindowedModeList.add(mode);
    }

    // TIA fullscreen mode
    for(uInt32 i = 0; i < myFullscreenDisplays.size(); ++i)
    {
      myTIAMaxZoom = maxZoomForScreen(baseWidth, baseHeight,
          myFullscreenDisplays[i].w * overscan,
          myFullscreenDisplays[i].h * overscan);

      // Add both normal aspect and filled modes
      // It's easier to define them both now, and simply switch between
      // them when necessary
      VideoMode mode1(baseWidth * myTIAMaxZoom, baseHeight * myTIAMaxZoom,
                      myFullscreenDisplays[i].w, myFullscreenDisplays[i].h,
                      VideoMode::Stretch::Preserve, overscan,
                      "Preserve aspect, no stretch", myTIAMaxZoom, i);
      myFullscreenModeLists[i].add(mode1);
      VideoMode mode2(baseWidth * myTIAMaxZoom, baseHeight * myTIAMaxZoom,
                      myFullscreenDisplays[i].w, myFullscreenDisplays[i].h,
                      VideoMode::Stretch::Fill, overscan,
                      "Ignore aspect, full stretch", myTIAMaxZoom, i);
      myFullscreenModeLists[i].add(mode2);
    }
  }
  else  // UI mode
  {
    // Windowed and fullscreen mode differ only in screen size
    myWindowedModeList.add(
        VideoMode(baseWidth, baseHeight, baseWidth, baseHeight,
        VideoMode::Stretch::None)
    );
    for(uInt32 i = 0; i < myFullscreenDisplays.size(); ++i)
    {
      myFullscreenModeLists[i].add(
          VideoMode(baseWidth, baseHeight,
                    myFullscreenDisplays[i].w, myFullscreenDisplays[i].h,
                    VideoMode::Stretch::None, 1.0, "", 1, i)
      );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FrameBuffer::VideoMode& FrameBuffer::getSavedVidMode(bool fullscreen)
{
  if(fullscreen)
  {
    Int32 i = getCurrentDisplayIndex();
    if(i < 0)
    {
      // default to the first display
      i = 0;
    }
    myCurrentModeList = &myFullscreenModeLists[i];
  }
  else
    myCurrentModeList = &myWindowedModeList;

  // Now select the best resolution depending on the state
  // UI modes (launcher and debugger) have only one supported resolution
  // so the 'current' one is the only valid one
  EventHandlerState state = myOSystem.eventHandler().state();
  if(state == EventHandlerState::DEBUGGER || state == EventHandlerState::LAUNCHER)
    myCurrentModeList->setByZoom(1);
  else  // TIA mode
  {
    if(fullscreen)
      myCurrentModeList->setByStretch(myOSystem.settings().getBool("tia.fs_stretch")
        ? VideoMode::Stretch::Fill : VideoMode::Stretch::Preserve);
    else
      myCurrentModeList->setByZoom(myOSystem.settings().getFloat("tia.zoom"));
  }

  return myCurrentModeList->current();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// VideoMode implementation
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::VideoMode::VideoMode(uInt32 iw, uInt32 ih, uInt32 sw, uInt32 sh,
                                  Stretch smode, float overscan, const string& desc,
                                  float zoomLevel, Int32 fsindex)
  : stretch(smode),
    description(desc),
    zoom(zoomLevel),
    fsIndex(fsindex)
{
  // First set default size and positioning
  sw = std::max(sw, TIAConstants::viewableWidth);
  sh = std::max(sh, TIAConstants::viewableHeight);
  iw = std::min(iw, sw);
  ih = std::min(ih, sh);
  int ix = (sw - iw) >> 1;
  int iy = (sh - ih) >> 1;
  image = Common::Rect(ix, iy, ix+iw, iy+ih);
  screen = Common::Size(sw, sh);

  // Now resize based on windowed/fullscreen mode and stretch factor
  iw = image.w();
  ih = image.h();

  if(fsIndex != -1)
  {
    switch(stretch)
    {
      case Stretch::Preserve:
      {
        float stretchFactor = 1.0;
        float scaleX = float(iw) / screen.w;
        float scaleY = float(ih) / screen.h;

        // Scale to all available space, keep aspect correct
        if(scaleX > scaleY)
          stretchFactor = float(screen.w) / iw;
        else
          stretchFactor = float(screen.h) / ih;

        iw = uInt32(stretchFactor * iw) * overscan;
        ih = uInt32(stretchFactor * ih) * overscan;
        break;
      }

      case Stretch::Fill:
        // Scale to all available space
        iw = screen.w * overscan;
        ih = screen.h * overscan;
        break;

      case Stretch::None:
        // Don't do any scaling at all, but obey overscan
        iw = std::min(iw, screen.w) * overscan;
        ih = std::min(ih, screen.h) * overscan;
        break;
    }
  }
  else
  {
    // In windowed mode, currently the size is scaled to the screen
    // TODO - this may be updated if/when we allow variable-sized windows
    switch(stretch)
    {
      case Stretch::Preserve:
      case Stretch::Fill:
        screen.w = iw;
        screen.h = ih;
        break;
      case Stretch::None:
        break;  // Do not change image or screen rects whatsoever
    }
  }

  // Now re-calculate the dimensions
  iw = std::min(iw, screen.w);
  ih = std::min(ih, screen.h);

  image.moveTo((screen.w - iw) >> 1, (screen.h - ih) >> 1);
  image.setWidth(iw);
  image.setHeight(ih);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// VideoModeList implementation
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::add(const VideoMode& mode)
{
  myModeList.emplace_back(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::clear()
{
  myModeList.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::VideoModeList::empty() const
{
  return myModeList.empty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBuffer::VideoModeList::size() const
{
  return uInt32(myModeList.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::previous()
{
  --myIdx;
  if(myIdx < 0) myIdx = int(myModeList.size()) - 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FrameBuffer::VideoMode& FrameBuffer::VideoModeList::current() const
{
  return myModeList[myIdx];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::next()
{
  myIdx = (myIdx + 1) % myModeList.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::setByZoom(float zoom)
{
  for(uInt32 i = 0; i < myModeList.size(); ++i)
  {
    if(myModeList[i].zoom == zoom)
    {
      myIdx = i;
      return;
    }
  }
  myIdx = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::setByStretch(FrameBuffer::VideoMode::Stretch stretch)
{
  for(uInt32 i = 0; i < myModeList.size(); ++i)
  {
    if(myModeList[i].stretch == stretch)
    {
      myIdx = i;
      return;
    }
  }
  myIdx = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
  Palette is defined as follows:
    *** Base colors ***
    kColor            Normal foreground color (non-text)
    kBGColor          Normal background color (non-text)
    kBGColorLo        Disabled background color dark (non-text)
    kBGColorHi        Disabled background color light (non-text)
    kShadowColor      Item is disabled
    *** Text colors ***
    kTextColor        Normal text color
    kTextColorHi      Highlighted text color
    kTextColorEm      Emphasized text color
    kTextColorInv     Color for selected text
    *** UI elements (dialog and widgets) ***
    kDlgColor         Dialog background
    kWidColor         Widget background
    kWidColorHi       Widget highlight color
    kWidFrameColor    Border for currently selected widget
    *** Button colors ***
    kBtnColor         Normal button background
    kBtnColorHi       Highlighted button background
    kBtnBorderColor,
    kBtnBorderColorHi,
    kBtnTextColor     Normal button font color
    kBtnTextColorHi   Highlighted button font color
    *** Checkbox colors ***
    kCheckColor       Color of 'X' in checkbox
    *** Scrollbar colors ***
    kScrollColor      Normal scrollbar color
    kScrollColorHi    Highlighted scrollbar color
    *** Debugger colors ***
    kDbgChangedColor      Background color for changed cells
    kDbgChangedTextColor  Text color for changed cells
    kDbgColorHi           Highlighted color in debugger data cells
    kDbgColorRed          Red color in debugger
    *** Slider colors ***
    kSliderColor          Enabled slider
    kSliderColorHi        Focussed slider
    kSliderBGColor        Enabled slider background
    kSliderBGColorHi      Focussed slider background
    kSliderBGColorLo      Disabled slider background
    *** Other colors ***
    kColorInfo            TIA output position color
    kColorTitleBar        Title bar color
    kColorTitleText       Title text color
    kColorTitleBarLo      Disabled title bar color
    kColorTitleTextLo     Disabled title text color
*/
UIPaletteArray FrameBuffer::ourStandardUIPalette = {
  { 0x686868, 0x000000, 0xa38c61, 0xdccfa5, 0x404040,           // base
    0x000000, 0xac3410, 0x9f0000, 0xf0f0cf,                     // text
    0xc9af7c, 0xf0f0cf, 0xd55941, 0xc80000,                     // UI elements
    0xac3410, 0xd55941, 0x686868, 0xdccfa5, 0xf0f0cf, 0xf0f0cf, // buttons
    0xac3410,                                                   // checkbox
    0xac3410, 0xd55941,                                         // scrollbar
    0xc80000, 0xffff80, 0xc8c8ff, 0xc80000,                     // debugger
    0xac3410, 0xd55941, 0xdccfa5, 0xf0f0cf, 0xa38c61,           // slider
    0xffffff, 0xac3410, 0xf0f0cf, 0x686868, 0xdccfa5            // other
  }
};

UIPaletteArray FrameBuffer::ourClassicUIPalette = {
  { 0x686868, 0x000000, 0x404040, 0x404040, 0x404040,           // base
    0x20a020, 0x00ff00, 0xc80000, 0x000000,                     // text
    0x000000, 0x000000, 0x00ff00, 0xc80000,                     // UI elements
    0x000000, 0x000000, 0x686868, 0x00ff00, 0x20a020, 0x00ff00, // buttons
    0x20a020,                                                   // checkbox
    0x20a020, 0x00ff00,                                         // scrollbar
    0xc80000, 0x00ff00, 0xc8c8ff, 0xc80000,                     // debugger
    0x20a020, 0x00ff00, 0x404040, 0x686868, 0x404040,           // slider
    0x00ff00, 0x20a020, 0x000000, 0x686868, 0x404040            // other
  }
};

UIPaletteArray FrameBuffer::ourLightUIPalette = {
  { 0x808080, 0x000000, 0xc0c0c0, 0xe1e1e1, 0x333333,           // base
    0x000000, 0xBDDEF9, 0x0078d7, 0x000000,                     // text
    0xf0f0f0, 0xffffff, 0x0078d7, 0x0f0f0f,                     // UI elements
    0xe1e1e1, 0xe5f1fb, 0x808080, 0x0078d7, 0x000000, 0x000000, // buttons
    0x333333,                                                   // checkbox
    0xc0c0c0, 0x808080,                                         // scrollbar
    0xffc0c0, 0x000000, 0xe00000, 0xc00000,                     // debugger
    0x333333, 0x0078d7, 0xc0c0c0, 0xffffff, 0xc0c0c0,           // slider 0xBDDEF9| 0xe1e1e1 | 0xffffff
    0xffffff, 0x333333, 0xf0f0f0, 0x808080, 0xc0c0c0            // other
  }
};
