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
#include <config/machine.hh>

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
		assert(context != EGL_NO_CONTEXT);
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
		auto win = XCreateWindow(dpy, RootWindow(dpy, screen),
				0, 0, xres, yres, 0, CopyFromParent, InputOutput,
				CopyFromParent, CWEventMask, &attr);

		if(!win)
		{
			return 0;
		}

		display  =  eglGetDisplay(Config::MACHINE_IS_PANDORA ? (EGLNativeDisplayType)nullptr : (EGLNativeDisplayType)dpy);

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

		#ifndef CONFIG_GFX_OPENGL_ES
		eglBindAPI(EGL_OPENGL_API);
		#endif

		//printEGLConfs(display);

		const EGLint *attribs = useMaxColorBits ? eglAttrWinRGB888 : eglAttrWinLowColor;
		EGLConfig config;
		EGLint configs = 0;
		eglChooseConfig(display, attribs, &config, 1, &configs);
		if(!configs)
		{
			if(useMaxColorBits)
			{
				logMsg("falling back to lowest color config");
				eglChooseConfig(display, eglAttrWinLowColor, &config, 1, &configs);
				if(!configs)
				{
					logErr("no valid EGL configs found");
					return 0;
				}
			}
			else
			{
				logErr("no valid EGL configs found");
				return 0;
			}
		}
		#ifndef NDEBUG
		printEGLConf(display, config);
		#endif

		surface = eglCreateWindowSurface(display, config,
			Config::MACHINE_IS_PANDORA ? (EGLNativeWindowType)EGL_DEFAULT_DISPLAY : (EGLNativeWindowType)win, nullptr);
		if(surface == EGL_NO_SURFACE)
		{
			logErr("error creating window surface: 0x%X", (int)eglGetError());
			return 0;
		}

		#ifdef CONFIG_GFX_OPENGL_ES
		EGLint ctxAttr[] =
		{
			EGL_CONTEXT_CLIENT_VERSION, 1,
			EGL_NONE
		};
		context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttr);
		#else
		context = eglCreateContext(display, config, EGL_NO_CONTEXT, nullptr);
		#endif
		if(context == EGL_NO_CONTEXT)
		{
			logErr("error creating context: 0x%X", (int)eglGetError());
			return 0;
		}

		return win;
	}

	void setSwapInterval(uint interval)
	{
		if(Config::MACHINE_IS_PANDORA)
			return; // no effect
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
