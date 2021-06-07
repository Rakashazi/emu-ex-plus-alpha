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
#include <imagine/base/Application.hh>
#include <imagine/base/Window.hh>
#include <imagine/util/string.h>
#include "xdnd.hh"
#include "xlibutils.h"

static constexpr char ASCII_LF = 0xA;
static constexpr char ASCII_CR = 0xD;

namespace Base
{

struct XGlibSource : public GSource
{
	::Display *xDisplay{};
	XApplication *appPtr{};
};

static GSourceFuncs x11SourceFuncs
{
	.prepare
	{
		[](GSource *src, gint *timeout)
		{
			*timeout = -1;
			auto xDisplay = static_cast<XGlibSource*>(src)->xDisplay;
			return (gboolean)XPending(xDisplay);
		}
	},
	.check
	{
		[](GSource *src)
		{
			auto xDisplay = static_cast<XGlibSource*>(src)->xDisplay;
			return (gboolean)XPending(xDisplay);
		}
	},
	.dispatch
	{
		[](GSource *src, GSourceFunc, gpointer)
		{
			//logMsg("events for X fd");
			auto &xGlibSrc = *static_cast<XGlibSource*>(src);
			xGlibSrc.appPtr->runX11Events(xGlibSrc.xDisplay);
			return (gboolean)TRUE;
		}
	},
	.finalize{},
	.closure_callback{},
	.closure_marshal{},
};

XApplication::XApplication(ApplicationInitParams initParams):
	LinuxApplication{initParams},
	supportedFrameTimer{testFrameTimers()}
{
	xEventSrc = makeXDisplayConnection(initParams.eventLoop);
}

XApplication::~XApplication()
{
	deinitWindows();
	deinitInputSystem();
	logMsg("closing X display");
	XCloseDisplay(dpy);
}

void XApplicationContext::setApplicationPtr(Application *appPtr_)
{
	appPtr = appPtr_;
}

Application &XApplicationContext::application() const
{
	return *static_cast<Application*>(appPtr);
}

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

Window *XApplication::windowForXWindow(::Window xWin) const
{
	for(auto &w : windows())
	{
		if(w->nativeObject() == xWin)
			return w.get();
	}
	return nullptr;
}

bool XApplication::eventHandler(XEvent event)
{
	//logMsg("got event type %s (%d)", xEventTypeToString(event.type), event.type);

	switch(event.type)
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
			win.updateSize({event.xconfigure.width, event.xconfigure.height});
		}
		bcase ClientMessage:
		{
			auto &win = *windowForXWindow(event.xclient.window);
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
			else if(Config::Base::XDND && xdndIsInit())
			{
				auto [draggerXWin, dragAction] = win.xdndData();
				handleXDNDEvent(dpy, xdndAtom, event.xclient, win.nativeObject(), draggerXWin, dragAction);
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
				XGetWindowProperty(dpy, win.nativeObject(), event.xselection.property, 0, 256, False, AnyPropertyType, &type, &format, &numItems, &bytesAfter, &prop);
				logMsg("property read %lu items, in %d format, %lu bytes left", numItems, format, bytesAfter);
				logMsg("property is %s", prop);
				auto [draggerXWin, dragAction] = win.xdndData();
				sendDNDFinished(win.nativeObject(), draggerXWin, dragAction);
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
			handleXI2GenericEvent(event);
		}
		bdefault:
		{
			logDMsg("got unhandled message type %d", event.type);
		}
		break;
	}

	return true;
}

void XApplication::runX11Events(_XDisplay *dpy)
{
	while(XPending(dpy))
	{
		XEvent event;
		XNextEvent(dpy, &event);
		eventHandler(event);
	}
}

void XApplication::runX11Events()
{
	runX11Events(dpy);
}

::Display *XApplication::xDisplay() const
{
	return dpy;
}

NativeDisplayConnection ApplicationContext::nativeDisplayConnection() const
{
	return (NativeDisplayConnection)application().xDisplay();
}

void XApplication::setWindowCursor(::Window xWin, bool on)
{
	auto cursor = on ? normalCursor : blankCursor;
	XDefineCursor(dpy, xWin, cursor);
}

void initXScreens(ApplicationContext ctx, Display *dpy)
{
	auto defaultScreenIdx = DefaultScreen(dpy);
	ctx.application().addScreen(ctx, std::make_unique<Screen>(ctx, Screen::InitParams{ScreenOfDisplay(dpy, defaultScreenIdx)}), false);
	if constexpr(Config::BASE_MULTI_SCREEN)
	{
		iterateTimes(ScreenCount(dpy), i)
		{
			if((int)i == defaultScreenIdx)
				continue;
			ctx.application().addScreen(ctx, std::make_unique<Screen>(ScreenOfDisplay(dpy, i)), false);
		}
	}
}

FDEventSource XApplication::makeXDisplayConnection(EventLoop loop)
{
	XInitThreads();
	auto xDisplay = XOpenDisplay(0);
	if(!xDisplay)
	{
		logErr("couldn't open display");
		return {};
	}
	dpy = xDisplay;
	ApplicationContext appCtx{*this};
	initXScreens(appCtx, xDisplay);
	initInputSystem();
	FDEventSource x11Src{"XServer", ConnectionNumber(xDisplay)};
	auto source = (XGlibSource*)g_source_new(&x11SourceFuncs, sizeof(XGlibSource));
	source->xDisplay = xDisplay;
	source->appPtr = this;
	x11Src.attach(loop, source);
	return x11Src;
}

}
