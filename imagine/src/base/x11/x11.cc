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

#define LOGTAG "X11"
#include <imagine/input/Input.hh>
#include <imagine/logger/logger.h>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/util/string.h>
#include "../common/windowPrivate.hh"
#include "../common/basePrivate.hh"
#include "x11.hh"
#include "internal.hh"
#include "xdnd.hh"
#include "xlibutils.h"

#define ASCII_LF 0xA
#define ASCII_CR 0xD

namespace Base
{

Display *xDisplay{};

static GSourceFuncs x11SourceFuncs
{
	[](GSource *, gint *timeout)
	{
		*timeout = -1;
		return (gboolean)XPending(xDisplay);
	},
	[](GSource *)
	{
		return (gboolean)XPending(xDisplay);
	},
	[](GSource *, GSourceFunc, gpointer)
	{
		//logMsg("events for X fd");
		runX11Events({}, xDisplay); // TODO: Supply an ApplicationContext object
		return (gboolean)TRUE;
	},
	nullptr
};

// TODO: move into generic header after testing
static void fileURLToPath(char *url)
{
	char *pathStart = url;
	//lookup the 3rd slash which will signify the root directory of the file system
	for(int i = 0; i < 3; i++)
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
			int msd = char_hexToInt(pathStart[srcPos]) << 4;
			srcPos++;
			int lsd = char_hexToInt(pathStart[srcPos]);
			url[destPos] = msd + lsd;
		}
	}
	url[destPos] = '\0';
}

static void ewmhFullscreen(Display *dpy, ::Window win, int action)
{
	assert(action == _NET_WM_STATE_REMOVE || action == _NET_WM_STATE_ADD || action == _NET_WM_STATE_TOGGLE);

	XEvent xev{};
	xev.xclient.type = ClientMessage;
	xev.xclient.send_event = True;
	xev.xclient.message_type = XInternAtom(dpy, "_NET_WM_STATE", False);
	xev.xclient.window = win;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = action;
	xev.xclient.data.l[1] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);

	// TODO: test if DefaultRootWindow(dpy) works on other screens
	XWindowAttributes attr;
	XGetWindowAttributes(dpy, win, &attr);
	if(!XSendEvent(dpy, attr.root, False,
		SubstructureRedirectMask | SubstructureNotifyMask, &xev))
	{
		logWarn("couldn't send root window NET_WM_STATE message");
	}
}

void toggleFullScreen(Display *dpy, ::Window xWin)
{
	logMsg("toggle fullscreen");
	ewmhFullscreen(dpy, xWin, _NET_WM_STATE_TOGGLE);
}

static int eventHandler(ApplicationContext app, Display *dpy, XEvent &event)
{
	//logMsg("got event type %s (%d)", xEventTypeToString(event.type), event.type);

	switch(event.type)
	{
		bcase Expose:
		{
			auto &win = *windowForXWindow(app, event.xexpose.window);
			if (event.xexpose.count == 0)
				win.postDraw();
		}
		bcase ConfigureNotify:
		{
			//logMsg("ConfigureNotify");
			auto &win = *windowForXWindow(app, event.xconfigure.window);
			if(event.xconfigure.width == win.width() && event.xconfigure.height == win.height())
				break;
			win.updateSize({event.xconfigure.width, event.xconfigure.height});
		}
		bcase ClientMessage:
		{
			auto &win = *windowForXWindow(app, event.xclient.window);
			auto type = event.xclient.message_type;
			char *clientMsgName = XGetAtomName(dpy, type);
			//logDMsg("got client msg %s", clientMsgName);
			if(string_equal(clientMsgName, "WM_PROTOCOLS"))
			{
				if((Atom)event.xclient.data.l[0] == XInternAtom(dpy, "WM_DELETE_WINDOW", True))
				{
					//logMsg("got window manager delete window message");
					win.dispatchDismissRequest();
				}
				else
				{
					logMsg("unknown WM_PROTOCOLS message");
				}
			}
			else if(Config::Base::XDND && dndInit)
			{
				auto [draggerXWin, dragAction] = win.xdndData();
				handleXDNDEvent(dpy, event.xclient, win.nativeObject(), draggerXWin, dragAction);
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
				auto &win = *windowForXWindow(app, event.xselection.requestor);
				int format;
				unsigned long numItems;
				unsigned long bytesAfter;
				unsigned char *prop;
				Atom type;
				XGetWindowProperty(dpy, win.nativeObject(), event.xselection.property, 0, 256, False, AnyPropertyType, &type, &format, &numItems, &bytesAfter, &prop);
				logMsg("property read %lu items, in %d format, %lu bytes left", numItems, format, bytesAfter);
				logMsg("property is %s", prop);
				auto [draggerXWin, dragAction] = win.xdndData();
				sendDNDFinished(dpy, win.nativeObject(), draggerXWin, dragAction);
				auto filename = (char*)prop;
				fileURLToPath(filename);
				win.dispatchDragDrop(filename);
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
			Input::handleXI2GenericEvent(app, dpy, event);
		}
		bdefault:
		{
			logDMsg("got unhandled message type %d", event.type);
		}
		break;
	}

	return 1;
}

void runX11Events(ApplicationContext app, Display *dpy)
{
	while(XPending(dpy))
	{
		XEvent event;
		XNextEvent(dpy, &event);
		eventHandler(app, dpy, event);
	}
}

void initXScreens(ApplicationContext app, Display *dpy)
{
	auto defaultScreenIdx = DefaultScreen(dpy);
	app.addScreen(std::make_unique<Screen>(ScreenOfDisplay(dpy, defaultScreenIdx)), false);
	if constexpr(Config::BASE_MULTI_SCREEN)
	{
		iterateTimes(ScreenCount(dpy), i)
		{
			if((int)i == defaultScreenIdx)
				continue;
			app.addScreen(std::make_unique<Screen>(ScreenOfDisplay(dpy, i)), false);
		}
	}
}

Display *initX11(ApplicationContext app, EventLoop loop)
{
	XInitThreads();
	auto dpy = XOpenDisplay(0);
	if(!dpy)
	{
		logErr("couldn't open display");
		return {};
	}
	xDisplay = dpy;
	initXScreens(app, dpy);
	initFrameTimer(loop, app.mainScreen());
	Input::init(dpy);
	app.addOnExit(
		[dpy](ApplicationContext, bool backgrounded)
		{
			if(!backgrounded)
			{
				Base::deinitWindows();
				Input::deinit(dpy);
				logMsg("closing X display");
				XCloseDisplay(dpy);
			}
			return true;
		}, 10000);
	return dpy;
}

FDEventSource makeAttachedX11EventSource(ApplicationContext app, Display *dpy, EventLoop loop)
{
	FDEventSource x11Src{"XServer", ConnectionNumber(dpy)};
	x11Src.attach(loop, nullptr, &x11SourceFuncs);
	return x11Src;
}

void ApplicationContext::setSysUIStyle(uint32_t flags) {}

bool hasTranslucentSysUI() { return false; }

bool hasHardwareNavButtons() { return false; }

}
