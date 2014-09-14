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
#include <imagine/base/Screen.hh>

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
	Window *windowForXWindow(::Window xWin);
	int indexOfScreen(Screen &screen);
	void toggleFullScreen(::Window xWin);
	void initFrameTimer();
	void frameTimerScheduleVSync();
	void frameTimerCancel();
}

namespace Input
{
	void initPerWindowData(::Window win);

	// returns true if event is XI2, false otherwise
	bool handleXI2GenericEvent(XEvent &event);
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
