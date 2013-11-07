#pragma once

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
	IG::Rect2<int> contentRect; // active window content
	float xDPI = 0, yDPI = 0; // Active DPI
	bool inDraw = false, inResize = false;

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

	bool isFirstInit()
	{
		return xDPI == 0;
	}
};

using WindowImpl = AndroidWindow;

}
