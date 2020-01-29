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

#define LOGTAG "EGL"
#include <imagine/base/GLContext.hh>
#include <imagine/base/EGLContextBase.hh>
#include <imagine/thread/Thread.hh>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <imagine/util/egl.hh>
#include <imagine/util/container/ArrayList.hh>
#ifdef __ANDROID__
#include <imagine/base/android/android.hh>
#endif

#ifndef EGL_OPENGL_ES3_BIT
#define EGL_OPENGL_ES3_BIT 0x0040
#endif

namespace Base
{

// Android reference counts the EGL display internally,
// use refCount on other platforms to simulate this
static constexpr bool HAS_DISPLAY_REF_COUNT = Config::envIsAndroid;
static uint32_t refCount = 0;

static constexpr bool CAN_USE_DEBUG_CONTEXT = !Config::MACHINE_IS_PANDORA;
static bool hasDummyPbuffConfig = false;
static EGLConfig dummyPbuffConfig{};
using EGLAttrList = StaticArrayList<int, 24>;
using EGLContextAttrList = StaticArrayList<int, 16>;

static EGLAttrList glConfigAttrsToEGLAttrs(GLContextAttributes ctxAttr, GLBufferConfigAttributes attr)
{
	EGLAttrList list;
	// don't accept slow configs
	list.push_back(EGL_CONFIG_CAVEAT);
	list.push_back(EGL_NONE);
	switch(attr.pixelFormat().id())
	{
		bdefault:
			bug_unreachable("format id == %d", attr.pixelFormat().id());
		bcase PIXEL_NONE:
			// don't set any color bits
		bcase PIXEL_RGB565:
			list.push_back(EGL_BUFFER_SIZE);
			list.push_back(16);
		bcase PIXEL_RGB888:
			list.push_back(EGL_RED_SIZE);
			list.push_back(8);
			list.push_back(EGL_GREEN_SIZE);
			list.push_back(8);
			list.push_back(EGL_BLUE_SIZE);
			list.push_back(8);
		bcase PIXEL_RGBX8888:
			list.push_back(EGL_RED_SIZE);
			list.push_back(8);
			list.push_back(EGL_GREEN_SIZE);
			list.push_back(8);
			list.push_back(EGL_BLUE_SIZE);
			list.push_back(8);
			list.push_back(EGL_BUFFER_SIZE);
			list.push_back(32);
		bcase PIXEL_RGBA8888:
			list.push_back(EGL_RED_SIZE);
			list.push_back(8);
			list.push_back(EGL_GREEN_SIZE);
			list.push_back(8);
			list.push_back(EGL_BLUE_SIZE);
			list.push_back(8);
			list.push_back(EGL_ALPHA_SIZE);
			list.push_back(8);
	}
	if(!ctxAttr.openGLESAPI())
	{
		list.push_back(EGL_RENDERABLE_TYPE);
		list.push_back(EGL_OPENGL_BIT);
		//logDMsg("using OpenGL renderable");
	}
	else if(ctxAttr.majorVersion() == 2)
	{
		list.push_back(EGL_RENDERABLE_TYPE);
		list.push_back(EGL_OPENGL_ES2_BIT);
		//logDMsg("using OpenGL ES2 renderable");
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
		//logDMsg("using OpenGL ES client version:%d", attr.majorVersion());
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

	if(CAN_USE_DEBUG_CONTEXT && attr.debug())
	{
		list.push_back(EGL_CONTEXT_FLAGS_KHR);
		list.push_back(EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR);
	}

	list.push_back(EGL_NONE);
	return list;
}

// GLContext

std::pair<bool, EGLConfig> EGLContextBase::chooseConfig(EGLDisplay display, GLContextAttributes ctxAttr, GLBufferConfigAttributes attr)
{
	EGLConfig config;
	EGLint configs = 0;
	{
		auto eglAttr = glConfigAttrsToEGLAttrs(ctxAttr, attr);
		eglChooseConfig(display, &eglAttr[0], &config, 1, &configs);
	}
	if(!configs && attr.pixelFormat() != Window::defaultPixelFormat())
	{
		logErr("no EGL configs found, retrying with default window format");
		attr.setPixelFormat(Window::defaultPixelFormat());
		auto eglAttr = glConfigAttrsToEGLAttrs(ctxAttr, attr);
		eglChooseConfig(display, &eglAttr[0], &config, 1, &configs);
	}
	if(!configs)
	{
		logErr("no EGL configs found, retrying with no color bits set");
		attr.setPixelFormat(IG::PIXEL_NONE);
		auto eglAttr = glConfigAttrsToEGLAttrs(ctxAttr, attr);
		eglChooseConfig(display, &eglAttr[0], &config, 1, &configs);
	}
	if(!configs)
	{
		logErr("no usable EGL configs found with major version:%u", ctxAttr.majorVersion());
		return std::make_pair(false, EGLConfig{});
	}
	if(Config::DEBUG_BUILD)
		printEGLConf(display, config);
	return std::make_pair(true, config);
}

void *GLContext::procAddress(const char *funcName)
{
	//logDMsg("getting proc address for:%s", funcName);
	return (void*)eglGetProcAddress(funcName);
}

EGLContextBase::EGLContextBase(EGLDisplay display, GLContextAttributes attr, EGLBufferConfig config, EGLContext shareContext, std::error_code &ec)
{
	logMsg("making context with version: %d.%d share context:%p", attr.majorVersion(), attr.minorVersion(), shareContext);
	context = eglCreateContext(display, config.glConfig, shareContext, &glContextAttrsToEGLAttrs(attr)[0]);
	if(context == EGL_NO_CONTEXT)
	{
		if(attr.debug())
		{
			logMsg("retrying without debug bit");
			attr.setDebug(false);
			context = eglCreateContext(display, config.glConfig, shareContext, &glContextAttrsToEGLAttrs(attr)[0]);
		}
		if(context == EGL_NO_CONTEXT)
		{
			if(Config::DEBUG_BUILD)
				logErr("error creating context: 0x%X", (int)eglGetError());
			ec = {EINVAL, std::system_category()};
			return;
		}
	}
	// TODO: EGL 1.5 or higher supports surfaceless without any extension
	bool supportsSurfaceless = strstr(eglQueryString(display, EGL_EXTENSIONS), "EGL_KHR_surfaceless_context");
	if(!supportsSurfaceless)
	{
		if(!hasDummyPbuffConfig)
		{
			logMsg("surfaceless context not supported");
			dummyPbuffConfig = config.glConfig;
			hasDummyPbuffConfig = true;
		}
		else
		{
			// all contexts must use same config if surfaceless isn't supported
			assert(dummyPbuffConfig == config.glConfig);
		}
	}
	ec = {};
}

void EGLContextBase::setCurrentContext(EGLDisplay display, EGLContext context, GLDrawable win)
{
	assert(display != EGL_NO_DISPLAY);
	if(context == EGL_NO_CONTEXT)
	{
		if(Config::DEBUG_BUILD)
		{
			logDMsg("setting no context current on thread:0x%llx", (long long)IG::this_thread::get_id());
		}
		assert(!win);
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	}
	else if(win)
	{
		assert(context != EGL_NO_CONTEXT);
		auto surface = win.eglSurface();
		if(Config::DEBUG_BUILD)
		{
			logDMsg("setting surface 0x%lX current on thread:0x%llx", (long)surface, (long long)IG::this_thread::get_id());
		}
		if(eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
		{
			logErr("error:0x%X setting surface current", eglGetError());
		}
	}
	else
	{
		assert(context != EGL_NO_CONTEXT);
		if(hasDummyPbuffConfig)
		{
			if(Config::DEBUG_BUILD)
			{
				logDMsg("setting dummy pbuffer surface current on thread:0x%llx", (long long)IG::this_thread::get_id());
			}
			auto dummyPbuff = makeDummyPbuffer(display, dummyPbuffConfig);
			assert(dummyPbuff != EGL_NO_SURFACE);
			if(eglMakeCurrent(display, dummyPbuff, dummyPbuff, context) == EGL_FALSE)
			{
				logErr("error:0x%X setting dummy pbuffer current", eglGetError());
			}
			eglDestroySurface(display, dummyPbuff);
		}
		else
		{
			if(Config::DEBUG_BUILD)
			{
				logDMsg("setting no surface current on thread:0x%llx", (long long)IG::this_thread::get_id());
			}
			if(eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context) == EGL_FALSE)
			{
				if(Config::DEBUG_BUILD)
					logErr("error:0x%X setting no surface current", eglGetError());
			}
		}
	}
}

void GLContext::setDrawable(GLDisplay display, GLDrawable win)
{
	setDrawable(display, win, current(display));
}

void GLContext::setDrawable(GLDisplay display, GLDrawable win, GLContext cachedCurrentContext)
{
	setCurrentContext(display.eglDisplay(), cachedCurrentContext.context, win);
}

GLContext GLContext::current(GLDisplay display)
{
	GLContext c;
	c.context = eglGetCurrentContext();
	return c;
}

bool GLContext::isCurrentDrawable(GLDisplay display, GLDrawable drawable)
{
	return drawable.eglSurface() == eglGetCurrentSurface(EGL_DRAW);
}

void EGLContextBase::swapBuffers(EGLDisplay display, GLDrawable &win)
{
	assert(display != EGL_NO_DISPLAY);
	auto surface = win.eglSurface();
	assert(surface != EGL_NO_SURFACE);
	if(eglSwapBuffers(display, surface) == EGL_FALSE)
	{
		logErr("error 0x%X swapping buffers for window: %p", eglGetError(), &win);
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

void EGLContextBase::deinit(EGLDisplay display)
{
	if(context != EGL_NO_CONTEXT)
	{
		logMsg("destroying EGL context");
		eglDestroyContext(display, context);
		context = EGL_NO_CONTEXT;
	}
}

NativeGLContext GLContext::nativeObject()
{
	return context;
}

// GLDisplay

GLDisplay GLDisplay::makeDefault(std::error_code &ec)
{
	auto display = getDefault();
	ec = initDisplay(display.display);
	return display;
}

std::error_code EGLDisplayConnection::initDisplay(EGLDisplay display)
{
	if(!eglInitialize(display, nullptr, nullptr))
	{
		logErr("error initializing EGL");
		return {EINVAL, std::system_category()};
	}
	if(!HAS_DISPLAY_REF_COUNT)
	{
		refCount++;
		logDMsg("referenced EGL display, count:%u", refCount);
	}
	//logMsg("initialized EGL with display %ld", (long)display);
	return {};
}

const char *EGLDisplayConnection::queryExtensions()
{
	return eglQueryString(display, EGL_EXTENSIONS);
}

GLDisplay::operator bool() const
{
	return display != EGL_NO_DISPLAY;
}

bool GLDisplay::operator ==(GLDisplay const &rhs) const
{
	return display == rhs.display;
}

bool GLDisplay::deinit()
{
	if(display == EGL_NO_DISPLAY)
		return true;
	if(!HAS_DISPLAY_REF_COUNT)
	{
		if(!refCount)
		{
			logErr("EGL display already has no references");
			return false;
		}
		refCount--;
		if(refCount)
		{
			logDMsg("unreferenced EGL display, count:%u", refCount);
			return true;
		}
	}
	logMsg("destroying EGL display");
	auto success = eglTerminate(display);
	display = EGL_NO_DISPLAY;
	return success;
}

GLDrawable GLDisplay::makeDrawable(Window &win, GLBufferConfig config, std::error_code &ec)
{
	auto surface = eglCreateWindowSurface(display, config.glConfig,
		Config::MACHINE_IS_PANDORA ? (EGLNativeWindowType)0 : (EGLNativeWindowType)win.nativeObject(),
		nullptr);
	if(surface == EGL_NO_SURFACE)
	{
		ec = {EINVAL, std::system_category()};
		return {};
	}
	ec = {};
	return {surface};
}

bool GLDisplay::deleteDrawable(GLDrawable &drawable)
{
	auto &surface = drawable.eglSurface();
	if(surface == EGL_NO_SURFACE)
		return true;
	auto success = eglDestroySurface(display, surface);
	surface = EGL_NO_SURFACE;
	return success;
}

void GLDisplay::logInfo()
{
	if(!Config::DEBUG_BUILD)
		return;
	logMsg("version: %s (%s)", eglQueryString(display, EGL_VENDOR), eglQueryString(display, EGL_VERSION));
	logMsg("APIs: %s", eglQueryString(display, EGL_CLIENT_APIS));
	logMsg("extensions: %s", eglQueryString(display, EGL_EXTENSIONS));
	//printEGLConfs(display);
}

// GLDrawable

void GLDrawable::freeCaches() {}

void GLDrawable::restoreCaches() {}

GLDrawable::operator bool() const
{
	return surface != EGL_NO_SURFACE;
}

bool GLDrawable::operator ==(GLDrawable const &rhs) const
{
	return surface == rhs.surface;
}

}
