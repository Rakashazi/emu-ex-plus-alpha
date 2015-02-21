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
#include <EGL/eglext.h>
#include <imagine/util/egl.hh>

#ifndef EGL_OPENGL_ES3_BIT
#define EGL_OPENGL_ES3_BIT 0x0040
#endif

namespace Base
{

static bool hasDummyPbuffConfig = false;
static EGLConfig dummyPbuffConfig{};
static EGLDisplay display = EGL_NO_DISPLAY;
using EGLAttrList = StaticArrayList<int, 24>;
using EGLContextAttrList = StaticArrayList<int, 16>;

static EGLAttrList glConfigAttrsToEGLAttrs(GLContextAttributes ctxAttr, GLBufferConfigAttributes attr, bool failsafe)
{
	EGLAttrList list;

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

	if(!ctxAttr.openGLESAPI())
	{
		list.push_back(EGL_RENDERABLE_TYPE);
		list.push_back(EGL_OPENGL_BIT);
		logMsg("using OpenGL renderable");
	}
	else if(ctxAttr.majorVersion() == 2)
	{
		list.push_back(EGL_RENDERABLE_TYPE);
		list.push_back(EGL_OPENGL_ES2_BIT);
	}
	else if(ctxAttr.majorVersion() == 3)
	{
		list.push_back(EGL_RENDERABLE_TYPE);
		list.push_back(EGL_OPENGL_ES3_BIT);
	}

	list.push_back(EGL_NONE);
	return list;
}

static EGLContextAttrList glContextAttrsToEGLAttrs(GLContextAttributes attr)
{
	EGLContextAttrList list;

	if(attr.openGLESAPI())
	{
		list.push_back(EGL_CONTEXT_CLIENT_VERSION);
		list.push_back(attr.majorVersion());
	}
	else
	{
		list.push_back(EGL_CONTEXT_MAJOR_VERSION_KHR);
		list.push_back(attr.majorVersion());
		list.push_back(EGL_CONTEXT_MINOR_VERSION_KHR);
		list.push_back(attr.minorVersion());

		if(attr.majorVersion() > 3
			|| (attr.majorVersion() == 3 && attr.minorVersion() >= 2))
		{
			list.push_back(EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR);
			list.push_back(EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR);
		}
	}

	if(attr.debug())
	{
		list.push_back(EGL_CONTEXT_FLAGS_KHR);
		list.push_back(EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR);
	}

	list.push_back(EGL_NONE);
	return list;
}

std::pair<CallResult, EGLConfig> EGLContextBase::chooseConfig(GLContextAttributes ctxAttr, GLBufferConfigAttributes attr)
{
	if(eglDisplay() == EGL_NO_DISPLAY)
	{
		logErr("unable to get EGL display");
		return std::make_pair(INVALID_PARAMETER, EGLConfig{});
	}
	EGLConfig config;
	EGLint configs = 0;
	{
		auto eglAttr = glConfigAttrsToEGLAttrs(ctxAttr, attr, false);
		eglChooseConfig(display, &eglAttr[0], &config, 1, &configs);
	}
	if(!configs)
	{
		logErr("no EGL configs found, retrying with failsafe config");
		auto eglAttr = glConfigAttrsToEGLAttrs(ctxAttr, attr, true);
		eglChooseConfig(display, &eglAttr[0], &config, 1, &configs);
		if(!configs)
		{
			logErr("no usable EGL configs found");
			return std::make_pair(INVALID_PARAMETER, EGLConfig{});
		}
	}
	if(Config::DEBUG_BUILD)
		printEGLConf(display, config);
	return std::make_pair(OK, config);
}

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
			return display;
		}
		//logMsg("initialized EGL with display %ld", (long)display);
		if(Config::DEBUG_BUILD)
		{
			logMsg("%s (%s), extensions: %s", eglQueryString(display, EGL_VENDOR), eglQueryString(display, EGL_VERSION), eglQueryString(display, EGL_EXTENSIONS));
		}
	}
	return display;
}

CallResult EGLContextBase::init(GLContextAttributes attr, GLBufferConfig config)
{
	if(eglDisplay() == EGL_NO_DISPLAY)
	{
		logErr("unable to get EGL display");
		return INVALID_PARAMETER;
	}
	logMsg("making context with version: %d.%d", attr.majorVersion(), attr.minorVersion());
	context = eglCreateContext(display, config.glConfig, EGL_NO_CONTEXT, &glContextAttrsToEGLAttrs(attr)[0]);
	if(context == EGL_NO_CONTEXT)
	{
		if(attr.debug())
		{
			logMsg("retrying without debug bit");
			attr.setDebug(false);
			context = eglCreateContext(display, config.glConfig, EGL_NO_CONTEXT, &glContextAttrsToEGLAttrs(attr)[0]);
		}
		if(context == EGL_NO_CONTEXT)
		{
			logErr("error creating context: 0x%X", (int)eglGetError());
			return INVALID_PARAMETER;
		}
	}
	// TODO: EGL 1.5 or higher supports surfaceless without any extension
	bool supportsSurfaceless = strstr(eglQueryString(display, EGL_EXTENSIONS), "EGL_KHR_surfaceless_context");
	if(!supportsSurfaceless)
	{
		logMsg("surfaceless context not supported");
		if(!hasDummyPbuffConfig)
		{
			dummyPbuffConfig = config.glConfig;
			hasDummyPbuffConfig = true;
		}
		else
		{
			// all contexts must use same config if surfaceless isn't supported
			assert(dummyPbuffConfig == config.glConfig);
		}
	}
	return OK;
}

void EGLContextBase::setCurrentContext(EGLContext context, Window *win)
{
	assert(display != EGL_NO_DISPLAY);
	if(context == EGL_NO_CONTEXT)
	{
		logMsg("making no context current");
		assert(!win);
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	}
	else if(win)
	{
		assert(context != EGL_NO_CONTEXT);
		logMsg("setting surface %ld current", (long)win->surface);
		if(eglMakeCurrent(display, win->surface, win->surface, context) == EGL_FALSE)
		{
			bug_exit("error setting surface current");
		}
	}
	else
	{
		assert(context != EGL_NO_CONTEXT);
		if(hasDummyPbuffConfig)
		{
			logMsg("setting dummy pbuffer surface current");
			auto dummyPbuff = makeDummyPbuffer(display, dummyPbuffConfig);
			assert(dummyPbuff != EGL_NO_SURFACE);
			if(eglMakeCurrent(display, dummyPbuff, dummyPbuff, context) == EGL_FALSE)
			{
				bug_exit("error setting dummy pbuffer current");
			}
			eglDestroySurface(display, dummyPbuff);
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
}

void GLContext::setDrawable(Window *win)
{
	setDrawable(win, current());
}

void GLContext::setDrawable(Window *win, GLContext cachedCurrentContext)
{
	setCurrentContext(cachedCurrentContext.context, win);
}

GLContext GLContext::current()
{
	GLContext c;
	c.context = eglGetCurrentContext();
	return c;
}

void EGLContextBase::swapBuffers(Window &win)
{
	assert(display != EGL_NO_DISPLAY);
	assert(win.surface != EGL_NO_SURFACE);
	if(eglSwapBuffers(display, win.surface) == EGL_FALSE)
	{
		bug_exit("error 0x%X swapping buffers for window: %p", eglGetError(), &win);
	}
}

GLContext::operator bool() const
{
	return context != EGL_NO_CONTEXT;
}

bool GLContext::operator ==(GLContext const &rhs) const
{
	return context == rhs.context;
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
