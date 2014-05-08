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

#define LOGTAG "XWindow"
#include "../common/windowPrivate.hh"
#include "x11.hh"
#include "xlibutils.h"
#include "xdnd.hh"
#include "GLContextHelper.hh"

#ifdef CONFIG_BASE_FBDEV_VSYNC
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#endif

namespace Base
{

extern int fbdev;
GLContextHelper glCtx;

bool Window::shouldAnimateContentBoundsChange() const
{
	return true;
}

void Window::setPixelBestColorHint(bool best)
{
	// should only call before initial window creation
	assert(!windows());
	glCtx.useMaxColorBits = best;
}

bool Window::pixelBestColorHintDefault()
{
	return 1; // always prefer the best color format
}

void Window::setAcceptDnd(bool on)
{
	if(!Config::Base::XDND)
		return;
	if(on)
		enableXdnd(dpy, xWin);
	else
		disableXdnd(dpy, xWin);
}

void Window::setTitle(const char *name)
{
	XTextProperty nameProp;
	char tempName[128];
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

void Window::postDraw()
{
	//logMsg("posting window");
	setNeedsDraw(true);
	screen().postFrame();
}

void Window::unpostDraw()
{
	setNeedsDraw(false);
}

void Window::setSurfaceCurrent()
{
	glCtx.makeCurrent(dpy, *this);
}

bool Window::hasSurface()
{
	return true;
}

void Window::setPresentInterval(int interval)
{
	glCtx.makeCurrent(dpy, *this);
	drawTargetWindow = nullptr;
	glCtx.setSwapInterval(interval);
}

static CallResult setupGLWindow(Base::Window &win, Screen &screen, uint xres, uint yres)
{
	auto rootWindow = RootWindowOfScreen(screen.xScreen);
	XSetWindowAttributes attr {0};
	attr.event_mask = ExposureMask | PropertyChangeMask | StructureNotifyMask;
	#if defined CONFIG_MACHINE_PANDORA
	win.xWin = XCreateWindow(dpy, rootWindow, 0, 0, xres, yres, 0,
		CopyFromParent, InputOutput, CopyFromParent,
		CWEventMask, &attr);
	#else
	attr.colormap = XCreateColormap(dpy, rootWindow, glCtx.vi->visual, AllocNone);
	win.xWin = XCreateWindow(dpy, rootWindow, 0, 0, xres, yres, 0,
		glCtx.vi->depth, InputOutput, glCtx.vi->visual,
		CWColormap | CWEventMask, &attr);
	#endif
	if(!win.xWin)
		return INVALID_PARAMETER;
	glCtx.initWindowSurface(win);
	logMsg("created window with XID %d, drawable depth %d", (int)win.xWin, xDrawableDepth(dpy, win.xWin));
	if(Config::BASE_MULTI_WINDOW && Window::windows())
	{
		win.setPresentInterval(0); // set interval to 0 so multiple swaps can occur at the same time
	}
	//auto sres = glXJoinSwapGroupNV(dpy, win.xWin, 1);
	//assert(sres == True);

	Input::initPerWindowData(win.xWin);
	if(Config::MACHINE_IS_PANDORA)
	{
		auto wmState = XInternAtom(dpy, "_NET_WM_STATE", False);
		auto wmFullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
		XChangeProperty(dpy, win.xWin, wmState, XA_ATOM, 32, PropModeReplace, (uchar*)&wmFullscreen, 1);
	}
	else
	{
		auto wmDelete = XInternAtom(dpy, "WM_DELETE_WINDOW", True);
		XSetWMProtocols(dpy, win.xWin, &wmDelete, 1);
		XSizeHints hints
		{
			PMinSize,
			0, 0, 0, 0,
			320, 240 // min size
		};
		XSetWMNormalHints(dpy, win.xWin, &hints);
	}

	// setup xinput2
	XIEventMask eventmask;
	uchar mask[XIMaskLen(XI_LASTEVENT)] {0};
	eventmask.deviceid = XIAllMasterDevices;
	eventmask.mask_len = sizeof(mask); // always in bytes
	eventmask.mask = mask;
	XISetMask(mask, XI_ButtonPress);
	XISetMask(mask, XI_ButtonRelease);
	XISetMask(mask, XI_Motion);
	XISetMask(mask, XI_FocusIn);
	XISetMask(mask, XI_Enter);
	XISetMask(mask, XI_FocusOut);
	XISetMask(mask, XI_Leave);
	XISetMask(mask, XI_KeyPress);
	XISetMask(mask, XI_KeyRelease);
	XISelectEvents(dpy, win.xWin, &eventmask, 1);

	return OK;
}

void Window::swapBuffers()
{
	#ifdef CONFIG_BASE_FBDEV_VSYNC
	if(fbdev >= 0)
	{
		int arg = 0;
		ioctl(fbdev, FBIO_WAITFORVSYNC, &arg);
	}
	#endif
	glCtx.swap(dpy, *this);
}

IG::WindowRect Window::contentBounds() const
{
	return bounds();
}

IG::Point2D<float> Window::pixelSizeAsMM(IG::Point2D<int> size)
{
	assert(screen().xMM);
	return {screen().xMM * ((float)size.x/(float)screen().x), screen().yMM * ((float)size.y/(float)screen().y)};
}

Window *windowForXWindow(::Window xWin)
{
	iterateTimes(Window::windows(), i)
	{
		auto w = Window::window(i);
		if(w->xWin == xWin)
			return w;
	}
	return nullptr;
}

CallResult Window::init(IG::Point2D<int> pos, IG::Point2D<int> size, WindowInitDelegate onInit)
{
	if(xWin != None)
	{
		// already init
		return OK;
	}
	if(!Config::BASE_MULTI_WINDOW && windows())
	{
		bug_exit("no multi-window support");
	}
	initDelegates();
	#ifdef CONFIG_BASE_MULTI_SCREEN
	screen_ = &mainScreen();
	#endif
	#ifdef CONFIG_MACHINE_PANDORA
	int x = 800;
	int y = 480;
	#else
	int x = 1024;
	int y = 768;

	// try to crop window to workable desktop area
	long *workArea;
	int format;
	unsigned long items;
	unsigned long bytesAfter;
	uchar *prop;
	Atom type;
	Atom _NET_WORKAREA = XInternAtom(dpy, "_NET_WORKAREA", 0);
	if(XGetWindowProperty(dpy, DefaultRootWindow(dpy),
		_NET_WORKAREA, 0, ~0, False,
		XA_CARDINAL, &type, &format, &items, &bytesAfter, (uchar **)&workArea) || !workArea)
	{
		logWarn("error getting desktop work area");
	}
	else
	{
		logMsg("%ld %ld work area: %ld:%ld:%ld:%ld", items, bytesAfter, workArea[0], workArea[1], workArea[2], workArea[3]);
		x = std::min(x, (int)workArea[2]);
		y = std::min(y, (int)workArea[3]);
		XFree(workArea);
	}
	#endif

	updateSize({x, y});
	if(setupGLWindow(*this, screen(), w, h) != OK)
	{
		logErr("error initializing window");
		return INVALID_PARAMETER;
	}
	#ifdef CONFIG_BASE_MULTI_WINDOW
	window_.push_back(this);
	#else
	mainWin = this;
	#endif
	onInit(*this);
	return OK;
}

void Window::deinit()
{
	if(xWin == None)
	{
		return;
	}
	logMsg("destroying window with ID %d", (int)xWin);
	if(drawTargetWindow == this)
	{
		glCtx.makeCurrentSurfaceless(dpy);
		drawTargetWindow = nullptr;
	}
	glCtx.deinitWindowSurface(*this);
	XDestroyWindow(dpy, xWin);
	#ifdef CONFIG_BASE_MULTI_WINDOW
	window_.remove(this);
	#else
	mainWin = nullptr;
	#endif
	*this = {};
}

void shutdownWindowSystem()
{
	logMsg("shutting down window system");
	#ifdef CONFIG_BASE_MULTI_WINDOW
	for(auto w : window_)
	{
		glCtx.deinitWindowSurface(*w);
		XDestroyWindow(dpy, w->xWin);
	}
	#else
	if(mainWin)
		mainWin->deinit();
	#endif
	glCtx.deinit(dpy);
}

void Window::show()
{
	assert(xWin != None);
	postDraw();
	XMapRaised(dpy, xWin);
}

void Window::setAutoOrientation(bool on) {}
void Window::setSystemOrientation(uint o) {}

}
