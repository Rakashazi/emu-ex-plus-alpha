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

#include <imagine/base/GLContext.hh>
#include <imagine/base/EGLContextBase.hh>
#include <imagine/util/egl.hh>
#include <EGL/eglext.h>

namespace Base
{

EGLDisplay EGLContextBase::display = EGL_NO_DISPLAY;
using EGLAttrList = StaticArrayList<int, 24>;

static EGLAttrList glConfigAttrsToEGLAttrs(const GLConfigAttributes &attr, bool supportsSurfaceless, bool failsafe)
{
	EGLAttrList list;

	if(!supportsSurfaceless)
	{
		list.push_back(EGL_SURFACE_TYPE);
		list.push_back(EGL_WINDOW_BIT|EGL_PBUFFER_BIT);
	}

	if(!failsafe)
	{
		// don't accept slow configs
		list.push_back(EGL_CONFIG_CAVEAT);
		list.push_back(EGL_NONE);
	}

	if(!failsafe && attr.preferredColorBits() > 16)
	{
		list.push_back(EGL_RED_SIZE);
		list.push_back(8);
		list.push_back(EGL_GREEN_SIZE);
		list.push_back(8);
		list.push_back(EGL_BLUE_SIZE);
		list.push_back(8);
	}
	else
	{
		logMsg("requesting lowest color config");
	}

	if(!attr.openGLESAPI())
	{
		list.push_back(EGL_RENDERABLE_TYPE);
		list.push_back(EGL_OPENGL_BIT);
		logMsg("using OpenGL renderable");
	}
	else if(attr.majorVersion() >= 2)
	{
		list.push_back(EGL_RENDERABLE_TYPE);
		list.push_back(EGL_OPENGL_ES2_BIT);
	}

	list.push_back(EGL_NONE);
	return list;
}

#ifndef CONFIG_GFX_OPENGL_ES
static EGLContext createContextForMajorVersion(uint version, EGLDisplay dpy, EGLConfig config)
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
				EGL_NONE
			};
			auto ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, attrCore3_2);
			if(ctx != EGL_NO_CONTEXT)
				return ctx;
			logErr("failed creating 3.2 core context");
		}
		{
			// Try 3.1
			const EGLint attr3_1[] =
			{
				EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
				EGL_CONTEXT_MINOR_VERSION_KHR, 1,
				EGL_NONE
			};
			auto ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, attr3_1);
			if(ctx != EGL_NO_CONTEXT)
				return ctx;
			logErr("failed creating 3.1 context");
		}
		{
			// Try 3.0
			const EGLint attr3_0[] =
			{
				EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
				EGL_CONTEXT_MINOR_VERSION_KHR, 0,
				EGL_NONE
			};
			auto ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, attr3_0);
			if(ctx != EGL_NO_CONTEXT)
				return ctx;
			logErr("failed creating 3.0 context");
		}
		// Fallback to 1.2
	}
	const EGLint attr[] =
	{
		EGL_CONTEXT_MAJOR_VERSION_KHR, 1,
		EGL_CONTEXT_MINOR_VERSION_KHR, 2,
		EGL_NONE
	};
	auto ctx = eglCreateContext(dpy, config, 0, attr);
	if(ctx != EGL_NO_CONTEXT)
		return ctx;
	logErr("failed creating 1.2 context");
	return EGL_NO_CONTEXT;
}
#endif

void *GLContext::procAddress(const char *funcName)
{
	return (void*)eglGetProcAddress(funcName);
}

EGLDisplay EGLContextBase::eglDisplay()
{
	if(display == EGL_NO_DISPLAY)
	{
		display = getDisplay();
		assert(display != EGL_NO_DISPLAY);
		if(!eglInitialize(display, nullptr, nullptr))
		{
			bug_exit("error initializing EGL");
			display = EGL_NO_DISPLAY;
		}
	}
	return display;
}

