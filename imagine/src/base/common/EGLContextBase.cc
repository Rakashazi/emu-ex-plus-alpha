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
#include <imagine/base/Window.hh>
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

#ifndef EGL_NO_CONFIG_KHR
#define EGL_NO_CONFIG_KHR ((EGLConfig)0)
#endif

namespace Base
{

// Android reference counts the EGL display internally,
// use refCount on other platforms to simulate this
static constexpr bool HAS_DISPLAY_REF_COUNT = Config::envIsAndroid;
static uint32_t refCount = 0;

static constexpr bool CAN_USE_DEBUG_CONTEXT = !Config::MACHINE_IS_PANDORA;
static uint8_t eglVersion = 0;
static bool supportsSurfaceless = false;
static bool supportsNoConfig = false;
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
		return {false, EGLConfig{}};
	}
	if(Config::DEBUG_BUILD)
		printEGLConf(display, config);
	return {true, config};
}

void *GLContext::procAddress(const char *funcName)
{
	//logDMsg("getting proc address for:%s", funcName);
	return (void*)eglGetProcAddress(funcName);
}

EGLContextBase::EGLContextBase(EGLDisplay display, GLContextAttributes attr, EGLBufferConfig config, EGLContext shareContext, std::error_code &ec)
{
	EGLConfig glConfig = supportsNoConfig ? EGL_NO_CONFIG_KHR : config.glConfig;
	logMsg("making context with version: %d.%d config:0x%llX share context:%p",
		attr.majorVersion(), attr.minorVersion(), (long long)glConfig, shareContext);
	context = eglCreateContext(display, glConfig, shareContext, &glContextAttrsToEGLAttrs(attr)[0]);
	if(context == EGL_NO_CONTEXT)
	{
		if(attr.debug())
		{
			logMsg("retrying without debug bit");
			attr.setDebug(false);
			context = eglCreateContext(display, glConfig, shareContext, &glContextAttrsToEGLAttrs(attr)[0]);
		}
		if(context == EGL_NO_CONTEXT)
		{
			if(Config::DEBUG_BUILD)
				logErr("error creating context: 0x%X", (int)eglGetError());
			ec = {EINVAL, std::system_category()};
			return;
		}
	}
	// Ignore surfaceless context support when using GL versions below 3.0 due to possible driver issues,
	// such as on Tegra 3 GPUs
	if(attr.majorVersion() <= 2 || !supportsSurfaceless)
	{
		if(!hasDummyPbuffConfig)
		{
			logMsg("surfaceless context not supported:%s, saving config for dummy pbuffer",
				supportsSurfaceless ? "context version below 3.0" : "missing extension");
			dummyPbuffConfig = config.glConfig;
			hasDummyPbuffConfig = true;
		}
		else
		{
			// all contexts must use same config if surfaceless isn't supported
			assert(dummyPbuffConfig == config.glConfig);
		}
	}
	//logDMsg("created context:%p", context);
	ec = {};
}

