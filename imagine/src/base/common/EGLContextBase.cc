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

#ifndef EGL_OPENGL_ES3_BIT
#define EGL_OPENGL_ES3_BIT 0x0040
#endif

#ifndef EGL_NO_CONFIG_KHR
#define EGL_NO_CONFIG_KHR ((EGLConfig)0)
#endif

#ifndef EGL_CONTEXT_OPENGL_NO_ERROR_KHR
#define EGL_CONTEXT_OPENGL_NO_ERROR_KHR 0x31B3
#endif

#ifndef EGL_TRIPLE_BUFFER_NV
#define EGL_TRIPLE_BUFFER_NV 0x3230
#endif

namespace Base
{

static constexpr bool HAS_DEBUG_CONTEXT = !Config::MACHINE_IS_PANDORA;
static constexpr bool HAS_NATIVE_WINDOW_TYPE = !Config::MACHINE_IS_PANDORA;
static constexpr bool HAS_RGB888_FORMAT = false; // no devices need this currently
using EGLSurfaceAttrList = StaticArrayList<int, 8>;
using EGLAttrList = StaticArrayList<int, 24>;
using EGLContextAttrList = StaticArrayList<int, 16>;

static EGLAttrList glConfigAttrsToEGLAttrs(int renderableType, GLBufferConfigAttributes attr)
{
	EGLAttrList list{};
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
			if(!HAS_RGB888_FORMAT)
				break;
			list.push_back(EGL_RED_SIZE);
			list.push_back(8);
			list.push_back(EGL_GREEN_SIZE);
			list.push_back(8);
			list.push_back(EGL_BLUE_SIZE);
			list.push_back(8);
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
	if(renderableType)
	{
		list.push_back(EGL_RENDERABLE_TYPE);
		list.push_back(renderableType);
	}
	list.push_back(EGL_NONE);
	return list;
}

static EGLContextAttrList glContextAttrsToEGLAttrs(GLContextAttributes attr)
{
	EGLContextAttrList list{};

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

	if(HAS_DEBUG_CONTEXT && attr.debug())
	{
		list.push_back(EGL_CONTEXT_FLAGS_KHR);
		list.push_back(EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR);
	}

	if(!attr.debug() && attr.noError())
	{
		list.push_back(EGL_CONTEXT_OPENGL_NO_ERROR_KHR);
		list.push_back(EGL_TRUE);
	}

	list.push_back(EGL_NONE);
	return list;
}

// GLDisplay

const char *EGLDisplayConnection::queryExtensions()
{
	return eglQueryString(display, EGL_EXTENSIONS);
}

void GLDisplay::resetCurrentContext() const
{
	if(Config::DEBUG_BUILD)
	{
		logDMsg("setting no context current on thread:0x%lx", IG::thisThreadID<long>());
	}
	if(eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE)
	{
		if(Config::DEBUG_BUILD)
			logErr("error:%s setting no context current", GLManager::errorString(eglGetError()));
	}
}

// GLDrawable

static EGLSurface createWindowSurface(EGLDisplay display, EGLConfig config, NativeWindow nativeWin, const EGLint *surfaceAttr)
{
	if(!HAS_NATIVE_WINDOW_TYPE)
		nativeWin = {};
	return eglCreateWindowSurface(display, config, (EGLNativeWindowType)nativeWin, surfaceAttr);
}

EGLDrawable::EGLDrawable(EGLDisplay display, Window &win, EGLConfig config, const EGLint *surfaceAttr, IG::ErrorCode &ec):
	surface
	{
		createWindowSurface(display, config, win.nativeObject(), surfaceAttr),
		{display}
	}
{
	if(!surface)
	{
		ec = {EINVAL};
	}
}

void EGLDrawable::destroySurface(EGLDisplay dpy, EGLSurface surface)
{
	if(!surface)
		return;
	logMsg("destroying surface:%p", surface);
	auto success = eglDestroySurface(dpy, surface);
	if(Config::DEBUG_BUILD && !success)
	{
		logErr("error:%s in eglDestroySurface(%p, %p)",
			GLManager::errorString(eglGetError()), (EGLDisplay)dpy, surface);
	}
}

GLDisplay GLDrawable::display() const
{
	return {surface.get_deleter().dpy};
}

// GLContext

EGLContextBase::EGLContextBase(EGLDisplay display, GLContextAttributes attr, EGLConfig config,
	EGLContext shareContext, bool savePBuffConfig, IG::ErrorCode &ec):
	context{eglCreateContext(display, config, shareContext, &glContextAttrsToEGLAttrs(attr)[0]), {display}}
{
	if(!context)
	{
		if(attr.debug())
		{
			logMsg("retrying without debug bit");
			attr.setDebug(false);
			context.reset(eglCreateContext(display, config, shareContext, &glContextAttrsToEGLAttrs(attr)[0]));
		}
		if(!context)
		{
			if(Config::DEBUG_BUILD)
				logErr("error creating context: 0x%X", (int)eglGetError());
			ec = {EINVAL};
			return;
		}
	}
	if(savePBuffConfig)
	{
		dummyPbuffConfig.emplace(config);
	}
	//logDMsg("created context:%p", context);
}

void EGLContextBase::destroyContext(EGLDisplay dpy, EGLContext context)
{
	if(!context)
		return;
	logMsg("destroying context:%p", context);
	auto success = eglDestroyContext(dpy, context);
	if(Config::DEBUG_BUILD && !success)
	{
		logErr("error:%s in eglDestroyContext(%p, %p)",
			GLManager::errorString(eglGetError()), dpy, context);
	}
}

GLDisplay GLContext::display() const
{
	return {context.get_deleter().dpy};
}

void GLContext::setCurrentContext(NativeGLDrawable surface) const
{
	assert((bool)context);
	if(!surface && dummyPbuffConfig)
	{
		if(Config::DEBUG_BUILD)
		{
			logDMsg("setting dummy pbuffer current on context:%p thread:0x%lx", context.get(), IG::thisThreadID<long>());
		}
		auto dpy = display();
		auto dummyPbuff = makeDummyPbuffer(dpy, *dummyPbuffConfig);
		if(!dummyPbuff)
		{
			if(Config::DEBUG_BUILD)
				logErr("error:%s making dummy pbuffer", GLManager::errorString(eglGetError()));
		}
		if(eglMakeCurrent(dpy, dummyPbuff, dummyPbuff, context.get()) == EGL_FALSE)
		{
			if(Config::DEBUG_BUILD)
				logErr("error:%s in eglMakeCurrent()", GLManager::errorString(eglGetError()));
		}
		eglDestroySurface(dpy, dummyPbuff);
		return;
	}
	if(Config::DEBUG_BUILD)
	{
		logDMsg("setting surface:%p current on context:%p thread:0x%lx", surface, context.get(), IG::thisThreadID<long>());
	}
	if(eglMakeCurrent(display(), surface, surface, context.get()) == EGL_FALSE)
	{
		if(Config::DEBUG_BUILD)
			logErr("error:%s in eglMakeCurrent()", GLManager::errorString(eglGetError()));
	}
}

void GLContext::setCurrentDrawable(NativeGLDrawable drawable) const
{
	setCurrentContext(drawable);
}

void GLContext::present(NativeGLDrawable drawable) const
{
	assert(drawable);
	if(eglSwapBuffers(display(), drawable) == EGL_FALSE)
	{
		if(Config::DEBUG_BUILD)
			logErr("error 0x%X swapping buffers for surface:%p", eglGetError(), drawable);
	}
}

// GLManager

GLManager::GLManager(Base::NativeDisplayConnection ctx, GL::API api)
{
	if(!bindAPI(api))
	{
		logErr("error binding requested API");
		return;
	}
	auto display = getDefaultDisplay(ctx);
	auto ec = initDisplay(display);
	if(ec)
		return;
	dpy.reset((EGLDisplay)display);
}

std::optional<EGLConfig> EGLManager::chooseConfig(GLDisplay display, int renderableType, GLBufferConfigAttributes attr, bool allowFallback)
{
	EGLConfig config;
	EGLint configs = 0;
	{
		auto eglAttr = glConfigAttrsToEGLAttrs(renderableType, attr);
		eglChooseConfig(display, &eglAttr[0], &config, 1, &configs);
	}
	if(allowFallback && !configs)
	{
		logErr("no EGL configs found, retrying with no color bits set");
		attr.setPixelFormat(IG::PIXEL_NONE);
		auto eglAttr = glConfigAttrsToEGLAttrs(renderableType, attr);
		eglChooseConfig(display, &eglAttr[0], &config, 1, &configs);
	}
	if(!configs)
	{
		logErr("no usable EGL configs found with renderable type:%s", eglRenderableTypeToStr(renderableType));
		return {};
	}
	if(Config::DEBUG_BUILD)
		printEGLConf(display, config);
	return config;
}

void *GLManager::procAddress(const char *funcName)
{
	//logDMsg("getting proc address for:%s", funcName);
	return (void*)eglGetProcAddress(funcName);
}

NativeGLContext GLManager::currentContext()
{
	return eglGetCurrentContext();
}

bool GLManager::hasCurrentDrawable(NativeGLDrawable drawable)
{
	return eglGetCurrentSurface(EGL_DRAW) == drawable;
}

bool GLManager::hasCurrentDrawable()
{
	return eglGetCurrentSurface(EGL_DRAW) != EGL_NO_SURFACE;
}

GLDisplay GLManager::display() const
{
	return {dpy.get()};
}

void EGLManager::logFeatures() const
{
	if(!Config::DEBUG_BUILD)
		return;
	std::string featuresStr{};

	if(supportsSurfaceless)
	{
		featuresStr.append(" [Surfaceless]");
	}
	if(supportsNoConfig)
	{
		featuresStr.append(" [No Config]");
	}
	if(supportsNoError)
	{
		featuresStr.append(" [No Error Mode]");
	}
	if(supportsTripleBufferSurfaces)
	{
		featuresStr.append(" [Triple Buffer Surfaces]");
	}
	if(supportsSrgbColorSpace)
	{
		featuresStr.append(" [sRGB Color Space]");
	}

	if(featuresStr.empty())
		return;
	logMsg("features:%s", featuresStr.c_str());
}

IG::ErrorCode EGLManager::initDisplay(EGLDisplay display)
{
	logMsg("initializing EGL with display:%p", display);
	EGLint major, minor;
	if(!eglInitialize(display, &major, &minor))
	{
		logErr("error initializing EGL for display:%p", display);
		return {EINVAL};
	}
	int eglVersion = 10 * major + minor;
	auto extStr = eglQueryString(display, EGL_EXTENSIONS);
	supportsSurfaceless = eglVersion >= 15 || strstr(extStr, "EGL_KHR_surfaceless_context");
	supportsNoConfig = strstr(extStr, "EGL_KHR_no_config_context");
	supportsNoError = strstr(extStr, "EGL_KHR_create_context_no_error");
	supportsSrgbColorSpace = eglVersion >= 15 || strstr(extStr, "EGL_KHR_gl_colorspace");
	if constexpr(Config::envIsLinux)
	{
		supportsTripleBufferSurfaces = strstr(extStr, "EGL_NV_triple_buffer");
	}
	logFeatures();
	return {};
}

GLContext GLManager::makeContext(GLContextAttributes attr, GLBufferConfig config, NativeGLContext shareContext, IG::ErrorCode &ec)
{
	if(hasNoConfigContext())
		config = EGL_NO_CONFIG_KHR;
	if(!hasNoErrorContextAttribute())
		attr.setNoError(false);
	logMsg("making context with version: %d.%d config:0x%llX share context:%p",
		attr.majorVersion(), attr.minorVersion(), (long long)(EGLConfig)config, shareContext);
	// Ignore surfaceless context support when using GL versions below 3.0 due to possible driver issues,
	// such as on Tegra 3 GPUs
	bool savePBuffConfig = attr.majorVersion() <= 2 || !supportsSurfaceless;
	GLContext ctx{display(), attr, config, shareContext, savePBuffConfig, ec};
	if(!ctx)
		return {};
	if(savePBuffConfig)
	{
		logMsg("surfaceless context not supported:%s, saving config for dummy pbuffer",
			supportsSurfaceless ? "context version below 3.0" : "missing extension");
	}
	return ctx;
}

const char *EGLManager::errorString(EGLint error)
{
	switch(error)
	{
		case EGL_SUCCESS: return "Success";
		case EGL_NOT_INITIALIZED: return "EGL not initialized";
		case EGL_BAD_ACCESS: return "Bad access";
		case EGL_BAD_ALLOC: return "Bad allocation";
		case EGL_BAD_ATTRIBUTE: return "Bad attribute";
		case EGL_BAD_CONTEXT: return "Bad context";
		case EGL_BAD_CONFIG: return "Bad frame buffer config";
		case EGL_BAD_CURRENT_SURFACE: return "Bad current surface";
		case EGL_BAD_DISPLAY: return "Bad display";
		case EGL_BAD_SURFACE: return "Bad surface";
		case EGL_BAD_MATCH: return "Inconsistent arguments";
		case EGL_BAD_PARAMETER: return "Bad parameter";
		case EGL_BAD_NATIVE_PIXMAP: return "Bad native pixmap";
		case EGL_BAD_NATIVE_WINDOW: return "Bad native window";
		case EGL_CONTEXT_LOST: return "Context lost";
	}
	return "Unknown error";
}

int EGLManager::makeRenderableType(GL::API api, unsigned majorVersion)
{
	if(api == GL::API::OPENGL)
	{
		return EGL_OPENGL_BIT;
	}
	else
	{
		switch(majorVersion)
		{
			default: return 0;
			case 2: return EGL_OPENGL_ES2_BIT;
			case 3: return EGL_OPENGL_ES3_BIT;
		}
	}
}

static bool supportsColorSpace(GLDisplay dpy, GLBufferConfig conf, GLColorSpace colorSpace)
{
	switch(colorSpace)
	{
		case GLColorSpace::LINEAR: return true;
		case GLColorSpace::SRGB:
		{
				EGLint redSize;
				eglGetConfigAttrib(dpy, conf, EGL_RED_SIZE, &redSize);
				return redSize >= 8;
		}
	}
	return false;
}

GLDrawable GLManager::makeDrawable(Window &win, GLDrawableAttributes attr, IG::ErrorCode &ec) const
{
	auto dpy = display();
	EGLSurfaceAttrList attrList{};
	if(Config::envIsLinux && supportsTripleBufferSurfaces)
	{
		// request triple-buffering on Nvidia GPUs
		attrList.push_back(EGL_RENDER_BUFFER);
		attrList.push_back(EGL_TRIPLE_BUFFER_NV);
	}
	bool useSrgbColorSpace = attr.colorSpace() == GLColorSpace::SRGB && hasSrgbColorSpace() &&
		supportsColorSpace(dpy, attr.bufferConfig(), GLColorSpace::SRGB);
	if(useSrgbColorSpace)
	{
		attrList.push_back(EGL_GL_COLORSPACE);
		attrList.push_back(EGL_GL_COLORSPACE_SRGB);
	}
	attrList.push_back(EGL_NONE);
	GLDrawable drawable{dpy, win, attr.bufferConfig(), attrList.data(), ec};
	if(ec)
	{
		return {};
	}
	logMsg("made surface:%p %s", (EGLSurface)drawable,
		useSrgbColorSpace ? "(SRGB color space)" : "");
	return drawable;
}

bool GLManager::hasDrawableConfig(GLBufferConfigAttributes attrs, GLColorSpace colorSpace) const
{
	if(!hasBufferConfig(attrs))
		return false;
	if(colorSpace == GLColorSpace::LINEAR)
		return true;
	// sRGB Color Space
	return hasSrgbColorSpace() && attrs.pixelFormat() != IG::PIXEL_RGB565;
}

bool GLManager::hasNoErrorContextAttribute() const
{
	return supportsNoError;
}

bool GLManager::hasNoConfigContext() const
{
	return supportsNoConfig;
}

bool GLManager::hasSrgbColorSpace() const
{
	return supportsSrgbColorSpace;
}

void GLManager::logInfo() const
{
	if(!Config::DEBUG_BUILD)
		return;
	logMsg("version: %s (%s)", eglQueryString(display(), EGL_VENDOR), eglQueryString(display(), EGL_VERSION));
	logMsg("APIs: %s", eglQueryString(display(), EGL_CLIENT_APIS));
	logMsg("extensions: %s", eglQueryString(display(), EGL_EXTENSIONS));
	//printEGLConfs(display);
}

void EGLManager::terminateEGL(EGLDisplay display)
{
	if(display == EGL_NO_DISPLAY)
		return;
	logMsg("terminating EGL display:%p", display);
	eglTerminate(display);
}

// EGLBufferConfig

EGLint EGLBufferConfig::renderableTypeBits(GLDisplay display) const
{
	EGLint bits;
	eglGetConfigAttrib(display, glConfig, EGL_RENDERABLE_TYPE, &bits);
	return bits;
}

bool EGLBufferConfig::maySupportGLES(GLDisplay display, unsigned majorVersion) const
{
	switch(majorVersion)
	{
		default: return false;
		case 1: return true;
		case 2: return renderableTypeBits(display) & EGL_OPENGL_ES2_BIT;
		case 3: return renderableTypeBits(display) & EGL_OPENGL_ES3_BIT;
	}
}

}
