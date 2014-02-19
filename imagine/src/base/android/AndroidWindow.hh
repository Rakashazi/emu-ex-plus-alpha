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

#include <engine-globals.h>
#include <util/operators.hh>
#include <EGL/egl.h>

struct ANativeWindow;
struct ANativeActivity;

namespace Base
{

class AndroidWindow : public NotEquals<AndroidWindow>
{
public:
	ANativeWindow *nWin = nullptr;
	EGLSurface surface = EGL_NO_SURFACE;
	IG::WindowRect contentRect; // active window content
	float xDPI = 0, yDPI = 0; // Active DPI
	bool ranInit = false;

	constexpr AndroidWindow() {}

	bool operator ==(AndroidWindow const &rhs) const
	{
		return nWin == rhs.nWin;
	}

	operator bool() const
	{
		return nWin;
	}

	bool isDrawable()
	{
		return surface != EGL_NO_SURFACE;
	}

	void initSurface(EGLDisplay display, EGLConfig config, ANativeWindow *win);

	EGLint width(EGLDisplay display)
	{
		assert(surface != EGL_NO_SURFACE);
		assert(display != EGL_NO_DISPLAY);
		EGLint w;
		eglQuerySurface(display, surface, EGL_WIDTH, &w);
		return w;
	}

	EGLint height(EGLDisplay display)
	{
		assert(surface != EGL_NO_SURFACE);
		assert(display != EGL_NO_DISPLAY);
		EGLint h;
		eglQuerySurface(display, surface, EGL_HEIGHT, &h);
		return h;
	}
};

using WindowImpl = AndroidWindow;

}
