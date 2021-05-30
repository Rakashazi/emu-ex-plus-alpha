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

#define LOGTAG "Window"
#include <imagine/base/Screen.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Application.hh>
#include <imagine/pixmap/PixelFormat.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/string.h>
#include "xlibutils.h"
#include <X11/Xatom.h>
#include <X11/Xutil.h>

namespace Base
{

IG::PixelFormat ApplicationContext::defaultWindowPixelFormat() const
{
	return Config::MACHINE_IS_PANDORA ? PIXEL_FMT_RGB565 : PIXEL_FMT_RGBA8888;
}

void Window::setAcceptDnd(bool on)
{
	if(!Config::Base::XDND)
		return;
	application().setXdnd(xWin, on);
}

void Window::setTitle(const char *name)
{
	XTextProperty nameProp;
	char tempName[128]{};
	if(name)
		string_copy(tempName, name);
	char *tempNameArr[1] {tempName};
	if(XStringListToTextProperty(tempNameArr, 1, &nameProp))
	{
		XSetWMName(dpy, xWin, &nameProp);
		XFree(nameProp.value);
	}
	else
	{
		logWarn("unable to set window title, out of memory in XStringListToTextProperty()");
	}
}

bool Window::hasSurface() const
{
	return true;
}

IG::WindowRect Window::contentBounds() const
{
	return bounds();
}

IG::Point2D<float> Window::pixelSizeAsMM(IG::Point2D<int> size)
{
	auto &s = *screen();
	auto [xMM, yMM] = s.mmSize();
	assert(xMM);
	return {xMM * ((float)size.x/(float)s.width()), yMM * ((float)size.y/(float)s.height())};
}

static IG::WindowRect makeWindowRectWithConfig(Display *dpy, const WindowConfig &config, ::Window rootWindow)
{
	IG::WindowRect workAreaRect;
	{
		long *workArea;
		int format;
		unsigned long items, bytesAfter;
		uint8_t *prop;
		Atom type;
		Atom _NET_WORKAREA = XInternAtom(dpy, "_NET_WORKAREA", 0);
		if(XGetWindowProperty(dpy, rootWindow,
			_NET_WORKAREA, 0, ~0, False,
			XA_CARDINAL, &type, &format, &items, &bytesAfter, (uint8_t **)&workArea) || !workArea)
		{
			logWarn("error getting desktop work area, using root window size");
			XWindowAttributes attr;
			XGetWindowAttributes(dpy, rootWindow, &attr);
			workAreaRect = {{}, {attr.width, attr.height}};
		}
		else
		{
			logMsg("work area: %ld:%ld:%ld:%ld", workArea[0], workArea[1], workArea[2], workArea[3]);
			workAreaRect = {{(int)workArea[0], (int)workArea[1]}, {(int)workArea[2], (int)workArea[3]}};
			XFree(workArea);
		}
	}

	IG::WindowRect winRect;

	// set window size
	if(config.isDefaultSize())
	{
		winRect.x2 = workAreaRect.xSize()/2;
		winRect.y2 = workAreaRect.ySize()/2;
	}
	else
	{
		winRect.x2 = config.size().x;
		winRect.y2 = config.size().y;
	}

	// reduce size to work area if too big
	if(winRect.xSize() > workAreaRect.xSize())
	{
		winRect.x2 = workAreaRect.xSize();
	}
	if(winRect.ySize() > workAreaRect.ySize())
	{
		winRect.y2 = workAreaRect.ySize();
	}
	assert(winRect.xSize() > 0);
	assert(winRect.ySize() > 0);

	// set window position
	if(config.isDefaultPosition())
	{
		// move to center of work area
		winRect.setPos(workAreaRect.pos(C2DO), C2DO);
	}
	else
	{
		winRect.setPos(config.position(), LT2DO);
	}

	// crop right & bottom to work area if overflowing
	if(winRect.x2 > workAreaRect.x2)
	{
		winRect.x2 = workAreaRect.x2;
	}
	if(winRect.y2 > workAreaRect.y2)
	{
		winRect.y2 = workAreaRect.y2;
	}
	logMsg("made window rect %d:%d:%d:%d", winRect.x, winRect.y, winRect.x2, winRect.y2);
	return winRect;
}

Window::Window(ApplicationContext ctx, WindowConfig config, InitDelegate):
	XWindow{ctx, config}
{
	auto &screen = ctx.mainScreen();
	this->screen_ = &screen;
	auto xScreen = (::Screen*)screen.nativeObject();
	auto rootWindow = RootWindowOfScreen(xScreen);
	auto dpy = DisplayOfScreen(xScreen);
	auto winRect = Config::MACHINE_IS_PANDORA ? IG::WindowRect{{}, {800, 480}} :
		makeWindowRectWithConfig(dpy, config, rootWindow);
	updateSize({winRect.xSize(), winRect.ySize()});
	{
		XSetWindowAttributes attr{};
		attr.event_mask = ExposureMask | PropertyChangeMask | StructureNotifyMask;
		unsigned long valueMask = CWEventMask;
		Visual *visual = DefaultVisualOfScreen(xScreen);
		if(config.format())
		{
			visual = (Visual*)config.format();
			attr.colormap = XCreateColormap(dpy, rootWindow, visual, AllocNone);
			valueMask |= CWColormap;
		}
		xWin = XCreateWindow(dpy, rootWindow, winRect.x, winRect.y, width(), height(), 0,
			CopyFromParent, InputOutput, visual, valueMask, &attr);
		if(!xWin)
		{
			logErr("error initializing window");
			return;
		}
		colormap = attr.colormap;
	}
	logMsg("made window with XID %d, drawable depth %d", (int)xWin, xDrawableDepth(dpy, xWin));
	ctx.application().initPerWindowInputData(xWin);
	if(Config::MACHINE_IS_PANDORA)
	{
		auto wmState = XInternAtom(dpy, "_NET_WM_STATE", False);
		auto wmFullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
		XChangeProperty(dpy, xWin, wmState, XA_ATOM, 32, PropModeReplace, (uint8_t*)&wmFullscreen, 1);
	}
	else
	{
		auto wmDelete = XInternAtom(dpy, "WM_DELETE_WINDOW", True);
		XSetWMProtocols(dpy, xWin, &wmDelete, 1);
		if(config.minimumSize().x || config.minimumSize().y)
		{
			XSizeHints hints{};
			hints.x = winRect.x;
			hints.y = winRect.y;
			hints.min_width = config.minimumSize().x;
			hints.min_height = config.minimumSize().y;
			hints.flags = PPosition | PMinSize;
			XSetWMNormalHints(dpy, xWin, &hints);
		}
	}
	this->dpy = dpy;
	if(config.title())
		setTitle(config.title());
}

XWindow::~XWindow()
{
	if(xWin != None)
	{
		logMsg("destroying window with ID %d", (int)xWin);
		XDestroyWindow(dpy, xWin);
	}
	if(colormap != None)
	{
		XFreeColormap(dpy, colormap);
	}
}

void Window::show()
{
	assert(xWin != None);
	XMapRaised(dpy, xWin);
	postDraw();
}

NativeWindow Window::nativeObject() const
{
	return xWin;
}

void Window::setIntendedFrameRate(double rate)
{
	screen()->setFrameRate(rate);
}

void Window::setFormat(NativeWindowFormat fmt) {}

void Window::setFormat(IG::PixelFormat) {}

IG::PixelFormat Window::pixelFormat() const
{
	auto xScreen = (::Screen*)screen()->nativeObject();
	if(DefaultDepthOfScreen(xScreen) == 16)
		return IG::PIXEL_FMT_RGB565;
	return IG::PIXEL_FMT_RGBA8888;
}

std::pair<unsigned long, unsigned long> XWindow::xdndData() const
{
	return {draggerXWin, dragAction};
}

bool Window::operator ==(Window const &rhs) const
{
	return xWin == rhs.xWin;
}

XWindow::operator bool() const
{
	return xWin != 0L;
}

void Window::setCursorVisible(bool on)
{
	if constexpr(Config::MACHINE_IS_PANDORA)
	{
		if(on)
			XFixesShowCursor(dpy, xWin);
		else
			XFixesHideCursor(dpy, xWin);
	}
	else
	{
		application().setWindowCursor(xWin, on);
	}
}

static void ewmhFullscreen(Display *dpy, ::Window win, int action)
{
	assert(action == _NET_WM_STATE_REMOVE || action == _NET_WM_STATE_ADD || action == _NET_WM_STATE_TOGGLE);
	XEvent xev
	{
		.xclient =
		{
			.type = ClientMessage,
			.serial = 0,
			.send_event = True,
			.display = {},
			.window = win,
			.message_type = XInternAtom(dpy, "_NET_WM_STATE", False),
			.format = 32,
			.data =	{	.l{action, (long)XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False)}	}
		}
	};
	XWindowAttributes attr;
	XGetWindowAttributes(dpy, win, &attr);
	if(!XSendEvent(dpy, attr.root, False,
		SubstructureRedirectMask | SubstructureNotifyMask, &xev))
	{
		logWarn("couldn't send root window NET_WM_STATE message");
	}
}

void XWindow::toggleFullScreen()
{
	logMsg("toggling fullscreen");
	ewmhFullscreen(dpy, xWin, _NET_WM_STATE_TOGGLE);
}

void WindowConfig::setFormat(IG::PixelFormat) {}

}
