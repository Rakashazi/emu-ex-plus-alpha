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

#include "GLContextHelper.hh"
#include <EGL/eglext.h>

namespace Base
{

EGLSurface dummyPbuff = EGL_NO_SURFACE; // dummy pbuffer when no windows exist

GLContextHelper::operator bool() const
{
	return ctx;
}

#ifndef CONFIG_GFX_OPENGL_ES
static EGLContext createContextForMajorVersion(uint version, Display *dpy, EGLConfig config)
{
	if(version >= 3)
	{
		{
			// Try 3.2 Core
			const EGLint attrCore3_2[]
			{
				EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
				EGL_CONTEXT_MINOR_VERSION_KHR, 2,
				EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
				None
			};
			auto ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, attrCore3_2);
			if(ctx)
				return ctx;
			logErr("failed creating 3.2 core context");
		}
		{
			// Try 3.1
			const EGLint attr3_1[] =
			{
				EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
				EGL_CONTEXT_MINOR_VERSION_KHR, 1,
				None
			};
			auto ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, attr3_1);
			if(ctx)
				return ctx;
			logErr("failed creating 3.1 context");
		}
		{
			// Try 3.0
			const EGLint attr3_0[] =
			{
				EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
				EGL_CONTEXT_MINOR_VERSION_KHR, 0,
				None
			};
			auto ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, attr3_0);
			if(ctx)
				return ctx;
			logErr("failed creating 3.0 context");
		}
		// Fallback to 1.2
	}
	const EGLint attr[] =
	{
		EGL_CONTEXT_MAJOR_VERSION_KHR, 1,
		EGL_CONTEXT_MINOR_VERSION_KHR, 2,
		None
	};
	auto ctx = eglCreateContext(dpy, config, 0, attr);
	if(ctx)
		return ctx;
	logErr("failed creating 1.2 context");
	return EGL_NO_CONTEXT;
}
#endif

CallResult GLContextHelper::init(Display *dpy, Screen &screen, bool multisample, uint version)
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
	#if defined CONFIG_GFX_OPENGL_ES
	const EGLint *attribs = useMaxColorBits ?
		(version == 1 ? eglAttrWinRGB888 : eglAttrWinRGB888ES2) :
		(version == 1 ? eglAttrWinLowColor : eglAttrWinLowColorES2);
	#else
	const EGLint *attribs = useMaxColorBits ? eglAttrWinRGB888 : eglAttrWinLowColor;
	#endif
	EGLint configs = 0;
	eglChooseConfig(display, attribs, &config, 1, &configs);
	if(!configs)
	{
		if(useMaxColorBits)
		{
			logMsg("falling back to lowest color config");
			#if defined CONFIG_GFX_OPENGL_ES
			eglChooseConfig(display, version == 1 ? eglAttrWinLowColor : eglAttrWinLowColorES2, &config, 1, &configs);
			#else
			eglChooseConfig(display, eglAttrWinLowColor, &config, 1, &configs);
			#endif
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
		#if defined NDEBUG || defined CONFIG_MACHINE_PANDORA
		auto attributes = version == 1 ? nullptr : eglAttrES2Ctx;
		#else
		auto attributes = version == 1 ? nullptr : eglAttrES2DebugCtx;
		#endif
	logMsg("making ES %d context", version);
	ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, attributes);
	#else
	ctx = createContextForMajorVersion(version, dpy, config);
	#endif
	if(ctx == EGL_NO_CONTEXT)
	{
		logErr("error creating context: 0x%X", (int)eglGetError());
		return INVALID_PARAMETER;
	}
	bool supportsSurfaceless = strstr(eglQueryString(display, EGL_EXTENSIONS), "EGL_KHR_surfaceless_context");
	if(!supportsSurfaceless)
	{
		logMsg("surfaceless context not supported");
		dummyPbuff = makeDummyPbuffer(display, config);
		assert(dummyPbuff != EGL_NO_SURFACE);
	}
	makeCurrentSurfaceless(display);
	return OK;
}

void GLContextHelper::makeCurrent(Display *dpy, XWindow const &win)
{
	assert(ctx != EGL_NO_CONTEXT);
	assert(win.surface != EGL_NO_SURFACE);
	eglMakeCurrent(display, win.surface, win.surface, ctx);
}

void makeCurrentSurfaceless(EGLDisplay display)
{
	assert(ctx != EGL_NO_CONTEXT);
	if(dummyPbuff != EGL_NO_SURFACE)
	{
		logMsg("setting dummy pbuffer surface current");
		if(eglMakeCurrent(display, dummyPbuff, dummyPbuff, context) == EGL_FALSE)
		{
			bug_exit("error setting dummy pbuffer current");
		}
	}
	else
	{
		logMsg("setting no surface current");
		if(eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context) == EGL_FALSE)
		{
			bug_exit("error setting no surface current");
		}
	}
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

void *glProcAddress(const char *funcName)
{
	return (void*)eglGetProcAddress(funcName);
}

}