void EGLContextBase::setCurrentContext(EGLDisplay display, EGLContext context, GLDrawable win)
{
	assert(display != EGL_NO_DISPLAY);
	if(Config::DEBUG_BUILD)
	{
		//logDMsg("called setCurrentContext() with current context:%p surface:%p", eglGetCurrentContext(), eglGetCurrentSurface(EGL_DRAW));
	}
	if(context == EGL_NO_CONTEXT)
	{
		if(Config::DEBUG_BUILD)
		{
			logDMsg("setting no context current on thread:0x%lx", IG::thisThreadID<long>());
		}
		assert(!win);
		if(eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE)
		{
			if(Config::DEBUG_BUILD)
				logErr("error:0x%X setting no context current", eglGetError());
		}
	}
	else if(win)
	{
		assert(context != EGL_NO_CONTEXT);
		auto surface = win.eglSurface();
		if(Config::DEBUG_BUILD)
		{
			logDMsg("setting surface %p current on context:%p thread:0x%lx", context, surface, IG::thisThreadID<long>());
		}
		if(eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
		{
			if(Config::DEBUG_BUILD)
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
				logDMsg("setting dummy pbuffer surface current on context:%p thread:0x%lx", context, IG::thisThreadID<long>());
			}
			auto dummyPbuff = makeDummyPbuffer(display, dummyPbuffConfig);
			if(dummyPbuff == EGL_NO_SURFACE)
			{
				if(Config::DEBUG_BUILD)
					logErr("error:0x%X making dummy pbuffer", eglGetError());
			}
			if(eglMakeCurrent(display, dummyPbuff, dummyPbuff, context) == EGL_FALSE)
			{
				if(Config::DEBUG_BUILD)
					logErr("error:0x%X setting dummy pbuffer current", eglGetError());
			}
			eglDestroySurface(display, dummyPbuff);
		}
		else
		{
			if(Config::DEBUG_BUILD)
			{
				logDMsg("setting no surface current on context:%p thread:0x%lx", context, IG::thisThreadID<long>());
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
		if(Config::DEBUG_BUILD)
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
		logMsg("destroying context:%p", context);
		eglDestroyContext(display, context);
		context = EGL_NO_CONTEXT;
	}
}

bool GLContext::supportsNoConfig()
{
	return Base::supportsNoConfig;
}

NativeGLContext GLContext::nativeObject()
{
	return context;
}

// GLDisplay

std::pair<std::error_code, GLDisplay> GLDisplay::makeDefault()
{
	auto display = getDefault();
	auto ec = initDisplay(display.display);
	return {ec, display};
}

std::pair<std::error_code, GLDisplay> GLDisplay::makeDefault(GLDisplay::API api)
{
	if(!bindAPI(api))
	{
		logErr("error binding requested API");
		return {{EINVAL, std::system_category()}, {}};
	}
	return makeDefault();
}

GLDisplay GLDisplay::getDefault(API api)
{
	if(!bindAPI(api))
	{
		logErr("error binding requested API");
		return {};
	}
	return getDefault();
}

std::error_code EGLDisplayConnection::initDisplay(EGLDisplay display)
{
	logMsg("initializing EGL with display:%p", display);
	EGLint major, minor;
	if(!eglInitialize(display, &major, &minor))
	{
		logErr("error initializing EGL for display:%p", display);
		return {EINVAL, std::system_category()};
	}
	if(!eglVersion)
	{
		eglVersion = 10 * major + minor;
		auto extStr = eglQueryString(display, EGL_EXTENSIONS);
		supportsSurfaceless = eglVersion >= 15 || strstr(extStr, "EGL_KHR_surfaceless_context");
		supportsNoConfig = strstr(extStr, "EGL_KHR_no_config_context");
		if(supportsSurfaceless || supportsNoConfig)
			logMsg("context features: surfaceless:%u no-config:%u", supportsSurfaceless, supportsNoConfig);
	}
	if(!HAS_DISPLAY_REF_COUNT)
	{
		refCount++;
		logDMsg("referenced EGL display:%p, count:%u", display, refCount);
	}
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
	auto dpy = display;
	display = EGL_NO_DISPLAY;
	#if defined __ANDROID__
	if(Base::androidSDK() < 26)
	{
		// don't call eglTerminate() on older Android versions due to possible driver issues (random freeze in libMali.so)
		return true;
	}
	#endif
	if(!HAS_DISPLAY_REF_COUNT)
	{
		if(!refCount)
		{
			logErr("EGL display:%p already has no references", dpy);
			return false;
		}
		refCount--;
		if(refCount)
		{
			logDMsg("unreferenced EGL display:%p, count:%u", dpy, refCount);
			return true;
		}
	}
	logMsg("terminating EGL display:%p", dpy);
	return eglTerminate(dpy);
}

std::pair<std::error_code, GLDrawable> GLDisplay::makeDrawable(Window &win, GLBufferConfig config) const
{
	auto surface = eglCreateWindowSurface(display, config.glConfig,
		Config::MACHINE_IS_PANDORA ? (EGLNativeWindowType)0 : (EGLNativeWindowType)win.nativeObject(),
		nullptr);
	if(surface == EGL_NO_SURFACE)
	{
		return {{EINVAL, std::system_category()}, {}};
	}
	return {{}, surface};
}

bool GLDisplay::deleteDrawable(GLDrawable &drawable) const
{
	auto &surface = drawable.eglSurface();
	if(surface == EGL_NO_SURFACE)
		return true;
	auto success = eglDestroySurface(display, surface);
	surface = EGL_NO_SURFACE;
	return success;
}

void GLDisplay::logInfo() const
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
