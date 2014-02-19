#pragma once

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

#include <base/x11/XWindow.hh>

namespace Base
{

class GLContextHelper
{
private:
	#ifdef CONFIG_BASE_X11_EGL
	EGLDisplay display = EGL_NO_DISPLAY;
	EGLContext ctx = EGL_NO_CONTEXT;
	EGLConfig config {};
	#else
	GLXContext ctx = nullptr;
	int (*glXSwapIntervalSGI)(int interval) = nullptr;
	int (*glXSwapIntervalMESA)(unsigned int interval) = nullptr;
	bool doubleBuffered = false;
	#endif

public:
	#if !defined CONFIG_MACHINE_PANDORA
	XVisualInfo *vi = nullptr;
	#endif
	bool useMaxColorBits = true;

	constexpr GLContextHelper() {}
	void makeCurrent(Display *dpy, const XWindow &win);
	void swap(Display *dpy, const XWindow &win);
	CallResult init(Display *dpy, int screen, bool multisample, uint version);
	CallResult initWindowSurface(XWindow &win);
	void deinitWindowSurface(XWindow &win);
	void setSwapInterval(uint interval);
	void deinit(Display *dpy);
	operator bool() const;
};

}
