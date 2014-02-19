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
#define BOOL X11BOOL
#include <X11/X.h>
#include <X11/Xutil.h>
#ifdef CONFIG_BASE_X11_EGL
#include <util/egl.hh>
#else
#include <base/x11/glxIncludes.h>
#endif
#undef BOOL

namespace Base
{

class XWindow : public NotEquals<XWindow>
{
public:
	::Window xWin = None;
	::Window draggerXWin = None;
	Atom dragAction = None;
	#ifdef CONFIG_BASE_X11_EGL
	EGLSurface surface = EGL_NO_SURFACE;
	#endif

	constexpr XWindow() {}

	bool operator ==(XWindow const &rhs) const
	{
		return xWin == rhs.xWin;
	}

	operator bool() const
	{
		return xWin != None;
	}
};

void shutdownWindowSystem();

using WindowImpl = XWindow;

}
