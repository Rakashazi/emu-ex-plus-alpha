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
// $Id: FrameBuffer.cxx 2270 2011-08-19 14:30:15Z stephena $
//============================================================================

#include <algorithm>
#include <sstream>

#include "bspf.hxx"

#include "CommandMenu.hxx"
#include "Console.hxx"
#include "EventHandler.hxx"
#include "Event.hxx"
#include "Font.hxx"
#include "Launcher.hxx"
#include "Menu.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "TIA.hxx"

#include "FrameBuffer.hxx"

#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::FrameBuffer(OSystem* osystem)
  : myOSystem(osystem),
    myScreen(0),
    mySDLFlags(0),
    myRedrawEntireFrame(true),
    myUsePhosphor(false),
    myPhosphorBlend(77),
    myInitializedCount(0),
    myPausedCount(0)
{
  myMsg.surface   = myStatsMsg.surface = NULL;
  myMsg.surfaceID = myStatsMsg.surfaceID = -1;
  myMsg.enabled   = myStatsMsg.enabled = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::~FrameBuffer(void)
{
  // Free all allocated surfaces
  while(!mySurfaceList.empty())
  {
    delete (*mySurfaceList.begin()).second;
    mySurfaceList.erase(mySurfaceList.begin());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBInitStatus FrameBuffer::initialize(const string& title,
                                     uInt32 width, uInt32 height)
{
  ostringstream buf;

  // Now (re)initialize the SDL video system
  // These things only have to be done one per FrameBuffer creation
  if(SDL_WasInit(SDL_INIT_VIDEO) == 0)
  {
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
    {
      buf << "ERROR: Couldn't initialize SDL: " << SDL_GetError() << endl;
      myOSystem->logMessage(buf.str(), 0);
      return kFailComplete;
    }
  }
  myInitializedCount++;

  // A 'windowed' system is defined as one where the window size can be
  // larger than the screen size, as there's some sort of window manager
  // that takes care of it (all current desktop systems fall in this category)
  // However, some systems have no concept of windowing, and have hard limits
  // on how large a window can be (ie, the size of the 'desktop' is the
  // absolute upper limit on window size)
  //
  // If the WINDOWED_SUPPORT macro is defined, we treat the system as the
  // former type; if not, as the latter type

  uInt32 flags = mySDLFlags;
#ifdef WINDOWED_SUPPORT
  // We assume that a desktop size of at least 640x480 means that we're
  // running on a 'large' system, and the window size requirements can
  // be relaxed
  // Otherwise, we treat the system as if WINDOWED_SUPPORT is not defined
  if(myOSystem->desktopWidth() < 640 && myOSystem->desktopHeight() < 480 &&
      (myOSystem->desktopWidth() < width || myOSystem->desktopHeight() < height))
    return kFailTooLarge;

  if(myOSystem->settings().getString("fullscreen") == "1")
  {
    if(myOSystem->desktopWidth() < width || myOSystem->desktopHeight() < height)
      return kFailTooLarge;

    flags |= SDL_FULLSCREEN;
  }
  else
    flags &= ~SDL_FULLSCREEN;
#else
  // Make sure this mode is even possible
  // We only really need to worry about it in non-windowed environments,
  // where requesting a window that's too large will probably cause a crash
  if(myOSystem->desktopWidth() < width || myOSystem->desktopHeight() < height)
    return kFailTooLarge;
#endif

  // Only update the actual flags if no errors were detected
  mySDLFlags = flags;

  // Set the available video modes for this framebuffer
  setAvailableVidModes(width, height);

  // Initialize video subsystem (make sure we get a valid mode)
  VideoMode mode = getSavedVidMode();
  if(width <= mode.screen_w && height <= mode.screen_h)
  {
    // Set window title and icon
    setWindowTitle(title);
    if(myInitializedCount == 1) setWindowIcon();

    if(initSubsystem(mode))
    {
      centerAppWindow(mode);

      myImageRect.setWidth(mode.image_w);
      myImageRect.setHeight(mode.image_h);
      myImageRect.moveTo(mode.image_x, mode.image_y);

      myScreenRect.setWidth(mode.screen_w);
      myScreenRect.setHeight(mode.screen_h);

      // Did we get the requested fullscreen state?
      const string& fullscreen = myOSystem->settings().getString("fullscreen");
      if(fullscreen != "-1")
        myOSystem->settings().setString("fullscreen", fullScreen() ? "1" : "0");
      setCursorState();
    }
    else
    {
      myOSystem->logMessage("ERROR: Couldn't initialize video subsystem\n", 0);
      return kFailNotSupported;
    }
  }
  else
    return kFailTooLarge;

  // Enable unicode so we can see translated key events
  // (lowercase vs. uppercase characters)
  SDL_EnableUNICODE(1);

  // Erase any messages from a previous run
  myMsg.counter = 0;

  // Create surfaces for TIA statistics and general messages
  myStatsMsg.color = kBtnTextColor;
  myStatsMsg.w = myOSystem->consoleFont().getMaxCharWidth() * 23 + 2;
  myStatsMsg.h = (myOSystem->consoleFont().getFontHeight() + 2) * 2;

 if(myStatsMsg.surface == NULL)
  {
    myStatsMsg.surfaceID = allocateSurface(myStatsMsg.w, myStatsMsg.h);
    myStatsMsg.surface   = surface(myStatsMsg.surfaceID);
  }
  if(myMsg.surface == NULL)
  {
    myMsg.surfaceID = allocateSurface(500, myOSystem->font().getFontHeight()+10);
    myMsg.surface   = surface(myMsg.surfaceID);
  }

  // Finally, show some information about the framebuffer,
  // but only on the first initialization
  if(myInitializedCount == 1)
    myOSystem->logMessage(about() + "\n", 1);

  return kSuccess;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::update()
{
  // Determine which mode we are in (from the EventHandler)
  // Take care of S_EMULATE mode here, otherwise let the GUI
  // figure out what to draw
  switch(myOSystem->eventHandler().state())
  {
    case EventHandler::S_EMULATE:
    {
      // Run the console for one frame
      // Note that the debugger can cause a breakpoint to occur, which changes
      // the EventHandler state 'behind our back' - we need to check for that
      myOSystem->console().tia().update();
  #ifdef DEBUGGER_SUPPORT
      if(myOSystem->eventHandler().state() != EventHandler::S_EMULATE) break;
  #endif
      if(myOSystem->eventHandler().frying())
        myOSystem->console().fry();

      // And update the screen
      drawTIA(myRedrawEntireFrame);

      // Show frame statistics
      if(myStatsMsg.enabled)
      {
        const ConsoleInfo& info = myOSystem->console().about();
        char msg[30];
        BSPF_snprintf(msg, 29, "%u @ %2.2ffps => %s",
                myOSystem->console().tia().scanlines(),
                myOSystem->console().getFramerate(), info.DisplayFormat.c_str());
        myStatsMsg.surface->fillRect(0, 0, myStatsMsg.w, myStatsMsg.h, kBGColor);
        myStatsMsg.surface->drawString(&myOSystem->consoleFont(),
          msg, 1, 1, myStatsMsg.w, myStatsMsg.color, kTextAlignLeft);
        myStatsMsg.surface->drawString(&myOSystem->consoleFont(),
          info.BankSwitch, 1, 15, myStatsMsg.w, myStatsMsg.color, kTextAlignLeft);
        myStatsMsg.surface->addDirtyRect(0, 0, 0, 0);  // force a full draw
        myStatsMsg.surface->setPos(myImageRect.x() + 1, myImageRect.y() + 1);
        myStatsMsg.surface->update();
      }
      break;  // S_EMULATE
    }

    case EventHandler::S_PAUSE:
    {
      // Only update the screen if it's been invalidated
      if(myRedrawEntireFrame)
        drawTIA(true);

      // Show a pause message every 5 seconds
      if(myPausedCount++ >= 7*myOSystem->frameRate())
      {
        myPausedCount = 0;
        showMessage("Paused", kMiddleCenter);
      }
      break;  // S_PAUSE
    }

    case EventHandler::S_MENU:
    {
      // When onscreen messages are enabled in double-buffer mode,
      // a full redraw is required
      myOSystem->menu().draw(myMsg.enabled && type() == kGLBuffer);
      break;  // S_MENU
    }

    case EventHandler::S_CMDMENU:
    {
      // When onscreen messages are enabled in double-buffer mode,
      // a full redraw is required
      myOSystem->commandMenu().draw(myMsg.enabled && type() == kGLBuffer);
      break;  // S_CMDMENU
    }

    case EventHandler::S_LAUNCHER:
    {
      // When onscreen messages are enabled in double-buffer mode,
      // a full redraw is required
      myOSystem->launcher().draw(myMsg.enabled && type() == kGLBuffer);
      break;  // S_LAUNCHER
    }

#ifdef DEBUGGER_SUPPORT
    case EventHandler::S_DEBUGGER:
    {
      // When onscreen messages are enabled in double-buffer mode,
      // a full redraw is required
      myOSystem->debugger().draw(myMsg.enabled && type() == kGLBuffer);
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

  // The frame doesn't need to be completely redrawn anymore
  myRedrawEntireFrame = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showMessage(const string& message, MessagePosition position,
                              bool force, uInt32 color)
{
  // Only show messages if they've been enabled
  if(!(force || myOSystem->settings().getBool("uimessages")))
    return;

  // Erase old messages on the screen
  if(myMsg.counter > 0)
  {
    myRedrawEntireFrame = true;
    refresh();
  }

  // Precompute the message coordinates
  myMsg.text    = message;
  myMsg.counter = uInt32(myOSystem->frameRate()) << 1; // Show message for 2 seconds
  myMsg.color   = color;

  myMsg.w = myOSystem->font().getStringWidth(myMsg.text) + 10;
  myMsg.h = myOSystem->font().getFontHeight() + 8;
  myMsg.surface->setWidth(myMsg.w);
  myMsg.surface->setHeight(myMsg.h);
  myMsg.position = position;
  myMsg.enabled = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleFrameStats()
{
  showFrameStats(!myOSystem->settings().getBool("stats"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showFrameStats(bool enable)
{
  myOSystem->settings().setBool("stats", enable);
  myStatsMsg.enabled = enable;
  refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::enableMessages(bool enable)
{
  if(enable)
  {
    // Only re-enable frame stats if they were already enabled before
    myStatsMsg.enabled = myOSystem->settings().getBool("stats");
  }
  else
  {
    // Temporarily disable frame stats
    myStatsMsg.enabled = false;

    // Erase old messages on the screen
    myMsg.enabled = false;
    myMsg.counter = 0;

    refresh();
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

  myMsg.surface->setPos(myMsg.x + myImageRect.x(), myMsg.y + myImageRect.y());
  myMsg.surface->fillRect(0, 0, myMsg.w-2, myMsg.h-4, kBGColor);
  myMsg.surface->box(0, 0, myMsg.w, myMsg.h-2, kColor, kShadowColor);
  myMsg.surface->drawString(&myOSystem->font(), myMsg.text, 4, 4,
                               myMsg.w, myMsg.color, kTextAlignLeft);
  myMsg.counter--;

  // Either erase the entire message (when time is reached),
  // or show again this frame
  if(myMsg.counter == 0)  // Force an immediate update
  {
    myMsg.enabled = false;
    refresh();
  }
  else
  {
    myMsg.surface->addDirtyRect(0, 0, 0, 0);
    myMsg.surface->update();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::refresh()
{
  // This method partly duplicates the behaviour in ::update()
  // Here, however, make sure to redraw *all* surfaces applicable to the
  // current EventHandler state
  // We also check for double-buffered modes, and when present
  // update both buffers accordingly
  //
  // This method is in essence a FULL refresh, putting all rendering
  // buffers in a known, fully redrawn state

  bool doubleBuffered = (type() == kGLBuffer);
  switch(myOSystem->eventHandler().state())
  {
    case EventHandler::S_EMULATE:
    case EventHandler::S_PAUSE:
      invalidate();
      drawTIA(true);
      if(doubleBuffered)
      {
        postFrameUpdate();
        invalidate();
        drawTIA(true);
      }
      break;

    case EventHandler::S_MENU:
      invalidate();
      drawTIA(true);
      myOSystem->menu().draw(true);
      if(doubleBuffered)
      {
        postFrameUpdate();
        invalidate();
        drawTIA(true);
        myOSystem->menu().draw(true);
      }
      break;

    case EventHandler::S_CMDMENU:
      invalidate();
      drawTIA(true);
      myOSystem->commandMenu().draw(true);
      if(doubleBuffered)
      {
        postFrameUpdate();
        invalidate();
        drawTIA(true);
        myOSystem->commandMenu().draw(true);
      }
      break;

    case EventHandler::S_LAUNCHER:
      invalidate();
      myOSystem->launcher().draw(true);
      if(doubleBuffered)
      {
        postFrameUpdate();
        invalidate();
        myOSystem->launcher().draw(true);
      }
      break;

  #ifdef DEBUGGER_SUPPORT
    case EventHandler::S_DEBUGGER:
      invalidate();
      myOSystem->debugger().draw(true);
      if(doubleBuffered)
      {
        postFrameUpdate();
        invalidate();
        myOSystem->debugger().draw(true);
      }
      break;
  #endif

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int FrameBuffer::allocateSurface(int w, int h, bool useBase)
{
  // Create a new surface
  FBSurface* surface = createSurface(w, h, useBase);

  // Add it to the list
  mySurfaceList.insert(make_pair(int(mySurfaceList.size()), surface));

  // Return a reference to it
  return mySurfaceList.size() - 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurface* FrameBuffer::surface(int id) const
{
  map<int,FBSurface*>::const_iterator iter = mySurfaceList.find(id);
  return iter != mySurfaceList.end() ? iter->second : NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::resetSurfaces(FBSurface* tiasurface)
{
  // Free all resources for each surface, then reload them
  // Due to possible timing and/or synchronization issues, all free()'s
  // are done first, then all reload()'s
  // Any derived FrameBuffer classes that call this method should be
  // aware of these restrictions, and act accordingly

  map<int,FBSurface*>::iterator iter;
  for(iter = mySurfaceList.begin(); iter != mySurfaceList.end(); ++iter)
    iter->second->free();
  if(tiasurface)
    tiasurface->free();
  for(iter = mySurfaceList.begin(); iter != mySurfaceList.end(); ++iter)
    iter->second->reload();
  if(tiasurface)
    tiasurface->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBuffer::tiaPixel(uInt32 idx, uInt8 shift) const
{
  uInt8 c = *(myOSystem->console().tia().currentFrameBuffer() + idx) | shift;
  uInt8 p = *(myOSystem->console().tia().previousFrameBuffer() + idx) | shift;

  return (!myUsePhosphor ? myDefPalette[c] : myAvgPalette[c][p]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setTIAPalette(const uInt32* palette)
{
  int i, j;

  // Set palette for normal fill
  for(i = 0; i < 256; ++i)
  {
    Uint8 r = (palette[i] >> 16) & 0xff;
    Uint8 g = (palette[i] >> 8) & 0xff;
    Uint8 b = palette[i] & 0xff;

    myDefPalette[i] = mapRGB(r, g, b);
    if(SDL_BYTEORDER == SDL_LIL_ENDIAN)
    {
      myDefPalette24[i][0] = b;
      myDefPalette24[i][1] = g;
      myDefPalette24[i][2] = r;
    }
    else
    {
      myDefPalette24[i][0] = r;
      myDefPalette24[i][1] = g;
      myDefPalette24[i][2] = b;
    }
  }

  // Set palette for phosphor effect
  for(i = 0; i < 256; ++i)
  {
    for(j = 0; j < 256; ++j)
    {
      uInt8 ri = (palette[i] >> 16) & 0xff;
      uInt8 gi = (palette[i] >> 8) & 0xff;
      uInt8 bi = palette[i] & 0xff;
      uInt8 rj = (palette[j] >> 16) & 0xff;
      uInt8 gj = (palette[j] >> 8) & 0xff;
      uInt8 bj = palette[j] & 0xff;

      Uint8 r = (Uint8) getPhosphor(ri, rj);
      Uint8 g = (Uint8) getPhosphor(gi, gj);
      Uint8 b = (Uint8) getPhosphor(bi, bj);

      myAvgPalette[i][j] = mapRGB(r, g, b);
    }
  }

  myRedrawEntireFrame = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setUIPalette(const uInt32* palette)
{
  // Set palette for GUI
  for(int i = 0, j = 256; i < kNumColors-256; ++i, ++j)
  {
    Uint8 r = (palette[i] >> 16) & 0xff;
    Uint8 g = (palette[i] >> 8) & 0xff;
    Uint8 b = palette[i] & 0xff;

    myDefPalette[j] = mapRGB(r, g, b);
    if(SDL_BYTEORDER == SDL_LIL_ENDIAN)
    {
      myDefPalette24[j][0] = b;
      myDefPalette24[j][1] = g;
      myDefPalette24[j][2] = r;
    }
    else
    {
      myDefPalette24[j][0] = r;
      myDefPalette24[j][1] = g;
      myDefPalette24[j][2] = b;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::stateChanged(EventHandler::State state)
{
  // Make sure any onscreen messages are removed
  myMsg.enabled = false;
  myMsg.counter = 0;

  myRedrawEntireFrame = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleFullscreen()
{
  setFullscreen(!fullScreen());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setFullscreen(bool enable)
{
#ifdef WINDOWED_SUPPORT
  // '-1' means fullscreen mode is completely disabled
  if(enable && myOSystem->settings().getString("fullscreen") != "-1" )
    mySDLFlags |= SDL_FULLSCREEN;
  else
    mySDLFlags &= ~SDL_FULLSCREEN;

  // Do a dummy call to getSavedVidMode to set up the modelists
  // and have it point to the correct 'current' mode
  getSavedVidMode();

  // Do a mode change to the 'current' mode by not passing a '+1' or '-1'
  // to changeVidMode()
  changeVidMode(0);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::changeVidMode(int direction)
{
  EventHandler::State state = myOSystem->eventHandler().state();
  bool inUIMode = (state == EventHandler::S_DEBUGGER ||
                   state == EventHandler::S_LAUNCHER);

  // Ignore any attempts to change video size while in UI mode
  if(inUIMode && direction != 0)
    return false;

  // Only save mode changes in TIA mode with a valid selector
  bool saveModeChange = !inUIMode && (direction == -1 || direction == +1);

  if(direction == +1)
    myCurrentModeList->next();
  else if(direction == -1)
    myCurrentModeList->previous();

  VideoMode vidmode = myCurrentModeList->current(myOSystem->settings(), fullScreen());
  if(setVidMode(vidmode))
  {
    centerAppWindow(vidmode);

    myImageRect.setWidth(vidmode.image_w);
    myImageRect.setHeight(vidmode.image_h);
    myImageRect.moveTo(vidmode.image_x, vidmode.image_y);

    myScreenRect.setWidth(vidmode.screen_w);
    myScreenRect.setHeight(vidmode.screen_h);

    // Did we get the requested fullscreen state?
    const string& fullscreen = myOSystem->settings().getString("fullscreen");
    if(fullscreen != "-1")
      myOSystem->settings().setString("fullscreen", fullScreen() ? "1" : "0");
    setCursorState();

    if(!inUIMode)
    {
      if(direction != 0)  // only show message when mode actually changes
        showMessage(vidmode.gfxmode.description);
    }
    if(saveModeChange)
      myOSystem->settings().setString("tia_filter", vidmode.gfxmode.name);

    refresh();
  }
  else
    return false;

  return true;
/*
cerr << "New mode:" << endl
	<< "    screen w = " << newmode.screen_w << endl
	<< "    screen h = " << newmode.screen_h << endl
	<< "    image x  = " << newmode.image_x << endl
	<< "    image y  = " << newmode.image_y << endl
	<< "    image w  = " << newmode.image_w << endl
	<< "    image h  = " << newmode.image_h << endl
	<< "    zoom     = " << newmode.zoom << endl
	<< "    name     = " << newmode.name << endl
	<< endl;
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setCursorState()
{
  // Always grab mouse in fullscreen or during emulation (if enabled),
  // and don't show the cursor during emulation
  bool emulation =
      myOSystem->eventHandler().state() == EventHandler::S_EMULATE;
  grabMouse(fullScreen() ||
    (emulation && myOSystem->settings().getBool("grabmouse")));
  showCursor(!emulation);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleGrabMouse()
{
  bool state = myOSystem->settings().getBool("grabmouse");
  myOSystem->settings().setBool("grabmouse", !state);
  setCursorState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showCursor(bool show)
{
  SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::grabMouse(bool grab)
{
  SDL_WM_GrabInput(grab ? SDL_GRAB_ON : SDL_GRAB_OFF);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::fullScreen() const
{
#ifdef WINDOWED_SUPPORT
  return mySDLFlags & SDL_FULLSCREEN;
#else
  return true;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setWindowTitle(const string& title)
{
  SDL_WM_SetCaption(title.c_str(), "stella");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setWindowIcon()
{
#ifndef MAC_OSX
  #include "stella.xpm"   // The Stella icon

  // Set the window icon
  uInt32 w, h, ncols, nbytes;
  uInt32 rgba[256], icon[32 * 32];
  uInt8  mask[32][4];

  sscanf(stella_icon[0], "%u %u %u %u", &w, &h, &ncols, &nbytes);
  if((w != 32) || (h != 32) || (ncols > 255) || (nbytes > 1))
  {
    myOSystem->logMessage("ERROR: Couldn't load the application icon.\n", 0);
    return;
  }

  for(uInt32 i = 0; i < ncols; i++)
  {
    unsigned char code;
    char color[32];
    uInt32 col;

    sscanf(stella_icon[1 + i], "%c c %s", &code, color);
    if(!strcmp(color, "None"))
      col = 0x00000000;
    else if(!strcmp(color, "black"))
      col = 0xFF000000;
    else if (color[0] == '#')
    {
      sscanf(color + 1, "%06x", &col);
      col |= 0xFF000000;
    }
    else
    {
      myOSystem->logMessage("ERROR: Couldn't load the application icon.\n", 0);
      return;
    }
    rgba[code] = col;
  }

  memset(mask, 0, sizeof(mask));
  for(h = 0; h < 32; h++)
  {
    const char* line = stella_icon[1 + ncols + h];
    for(w = 0; w < 32; w++)
    {
      icon[w + 32 * h] = rgba[(int)line[w]];
      if(rgba[(int)line[w]] & 0xFF000000)
        mask[h][w >> 3] |= 1 << (7 - (w & 0x07));
    }
  }

  SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(icon, 32, 32, 32,
                         32 * 4, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF000000);
  SDL_WM_SetIcon(surface, (unsigned char *) mask);
  SDL_FreeSurface(surface);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 FrameBuffer::getPhosphor(uInt8 c1, uInt8 c2) const
{
  if(c2 > c1)
    BSPF_swap(c1, c2);

  return ((c1 - c2) * myPhosphorBlend)/100 + c2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const StringMap& FrameBuffer::supportedTIAFilters(const string& type)
{
  uInt32 max_zoom = maxWindowSizeForScreen(320, 210,
                    myOSystem->desktopWidth(), myOSystem->desktopHeight());
  uInt8 mask = (type == "soft" ? 0x1 : 0x2);

  uInt32 firstmode = 1;
  if(myOSystem->desktopWidth() < 640 || myOSystem->desktopHeight() < 480)
    firstmode = 0;

  myTIAFilters.clear();
  for(uInt32 i = firstmode; i < GFX_NumModes; ++i)
  {
    // For now, just include all filters
    // This will change once OpenGL-only filters are added
    if((ourGraphicsModes[i].avail & mask) && ourGraphicsModes[i].zoom <= max_zoom)
    {
      myTIAFilters.push_back(ourGraphicsModes[i].description,
                             ourGraphicsModes[i].name);
    }
  }
  return myTIAFilters;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBuffer::maxWindowSizeForScreen(uInt32 baseWidth, uInt32 baseHeight,
                    uInt32 screenWidth, uInt32 screenHeight)
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
  // Modelists are different depending on what state we're in
  EventHandler::State state = myOSystem->eventHandler().state();
  bool inUIMode = (state == EventHandler::S_DEBUGGER ||
                   state == EventHandler::S_LAUNCHER);

  myWindowedModeList.clear();
  myFullscreenModeList.clear();

  // In UI/windowed mode, there's only one valid video mode we can use
  // We don't use maxWindowSizeForScreen here, since UI mode has to open its
  // window at the requested size
  if(inUIMode)
  {
    VideoMode m;
    m.image_x = m.image_y = 0;
    m.image_w = m.screen_w = baseWidth;
    m.image_h = m.screen_h = baseHeight;
    m.gfxmode = ourGraphicsModes[0];  // this should be zoom1x

    addVidMode(m);
  }
  else
  {
    // Scan list of filters, adding only those which are appropriate
    // for the given dimensions
    uInt32 max_zoom = maxWindowSizeForScreen(baseWidth, baseHeight,
                      myOSystem->desktopWidth(), myOSystem->desktopHeight());

    // Figure our the smallest zoom level we can use
    uInt32 firstmode = 1;
    if(myOSystem->desktopWidth() < 640 || myOSystem->desktopHeight() < 480)
      firstmode = 0;

    for(uInt32 i = firstmode; i < GFX_NumModes; ++i)
    {
      uInt32 zoom = ourGraphicsModes[i].zoom;
      if(zoom <= max_zoom)
      {
        VideoMode m;
        m.image_x = m.image_y = 0;
        m.image_w = m.screen_w = baseWidth * zoom;
        m.image_h = m.screen_h = baseHeight * zoom;
        m.gfxmode = ourGraphicsModes[i];

        addVidMode(m);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::addVidMode(VideoMode& mode)
{
  // The are minimum size requirements on a screen, no matter is in fullscreen
  // or windowed mode
  // Various part of the UI system depend on having at least 320x240 pixels
  // available, so we must enforce it here

  // Windowed modes can be sized exactly as required, since there's normally
  // no restriction on window size (between the minimum and maximum size)
  mode.screen_w = BSPF_max(mode.screen_w, 320u);
  mode.screen_h = BSPF_max(mode.screen_h, 240u);
  mode.image_x = (mode.screen_w - mode.image_w) >> 1;
  mode.image_y = (mode.screen_h - mode.image_h) >> 1;
  myWindowedModeList.add(mode);

  // There are often stricter requirements on fullscreen modes, and they're
  // normally different depending on the OSystem in use
  // As well, we usually can't get fullscreen modes in the exact size
  // we want, so we need to calculate image offsets
  const ResolutionList& res = myOSystem->supportedResolutions();
  for(uInt32 i = 0; i < res.size(); ++i)
  {
    if(mode.screen_w <= res[i].width && mode.screen_h <= res[i].height)
    {
      // Auto-calculate 'smart' centering; platform-specific framebuffers are
      // free to ignore or augment it
      mode.screen_w = BSPF_max(res[i].width, 320u);
      mode.screen_h = BSPF_max(res[i].height, 240u);
      mode.image_x = (mode.screen_w - mode.image_w) >> 1;
      mode.image_y = (mode.screen_h - mode.image_h) >> 1;
      break;
    }
  }
  myFullscreenModeList.add(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::VideoMode FrameBuffer::getSavedVidMode()
{
  EventHandler::State state = myOSystem->eventHandler().state();

  if(fullScreen())
    myCurrentModeList = &myFullscreenModeList;
  else
    myCurrentModeList = &myWindowedModeList;

  // Now select the best resolution depending on the state
  // UI modes (launcher and debugger) have only one supported resolution
  // so the 'current' one is the only valid one
  if(state == EventHandler::S_DEBUGGER || state == EventHandler::S_LAUNCHER)
  {
    myCurrentModeList->setByGfxMode(GFX_Zoom1x);
  }
  else
  {
    const string& name = myOSystem->settings().getString("tia_filter");
    myCurrentModeList->setByGfxMode(name);
  }

  return myCurrentModeList->current(myOSystem->settings(), fullScreen());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::centerAppWindow(const VideoMode& mode)
{
  // Attempt to center the application window in non-fullscreen mode
  if(!fullScreen() && myOSystem->settings().getBool("center"))
  {
    int x = mode.screen_w >= myOSystem->desktopWidth() ? 0 : 
      ((myOSystem->desktopWidth() - mode.screen_w) >> 1);
    int y = mode.screen_h >= myOSystem->desktopHeight() ? 0 : 
      ((myOSystem->desktopHeight() - mode.screen_h) >> 1);
    myOSystem->setAppWindowPos(x, y, mode.screen_w, mode.screen_h);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::VideoModeList::VideoModeList()
  : myIdx(-1)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::VideoModeList::~VideoModeList()
{
  clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::add(VideoMode mode)
{
  myModeList.push_back(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::clear()
{
  myModeList.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::VideoModeList::isEmpty() const
{
  return myModeList.isEmpty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBuffer::VideoModeList::size() const
{
  return myModeList.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::previous()
{
  --myIdx;
  if(myIdx < 0) myIdx = myModeList.size() - 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FrameBuffer::VideoMode FrameBuffer::
  VideoModeList::current(const Settings& settings, bool isFullscreen) const
{
  // Fullscreen modes are related to the 'fullres' setting
  //   If it's 'auto', we just use the mode as already previously defined
  //   If it's not 'auto', attempt to fit the mode into the resolution
  //   specified by 'fullres' (if possible)
  if(isFullscreen && !BSPF_equalsIgnoreCase(settings.getString("fullres"), "auto"))
  {
    // Only use 'fullres' if it's *bigger* than the requested mode
    int w, h;
    settings.getSize("fullres", w, h);

    if(w != -1 && h != -1 && (uInt32)w >= myModeList[myIdx].screen_w &&
      (uInt32)h >= myModeList[myIdx].screen_h)
    {
      VideoMode mode = myModeList[myIdx];
      mode.screen_w = w;
      mode.screen_h = h;
      mode.image_x = (mode.screen_w - mode.image_w) >> 1;
      mode.image_y = (mode.screen_h - mode.image_h) >> 1;

      return mode;
    }
  }

  // Otherwise, we just use the mode has it was defined in ::addVidMode()
  return myModeList[myIdx];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::next()
{
  myIdx = (myIdx + 1) % myModeList.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::setByGfxMode(GfxID id)
{
  // First we determine which graphics mode is being requested
  bool found = false;
  GraphicsMode gfxmode;
  for(uInt32 i = 0; i < GFX_NumModes; ++i)
  {
    if(ourGraphicsModes[i].type == id)
    {
      gfxmode = ourGraphicsModes[i];
      found = true;
      break;
    }
  }
  if(!found) gfxmode = ourGraphicsModes[0];

  // Now we scan the list for the applicable video mode
  set(gfxmode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::setByGfxMode(const string& name)
{
  // First we determine which graphics mode is being requested
  bool found = false;
  GraphicsMode gfxmode;
  for(uInt32 i = 0; i < GFX_NumModes; ++i)
  {
    if(BSPF_equalsIgnoreCase(ourGraphicsModes[i].name, name) ||
       BSPF_equalsIgnoreCase(ourGraphicsModes[i].description, name))
    {
      gfxmode = ourGraphicsModes[i];
      found = true;
      break;
    }
  }
  if(!found) gfxmode = ourGraphicsModes[0];

  // Now we scan the list for the applicable video mode
  set(gfxmode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::set(const GraphicsMode& gfxmode)
{
  // Attempt to point the current mode to the one given
  myIdx = -1;

  // First search for the given gfx id
  for(unsigned int i = 0; i < myModeList.size(); ++i)
  {
    if(myModeList[i].gfxmode.type == gfxmode.type)
    {
      myIdx = i;
      return;
    }
  }

  // If we get here, then the gfx type couldn't be found, so we search
  // for the first mode with the same zoomlevel (making sure that the
  // requested mode can fit inside the current screen)
  if(gfxmode.zoom > myModeList[myModeList.size()-1].gfxmode.zoom)
  {
    myIdx = myModeList.size()-1;
    return;
  }
  else
  {
    for(unsigned int i = 0; i < myModeList.size(); ++i)
    {
      if(myModeList[i].gfxmode.zoom == gfxmode.zoom)
      {
        myIdx = i;
        return;
      }
    }
  }

  // Finally, just pick the lowest video mode
  myIdx = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::print()
{
  cerr << "VideoModeList: " << endl << endl;
  for(Common::Array<VideoMode>::const_iterator i = myModeList.begin();
      i != myModeList.end(); ++i)
  {
    cerr << "  Mode " << i << endl;
    print(*i);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::print(const VideoMode& mode)
{
  cerr << "    screen w = " << mode.screen_w << endl
       << "    screen h = " << mode.screen_h << endl
       << "    image x  = " << mode.image_x << endl
       << "    image y  = " << mode.image_y << endl
       << "    image w  = " << mode.image_w << endl
       << "    image h  = " << mode.image_h << endl
       << "    gfx id   = " << mode.gfxmode.type << endl
       << "    gfx name = " << mode.gfxmode.name << endl
       << "    gfx desc = " << mode.gfxmode.description << endl
       << "    gfx zoom = " << mode.gfxmode.zoom << endl
       << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::box(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                    uInt32 colorA, uInt32 colorB)
{
  hLine(x + 1, y,     x + w - 2, colorA);
  hLine(x,     y + 1, x + w - 1, colorA);
  vLine(x,     y + 1, y + h - 2, colorA);
  vLine(x + 1, y,     y + h - 1, colorA);

  hLine(x + 1,     y + h - 2, x + w - 1, colorB);
  hLine(x + 1,     y + h - 1, x + w - 2, colorB);
  vLine(x + w - 1, y + 1,     y + h - 2, colorB);
  vLine(x + w - 2, y + 1,     y + h - 1, colorB);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::frameRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                          uInt32 color, FrameStyle style)
{
  switch(style)
  {
    case kSolidLine:
      hLine(x,         y,         x + w - 1, color);
      hLine(x,         y + h - 1, x + w - 1, color);
      vLine(x,         y,         y + h - 1, color);
      vLine(x + w - 1, y,         y + h - 1, color);
      break;

    case kDashLine:
      unsigned int i, skip, lwidth = 1;

      for(i = x, skip = 1; i < x+w-1; i=i+lwidth+1, ++skip)
      {
        if(skip % 2)
        {
          hLine(i, y,         i + lwidth, color);
          hLine(i, y + h - 1, i + lwidth, color);
        }
      }
      for(i = y, skip = 1; i < y+h-1; i=i+lwidth+1, ++skip)
      {
        if(skip % 2)
        {
          vLine(x,         i, i + lwidth, color);
          vLine(x + w - 1, i, i + lwidth, color);
        }
      }
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::drawString(const GUI::Font* font, const string& s,
                           int x, int y, int w,
                           uInt32 color, TextAlignment align,
                           int deltax, bool useEllipsis)
{
  const int leftX = x, rightX = x + w;
  unsigned int i;
  int width = font->getStringWidth(s);
  string str;
	
  if(useEllipsis && width > w)
  {
    // String is too wide. So we shorten it "intelligently", by replacing
    // parts of it by an ellipsis ("..."). There are three possibilities
    // for this: replace the start, the end, or the middle of the string.
    // What is best really depends on the context; but unless we want to
    // make this configurable, replacing the middle probably is a good
    // compromise.
    const int ellipsisWidth = font->getStringWidth("...");
		
    // SLOW algorithm to remove enough of the middle. But it is good enough for now.
    const int halfWidth = (w - ellipsisWidth) / 2;
    int w2 = 0;
		
    for(i = 0; i < s.size(); ++i)
    {
      int charWidth = font->getCharWidth(s[i]);
      if(w2 + charWidth > halfWidth)
        break;

      w2 += charWidth;
      str += s[i];
    }

    // At this point we know that the first 'i' chars are together 'w2'
    // pixels wide. We took the first i-1, and add "..." to them.
    str += "...";
		
    // The original string is width wide. Of those we already skipped past
    // w2 pixels, which means (width - w2) remain.
    // The new str is (w2+ellipsisWidth) wide, so we can accomodate about
    // (w - (w2+ellipsisWidth)) more pixels.
    // Thus we skip ((width - w2) - (w - (w2+ellipsisWidth))) =
    // (width + ellipsisWidth - w)
    int skip = width + ellipsisWidth - w;
    for(; i < s.size() && skip > 0; ++i)
      skip -= font->getCharWidth(s[i]);

    // Append the remaining chars, if any
    for(; i < s.size(); ++i)
      str += s[i];

    width = font->getStringWidth(str);
  }
  else
    str = s;

  if(align == kTextAlignCenter)
    x = x + (w - width - 1)/2;
  else if(align == kTextAlignRight)
    x = x + w - width;

  x += deltax;
  for(i = 0; i < str.size(); ++i)
  {
    w = font->getCharWidth(str[i]);
    if(x+w > rightX)
      break;
    if(x >= leftX)
      drawChar(font, str[i], x, y, color);

    x += w;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::GraphicsMode FrameBuffer::ourGraphicsModes[GFX_NumModes] = {
  { GFX_Zoom1x,  "zoom1x",  "Zoom 1x",  1,  0x3 },
  { GFX_Zoom2x,  "zoom2x",  "Zoom 2x",  2,  0x3 },
  { GFX_Zoom3x,  "zoom3x",  "Zoom 3x",  3,  0x3 },
  { GFX_Zoom4x,  "zoom4x",  "Zoom 4x",  4,  0x3 },
  { GFX_Zoom5x,  "zoom5x",  "Zoom 5x",  5,  0x3 },
  { GFX_Zoom6x,  "zoom6x",  "Zoom 6x",  6,  0x3 },
  { GFX_Zoom7x,  "zoom7x",  "Zoom 7x",  7,  0x3 },
  { GFX_Zoom8x,  "zoom8x",  "Zoom 8x",  8,  0x3 },
  { GFX_Zoom9x,  "zoom9x",  "Zoom 9x",  9,  0x3 },
  { GFX_Zoom10x, "zoom10x", "Zoom 10x", 10, 0x3 }
};
