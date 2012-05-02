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
// $Id: OSystem.cxx 2270 2011-08-19 14:30:15Z stephena $
//============================================================================

#include <cassert>
#include <sstream>
#include <fstream>
#include <zlib.h>

#include <ctime>
#ifdef HAVE_GETTIMEOFDAY
  #include <sys/time.h>
#endif

#include "bspf.hxx"

#include "MediaFactory.hxx"
#include "Sound.hxx"

#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif

#ifdef CHEATCODE_SUPPORT
  #include "CheatManager.hxx"
#endif

#include "SerialPort.hxx"
#if defined(UNIX)
  #include "SerialPortUNIX.hxx"
#elif defined(WIN32)
  #include "SerialPortWin32.hxx"
#elif defined(MAC_OSX)
  #include "SerialPortMACOSX.hxx"
#endif

#include "FSNode.hxx"
#include "unzip.h"
#include "MD5.hxx"
#include "Cart.hxx"
#include "Settings.hxx"
#include "PropsSet.hxx"
#include "EventHandler.hxx"
#include "Menu.hxx"
#include "CommandMenu.hxx"
#include "Launcher.hxx"
#include "Font.hxx"
#include "StellaFont.hxx"
#include "StellaMediumFont.hxx"
#include "StellaLargeFont.hxx"
#include "ConsoleFont.hxx"
#include "Widget.hxx"
#include "Console.hxx"
#include "Random.hxx"
#include "StateManager.hxx"
#include "Version.hxx"

#include "OSystem.hxx"

