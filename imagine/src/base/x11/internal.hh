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

#include <imagine/base/Screen.hh>
#include <imagine/base/EventLoop.hh>
#include <X11/Xlib.h>

namespace Base
{
	extern Display *xDisplay;

	Window *windowForXWindow(::Window xWin);
	int indexOfScreen(Screen &screen);
	void toggleFullScreen(Display *dpy, ::Window xWin);
	void initFrameTimer(EventLoop loop, Screen &screen);
	void deinitFrameTimer();
	void frameTimerScheduleVSync();
	void frameTimerCancel();
	bool frameTimeIsSimulated();
}

namespace Input
{
	void init(Display *dpy);
	void initPerWindowData(Display *dpy, ::Window win);
	void deinit(Display *dpy);
	// returns true if event is XI2, false otherwise
	bool handleXI2GenericEvent(Display *dpy, XEvent &event);
}

namespace Config
{
	namespace Base
	{
	static constexpr bool XDND = !Config::MACHINE_IS_PANDORA;
	}
}
