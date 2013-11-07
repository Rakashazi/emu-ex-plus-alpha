#pragma once

#include <engine-globals.h>
#include <util/operators.hh>
#define Time X11Time_
#define Pixmap X11Pixmap_
#define GC X11GC_
#define BOOL X11BOOL
#include <X11/X.h>
#include <X11/Xutil.h>
#ifdef CONFIG_BASE_X11_EGL
#include <util/egl.hh>
#else
#include <GL/glx.h>
#endif
#undef Time
#undef Pixmap
#undef GC
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
	void updateSize(int width, int height);
	void calcPhysicalSize();

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
