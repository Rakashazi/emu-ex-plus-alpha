#pragma once

#include <config.h>
#include <config/machine.hh>

#define Time X11Time_
#define Pixmap X11Pixmap_
#define GC X11GC_
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
#undef Time
#undef Pixmap
#undef GC
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
