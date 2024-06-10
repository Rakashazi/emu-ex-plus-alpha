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
#include <imagine/base/Window.hh>
#include <imagine/thread/Thread.hh>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <imagine/util/egl.hh>
#include <imagine/util/container/ArrayList.hh>
#include <imagine/util/string.h>

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

namespace IG
{

constexpr SystemLogger log{"EGL"};
constexpr bool HAS_DEBUG_CONTEXT = !Config::MACHINE_IS_PANDORA;
constexpr bool HAS_NATIVE_WINDOW_TYPE = !Config::MACHINE_IS_PANDORA;
using EGLSurfaceAttrList = StaticArrayList<int, 8>;
using EGLAttrList = StaticArrayList<int, 24>;
using EGLContextAttrList = StaticArrayList<int, 16>;

static EGLAttrList glConfigAttrsToEGLAttrs(int renderableType, GLBufferConfigAttributes attr)
{
	EGLAttrList list;
	// don't accept slow configs
	list.push_back(EGL_CONFIG_CAVEAT);
	list.push_back(EGL_NONE);
	switch(attr.pixelFormat)
	{
		case PixelFmtUnset:
			break; // don't set any color bits
		case PixelFmtRGB565:
			list.push_back(EGL_BUFFER_SIZE);
			list.push_back(16);
			break;
		case PixelFmtRGBA8888:
			if(attr.useAlpha)
			{
				list.push_back(EGL_ALPHA_SIZE);
				list.push_back(8);
				list.push_back(EGL_BUFFER_SIZE);
				list.push_back(32);
			}
			else
			{
				list.push_back(EGL_BUFFER_SIZE);
				list.push_back(24);
			}
			break;
		default: std::unreachable();
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
	EGLContextAttrList list;

	if(attr.api == GL::API::OpenGLES)
	{
		list.push_back(EGL_CONTEXT_CLIENT_VERSION);
		list.push_back(attr.version.major);
		//log.debug("using OpenGL ES client version:{}", attr.majorVersion());
	}
	else
	{
		list.push_back(EGL_CONTEXT_MAJOR_VERSION_KHR);
		list.push_back(attr.version.major);
		list.push_back(EGL_CONTEXT_MINOR_VERSION_KHR);
		list.push_back(attr.version.minor);

		if(attr.version.major > 3
			|| (attr.version.major == 3 && attr.version.minor >= 2))
		{
			list.push_back(EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR);
			list.push_back(EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR);
		}
	}

	if(HAS_DEBUG_CONTEXT && attr.debug)
	{
		list.push_back(EGL_CONTEXT_FLAGS_KHR);
		list.push_back(EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR);
	}

	if(!attr.debug && attr.noError)
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
		log.debug("setting no context current on thread:{}", IG::thisThreadId());
	}
	if(eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE)
	{
		if(Config::DEBUG_BUILD)
			log.error("error:{} setting no context current", GLManager::errorString(eglGetError()));
	}
}

// GLDrawable

static EGLSurface createWindowSurface(EGLDisplay display, EGLConfig config, NativeWindow nativeWin, const EGLint *surfaceAttr)
{
	if(!HAS_NATIVE_WINDOW_TYPE)
		nativeWin = {};
	return eglCreateWindowSurface(display, config, (EGLNativeWindowType)nativeWin, surfaceAttr);
}

EGLDrawable::EGLDrawable(EGLDisplay display, Window &win, EGLConfig config, const EGLint *surfaceAttr):
	surface
	{
		createWindowSurface(display, config, win.nativeObject(), surfaceAttr),
		{display}
	}
{
	if(!surface)
	{
		if(Config::DEBUG_BUILD)
			log.error("eglCreateWindowSurface returned:{}", GLManager::errorString(eglGetError()));
		throw std::runtime_error("Error creating window surface");
	}
}

void EGLDrawable::destroySurface(EGLDisplay dpy, EGLSurface surface)
{
	if(!surface)
		return;
	log.info("destroying surface:{}", surface);
	auto success = eglDestroySurface(dpy, surface);
	if(Config::DEBUG_BUILD && !success)
	{
		log.error("error:{} in eglDestroySurface({}, {})",
			GLManager::errorString(eglGetError()), (EGLDisplay)dpy, surface);
	}
}

GLDisplay GLDrawable::display() const
{
	return {surface.get_deleter().dpy};
}

// GLContext

EGLContextBase::EGLContextBase(EGLDisplay display, GLContextAttributes attr, EGLConfig config,
	EGLContext shareContext, bool savePBuffConfig):
	context{eglCreateContext(display, config, shareContext, &glContextAttrsToEGLAttrs(attr)[0]), {display}}
{
	if(!context)
	{
		if(attr.debug)
		{
			log.info("retrying without debug bit");
			attr.debug = false;
			context.reset(eglCreateContext(display, config, shareContext, &glContextAttrsToEGLAttrs(attr)[0]));
		}
		if(!context)
		{
			if(Config::DEBUG_BUILD)
				log.error("error creating context: {:X}", (int)eglGetError());
			throw std::runtime_error("Error creating GL context");
		}
	}
	if(savePBuffConfig)
	{
		dummyPbuffConfig.emplace(config);
	}
	//log.debug("created context:{}", context);
}

void EGLContextBase::destroyContext(EGLDisplay dpy, EGLContext context)
{
	if(!context)
		return;
	log.info("destroying context:{}", context);
	auto success = eglDestroyContext(dpy, context);
	if(Config::DEBUG_BUILD && !success)
	{
		log.error("error:{} in eglDestroyContext({}, {})",
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
			log.debug("setting dummy pbuffer current on context:{} thread:{}", context.get(), IG::thisThreadId());
		}
		auto dpy = display();
		auto dummyPbuff = makeDummyPbuffer(dpy, *dummyPbuffConfig);
		if(!dummyPbuff)
		{
			if(Config::DEBUG_BUILD)
				log.error("error:{} making dummy pbuffer", GLManager::errorString(eglGetError()));
		}
		if(eglMakeCurrent(dpy, dummyPbuff, dummyPbuff, context.get()) == EGL_FALSE)
		{
			if(Config::DEBUG_BUILD)
				log.error("error:{} in eglMakeCurrent()", GLManager::errorString(eglGetError()));
		}
		eglDestroySurface(dpy, dummyPbuff);
		return;
	}
	if(Config::DEBUG_BUILD)
	{
		log.debug("setting surface:{} current on context:{} thread:{}", surface, context.get(), IG::thisThreadId());
	}
	if(eglMakeCurrent(display(), surface, surface, context.get()) == EGL_FALSE)
	{
		if(Config::DEBUG_BUILD)
			log.error("error:{} in eglMakeCurrent()", GLManager::errorString(eglGetError()));
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
			log.error("error {:X} swapping buffers for surface:{}", eglGetError(), drawable);
	}
}

void GLContext::setSwapInterval(int i)
{
	bool success = eglSwapInterval(display(), i);
	if(Config::DEBUG_BUILD && !success)
	{
		log.error("error:{} in eglSwapInterval({})", EGLManager::errorString(eglGetError()), i);
	}
}

// GLManager

GLManager::GLManager(NativeDisplayConnection ctx, GL::API api)
{
	if(!bindAPI(api))
		throw std::runtime_error("Error binding requested EGL API");
	auto display = getDefaultDisplay(ctx);
	if(!initDisplay(display))
		throw std::runtime_error("Error initializing EGL");
	dpy.reset((EGLDisplay)display);
}

std::optional<EGLConfig> EGLManager::chooseConfig(GLDisplay display, int renderableType, GLBufferConfigAttributes attr, bool allowFallback)
{
	EGLConfig config;
	int configCount = chooseConfigs(display, renderableType, attr, std::span{&config, 1});
	if(allowFallback && !configCount)
	{
		log.error("no EGL configs found, retrying with no color bits set");
		attr.pixelFormat = {};
		configCount = chooseConfigs(display, renderableType, attr, std::span{&config, 1});
	}
	if(!configCount)
	{
		log.error("no usable EGL configs found with renderable type:{}", eglRenderableTypeToStr(renderableType));
		return {};
	}
	if(Config::DEBUG_BUILD)
		printEGLConf(display, config);
	return config;
}

int EGLManager::chooseConfigs(GLDisplay display, int renderableType, GLBufferConfigAttributes attr, std::span<EGLConfig> configs)
{
	auto eglAttr = glConfigAttrsToEGLAttrs(renderableType, attr);
	EGLint count{};
	eglChooseConfig(display, &eglAttr[0], configs.data(), configs.size(), &count);
	return count;
}

void *GLManager::procAddress(const char *funcName)
{
	//log.debug("getting proc address for:{}", funcName);
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
	if(presentationTime)
	{
		featuresStr.append(" [Presentation Time]");
	}
	if(featuresStr.empty())
		return;
	log.info("features:{}", featuresStr);
}

bool EGLManager::initDisplay(EGLDisplay display)
{
	log.info("initializing EGL with display:{}", display);
	EGLint major, minor;
	if(!eglInitialize(display, &major, &minor))
	{
		if(Config::DEBUG_BUILD)
			log.error("error:{} in eglInitialize() for display:{}", GLManager::errorString(eglGetError()), display);
		return false;
	}
	int eglVersion = 10 * major + minor;
	std::string_view extStr{eglQueryString(display, EGL_EXTENSIONS)};
	supportsSurfaceless = eglVersion >= 15 || extStr.contains("EGL_KHR_surfaceless_context");
	supportsNoConfig = extStr.contains("EGL_KHR_no_config_context");
	supportsNoError = extStr.contains("EGL_KHR_create_context_no_error");
	supportsSrgbColorSpace = eglVersion >= 15 || extStr.contains("EGL_KHR_gl_colorspace");
	if constexpr(Config::envIsLinux)
	{
		supportsTripleBufferSurfaces = extStr.contains("EGL_NV_triple_buffer");
	}
	doIfUsed(presentationTime, [&](auto &presentationTime)
	{
		if(extStr.contains("EGL_ANDROID_presentation_time"))
		{
			GLManager::loadSymbol(presentationTime, "eglPresentationTimeANDROID");
		}
	});
	logFeatures();
	return true;
}

GLContext GLManager::makeContext(GLContextAttributes attr, GLBufferConfig config, NativeGLContext shareContext)
{
	if(hasNoConfigContext())
		config = EGL_NO_CONFIG_KHR;
	if(!hasNoErrorContextAttribute())
		attr.noError = false;
	log.info("making context with version: {}.{} config:{} share context:{}",
		attr.version.major, attr.version.minor, (EGLConfig)config, shareContext);
	// Ignore surfaceless context support when using GL versions below 3.0 due to possible driver issues,
	// such as on Tegra 3 GPUs
	bool savePBuffConfig = attr.version.major <= 2 || !supportsSurfaceless;
	GLContext ctx{display(), attr, config, shareContext, savePBuffConfig};
	if(!ctx)
		return {};
	if(savePBuffConfig)
	{
		log.info("surfaceless context not supported:{}, saving config for dummy pbuffer",
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

int EGLManager::makeRenderableType(GL::API api, GL::Version version)
{
	if(api == GL::API::OpenGL)
	{
		return EGL_OPENGL_BIT;
	}
	else
	{
		switch(version.major)
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

GLDrawable GLManager::makeDrawable(Window &win, GLDrawableAttributes attr) const
{
	auto dpy = display();
	EGLSurfaceAttrList attrList;
	if(Config::envIsLinux && supportsTripleBufferSurfaces && attr.wantedRenderBuffers >= 3)
	{
		// request triple-buffering on Nvidia GPUs
		attrList.push_back(EGL_RENDER_BUFFER);
		attrList.push_back(EGL_TRIPLE_BUFFER_NV);
	}
	bool useSrgbColorSpace = attr.colorSpace == GLColorSpace::SRGB && hasSrgbColorSpace() &&
		supportsColorSpace(dpy, attr.bufferConfig, GLColorSpace::SRGB);
	if(useSrgbColorSpace)
	{
		attrList.push_back(EGL_GL_COLORSPACE);
		attrList.push_back(EGL_GL_COLORSPACE_SRGB);
	}
	attrList.push_back(EGL_NONE);
	GLDrawable drawable{dpy, win, attr.bufferConfig, attrList.data()};
	log.info("made surface:{} {}", (EGLSurface)drawable,
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
	return hasSrgbColorSpace() && attrs.pixelFormat != PixelFmtRGB565;
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

bool GLManager::hasPresentationTime() const { return presentationTime; }

void GLManager::setPresentationTime(NativeGLDrawable drawable, SteadyClockTimePoint time) const
{
	if(!hasPresentationTime())
		return;
	/* From window.h in Android frameworks:
	Special timestamp value to indicate that timestamps should be auto-generated
	by the native window when queueBuffer is called.  This is equal to INT64_MIN,
	defined directly to avoid problems with C99/C++ inclusion of stdint.h. */
	constexpr int64_t NATIVE_WINDOW_TIMESTAMP_AUTO = (-9223372036854775807LL-1);
	auto timestamp = hasTime(time) ? time.time_since_epoch().count() : NATIVE_WINDOW_TIMESTAMP_AUTO;
	bool success = presentationTime(display(), drawable, timestamp);
	if(Config::DEBUG_BUILD && !success)
	{
		log.error("error:{} in eglPresentationTimeANDROID({}, {})",
			errorString(eglGetError()), drawable, timestamp);
	}
}

void GLManager::logInfo() const
{
	if(!Config::DEBUG_BUILD)
		return;
	log.info("version: {} ({})", eglQueryString(display(), EGL_VENDOR), eglQueryString(display(), EGL_VERSION));
	log.info("APIs: {}", eglQueryString(display(), EGL_CLIENT_APIS));
	log.info("extensions: {}", eglQueryString(display(), EGL_EXTENSIONS));
	//printEGLConfs(display);
}

void EGLManager::terminateEGL(EGLDisplay display)
{
	if(display == EGL_NO_DISPLAY)
		return;
	log.info("terminating EGL display:{}", display);
	eglTerminate(display);
}

// EGLBufferConfig

EGLint EGLBufferConfig::renderableTypeBits(GLDisplay display) const
{
	EGLint bits;
	eglGetConfigAttrib(display, glConfig, EGL_RENDERABLE_TYPE, &bits);
	return bits;
}

bool EGLBufferConfig::maySupportGLES(GLDisplay display, int majorVersion) const
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