#define MAX_ROM_SIZE  512 * 1024

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem::OSystem()
  : myEventHandler(NULL),
    myFrameBuffer(NULL),
    mySound(NULL),
    mySettings(NULL),
    myPropSet(NULL),
    myConsole(NULL),
    mySerialPort(NULL),
    myMenu(NULL),
    myCommandMenu(NULL),
    myLauncher(NULL),
    myDebugger(NULL),
    myCheatManager(NULL),
    myStateManager(NULL),
    myQuitLoop(false),
    myRomFile(""),
    myRomMD5(""),
    myFeatures(""),
    myBuildInfo(""),
    myFont(NULL),
    myConsoleFont(NULL)
{
  // Calculate startup time
  myMillisAtStart = (uInt32)(time(NULL) * 1000);

  // Get built-in features
  #ifdef DISPLAY_OPENGL
    myFeatures += "OpenGL ";
  #endif
  #ifdef SOUND_SUPPORT
    myFeatures += "Sound ";
  #endif
  #ifdef JOYSTICK_SUPPORT
    myFeatures += "Joystick ";
  #endif
  #ifdef DEBUGGER_SUPPORT
    myFeatures += "Debugger ";
  #endif
  #ifdef CHEATCODE_SUPPORT
    myFeatures += "Cheats";
  #endif

  // Get build info
  ostringstream info;
  const SDL_version* ver = SDL_Linked_Version();

  info << "Build " << STELLA_BUILD << ", using ";
  info << "SDL " << (int)ver->major << "." << (int)ver->minor << "." << (int)ver->patch << " ";
  info << "[" << BSPF_ARCH << "]";
  myBuildInfo = info.str();

#if 0
  // Debugging info for the GUI widgets
  ostringstream buf;
  buf << "  kStaticTextWidget   = " << kStaticTextWidget   << endl
      << "  kEditTextWidget     = " << kEditTextWidget     << endl
      << "  kButtonWidget       = " << kButtonWidget       << endl
      << "  kCheckboxWidget     = " << kCheckboxWidget     << endl
      << "  kSliderWidget       = " << kSliderWidget       << endl
      << "  kListWidget         = " << kListWidget         << endl
      << "  kScrollBarWidget    = " << kScrollBarWidget    << endl
      << "  kPopUpWidget        = " << kPopUpWidget        << endl
      << "  kTabWidget          = " << kTabWidget          << endl
      << "  kEventMappingWidget = " << kEventMappingWidget << endl
      << "  kEditableWidget     = " << kEditableWidget     << endl
      << "  kAudioWidget        = " << kAudioWidget        << endl
      << "  kColorWidget        = " << kColorWidget        << endl
      << "  kCpuWidget          = " << kCpuWidget          << endl
      << "  kDataGridOpsWidget  = " << kDataGridOpsWidget  << endl
      << "  kDataGridWidget     = " << kDataGridWidget     << endl
      << "  kPromptWidget       = " << kPromptWidget       << endl
      << "  kRamWidget          = " << kRamWidget          << endl
      << "  kRomListWidget      = " << kRomListWidget      << endl
      << "  kRomWidget          = " << kRomWidget          << endl
      << "  kTiaInfoWidget      = " << kTiaInfoWidget      << endl
      << "  kTiaOutputWidget    = " << kTiaOutputWidget    << endl
      << "  kTiaWidget          = " << kTiaWidget          << endl
      << "  kTiaZoomWidget      = " << kTiaZoomWidget      << endl
      << "  kToggleBitWidget    = " << kToggleBitWidget    << endl
      << "  kTogglePixelWidget  = " << kTogglePixelWidget  << endl
      << "  kToggleWidget       = " << kToggleWidget       << endl;
  logMessage(buf.str(), 0);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem::~OSystem()
{
  delete myMenu;
  delete myCommandMenu;
  delete myLauncher;
  delete myFont;
  delete myInfoFont;
  delete mySmallFont;
  delete myConsoleFont;
  delete myLauncherFont;

  // Remove any game console that is currently attached
  deleteConsole();

  // OSystem takes responsibility for framebuffer and sound,
  // since it created them
  delete myFrameBuffer;
  delete mySound;

  // These must be deleted after all the others
  // This is a bit hacky, since it depends on ordering
  // of d'tor calls
#ifdef DEBUGGER_SUPPORT
  delete myDebugger;
#endif
#ifdef CHEATCODE_SUPPORT
  delete myCheatManager;
#endif

  delete myStateManager;
  delete myPropSet;
  delete myEventHandler;

  delete mySerialPort;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::create()
{
  // Get updated paths for all configuration files
  setConfigPaths();
  ostringstream buf;
  buf << "Base directory:       '"
      << FilesystemNode(myBaseDir).getPath(false) << "'" << endl
      << "Configuration file:   '"
      << FilesystemNode(myConfigFile).getPath(false) << "'" << endl
      << "User game properties: '"
      << FilesystemNode(myPropertiesFile).getPath(false) << "'" << endl
      << endl;
  logMessage(buf.str(), 1);

  // Get relevant information about the video hardware
  // This must be done before any graphics context is created, since
  // it may be needed to initialize the size of graphical objects
  if(!queryVideoHardware())
    return false;

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
  bool smallScreen = myDesktopWidth < 640 || myDesktopHeight < 480;

  // The console font is always the same size (for now at least)
  myConsoleFont  = new GUI::Font(GUI::consoleDesc);

  // This font is used in a variety of situations when a really small
  // font is needed; we let the specific widget/dialog decide when to
  // use it
  mySmallFont = new GUI::Font(GUI::stellaDesc);

  // The general font used in all UI elements
  // This is determined by the size of the framebuffer
  myFont = new GUI::Font(smallScreen ? GUI::stellaDesc : GUI::stellaMediumDesc);

  // The info font used in all UI elements
  // This is determined by the size of the framebuffer
  myInfoFont = new GUI::Font(smallScreen ? GUI::stellaDesc : GUI::consoleDesc);

  // The font used by the ROM launcher
  // Normally, this is configurable by the user, except in the case of
  // very small screens
  if(!smallScreen)
  {    
    if(mySettings->getString("launcherfont") == "small")
      myLauncherFont = new GUI::Font(GUI::consoleDesc);
    else if(mySettings->getString("launcherfont") == "medium")
      myLauncherFont = new GUI::Font(GUI::stellaMediumDesc);
    else
      myLauncherFont = new GUI::Font(GUI::stellaLargeDesc);
  }
  else
    myLauncherFont = new GUI::Font(GUI::stellaDesc);

  // Create the event handler for the system
  myEventHandler = new EventHandler(this);
  myEventHandler->initialize();

  // Create a properties set for us to use and set it up
  myPropSet = new PropertiesSet(this);

#ifdef CHEATCODE_SUPPORT
  myCheatManager = new CheatManager(this);
  myCheatManager->loadCheatDatabase();
#endif

  // Create menu and launcher GUI objects
  myMenu = new Menu(this);
  myCommandMenu = new CommandMenu(this);
  myLauncher = new Launcher(this);
#ifdef DEBUGGER_SUPPORT
  myDebugger = new Debugger(this);
#endif
  myStateManager = new StateManager(this);

  // Create the sound object; the sound subsystem isn't actually
  // opened until needed, so this is non-blocking (on those systems
  // that only have a single sound device (no hardware mixing)
  createSound();

  // Create the serial port object
  // This is used by any controller that wants to directly access
  // a real serial port on the system
#if defined(UNIX)
  mySerialPort = new SerialPortUNIX();
#elif defined(WIN32)
  mySerialPort = new SerialPortWin32();
#elif defined(MAC_OSX)
  mySerialPort = new SerialPortMACOSX();
#else
  // Create an 'empty' serial port
  mySerialPort = new SerialPort();
#endif

  // Let the random class know about us; it needs access to getTicks()
  Random::setSystem(this);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setConfigPaths()
{
  // Paths are saved with special characters preserved ('~' or '.')
  // We do some error checking here, so the rest of the codebase doesn't
  // have to worry about it
  FilesystemNode node;
  string s;

  validatePath("statedir", "state", myStateDir);

  validatePath("ssdir", "snapshots", mySnapshotDir);

  validatePath("eepromdir", "", myEEPROMDir);

  validatePath("cfgdir", "cfg", myCfgDir);

  s = mySettings->getString("cheatfile");
  if(s == "") s = myBaseDir + "stella.cht";
  node = FilesystemNode(s);
  myCheatFile = node.getPath();
  mySettings->setString("cheatfile", node.getPath(false));

  s = mySettings->getString("palettefile");
  if(s == "") s = myBaseDir + "stella.pal";
  node = FilesystemNode(s);
  myPaletteFile = node.getPath();
  mySettings->setString("palettefile", node.getPath(false));

  s = mySettings->getString("propsfile");
  if(s == "") s = myBaseDir + "stella.pro";
  node = FilesystemNode(s);
  myPropertiesFile = node.getPath();
  mySettings->setString("propsfile", node.getPath(false));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setUIPalette()
{
  int palette = mySettings->getInt("uipalette") - 1;
  if(palette < 0 || palette >= kNumUIPalettes) palette = 0;
  myFrameBuffer->setUIPalette(&ourGUIColors[palette][0]);
  myFrameBuffer->refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setBaseDir(const string& basedir)
{
  FilesystemNode node(basedir);
  myBaseDir = node.getPath();
  if(!node.isDirectory())
  {
    AbstractFilesystemNode::makeDir(myBaseDir);
    myBaseDir = FilesystemNode(node.getPath()).getPath();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setConfigFile(const string& file)
{
  FilesystemNode node(file);
  myConfigFile = node.getPath();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setFramerate(float framerate)
{
  if(framerate > 0.0)
  {
    myDisplayFrameRate = framerate;
    myTimePerFrame = (uInt32)(1000000.0 / myDisplayFrameRate);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBInitStatus OSystem::createFrameBuffer()
{
  // There is only ever one FrameBuffer created per run of Stella
  // Due to the multi-surface nature of the FrameBuffer, repeatedly
  // creating and destroying framebuffer objects causes crashes which
  // are far too invasive to fix right now
  // Besides, how often does one really switch between software and
  // OpenGL rendering modes, and even when they do, does it really
  // need to be dynamic?

  bool firstTime = (myFrameBuffer == NULL);
  if(firstTime)
    myFrameBuffer = MediaFactory::createVideo(this);

  // Re-initialize the framebuffer to current settings
  FBInitStatus fbstatus = kFailComplete;
  switch(myEventHandler->state())
  {
    case EventHandler::S_EMULATE:
    case EventHandler::S_PAUSE:
    case EventHandler::S_MENU:
    case EventHandler::S_CMDMENU:
      fbstatus = myConsole->initializeVideo();
      if(fbstatus != kSuccess)
        goto fallback;
      break;  // S_EMULATE, S_PAUSE, S_MENU, S_CMDMENU

    case EventHandler::S_LAUNCHER:
      fbstatus = myLauncher->initializeVideo();
      if(fbstatus != kSuccess)
        goto fallback;
      break;  // S_LAUNCHER

#ifdef DEBUGGER_SUPPORT
    case EventHandler::S_DEBUGGER:
      fbstatus = myDebugger->initializeVideo();
      if(fbstatus != kSuccess)
        goto fallback;
      break;  // S_DEBUGGER
#endif

    default:  // Should never happen
      logMessage("ERROR: Unknown emulation state in createFrameBuffer()\n", 0);
      break;
  }

  // The following only need to be done once
  if(firstTime)
  {
    // Setup the SDL joysticks (must be done after FrameBuffer is created)
    myEventHandler->setupJoysticks();

    // Update the UI palette
    setUIPalette();
  }

  return fbstatus;

  // GOTO are normally considered evil, unless well documented :)
  // If initialization of video system fails while in OpenGL mode
  // because OpenGL is unavailable, attempt to fallback to software mode
  // Otherwise, pass the error to the parent
fallback:
  if(fbstatus == kFailNotSupported && myFrameBuffer &&
     myFrameBuffer->type() == kGLBuffer)
  {
    logMessage("ERROR: OpenGL mode failed, fallback to software\n", 0);
    delete myFrameBuffer; myFrameBuffer = NULL;
    mySettings->setString("video", "soft");
    FBInitStatus newstatus = createFrameBuffer();
    if(newstatus == kSuccess)
    {
      setFramerate(60);
      myFrameBuffer->showMessage("OpenGL mode failed, fallback to software",
                                 kMiddleCenter, true);
    }
    return newstatus;
  }
  else
    return fbstatus;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::createSound()
{
  delete mySound;  mySound = NULL;
  mySound = MediaFactory::createAudio(this);
#ifndef SOUND_SUPPORT
  mySettings->setBool("sound", false);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::createConsole(const string& romfile, const string& md5sum)
{
  ostringstream buf;

  // Do a little error checking; it shouldn't be necessary
  if(myConsole) deleteConsole();

  bool showmessage = false;

  // If a blank ROM has been given, we reload the current one (assuming one exists)
  if(romfile == "")
  {
    showmessage = true;  // we show a message if a ROM is being reloaded
    if(myRomFile == "")
    {
      logMessage("ERROR: Rom file not specified ...\n", 0);
      return false;
    }
  }
  else
  {
    myRomFile = romfile;
    myRomMD5  = md5sum;

    // Each time a new console is loaded, we simulate a cart removal
    // Some carts need knowledge of this, as they behave differently
    // based on how many power-cycles they've been through since plugged in
    mySettings->setInt("romloadcount", 0);
  }

  // Create an instance of the 2600 game console
  string type, id;
  myConsole = openConsole(myRomFile, myRomMD5, type, id);
  if(myConsole)
  {
  #ifdef CHEATCODE_SUPPORT
    myCheatManager->loadCheats(myRomMD5);
  #endif
    bool audiofirst = mySettings->getBool("audiofirst");
    //////////////////////////////////////////////////////////////////////////
    // For some reason, ATI video drivers for OpenGL in Win32 cause problems
    // if the sound isn't initialized before the video
    // According to the SDL documentation, it shouldn't matter what order the
    // systems are initialized, but apparently it *does* matter
    // For now, I'll just reverse the ordering, as suggested by 'zagon' at
    // http://www.atariage.com/forums/index.php?showtopic=126090&view=findpost&p=1648693
    // Hopefully it won't break anything else
    //////////////////////////////////////////////////////////////////////////
    if(audiofirst)  myConsole->initializeAudio();
    myEventHandler->reset(EventHandler::S_EMULATE);
    if(createFrameBuffer() != kSuccess)  // Takes care of initializeVideo()
    {
      logMessage("ERROR: Couldn't create framebuffer for console\n", 0);
      myEventHandler->reset(EventHandler::S_LAUNCHER);
      return false;
    }
    if(!audiofirst)  myConsole->initializeAudio();

    if(showmessage)
    {
      if(id == "")
        myFrameBuffer->showMessage("New console created");
      else
        myFrameBuffer->showMessage("Multicart " + type + ", loading ROM" + id);
    }
    buf << "Game console created:" << endl
        << "  ROM file: " << FilesystemNode(myRomFile).getPath(false) << endl << endl
        << getROMInfo(myConsole) << endl;
    logMessage(buf.str(), 1);

    // Update the timing info for a new console run
    resetLoopTiming();

    myFrameBuffer->setCursorState();

    // Also check if certain virtual buttons should be held down
    // These must be checked each time a new console is being created
    if(mySettings->getBool("holdreset"))
      myEventHandler->handleEvent(Event::ConsoleReset, 1);
    if(mySettings->getBool("holdselect"))
      myEventHandler->handleEvent(Event::ConsoleSelect, 1);
    if(mySettings->getBool("holdbutton0"))
      myEventHandler->handleEvent(Event::JoystickZeroFire1, 1);

    return true;
  }
  else
  {
    buf << "ERROR: Couldn't create console for " << myRomFile << endl;
    logMessage(buf.str(), 0);
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::deleteConsole()
{
  if(myConsole)
  {
    mySound->close();
  #ifdef CHEATCODE_SUPPORT
    myCheatManager->saveCheats(myConsole->properties().get(Cartridge_MD5));
  #endif
    ostringstream buf;
    double executionTime   = (double) myTimingInfo.totalTime / 1000000.0;
    double framesPerSecond = (double) myTimingInfo.totalFrames / executionTime;
    buf << "Game console stats:" << endl
        << "  Total frames drawn: " << myTimingInfo.totalFrames << endl
        << "  Total time (sec):   " << executionTime << endl
        << "  Frames per second:  " << framesPerSecond << endl
        << endl;
    logMessage(buf.str(), 1);

    delete myConsole;  myConsole = NULL;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::createLauncher()
{
  myEventHandler->reset(EventHandler::S_LAUNCHER);
  if(createFrameBuffer() != kSuccess)
  {
    logMessage("ERROR: Couldn't create launcher\n", 0);
    return false;
  }
  myLauncher->reStack();
  myFrameBuffer->setCursorState();
  myFrameBuffer->refresh();

  setFramerate(60);
  resetLoopTiming();

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string OSystem::getROMInfo(const string& romfile)
{
  string md5, type, id, result = "";
  Console* console = openConsole(romfile, md5, type, id);
  if(console)
  {
    result = getROMInfo(console);
    delete console;
  }
  else
    result = "ERROR: Couldn't get ROM info for " + romfile + " ...";

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string OSystem::MD5FromFile(const string& filename)
{
  string md5 = "";

  uInt8* image = 0;
  uInt32 size  = 0;
  if((image = openROM(filename, md5, size)) != 0)
    if(image != 0 && size > 0)
      delete[] image;

  return md5;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::logMessage(const string& message, uInt8 level)
{
  if(level == 0)
  {
    cout << message << flush;
    myLogMessages += message;
  }
  else if(level <= (uInt8)mySettings->getInt("loglevel"))
  {
    if(mySettings->getBool("logtoconsole"))
      cout << message << flush;
    myLogMessages += message;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console* OSystem::openConsole(const string& romfile, string& md5,
                              string& type, string& id)
{
#define CMDLINE_PROPS_UPDATE(cl_name, prop_name) \
  s = mySettings->getString(cl_name);            \
  if(s != "") props.set(prop_name, s);

  Console* console = (Console*) NULL;

  // Open the cartridge image and read it in
  uInt8* image = 0;
  uInt32 size  = 0;
  if((image = openROM(romfile, md5, size)) != 0)
  {
    // Get a valid set of properties, including any entered on the commandline
    // For initial creation of the Cart, we're only concerned with the BS type
    Properties props;
    myPropSet->getMD5(md5, props);
    string s = "";
    CMDLINE_PROPS_UPDATE("bs", Cartridge_Type);
    CMDLINE_PROPS_UPDATE("type", Cartridge_Type);

    // Now create the cartridge
    string cartmd5 = md5;
    type = props.get(Cartridge_Type);
    Cartridge* cart =
      Cartridge::create(image, size, cartmd5, type, id, *mySettings);

    // It's possible that the cart created was from a piece of the image,
    // and that the md5 (and hence the cart) has changed
    if(props.get(Cartridge_MD5) != cartmd5)
    {
      string name = props.get(Cartridge_Name);
      if(!myPropSet->getMD5(cartmd5, props))
      {
        // Cart md5 wasn't found, so we create a new props for it
        props.set(Cartridge_MD5, cartmd5);
        props.set(Cartridge_Name, name+id);
        myPropSet->insert(props, false);
      }
    }

    CMDLINE_PROPS_UPDATE("channels", Cartridge_Sound);
    CMDLINE_PROPS_UPDATE("ld", Console_LeftDifficulty);
    CMDLINE_PROPS_UPDATE("rd", Console_RightDifficulty);
    CMDLINE_PROPS_UPDATE("tv", Console_TelevisionType);
    CMDLINE_PROPS_UPDATE("sp", Console_SwapPorts);
    CMDLINE_PROPS_UPDATE("lc", Controller_Left);
    CMDLINE_PROPS_UPDATE("rc", Controller_Right);
    s = mySettings->getString("bc");
    if(s != "") { props.set(Controller_Left, s); props.set(Controller_Right, s); }
    CMDLINE_PROPS_UPDATE("cp", Controller_SwapPaddles);
    CMDLINE_PROPS_UPDATE("format", Display_Format);
    CMDLINE_PROPS_UPDATE("ystart", Display_YStart);
    CMDLINE_PROPS_UPDATE("height", Display_Height);
    CMDLINE_PROPS_UPDATE("pp", Display_Phosphor);
    CMDLINE_PROPS_UPDATE("ppblend", Display_PPBlend);

    // Finally, create the cart with the correct properties
    if(cart)
      console = new Console(this, cart, props);
  }
  else
  {
    ostringstream buf;
    buf << "ERROR: Couldn't open \'" << romfile << "\'" << endl;
    logMessage(buf.str(), 0);
  }

  // Free the image since we don't need it any longer
  if(image != 0 && size > 0)
    delete[] image;

  return console;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8* OSystem::openROM(string file, string& md5, uInt32& size)
{
  // This method has a documented side-effect:
  // It not only loads a ROM and creates an array with its contents,
  // but also adds a properties entry if the one for the ROM doesn't
  // contain a valid name

  uInt8* image = 0;

  // Try to open the file as a zipped archive
  // If that fails, we assume it's just a gzipped or normal data file
  unzFile tz;
  if((tz = unzOpen(file.c_str())) != NULL)
  {
    if(unzGoToFirstFile(tz) == UNZ_OK)
    {
      unz_file_info ufo;

      for(;;)  // Loop through all files for valid 2600 images
      {
        // Longer filenames might be possible, but I don't
        // think people would name files that long in zip files...
        char filename[1024];

        unzGetCurrentFileInfo(tz, &ufo, filename, 1024, 0, 0, 0, 0);
        filename[1023] = '\0';

        if(strlen(filename) >= 4)
        {
          // Grab 3-character extension
          const char* ext = filename + strlen(filename) - 4;

          if(BSPF_equalsIgnoreCase(ext, ".a26") || BSPF_equalsIgnoreCase(ext, ".bin") ||
             BSPF_equalsIgnoreCase(ext, ".rom"))
          {
            file = filename;
            break;
          }
        }

        // Scan the next file in the zip
        if(unzGoToNextFile(tz) != UNZ_OK)
          break;
      }

      // Now see if we got a valid image
      if(ufo.uncompressed_size <= 0)
      {
        unzClose(tz);
        return image;
      }
      size  = ufo.uncompressed_size;
      image = new uInt8[size];

      // We don't have to check for any return errors from these functions,
      // since if there are, 'image' will not contain a valid ROM and the
      // calling method can take care of it
      unzOpenCurrentFile(tz);
      unzReadCurrentFile(tz, image, size);
      unzCloseCurrentFile(tz);
      unzClose(tz);
    }
    else
    {
      unzClose(tz);
      return image;
    }
  }
  else
  {
    // Assume the file is either gzip'ed or not compressed at all
    gzFile f = gzopen(file.c_str(), "rb");
    if(!f)
      return image;

    image = new uInt8[MAX_ROM_SIZE];
    size = gzread(f, image, MAX_ROM_SIZE);
    gzclose(f);
  }

  // Zero-byte files should be automatically discarded
  if(size == 0)
  {
    delete[] image;
    return (uInt8*) 0;
  }

  // If we get to this point, we know we have a valid file to open
  // Now we make sure that the file has a valid properties entry
  // To save time, only generate an MD5 if we really need one
  if(md5 == "")
    md5 = MD5(image, size);

  // Some games may not have a name, since there may not
  // be an entry in stella.pro.  In that case, we use the rom name
  // and reinsert the properties object
  Properties props;
  if(!myPropSet->getMD5(md5, props))
  {
    // Get the filename from the rom pathname
    FilesystemNode node(file);
    file = node.getDisplayName();

    props.set(Cartridge_MD5, md5);
    props.set(Cartridge_Name, file);
    myPropSet->insert(props, false);
  }

  return image;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string OSystem::getROMInfo(const Console* console)
{
  const ConsoleInfo& info = console->about();
  ostringstream buf;

  buf << "  Cart Name:       " << info.CartName << endl
      << "  Cart MD5:        " << info.CartMD5 << endl
      << "  Controller 0:    " << info.Control0 << endl
      << "  Controller 1:    " << info.Control1 << endl
      << "  Display Format:  " << info.DisplayFormat << endl
      << "  Bankswitch Type: " << info.BankSwitch << endl;

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::resetLoopTiming()
{
  myTimingInfo.start = myTimingInfo.virt = getTicks();
  myTimingInfo.current = 0;
  myTimingInfo.totalTime = 0;
  myTimingInfo.totalFrames = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::validatePath(const string& setting, const string& partialpath,
                           string& fullpath)
{
  const string& s = mySettings->getString(setting) != "" ?
    mySettings->getString(setting) : myBaseDir + partialpath;
  FilesystemNode node(s);
  if(!node.isDirectory())
  {
    AbstractFilesystemNode::makeDir(s);
    node = FilesystemNode(node.getPath());
  }
  fullpath = node.getPath();
  mySettings->setString(setting, node.getPath(false));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setDefaultJoymap(Event::Type event, EventMode mode)
{
#define SET_DEFAULT_BTN(sdb_event, sdb_mode, sdb_stick, sdb_button, sdb_cmp_event) \
  if(eraseAll || sdb_cmp_event == sdb_event) \
    myEventHandler->setDefaultJoyMapping(sdb_event, sdb_mode, sdb_stick, sdb_button);

  bool eraseAll = (event == Event::NoType);
  switch(mode)
  {
    case kEmulationMode:  // Default emulation events
      // Left joystick (assume joystick zero, button zero)
      SET_DEFAULT_BTN(Event::JoystickZeroFire1, mode, 0, 0, event);
      // Right joystick (assume joystick one, button zero)
      SET_DEFAULT_BTN(Event::JoystickOneFire1, mode, 1, 0, event);
      break;

    case kMenuMode:  // Default menu/UI events
      // Left joystick (assume joystick zero, button zero)
      SET_DEFAULT_BTN(Event::UISelect, mode, 0, 0, event);
      // Right joystick (assume joystick one, button zero)
      SET_DEFAULT_BTN(Event::UISelect, mode, 1, 0, event);
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setDefaultJoyAxisMap(Event::Type event, EventMode mode)
{
#define SET_DEFAULT_AXIS(sda_event, sda_mode, sda_stick, sda_axis, sda_val, sda_cmp_event) \
  if(eraseAll || sda_cmp_event == sda_event) \
    myEventHandler->setDefaultJoyAxisMapping(sda_event, sda_mode, sda_stick, sda_axis, sda_val);

  bool eraseAll = (event == Event::NoType);
  switch(mode)
  {
    case kEmulationMode:  // Default emulation events
      // Left joystick left/right directions (assume joystick zero)
      SET_DEFAULT_AXIS(Event::JoystickZeroLeft, mode, 0, 0, 0, event);
      SET_DEFAULT_AXIS(Event::JoystickZeroRight, mode, 0, 0, 1, event);
      // Left joystick up/down directions (assume joystick zero)
      SET_DEFAULT_AXIS(Event::JoystickZeroUp, mode, 0, 1, 0, event);
      SET_DEFAULT_AXIS(Event::JoystickZeroDown, mode, 0, 1, 1, event);
      // Right joystick left/right directions (assume joystick one)
      SET_DEFAULT_AXIS(Event::JoystickOneLeft, mode, 1, 0, 0, event);
      SET_DEFAULT_AXIS(Event::JoystickOneRight, mode, 1, 0, 1, event);
      // Right joystick left/right directions (assume joystick one)
      SET_DEFAULT_AXIS(Event::JoystickOneUp, mode, 1, 1, 0, event);
      SET_DEFAULT_AXIS(Event::JoystickOneDown, mode, 1, 1, 1, event);
      break;

    case kMenuMode:  // Default menu/UI events
      SET_DEFAULT_AXIS(Event::UILeft, mode, 0, 0, 0, event);
      SET_DEFAULT_AXIS(Event::UIRight, mode, 0, 0, 1, event);
      SET_DEFAULT_AXIS(Event::UIUp, mode, 0, 1, 0, event);
      SET_DEFAULT_AXIS(Event::UIDown, mode, 0, 1, 1, event);
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setDefaultJoyHatMap(Event::Type event, EventMode mode)
{
#define SET_DEFAULT_HAT(sdh_event, sdh_mode, sdh_stick, sdh_hat, sdh_dir, sdh_cmp_event) \
  if(eraseAll || sdh_cmp_event == sdh_event) \
    myEventHandler->setDefaultJoyHatMapping(sdh_event, sdh_mode, sdh_stick, sdh_hat, sdh_dir);

  bool eraseAll = (event == Event::NoType);
  switch(mode)
  {
    case kEmulationMode:  // Default emulation events
      // Left joystick left/right directions (assume joystick zero and hat 0)
      SET_DEFAULT_HAT(Event::JoystickZeroLeft, mode, 0, 0, EVENT_HATLEFT, event);
      SET_DEFAULT_HAT(Event::JoystickZeroRight, mode, 0, 0, EVENT_HATRIGHT, event);
      // Left joystick up/down directions (assume joystick zero and hat 0)
      SET_DEFAULT_HAT(Event::JoystickZeroUp, mode, 0, 0, EVENT_HATUP, event);
      SET_DEFAULT_HAT(Event::JoystickZeroDown, mode, 0, 0, EVENT_HATDOWN, event);
      break;

    case kMenuMode:  // Default menu/UI events
      SET_DEFAULT_HAT(Event::UILeft, mode, 0, 0, EVENT_HATLEFT, event);
      SET_DEFAULT_HAT(Event::UIRight, mode, 0, 0, EVENT_HATRIGHT, event);
      SET_DEFAULT_HAT(Event::UIUp, mode, 0, 0, EVENT_HATUP, event);
      SET_DEFAULT_HAT(Event::UIDown, mode, 0, 0, EVENT_HATDOWN, event);
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::pollEvent()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::joyButtonHandled(int button)
{
  // Since we don't do any platform-specific event polling,
  // no button is ever handled at this level
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::stateChanged(EventHandler::State state)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt64 OSystem::getTicks() const
{
#ifdef HAVE_GETTIMEOFDAY
  // Gettimeofday natively refers to the UNIX epoch (a set time in the past)
  timeval now;
  gettimeofday(&now, 0);

  return uInt64(now.tv_sec) * 1000000 + now.tv_usec;
#else
  // We use SDL_GetTicks, but add in the time when the application was
  // initialized.  This is necessary, since SDL_GetTicks only measures how
  // long SDL has been running, which can be the same between multiple runs
  // of the application.
  return uInt64(SDL_GetTicks() + myMillisAtStart) * 1000;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::mainLoop()
{
  if(mySettings->getString("timing") == "sleep")
  {
    // Sleep-based wait: good for CPU, bad for graphical sync
    for(;;)
    {
      myTimingInfo.start = getTicks();
      myEventHandler->poll(myTimingInfo.start);
      if(myQuitLoop) break;  // Exit if the user wants to quit
      myFrameBuffer->update();
      myTimingInfo.current = getTicks();
      myTimingInfo.virt += myTimePerFrame;

      // Timestamps may periodically go out of sync, particularly on systems
      // that can have 'negative time' (ie, when the time seems to go backwards)
      // This normally results in having a very large delay time, so we check
      // for that and reset the timers when appropriate
      if((myTimingInfo.virt - myTimingInfo.current) > (myTimePerFrame << 1))
      {
        myTimingInfo.start = myTimingInfo.current = myTimingInfo.virt = getTicks();
      }

      if(myTimingInfo.current < myTimingInfo.virt)
        SDL_Delay((myTimingInfo.virt - myTimingInfo.current) / 1000);

      myTimingInfo.totalTime += (getTicks() - myTimingInfo.start);
      myTimingInfo.totalFrames++;
    }
  }
  else
  {
    // Busy-wait: bad for CPU, good for graphical sync
    for(;;)
    {
      myTimingInfo.start = getTicks();
      myEventHandler->poll(myTimingInfo.start);
      if(myQuitLoop) break;  // Exit if the user wants to quit
      myFrameBuffer->update();
      myTimingInfo.virt += myTimePerFrame;

      while(getTicks() < myTimingInfo.virt)
        ;  // busy-wait

      myTimingInfo.totalTime += (getTicks() - myTimingInfo.start);
      myTimingInfo.totalFrames++;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::queryVideoHardware()
{
  // Go ahead and open the video hardware; we're going to need it eventually
  if(SDL_WasInit(SDL_INIT_VIDEO) == 0)
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
      return false;

  // First get the maximum windowed desktop resolution
  // Check the 'maxres' setting, which is an undocumented developer feature
  // that specifies the desktop size
  // Normally, this wouldn't be set, and we ask SDL directly
  int w, h;
  mySettings->getSize("maxres", w, h);
  if(w <= 0 || h <= 0)
  {
    const SDL_VideoInfo* info = SDL_GetVideoInfo();
    myDesktopWidth  = info->current_w;
    myDesktopHeight = info->current_h;
  }
  else
  {
    myDesktopWidth  = BSPF_max(w, 320);
    myDesktopHeight = BSPF_max(h, 240);
  }

  // Various parts of the codebase assume a minimum screen size of 320x240
  assert(myDesktopWidth >= 320 && myDesktopHeight >= 240);

  // Then get the valid fullscreen modes
  // If there are any errors, just use the desktop resolution
  ostringstream buf;
  SDL_Rect** modes = SDL_ListModes(NULL, SDL_FULLSCREEN);
  if((modes == (SDL_Rect**)0) || (modes == (SDL_Rect**)-1))
  {
    Resolution r;
    r.width  = myDesktopWidth;
    r.height = myDesktopHeight;
    buf << r.width << "x" << r.height;
    r.name = buf.str();
    myResolutions.push_back(r);
  }
  else
  {
    // All modes must fit between the lower and upper limits of the desktop
    // For 'small' desktop, this means larger than 320x240
    // For 'large'/normal desktop, exclude all those less than 640x480
    bool largeDesktop = myDesktopWidth >= 640 && myDesktopHeight >= 480;
    uInt32 lowerWidth  = largeDesktop ? 640 : 320,
           lowerHeight = largeDesktop ? 480 : 240;
    for(uInt32 i = 0; modes[i]; ++i)
    {
      if(modes[i]->w >= lowerWidth && modes[i]->w <= myDesktopWidth &&
         modes[i]->h >= lowerHeight && modes[i]->h <= myDesktopHeight)
      {
        Resolution r;
        r.width  = modes[i]->w;
        r.height = modes[i]->h;
        buf.str("");
        buf << r.width << "x" << r.height;
        r.name = buf.str();
        myResolutions.insert_at(0, r);  // insert in opposite (of descending) order
      }
    }
    // If no modes were valid, use the desktop dimensions
    if(myResolutions.size() == 0)
    {
      Resolution r;
      r.width  = myDesktopWidth;
      r.height = myDesktopHeight;
      buf << r.width << "x" << r.height;
      r.name = buf.str();
      myResolutions.push_back(r);
    }
  }

  return true;
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
uInt32 OSystem::ourGUIColors[kNumUIPalettes][kNumColors-256] = {
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem::OSystem(const OSystem& osystem)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem& OSystem::operator = (const OSystem&)
{
  assert(false);
  return *this;
}
