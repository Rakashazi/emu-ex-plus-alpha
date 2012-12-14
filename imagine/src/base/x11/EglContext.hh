#pragma once

#define Time X11Time_
#define Pixmap X11Pixmap_
#define GC X11GC_
#define Window X11Window
#define BOOL X11BOOL
#include <util/egl.hh>
#undef Time
#undef Pixmap
#undef GC
#undef Window
#undef BOOL

class EglContext
{
	EGLDisplay display = EGL_NO_DISPLAY;
	EGLSurface surface = EGL_NO_SURFACE;
	EGLContext context = EGL_NO_CONTEXT;

public:
	constexpr EglContext() { }
	bool useMaxColorBits = 1;

	void makeCurrent()
	{
		assert(context);
		eglMakeCurrent(display, surface, surface, context);
	}

	void swap()
	{
		eglSwapBuffers(display, surface);
	}

	X11Window init(Display *dpy, int screen, uint xres, uint yres, bool multisample, long event_mask)
	{
		logMsg("setting up EGL window");
		XSetWindowAttributes attr = { 0 };
		attr.event_mask = event_mask;
		X11Window win = XCreateWindow(dpy, RootWindow(dpy, screen),
		              0, 0, xres, yres, 0,
		              CopyFromParent, InputOutput,
		              CopyFromParent, CWEventMask,
		              &attr);

		if(!win)
		{
			return 0;
		}

		display  =  eglGetDisplay((EGLNativeDisplayType)dpy);
		if(display == EGL_NO_DISPLAY)
		{
			logErr("error getting EGL display");
			return 0;
		}

		if(!eglInitialize(display, nullptr, nullptr))
		{
			logErr("error initializing EGL");
			return 0;
		}

		//printEGLConfs(display);

		const EGLint *attribs = useMaxColorBits ? eglAttrWinMaxRGB : eglAttrWinLowColor;
		EGLConfig config;
		EGLint configs;
		if(!eglChooseConfig(display, attribs, &config, 1, &configs))
		{
			logErr("error choosing config: 0x%X", (int)eglGetError());
			return 0;
		}
		#ifndef NDEBUG
		printEGLConf(display, config);
		#endif

		surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)win, nullptr);
		if(surface == EGL_NO_SURFACE)
		{
			logErr("error creating window surface: 0x%X", (int)eglGetError());
			return 0;
		}

		EGLint ctxAttr[] = {
		EGL_CONTEXT_CLIENT_VERSION, 1,
		EGL_NONE
		};
		context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttr);
		if(context == EGL_NO_CONTEXT)
		{
			logErr("error creating context: 0x%X", (int)eglGetError());
			return 0;
		}

		return win;
	}

	void setSwapInterval(uint interval)
	{
		assert(interval > 0);
		logMsg("set swap interval %d", interval);
		eglSwapInterval(display, interval);
	}

	void deinit()
	{
		eglDestroyContext(display, context);
		eglDestroySurface(display, surface);
		eglTerminate(display);
		display = EGL_NO_DISPLAY;
		surface = EGL_NO_SURFACE;
		context = EGL_NO_CONTEXT;
	}
};