CallResult EGLContextBase::init(const GLConfigAttributes &attr)
{
	if(eglDisplay() == EGL_NO_DISPLAY)
	{
		logErr("unable to get EGL display");
		return INVALID_PARAMETER;
	}
	#ifndef NDEBUG
	logMsg("%s (%s), extensions: %s", eglQueryString(display, EGL_VENDOR), eglQueryString(display, EGL_VERSION), eglQueryString(display, EGL_EXTENSIONS));
	#endif
	//printEGLConfs(display);
	//printEGLConfsWithAttr(display, eglAttrWinMaxRGBA);
	//printEGLConfsWithAttr(display, eglAttrWinRGB888);
	//printEGLConfsWithAttr(display, eglAttrWinLowColor);

	#ifndef CONFIG_GFX_OPENGL_ES
	if(!eglBindAPI(EGL_OPENGL_API))
	{
		bug_exit("OpenGL not a supported EGL API");
	}
	#endif

	bool supportsSurfaceless = strstr(eglQueryString(display, EGL_EXTENSIONS), "EGL_KHR_surfaceless_context");
	EGLint configs = 0;
	{
		auto eglAttr = glConfigAttrsToEGLAttrs(attr, supportsSurfaceless, false);
		eglChooseConfig(display, &eglAttr[0], &config, 1, &configs);
	}
	if(!configs)
	{
		logErr("no EGL configs found, retrying with failsafe config");
		auto eglAttr = glConfigAttrsToEGLAttrs(attr, supportsSurfaceless, true);
		eglChooseConfig(display, &eglAttr[0], &config, 1, &configs);
		if(!configs)
		{
			logErr("no usable EGL configs found");
			return INVALID_PARAMETER;
		}
	}
	#ifndef NDEBUG
	printEGLConf(display, config);
	#endif

	// create context
	#ifdef CONFIG_GFX_OPENGL_ES
		#if defined NDEBUG || defined CONFIG_MACHINE_PANDORA || defined __ANDROID__
		auto attributes = attr.majorVersion() == 1 ? nullptr : eglAttrES2Ctx;
		#else
		auto attributes = attr.majorVersion() == 1 ? nullptr : eglAttrES2DebugCtx;
		#endif
	logMsg("making ES %d context", attr.majorVersion());
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, attributes);
	#else
	context = createContextForMajorVersion(attr.majorVersion(), display, config);
	#endif
	if(context == EGL_NO_CONTEXT)
	{
		logErr("error creating context: 0x%X", (int)eglGetError());
		return INVALID_PARAMETER;
	}
	if(!supportsSurfaceless)
	{
		logMsg("surfaceless context not supported");
		dummyPbuff = makeDummyPbuffer(display, config);
		assert(dummyPbuff != EGL_NO_SURFACE);
	}
	return OK;
}

void EGLContextBase::setCurrentContext(EGLContextBase *c, Window *win)
{
	assert(display != EGL_NO_DISPLAY);
	if(!c)
	{
		logMsg("making no context current");
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	}
	else if(win)
	{
		assert(c->context != EGL_NO_CONTEXT);
		logMsg("setting surface %ld current", (long)win->surface);
		eglMakeCurrent(display, win->surface, win->surface, c->context);
	}
	else
	{
		assert(c->context != EGL_NO_CONTEXT);
		if(c->dummyPbuff != EGL_NO_SURFACE)
		{
			logMsg("setting dummy pbuffer surface current");
			if(eglMakeCurrent(display, c->dummyPbuff, c->dummyPbuff, c->context) == EGL_FALSE)
			{
				bug_exit("error setting dummy pbuffer current");
			}
		}
		else
		{
			logMsg("setting no surface current");
			if(eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, c->context) == EGL_FALSE)
			{
				bug_exit("error setting no surface current");
			}
		}
	}
}

void EGLContextBase::setCurrentDrawable(Window *win)
{
	setCurrentContext(this, win);
}

void EGLContextBase::present(Window &win)
{
	assert(display != EGL_NO_DISPLAY);
	assert(win.surface != EGL_NO_SURFACE);
	if(eglSwapBuffers(display, win.surface) == EGL_FALSE)
	{
		bug_exit("error 0x%X swapping buffers for window: %p", eglGetError(), &win);
	}
}

bool EGLContextBase::isRealCurrentContext()
{
	return eglGetCurrentContext() == context;
}

GLContext::operator bool() const
{
	return context != EGL_NO_CONTEXT;
}

void EGLContextBase::deinit()
{
	if(context != EGL_NO_CONTEXT)
	{
		logMsg("destroying EGL context");
		eglDestroyContext(display, context);
		context = EGL_NO_CONTEXT;
	}
}

}
