/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "Base"
#include <cstdlib>
#include <unistd.h>
#include <engine-globals.h>
#include <input/Input.hh>
#include <logger/interface.h>
#include <base/Base.hh>
#include <base/common/windowPrivate.hh>
#include <util/strings.h>
#include <util/time/sys.hh>
#include <util/fd-utils.h>
#include <util/string/generic.h>
#include <util/algorithm.h>
#include <base/common/funcs.h>
#include <base/x11/x11.hh>
#include <base/x11/xdnd.hh>
#include <config/machine.hh>
#include <algorithm>

#ifdef CONFIG_FS
#include <fs/sys.hh>
#endif

#ifdef CONFIG_INPUT_EVDEV
#include <input/evdev/evdev.hh>
#endif

#include <time.h>
#include <errno.h>

#include "input.hh"
#include "xlibutils.h"
#include "dbus.hh"

namespace Base
{

const char *appPath = nullptr;
int screen;
float dispXMM, dispYMM;
int dispX, dispY;
int fbdev = -1;
Display *dpy;
extern void runMainEventLoop();
extern void initMainEventLoop();

uint appActivityState() { return APP_RUNNING; }

static void cleanup()
{
	#ifdef CONFIG_BASE_DBUS
	deinitDBus();
	#endif
	shutdownWindowSystem();
	XCloseDisplay(dpy);
}

// TODO: move into generic header after testing
static void fileURLToPath(char *url)
{
	char *pathStart = url;
	//lookup the 3rd slash which will signify the root directory of the file system
	for(uint i = 0; i < 3; i++)
	{
		pathStart = strchr(pathStart + 1, '/');
	}
	assert(pathStart != NULL);

	// strip trailing new line junk at the end, needed for Nautilus
	char *pathEnd = &pathStart[strlen(pathStart)-1];
	assert(pathEnd >= pathStart);
	for(; *pathEnd == ASCII_LF || *pathEnd == ASCII_CR || *pathEnd == ' '; pathEnd--)
	{
		*pathEnd = '\0';
	}

	// copy the path over the old string
	size_t destPos = 0;
	for(size_t srcPos = 0; pathStart[srcPos] != '\0'; ({srcPos++; destPos++;}))
	{
		if(pathStart[srcPos] != '%') // plain copy case
		{
			url[destPos] = pathStart[srcPos];
		}
		else // decode the next 2 chars as hex digits
		{
			srcPos++;
			uchar msd = hexToInt(pathStart[srcPos]) << 4;
			srcPos++;
			uchar lsd = hexToInt(pathStart[srcPos]);
			url[destPos] = msd + lsd;
		}
	}
	url[destPos] = '\0';
}

uint Screen::refreshRate()
{
	auto conf = XRRGetScreenInfo(dpy, RootWindow(dpy, 0));
	auto rate = XRRConfigCurrentRate(conf);
	logMsg("refresh rate %d", (int)rate);
	return rate;
}

void Screen::setRefreshRate(uint rate)
{
	if(Config::MACHINE_IS_PANDORA)
	{
		if(rate == REFRESH_RATE_DEFAULT)
			rate = 60;
		if(rate != 50 && rate != 60)
		{
			logWarn("tried to set unsupported refresh rate: %u", rate);
		}
		char cmd[64];
		string_printf(cmd, "sudo /usr/pandora/scripts/op_lcdrate.sh %u", rate);
		int err = system(cmd);
		if(err)
		{
			logErr("error setting refresh rate, %d", err);
		}
	}
}

static void toggleFullScreen(Base::Window &win)
{
	logMsg("toggle fullscreen");
	ewmhFullscreen(dpy, win.xWin, _NET_WM_STATE_TOGGLE);
	win.postDraw();
}

void exit(int returnVal)
{
	onExit(0);
	cleanup();
	::exit(returnVal);
}

void abort() { ::abort(); }

static CallResult initX()
{
	dpy = XOpenDisplay(0);
	if(!dpy)
	{
		logErr("couldn't open display");
		return INVALID_PARAMETER;
	}
	
	screen = DefaultScreen(dpy);
	logMsg("using default screen %d", screen);

	return OK;
}

/*void openURL(const char *url)
{
	// TODO
	logMsg("openURL called with %s", url);
}*/

#ifdef CONFIG_FS
const char *storagePath()
{
	if(Config::MACHINE_IS_PANDORA)
	{
		static FsSys::cPath sdPath {""};
		FsSys dir;
		// look for the first mounted SD card
		if(dir.openDir("/media", 0,
			[](const char *name, int type) -> int
			{
				return type == Fs::TYPE_DIR && strstr(name, "mmcblk");
			}
		) == OK)
		{
			if(dir.numEntries())
			{
				//logMsg("storage dir: %s", dir.entryFilename(0));
				string_printf(sdPath, "/media/%s", dir.entryFilename(0));
			}
			else
				sdPath[0] = 0;
			dir.closeDir();
			if(strlen(sdPath))
			{
				return sdPath;
			}
		}
		// fall back to appPath
	}
	return appPath;
}
#endif

static int eventHandler(XEvent event)
{
	//logMsg("got event type %s (%d)", xEventTypeToString(event.type), event.type);
	
	switch (event.type)
	{
		bcase Expose:
		{
			auto &win = *windowForXWindow(event.xexpose.window);
			if (event.xexpose.count == 0)
				win.postDraw();
		}
		bcase ConfigureNotify:
		{
			//logMsg("ConfigureNotify");
			auto &win = *windowForXWindow(event.xconfigure.window);
			if(event.xconfigure.width == win.width() && event.xconfigure.height == win.height())
				break;
			if(win.updateSize({event.xconfigure.width, event.xconfigure.height}))
				win.postResize();
		}
		bcase ClientMessage:
		{
			auto &win = *windowForXWindow(event.xclient.window);
			auto type = event.xclient.message_type;
			char *clientMsgName = XGetAtomName(dpy, type);
			//logDMsg("got client msg %s", clientMsgName);
			if(string_equal(clientMsgName, "WM_PROTOCOLS"))
			{
				logMsg("exiting via window manager");
				XFree(clientMsgName);
				exit(0);
			}
			else if(Config::Base::XDND && dndInit)
			{
				handleXDNDEvent(dpy, event.xclient, win.xWin, win.draggerXWin, win.dragAction);
			}
			XFree(clientMsgName);
		}
		bcase PropertyNotify:
		{
			//logMsg("PropertyNotify");
		}
		bcase SelectionNotify:
		{
			logMsg("SelectionNotify");
			if(Config::Base::XDND && event.xselection.property != None)
			{
				auto &win = *windowForXWindow(event.xselection.requestor);
				int format;
				unsigned long numItems;
				unsigned long bytesAfter;
				unsigned char *prop;
				Atom type;
				XGetWindowProperty(dpy, win.xWin, event.xselection.property, 0, 256, False, AnyPropertyType, &type, &format, &numItems, &bytesAfter, &prop);
				logMsg("property read %lu items, in %d format, %lu bytes left", numItems, format, bytesAfter);
				logMsg("property is %s", prop);
				sendDNDFinished(dpy, win.xWin, win.draggerXWin, win.dragAction);
				auto filename = (char*)prop;
				fileURLToPath(filename);
				onDragDrop(win, filename);
				XFree(prop);
			}
		}
		bcase MapNotify:
		{
			//logDMsg("MapNotfiy");
		}
		bcase ReparentNotify:
		{
			//logDMsg("ReparentNotify");
		}
		bcase GenericEvent:
		{
			if(event.xcookie.extension == Input::xI2opcode && XGetEventData(dpy, &event.xcookie))
			{
				XGenericEventCookie *cookie = &event.xcookie;
				auto &ievent = *((XIDeviceEvent*)cookie->data);
				// XI_HierarchyChanged isn't window-specific
				if(unlikely(ievent.evtype == XI_HierarchyChanged))
				{
					//logMsg("input device hierarchy changed");
					auto &ev = *((XIHierarchyEvent*)cookie->data);
					iterateTimes(ev.num_info, i)
					{
						if(ev.info[i].flags & XISlaveAdded)
						{
							int devices;
							XIDeviceInfo *device = XIQueryDevice(dpy, ev.info[i].deviceid, &devices);
							if(devices)
							{
								if(device->use == XISlaveKeyboard)
								{
									Input::addXInputDevice(*device, true);
								}
								XIFreeDeviceInfo(device);
							}
						}
						else if(ev.info[i].flags & XISlaveRemoved)
						{
							Input::removeXInputDevice(ev.info[i].deviceid);
						}
					}
					XFreeEventData(dpy, &event.xcookie);
					break;
				}
				// others events are for specific windows
				auto destWin = windowForXWindow(ievent.event);
				if(unlikely(!destWin))
				{
					//logWarn("ignored event for unknown window");
					XFreeEventData(dpy, &event.xcookie);
					break;
				}
				auto &win = *destWin;
				//logMsg("device %d, event %s", ievent.deviceid, xIEventTypeToStr(ievent.evtype));
				switch(ievent.evtype)
				{
					bcase XI_ButtonPress:
						handlePointerButton(win, ievent.detail, Input::devIdToPointer(ievent.deviceid), Input::PUSHED, ievent.event_x, ievent.event_y, ievent.time);
					bcase XI_ButtonRelease:
						handlePointerButton(win, ievent.detail, Input::devIdToPointer(ievent.deviceid), Input::RELEASED, ievent.event_x, ievent.event_y, ievent.time);
					bcase XI_Motion:
						handlePointerMove(win, ievent.event_x, ievent.event_y, Input::devIdToPointer(ievent.deviceid), ievent.time);
					bcase XI_Enter:
						handlePointerEnter(win, Input::devIdToPointer(ievent.deviceid), ievent.event_x, ievent.event_y, ievent.time);
					bcase XI_Leave:
						handlePointerLeave(win, Input::devIdToPointer(ievent.deviceid), ievent.event_x, ievent.event_y, ievent.time);
					bcase XI_FocusIn:
						onFocusChange(win, 1);
					bcase XI_FocusOut:
						onFocusChange(win, 0);
					bcase XI_KeyPress:
					{
						auto dev = Input::deviceForInputId(ievent.sourceid);
						KeySym k;
						if(Input::translateKeycodes)
						{
							unsigned int modsReturn;
							XkbTranslateKeyCode(Input::coreKeyboardDesc, ievent.detail, ievent.mods.effective, &modsReturn, &k);
						}
						else
							k = XkbKeycodeToKeysym(dpy, ievent.detail, 0, 0);
						bool repeated = ievent.flags & XIKeyRepeat;
						//logMsg("press KeySym %d, KeyCode %d, repeat: %d", (int)k, ievent.detail, repeated);
						if(k == XK_Return && (ievent.mods.effective & Mod1Mask) && !repeated)
						{
							toggleFullScreen(win);
						}
						else
						{
							using namespace Input;
							if(!repeated || Input::allowKeyRepeats)
							{
								//logMsg("push KeySym %d, KeyCode %d", (int)k, ievent.detail);
								#ifdef CONFIG_INPUT_ICADE
								if(!dev->iCadeMode()
									|| (dev->iCadeMode() && !processICadeKey(Keycode::decodeAscii(k, 0), Input::PUSHED, *dev, win)))
								#endif
								{
									handleKeyEv(win, k, Input::PUSHED, ievent.mods.effective & ShiftMask, ievent.time, dev);
								}
							}
						}
					}
					bcase XI_KeyRelease:
					{
						auto dev = Input::deviceForInputId(ievent.sourceid);
						KeySym k;
						if(Input::translateKeycodes)
						{
							unsigned int modsReturn;
							XkbTranslateKeyCode(Input::coreKeyboardDesc, ievent.detail, ievent.mods.effective, &modsReturn, &k);
						}
						else
							k = XkbKeycodeToKeysym(dpy, ievent.detail, 0, 0);
						using namespace Input;
						//logMsg("release KeySym %d, KeyCode %d", (int)k, ievent.detail);
						#ifdef CONFIG_INPUT_ICADE
						if(!dev->iCadeMode()
							|| (dev->iCadeMode() && !processICadeKey(Keycode::decodeAscii(k, 0), Input::RELEASED, *dev, win)))
						#endif
						{
							handleKeyEv(win, k, Input::RELEASED, 0, ievent.time, dev);
						}
					}
				}
				XFreeEventData(dpy, &event.xcookie);
				return 1;
			}
		}
		bdefault:
		{
			logDMsg("got unhandled message type %d", event.type);
		}
		break;
	}

	return 1;
}

bool x11FDPending()
{
  return XPending(dpy) > 0;
}

void x11FDHandler()
{
	while(x11FDPending())
	{
		XEvent event;
		XNextEvent(dpy, &event);
		eventHandler(event);
	}
}

void setupScreenSizeFromX11()
{
	dispXMM = DisplayWidthMM(dpy, screen);
	dispYMM = DisplayHeightMM(dpy, screen);
}

}

int main(int argc, char** argv)
{
	using namespace Base;
	doOrAbort(logger_init());
	engineInit();

	#ifdef CONFIG_FS
	FsSys::changeToAppDir(argv[0]);
	#endif

	initMainEventLoop();

	if(Config::Base::FBDEV_VSYNC)
	{
		fbdev = open("/dev/fb0", O_RDONLY);
		if(fbdev == -1)
		{
			logWarn("unable to open framebuffer device");
		}
	}

	doOrElse(initX(), return 1);
	#ifdef CONFIG_INPUT
	doOrAbort(Input::init());
	#endif
	#ifdef CONFIG_INPUT_EVDEV
	Input::initEvdev();
	#endif
	dispX = DisplayWidth(dpy, screen);
	dispY = DisplayHeight(dpy, screen);
	setupScreenSizeFromX11();
	
	EventLoopFileSource x11Src;
	x11Src.initX(ConnectionNumber(dpy));

	doOrAbort(onInit(argc, argv));
	runMainEventLoop();
	return 0;
}
