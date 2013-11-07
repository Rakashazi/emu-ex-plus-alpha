#include "GLContextHelper.hh"
#include <config/machine.hh>

namespace Base
{

GLContextHelper::operator bool() const
{
	return ctx;
}

CallResult GLContextHelper::init(Display *dpy, int screen, bool multisample)
{
	// init display
	display  = eglGetDisplay(Config::MACHINE_IS_PANDORA ? EGL_DEFAULT_DISPLAY : (EGLNativeDisplayType)dpy);
	if(display == EGL_NO_DISPLAY)
	{
		logErr("error getting EGL display");
		return INVALID_PARAMETER;
	}
	if(!eglInitialize(display, nullptr, nullptr))
	{
		logErr("error initializing EGL");
		return INVALID_PARAMETER;
	}
	//printEGLConfs(display);

	#ifndef CONFIG_GFX_OPENGL_ES
	if(!eglBindAPI(EGL_OPENGL_API))
	{
		bug_exit("OpenGL not a supported EGL API");
	}
	#endif


	// select config
	const EGLint *attribs = useMaxColorBits ? eglAttrWinRGB888 : eglAttrWinLowColor;
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
				return INVALID_PARAMETER;
			}
		}
		else
		{
			logErr("no valid EGL configs found");
			return INVALID_PARAMETER;
		}
	}
	#ifndef NDEBUG
	printEGLConf(display, config);
	#endif

	// get matching x visual
	#if !defined CONFIG_MACHINE_PANDORA
	{
		EGLint nativeID;
		eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &nativeID);
		XVisualInfo viTemplate;
		int visuals;
		viTemplate.visualid = nativeID;
		vi = XGetVisualInfo(dpy, VisualIDMask, &viTemplate, &visuals);
		if(!vi)
		{
			logErr("unable to find matching X Visual");
			return INVALID_PARAMETER;
		}
	}
	#endif

	// create context
	#ifdef CONFIG_GFX_OPENGL_ES
	EGLint ctxAttr[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 1,
		EGL_NONE
	};
	ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttr);
	#else
	ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, nullptr);
	#endif
	if(ctx == EGL_NO_CONTEXT)
	{
		logErr("error creating context: 0x%X", (int)eglGetError());
		return INVALID_PARAMETER;
	}

	return OK;
}

void GLContextHelper::makeCurrent(Display *dpy, XWindow const &win)
{
	assert(ctx != EGL_NO_CONTEXT);
	eglMakeCurrent(display, win.surface, win.surface, ctx);
}

void GLContextHelper::swap(Display *dpy, XWindow const &win)
{
	eglSwapBuffers(display, win.surface);
}

CallResult GLContextHelper::initWindowSurface(XWindow &win)
{
	//logMsg("setting up EGL window surface");
	assert(display != EGL_NO_DISPLAY);
	win.surface = eglCreateWindowSurface(display, config,
		Config::MACHINE_IS_PANDORA ? (EGLNativeWindowType)0 : (EGLNativeWindowType)win.xWin, nullptr);
	if(win.surface == EGL_NO_SURFACE)
	{
		logErr("error creating window surface: 0x%X", (int)eglGetError());
		return INVALID_PARAMETER;
	}
	return OK;
}

void GLContextHelper::deinitWindowSurface(XWindow &win)
{
	//logMsg("destroying EGL surface for XID: %d", (int)win.xWin);
	eglDestroySurface(display, win.surface);
	win.surface = EGL_NO_SURFACE;
}

void GLContextHelper::setSwapInterval(uint interval)
{
	if(Config::MACHINE_IS_PANDORA)
		return; // no effect
	assert(interval > 0);
	logMsg("set swap interval %d", interval);
	eglSwapInterval(display, interval);
}

void GLContextHelper::deinit(Display *dpy)
{
	logMsg("destroying EGL context & terminating display");
	eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	#if !defined CONFIG_MACHINE_PANDORA
	if(vi)
	{
		XFree(vi);
		vi = nullptr;
	}
	#endif
	eglDestroyContext(display, ctx);
	eglTerminate(display);
	display = EGL_NO_DISPLAY;
	ctx = EGL_NO_CONTEXT;
}

}
