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
	EGLDisplay eglDpy;
	EGLSurface eglSurface;
	EGLContext eglCtx;

public:

	void makeCurrent()
	{
		assert(eglCtx);
		eglMakeCurrent(eglDpy, eglSurface, eglSurface, eglCtx);
	}

	void swap()
	{
		eglSwapBuffers(eglDpy, eglSurface);
	}

	X11Window init(Display *dpy, int screen, uint xres, uint yres, bool multisample, long event_mask)
	{
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

		eglDpy  =  eglGetDisplay((EGLNativeDisplayType)dpy);
		if(eglDpy == EGL_NO_DISPLAY)
		{
			logErr("error getting EGL display");
			return 0;
		}

		if(!eglInitialize(eglDpy, NULL, NULL))
		{
			logErr("error initializing EGL");
			return 0;
		}

		printEGLConfs(eglDpy);

		static EGLint eglAttr[] =
		{
			//EGL_BUFFER_SIZE, 16,
			//EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
			EGL_NONE
		};

		EGLConfig config;
		EGLint configs;
		if(!eglChooseConfig(eglDpy, eglAttr, &config, 1, &configs))
		{
			logErr("error choosing config: 0x%X", (int)eglGetError());
			return 0;
		}

		eglSurface = eglCreateWindowSurface(eglDpy, config, win, NULL);
		if(eglSurface == EGL_NO_SURFACE )
		{
			logErr("error creating window surface: 0x%X", (int)eglGetError());
			return 0;
		}

		EGLint ctxAttr[] = {
		EGL_CONTEXT_CLIENT_VERSION, 1,
		EGL_NONE
		};
		eglCtx = eglCreateContext(eglDpy, config, EGL_NO_CONTEXT, ctxAttr);
		if(eglCtx == EGL_NO_CONTEXT)
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
		eglSwapInterval(eglDpy, interval);
	}

	void deinit()
	{
		eglDestroyContext(eglDpy, eglCtx);
		eglDestroySurface(eglDpy, eglSurface);
		eglTerminate(eglDpy);
		eglCtx = 0;
		eglSurface = 0;
		eglDpy = 0;
	}
};
