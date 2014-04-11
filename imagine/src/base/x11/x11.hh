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

#include <imagine/engine-globals.h>

#define BOOL X11BOOL
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>
#include <X11/cursorfont.h>
#include <X11/XKBlib.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrandr.h>
#undef BOOL

namespace Base
{
	extern Display *dpy;
	extern int screen;
	extern float dispXMM, dispYMM;
	extern int dispX, dispY;

	void setupScreenSizeFromX11();
	Window *windowForXWindow(::Window xWin);
}

namespace Input
{
	void initPerWindowData(::Window win);
}

namespace Config
{
	namespace Base
	{
	#if defined CONFIG_MACHINE_PANDORA
	#define CONFIG_BASE_FBDEV_VSYNC
	static constexpr bool FBDEV_VSYNC = true;
	#else
	static constexpr bool FBDEV_VSYNC = false;
	#endif

	static constexpr bool XDND = !Config::MACHINE_IS_PANDORA;
	}
}
