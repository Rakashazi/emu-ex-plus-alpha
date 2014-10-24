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
#include "internal.hh"
#include "xlibutils.h"
#include "xdnd.hh"
#include <imagine/base/GLContext.hh>

namespace Base
{

uint GLBufferConfigAttributes::defaultColorBits()
{
	return 24;
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
	screen()->postFrame();
}

void Window::unpostDraw()
{
	setNeedsDraw(false);
}

bool Window::hasSurface()
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
	assert(s.xMM);
	return {s.xMM * ((float)size.x/(float)s.width()), s.yMM * ((float)size.y/(float)s.height())};
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

static IG::WindowRect makeWindowRectWithConfig(const WindowConfig &config, ::Window rootWindow)
{
	IG::WindowRect workAreaRect;
	{
		long *workArea;
		int format;
		unsigned long items, bytesAfter;
		uchar *prop;
		Atom type;
		Atom _NET_WORKAREA = XInternAtom(dpy, "_NET_WORKAREA", 0);
		if(XGetWindowProperty(dpy, rootWindow,
			_NET_WORKAREA, 0, ~0, False,
			XA_CARDINAL, &type, &format, &items, &bytesAfter, (uchar **)&workArea) || !workArea)
		{
			logWarn("error getting desktop work area, using root window size");
			XWindowAttributes attr;
			XGetWindowAttributes(dpy, rootWindow, &attr);
			workAreaRect = {0, 0, attr.width, attr.height};
		}
		else
		{
			logMsg("work area: %ld:%ld:%ld:%ld", workArea[0], workArea[1], workArea[2], workArea[3]);
			workAreaRect = {(int)workArea[0], (int)workArea[1], (int)workArea[2], (int)workArea[3]};
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

CallResult Window::init(const WindowConfig &config)
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
	BaseWindow::init(config);
	#ifdef CONFIG_BASE_MULTI_SCREEN
	screen_ = &mainScreen();
	#endif
	auto rootWindow = RootWindowOfScreen(screen()->xScreen);
	#ifdef CONFIG_MACHINE_PANDORA
	IG::WindowRect winRect{0, 0, 800, 480};
	#else
	auto winRect = makeWindowRectWithConfig(config, rootWindow);
	#endif
	updateSize({winRect.xSize(), winRect.ySize()});
	XSetWindowAttributes attr{0};
	attr.event_mask = ExposureMask | PropertyChangeMask | StructureNotifyMask;
	#if defined CONFIG_MACHINE_PANDORA
	xWin = XCreateWindow(dpy, rootWindow, 0, 0, w, h, 0,
		CopyFromParent, InputOutput, CopyFromParent,
		CWEventMask, &attr);
	#else
	pos = {winRect.x, winRect.y};
	{
		attr.colormap = XCreateColormap(dpy, rootWindow, config.glConfig().visual, AllocNone);
		xWin = XCreateWindow(dpy, rootWindow, 0, 0, w, h, 0,
			config.glConfig().depth, InputOutput, config.glConfig().visual,
			CWColormap | CWEventMask, &attr);
	}
	#endif
	if(!xWin)
	{
		logErr("error initializing window");
		deinit();
		return INVALID_PARAMETER;
	}
	#ifdef CONFIG_BASE_X11_EGL
	//logMsg("setting up EGL window surface");
	surface = eglCreateWindowSurface(GLContext::eglDisplay(), config.glConfig().glConfig,
		Config::MACHINE_IS_PANDORA ? (EGLNativeWindowType)0 : (EGLNativeWindowType)xWin, nullptr);
	if(surface == EGL_NO_SURFACE)
	{
		logErr("error creating window surface: 0x%X", (int)eglGetError());
		deinit();
		return INVALID_PARAMETER;
	}
	#endif
	logMsg("created window with XID %d, drawable depth %d", (int)xWin, xDrawableDepth(dpy, xWin));
	Input::initPerWindowData(xWin);
	if(Config::MACHINE_IS_PANDORA)
	{
		auto wmState = XInternAtom(dpy, "_NET_WM_STATE", False);
		auto wmFullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
		XChangeProperty(dpy, xWin, wmState, XA_ATOM, 32, PropModeReplace, (uchar*)&wmFullscreen, 1);
	}
	else
	{
		auto wmDelete = XInternAtom(dpy, "WM_DELETE_WINDOW", True);
		XSetWMProtocols(dpy, xWin, &wmDelete, 1);
		if(config.minimumSize().x || config.minimumSize().y)
		{
			XSizeHints hints{0};
			hints.flags = PMinSize;
			hints.min_width = config.minimumSize().x;
			hints.min_height = config.minimumSize().y;
			XSetWMNormalHints(dpy, xWin, &hints);
		}
	}

	#ifdef CONFIG_BASE_MULTI_WINDOW
	window_.push_back(this);
	#else
	mainWin = this;
	#endif

	return OK;
}

void Window::deinit()
{
	#ifdef CONFIG_BASE_X11_EGL
	if(surface != EGL_NO_SURFACE)
	{
		eglDestroySurface(GLContext::eglDisplay(), surface);
		surface = EGL_NO_SURFACE;
	}
	#endif
	if(xWin != None)
	{
		logMsg("destroying window with ID %d", (int)xWin);
		XDestroyWindow(dpy, xWin);
		xWin = None;
	}
}

void deinitWindowSystem()
{
	logMsg("shutting down window system");
	GLContext::current().deinit();
	GLContext::setCurrent({}, nullptr);
	iterateTimes(Window::windows(), i)
	{
		Window::window(i)->deinit();
	}
	XCloseDisplay(dpy);
}

void Window::show()
{
	assert(xWin != None);
	postDraw();
	XMapRaised(dpy, xWin);
	#ifndef CONFIG_MACHINE_PANDORA
	XMoveWindow(dpy, xWin, pos.x, pos.y);
	#endif
}

bool Window::systemAnimatesRotation()
{
	return false;
}

}
