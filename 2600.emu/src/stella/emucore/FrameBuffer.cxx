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
// $Id: FrameBuffer.cxx 3304 2016-04-03 00:35:00Z stephena $
//============================================================================

#include <algorithm>
#include <sstream>

#include "bspf.hxx"

#include "CommandMenu.hxx"
#include "Console.hxx"
#include "EventHandler.hxx"
#include "Event.hxx"
#include "Font.hxx"
#include "StellaFont.hxx"
#include "StellaMediumFont.hxx"
#include "StellaLargeFont.hxx"
#include "ConsoleFont.hxx"
#include "Launcher.hxx"
#include "Menu.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "TIA.hxx"

#include "FBSurface.hxx"
#include "TIASurface.hxx"
#include "FrameBuffer.hxx"

#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::FrameBuffer(OSystem& osystem)
  : myOSystem(osystem),
    myInitializedCount(0),
    myPausedCount(0),
    myCurrentModeList(nullptr)
{
  myMsg.surface = myStatsMsg.surface = nullptr;
  myMsg.enabled = myStatsMsg.enabled = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::initialize()
{
  // Get desktop resolution and supported renderers
  queryHardware(myDisplays, myRenderers);
  uInt32 query_w = myDisplays[0].w, query_h = myDisplays[0].h;

  // Check the 'maxres' setting, which is an undocumented developer feature
  // that specifies the desktop size (not normally set)
  const GUI::Size& s = myOSystem.settings().getSize("maxres");
  if(s.valid())
  {
    query_w = s.w;
    query_h = s.h;
  }
  // Various parts of the codebase assume a minimum screen size
  myDesktopSize.w = std::max(query_w, uInt32(kFBMinW));
  myDesktopSize.h = std::max(query_h, uInt32(kFBMinH));

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
  bool smallScreen = myDesktopSize.w < kFBMinW || myDesktopSize.h < kFBMinH;

  // This font is used in a variety of situations when a really small
  // font is needed; we let the specific widget/dialog decide when to
  // use it
  mySmallFont = make_ptr<GUI::Font>(GUI::stellaDesc);

  // The general font used in all UI elements
  // This is determined by the size of the framebuffer
  myFont = make_ptr<GUI::Font>(smallScreen ? GUI::stellaDesc : GUI::stellaMediumDesc);

  // The info font used in all UI elements
  // This is determined by the size of the framebuffer
  myInfoFont = make_ptr<GUI::Font>(smallScreen ? GUI::stellaDesc : GUI::consoleDesc);

  // The font used by the ROM launcher
  // Normally, this is configurable by the user, except in the case of
  // very small screens
  if(!smallScreen)
  {    
    const string& lf = myOSystem.settings().getString("launcherfont");
    if(lf == "small")
      myLauncherFont = make_ptr<GUI::Font>(GUI::consoleDesc);
    else if(lf == "medium")
      myLauncherFont = make_ptr<GUI::Font>(GUI::stellaMediumDesc);
    else
      myLauncherFont = make_ptr<GUI::Font>(GUI::stellaLargeDesc);
  }
  else
    myLauncherFont = make_ptr<GUI::Font>(GUI::stellaDesc);

  // Determine possible TIA windowed zoom levels
  uInt32 maxZoom = maxWindowSizeForScreen(uInt32(kTIAMinW), uInt32(kTIAMinH),
                     myDesktopSize.w, myDesktopSize.h);

  // Figure our the smallest zoom level we can use
  uInt32 firstZoom = smallScreen ? 1 : 2;
  for(uInt32 zoom = firstZoom; zoom <= maxZoom; ++zoom)
  {
    ostringstream desc;
    desc << "Zoom " << zoom << "x";
    VarList::push_back(myTIAZoomLevels, desc.str(), zoom);
  }

  // Set palette for GUI (upper area of array, doesn't change during execution)
  int palID = myOSystem.settings().getString("uipalette") == "classic" ? 1 : 0;

  for(int i = 0, j = 256; i < kNumColors-256; ++i, ++j)
  {
    Uint8 r = (ourGUIColors[palID][i] >> 16) & 0xff;
    Uint8 g = (ourGUIColors[palID][i] >> 8) & 0xff;
    Uint8 b = ourGUIColors[palID][i] & 0xff;

    myPalette[j] = mapRGB(r, g, b);
  }
  FBSurface::setPalette(myPalette);

  // Create a TIA surface; we need it for rendering TIA images
  myTIASurface = make_ptr<TIASurface>(myOSystem);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBInitStatus FrameBuffer::createDisplay(const string& title,
                                        uInt32 width, uInt32 height)
{
  myInitializedCount++;
  myScreenTitle = title;

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
  if(myDesktopSize.w < kFBMinW && myDesktopSize.h < kFBMinH &&
     (myDesktopSize.w < width || myDesktopSize.h < height))
    return kFailTooLarge;

  useFullscreen = myOSystem.settings().getBool("fullscreen");
#else
  // Make sure this mode is even possible
  // We only really need to worry about it in non-windowed environments,
  // where requesting a window that's too large will probably cause a crash
  if(myDesktopSize.w < width || myDesktopSize.h < height)
    return kFailTooLarge;
#endif

  // Set the available video modes for this framebuffer
  setAvailableVidModes(width, height);

  // Initialize video subsystem (make sure we get a valid mode)
  string pre_about = about();
  const VideoMode& mode = getSavedVidMode(useFullscreen);
  if(width <= mode.screen.w && height <= mode.screen.h)
  {
    if(setVideoMode(myScreenTitle, mode))
    {
      myImageRect = mode.image;
      myScreenSize = mode.screen;

      // Inform TIA surface about new mode
      if(myOSystem.eventHandler().state() != EventHandler::S_LAUNCHER &&
         myOSystem.eventHandler().state() != EventHandler::S_DEBUGGER)
        myTIASurface->initialize(myOSystem.console(), mode);

      // Did we get the requested fullscreen state?
      myOSystem.settings().setValue("fullscreen", fullScreen());
      resetSurfaces();
      setCursorState();
    }
    else
    {
      myOSystem.logMessage("ERROR: Couldn't initialize video subsystem", 0);
      return kFailNotSupported;
    }
  }
  else
    return kFailTooLarge;

  // Erase any messages from a previous run
  myMsg.counter = 0;

  // Create surfaces for TIA statistics and general messages
  myStatsMsg.color = kBtnTextColor;
  myStatsMsg.w = infoFont().getMaxCharWidth() * 24 + 2;
  myStatsMsg.h = (infoFont().getFontHeight() + 2) * 2;

  if(!myStatsMsg.surface)
    myStatsMsg.surface = allocateSurface(myStatsMsg.w, myStatsMsg.h);

  if(!myMsg.surface)
    myMsg.surface = allocateSurface(kFBMinW, font().getFontHeight()+10);

  // Print initial usage message, but only print it later if the status has changed
  if(myInitializedCount == 1)
  {
    myOSystem.logMessage(about(), 1);
  }
  else
  {
    string post_about = about();
    if(post_about != pre_about)
      myOSystem.logMessage(post_about, 1);
  }

  return kSuccess;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::update()
{
  // Determine which mode we are in (from the EventHandler)
  // Take care of S_EMULATE mode here, otherwise let the GUI
  // figure out what to draw

  invalidate();
  switch(myOSystem.eventHandler().state())
  {
    case EventHandler::S_EMULATE:
    {
      // Run the console for one frame
      // Note that the debugger can cause a breakpoint to occur, which changes
      // the EventHandler state 'behind our back' - we need to check for that
      myOSystem.console().tia().update();
  #ifdef DEBUGGER_SUPPORT
      if(myOSystem.eventHandler().state() != EventHandler::S_EMULATE) break;
  #endif
      if(myOSystem.eventHandler().frying())
        myOSystem.console().fry();

      // And update the screen
      myTIASurface->render();

      // Show frame statistics
      if(myStatsMsg.enabled)
      {
        const ConsoleInfo& info = myOSystem.console().about();
        char msg[30];
        std::snprintf(msg, 30, "%3u @ %3.2ffps => %s",
                myOSystem.console().tia().scanlines(),
                myOSystem.console().getFramerate(), info.DisplayFormat.c_str());
        myStatsMsg.surface->fillRect(0, 0, myStatsMsg.w, myStatsMsg.h, kBGColor);
        myStatsMsg.surface->drawString(infoFont(),
          msg, 1, 1, myStatsMsg.w, myStatsMsg.color, kTextAlignLeft);
        myStatsMsg.surface->drawString(infoFont(),
          info.BankSwitch, 1, 15, myStatsMsg.w, myStatsMsg.color, kTextAlignLeft);
        myStatsMsg.surface->setDirty();
        myStatsMsg.surface->setDstPos(myImageRect.x() + 1, myImageRect.y() + 1);
        myStatsMsg.surface->render();
      }
      break;  // S_EMULATE
    }

    case EventHandler::S_PAUSE:
    {
      myTIASurface->render();

      // Show a pause message every 5 seconds
      if(myPausedCount++ >= 7*myOSystem.frameRate())
      {
        myPausedCount = 0;
        showMessage("Paused", kMiddleCenter);
      }
      break;  // S_PAUSE
    }

    case EventHandler::S_MENU:
    {
      myTIASurface->render();
      myOSystem.menu().draw(true);
      break;  // S_MENU
    }

    case EventHandler::S_CMDMENU:
    {
      myTIASurface->render();
      myOSystem.commandMenu().draw(true);
      break;  // S_CMDMENU
    }

    case EventHandler::S_LAUNCHER:
    {
      myOSystem.launcher().draw(true);
      break;  // S_LAUNCHER
    }

#ifdef DEBUGGER_SUPPORT
    case EventHandler::S_DEBUGGER:
    {
      myOSystem.debugger().draw(true);
      break;  // S_DEBUGGER
    }
#endif

    default:
      return;
  }

  // Draw any pending messages
  if(myMsg.enabled)
    drawMessage();

  // Do any post-frame stuff
  postFrameUpdate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showMessage(const string& message, MessagePosition position,
                              bool force)
{
  // Only show messages if they've been enabled
  if(!(force || myOSystem.settings().getBool("uimessages")))
    return;

  // Precompute the message coordinates
  myMsg.text    = message;
  myMsg.counter = uInt32(myOSystem.frameRate()) << 1; // Show message for 2 seconds
  myMsg.color   = kBtnTextColor;

  myMsg.w = font().getStringWidth(myMsg.text) + 10;
  myMsg.h = font().getFontHeight() + 8;
  myMsg.surface->setSrcSize(myMsg.w, myMsg.h);
  myMsg.surface->setDstSize(myMsg.w, myMsg.h);
  myMsg.position = position;
  myMsg.enabled = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleFrameStats()
{
  showFrameStats(!myOSystem.settings().getBool("stats"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showFrameStats(bool enable)
{
  myOSystem.settings().setValue("stats", enable);
  myStatsMsg.enabled = enable;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::enableMessages(bool enable)
{
  if(enable)
  {
    // Only re-enable frame stats if they were already enabled before
    myStatsMsg.enabled = myOSystem.settings().getBool("stats");
  }
  else
  {
    // Temporarily disable frame stats
    myStatsMsg.enabled = false;

    // Erase old messages on the screen
    myMsg.enabled = false;
    myMsg.counter = 0;
    update();  // Force update immediately
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void FrameBuffer::drawMessage()
{
  // Draw the bounded box and text
  switch(myMsg.position)
  {
    case kTopLeft:
      myMsg.x = 5;
      myMsg.y = 5;
      break;

    case kTopCenter:
      myMsg.x = (myImageRect.width() - myMsg.w) >> 1;
      myMsg.y = 5;
      break;

    case kTopRight:
      myMsg.x = myImageRect.width() - myMsg.w - 5;
      myMsg.y = 5;
      break;

    case kMiddleLeft:
      myMsg.x = 5;
      myMsg.y = (myImageRect.height() - myMsg.h) >> 1;
      break;

    case kMiddleCenter:
      myMsg.x = (myImageRect.width() - myMsg.w) >> 1;
      myMsg.y = (myImageRect.height() - myMsg.h) >> 1;
      break;

    case kMiddleRight:
      myMsg.x = myImageRect.width() - myMsg.w - 5;
      myMsg.y = (myImageRect.height() - myMsg.h) >> 1;
      break;

    case kBottomLeft:
      myMsg.x = 5;
      myMsg.y = myImageRect.height() - myMsg.h - 5;
      break;

    case kBottomCenter:
      myMsg.x = (myImageRect.width() - myMsg.w) >> 1;
      myMsg.y = myImageRect.height() - myMsg.h - 5;
      break;

    case kBottomRight:
      myMsg.x = myImageRect.width() - myMsg.w - 5;
      myMsg.y = myImageRect.height() - myMsg.h - 5;
      break;
  }

  myMsg.surface->setDstPos(myMsg.x + myImageRect.x(), myMsg.y + myImageRect.y());
  myMsg.surface->fillRect(1, 1, myMsg.w-2, myMsg.h-2, kBtnColor);
  myMsg.surface->box(0, 0, myMsg.w, myMsg.h, kColor, kShadowColor);
  myMsg.surface->drawString(font(), myMsg.text, 4, 4,
                            myMsg.w, myMsg.color, kTextAlignLeft);

  // Either erase the entire message (when time is reached),
  // or show again this frame
  if(myMsg.counter-- > 0)
  {
    myMsg.surface->setDirty();
    myMsg.surface->render();
  }
  else
    myMsg.enabled = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
shared_ptr<FBSurface> FrameBuffer::allocateSurface(int w, int h, const uInt32* data)
{
  // Add new surface to the list
  mySurfaceList.push_back(createSurface(w, h, data));

  // And return a pointer to it (pointer should be treated read-only)
  return mySurfaceList.at(mySurfaceList.size() - 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::resetSurfaces()
{
  // Free all resources for each surface, then reload them
  // Due to possible timing and/or synchronization issues, all free()'s
  // are done first, then all reload()'s
  // Any derived FrameBuffer classes that call this method should be
  // aware of these restrictions, and act accordingly

  for(auto& s: mySurfaceList)
    s->free();
  for(auto& s: mySurfaceList)
    s->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setPalette(const uInt32* raw_palette)
{
  // Set palette for normal fill
  for(int i = 0; i < 256; ++i)
  {
    Uint8 r = (raw_palette[i] >> 16) & 0xff;
    Uint8 g = (raw_palette[i] >> 8) & 0xff;
    Uint8 b = raw_palette[i] & 0xff;

    myPalette[i] = mapRGB(r, g, b);
  }

  // Let the TIA surface know about the new palette
  myTIASurface->setPalette(myPalette, raw_palette);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::stateChanged(EventHandler::State state)
{
  // Make sure any onscreen messages are removed
  myMsg.enabled = false;
  myMsg.counter = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setFullscreen(bool enable)
{
  const VideoMode& mode = getSavedVidMode(enable);
  if(setVideoMode(myScreenTitle, mode))
  {
    myImageRect = mode.image;
    myScreenSize = mode.screen;

    // Inform TIA surface about new mode
    if(myOSystem.eventHandler().state() != EventHandler::S_LAUNCHER &&
       myOSystem.eventHandler().state() != EventHandler::S_DEBUGGER)
      myTIASurface->initialize(myOSystem.console(), mode);

    // Did we get the requested fullscreen state?
    myOSystem.settings().setValue("fullscreen", fullScreen());
    resetSurfaces();
    setCursorState();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleFullscreen()
{
  setFullscreen(!fullScreen());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::changeWindowedVidMode(int direction)
{
#ifdef WINDOWED_SUPPORT
  EventHandler::State state = myOSystem.eventHandler().state();
  bool tiaMode = (state != EventHandler::S_DEBUGGER &&
                  state != EventHandler::S_LAUNCHER);

  // Ignore any attempts to change video size while in invalid modes
  if(!tiaMode || fullScreen())
    return false;

  if(direction == +1)
    myCurrentModeList->next();
  else if(direction == -1)
    myCurrentModeList->previous();
  else
    return false;

  const VideoMode& mode = myCurrentModeList->current();
  if(setVideoMode(myScreenTitle, mode))
  {
    myImageRect = mode.image;
    myScreenSize = mode.screen;

    // Inform TIA surface about new mode
    myTIASurface->initialize(myOSystem.console(), mode);

    resetSurfaces();
    showMessage(mode.description);
    myOSystem.settings().setValue("tia.zoom", mode.zoom);
    return true;
  }
#endif
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setCursorState()
{
  // Always grab mouse in emulation (if enabled)
  bool emulation =
      myOSystem.eventHandler().state() == EventHandler::S_EMULATE;
  grabMouse(emulation && myOSystem.settings().getBool("grabmouse"));

  // Show/hide cursor in UI/emulation mode based on 'cursor' setting
  switch(myOSystem.settings().getInt("cursor"))
  {
    case 0: showCursor(false);      break;
    case 1: showCursor(emulation);  break;
    case 2: showCursor(!emulation); break;
    case 3: showCursor(true);       break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleGrabMouse()
{
  bool state = myOSystem.settings().getBool("grabmouse");
  myOSystem.settings().setValue("grabmouse", !state);
  setCursorState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBuffer::maxWindowSizeForScreen(uInt32 baseWidth, uInt32 baseHeight,
                    uInt32 screenWidth, uInt32 screenHeight) const
{
  uInt32 multiplier = 1;
  for(;;)
  {
    // Figure out the zoomed size of the window
    uInt32 width  = baseWidth * multiplier;
    uInt32 height = baseHeight * multiplier;

    if((width > screenWidth) || (height > screenHeight))
      break;

    ++multiplier;
  }
  return multiplier > 1 ? multiplier - 1 : 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setAvailableVidModes(uInt32 baseWidth, uInt32 baseHeight)
{
  myWindowedModeList.clear();

  for(auto& mode: myFullscreenModeLists)
    mode.clear();
  for(size_t i = myFullscreenModeLists.size(); i < myDisplays.size(); ++i)
    myFullscreenModeLists.push_back(VideoModeList());

  // Check if zooming is allowed for this state (currently only allowed
  // for TIA screens)
  EventHandler::State state = myOSystem.eventHandler().state();
  bool tiaMode = (state != EventHandler::S_DEBUGGER &&
                  state != EventHandler::S_LAUNCHER);

  // TIA mode allows zooming at integral factors in windowed modes,
  // and also non-integral factors in fullscreen mode
  if(tiaMode)
  {
    // TIA windowed modes
    uInt32 maxZoom = maxWindowSizeForScreen(baseWidth, baseHeight,
                     myDesktopSize.w, myDesktopSize.h);

    // Aspect ratio
    bool ntsc = myOSystem.console().about().InitialFrameRate == "60";
    uInt32 aspect = myOSystem.settings().getInt(ntsc ?
                      "tia.aspectn" : "tia.aspectp");

    // Figure our the smallest zoom level we can use
    uInt32 firstZoom = 2;
    if(myDesktopSize.w < 640 || myDesktopSize.h < 480)
      firstZoom = 1;
    for(uInt32 zoom = firstZoom; zoom <= maxZoom; ++zoom)
    {
      ostringstream desc;
      desc << "Zoom " << zoom << "x";
      
      VideoMode mode(baseWidth*zoom, baseHeight*zoom,
              baseWidth*zoom, baseHeight*zoom, -1, zoom, desc.str());
      mode.applyAspectCorrection(aspect);
      myWindowedModeList.add(mode);
    }

    // TIA fullscreen mode
    for(uInt32 i = 0; i < myDisplays.size(); ++i)
    {
      maxZoom = maxWindowSizeForScreen(baseWidth, baseHeight,
                                       myDisplays[i].w, myDisplays[i].h);
      VideoMode mode(baseWidth*maxZoom, baseHeight*maxZoom,
                     myDisplays[i].w, myDisplays[i].h, i);
      mode.applyAspectCorrection(aspect, myOSystem.settings().getBool("tia.fsfill"));
      myFullscreenModeLists[i].add(mode);
    }
  }
  else  // UI mode
  {
    // Windowed and fullscreen mode differ only in screen size
    myWindowedModeList.add(
        VideoMode(baseWidth, baseHeight, baseWidth, baseHeight, -1)
    );
    for(uInt32 i = 0; i < myDisplays.size(); ++i)
    {
      myFullscreenModeLists[i].add(
          VideoMode(baseWidth, baseHeight, myDisplays[i].w, myDisplays[i].h, i)
      );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const VideoMode& FrameBuffer::getSavedVidMode(bool fullscreen)
{
  EventHandler::State state = myOSystem.eventHandler().state();

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
  if(state == EventHandler::S_DEBUGGER || state == EventHandler::S_LAUNCHER)
    myCurrentModeList->setZoom(1);
  else
    myCurrentModeList->setZoom(myOSystem.settings().getInt("tia.zoom"));

  return myCurrentModeList->current();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// VideoMode implementation
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VideoMode::VideoMode()
  : fsIndex(-1),
    zoom(1),
    description("")
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VideoMode::VideoMode(uInt32 iw, uInt32 ih, uInt32 sw, uInt32 sh,
                     Int32 full, uInt32 z, const string& desc)
  : fsIndex(full),
    zoom(z),
    description(desc)
{
  sw = std::max(sw, uInt32(FrameBuffer::kTIAMinW));
  sh = std::max(sh, uInt32(FrameBuffer::kTIAMinH));
  iw = std::min(iw, sw);
  ih = std::min(ih, sh);
  int ix = (sw - iw) >> 1;
  int iy = (sh - ih) >> 1;
  image = GUI::Rect(ix, iy, ix+iw, iy+ih);
  screen = GUI::Size(sw, sh);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoMode::applyAspectCorrection(uInt32 aspect, bool stretch)
{
  // Width is modified by aspect ratio; other factors may be applied below
  uInt32 iw = uInt32(float(image.width() * aspect) / 100.0);
  uInt32 ih = image.height();

  if(fsIndex != -1)
  {
    // Fullscreen mode stretching
    float stretchFactor = 1.0;
    float scaleX = float(iw) / screen.w;
    float scaleY = float(ih) / screen.h;

    // Scale to actual or integral factors
    if(stretch)
    {
      // Scale to full (non-integral) available space
      if(scaleX > scaleY)
        stretchFactor = float(screen.w) / iw;
      else
        stretchFactor = float(screen.h) / ih;
    }
    else
    {
      // Only scale to an integral amount
      if(scaleX > scaleY)
      {
        int bw = iw / zoom;
        stretchFactor = float(int(screen.w / bw) * bw) / iw;
      }
      else
      {
        int bh = ih / zoom;
        stretchFactor = float(int(screen.h / bh) * bh) / ih;
      }
    }
    iw = uInt32(stretchFactor * iw);
    ih = uInt32(stretchFactor * ih);
  }
  else
  {
    // In windowed mode, the screen size changes to match the image width
    // Height is never modified in this mode
    screen.w = iw;
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
FrameBuffer::VideoModeList::VideoModeList()
  : myIdx(-1)
{
}

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
const VideoMode& FrameBuffer::VideoModeList::current() const
{
  return myModeList[myIdx];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::next()
{
  myIdx = (myIdx + 1) % myModeList.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::setZoom(uInt32 zoom)
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
/*
  Palette is defined as follows:
    // Base colors
    kColor            Normal foreground color (non-text)
    kBGColor          Normal background color (non-text)
    kShadowColor      Item is disabled
    kTextColor        Normal text color
    kTextColorHi      Highlighted text color
    kTextColorEm      Emphasized text color

    // UI elements (dialog and widgets)
    kDlgColor         Dialog background
    kWidColor         Widget background
    kWidFrameColor    Border for currently selected widget

    // Button colors
    kBtnColor         Normal button background
    kBtnColorHi       Highlighted button background
    kBtnTextColor     Normal button font color
    kBtnTextColorHi   Highlighted button font color

    // Checkbox colors
    kCheckColor       Color of 'X' in checkbox

    // Scrollbar colors
    kScrollColor      Normal scrollbar color
    kScrollColorHi    Highlighted scrollbar color

    // Debugger colors
    kDbgChangedColor      Background color for changed cells
    kDbgChangedTextColor  Text color for changed cells
    kDbgColorHi           Highlighted color in debugger data cells
*/
uInt32 FrameBuffer::ourGUIColors[2][kNumColors-256] = {
  // Standard
  { 0x686868, 0x000000, 0x404040, 0x000000, 0x62a108, 0x9f0000,
    0xc9af7c, 0xf0f0cf, 0xc80000,
    0xac3410, 0xd55941, 0xffffff, 0xffd652,
    0xac3410,
    0xac3410, 0xd55941,
    0xac3410, 0xd55941,
    0xc80000, 0x00ff00, 0xc8c8ff
  },

  // Classic
  { 0x686868, 0x000000, 0x404040, 0x20a020, 0x00ff00, 0xc80000,
    0x000000, 0x000000, 0xc80000,
    0x000000, 0x000000, 0x20a020, 0x00ff00,
    0x20a020,
    0x20a020, 0x00ff00,
    0x20a020, 0x00ff00,
    0xc80000, 0x00ff00, 0xc8c8ff
  }
};
