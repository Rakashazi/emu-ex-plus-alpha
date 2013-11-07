#pragma once

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
	CallResult init(Display *dpy, int screen, bool multisample);
	CallResult initWindowSurface(XWindow &win);
	void deinitWindowSurface(XWindow &win);
	void setSwapInterval(uint interval);
	void deinit(Display *dpy);
	operator bool() const;
};

}
