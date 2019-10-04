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

#define LOGTAG "GLRenderer"
#include <assert.h>
#include <imagine/gfx/Gfx.hh>
#include <imagine/base/Base.hh>
#include <imagine/base/Window.hh>
#include <imagine/util/string.h>
#include "private.hh"
#include "utils.h"
#ifdef __ANDROID__
#include "../../base/android/android.hh"
#endif

#if defined CONFIG_BASE_GLAPI_EGL && defined CONFIG_GFX_OPENGL_ES
#define CAN_USE_EGL_SYNC
	#if __ANDROID_API__ < 18 || defined CONFIG_MACHINE_PANDORA
	#define EGL_SYNC_NEEDS_PROC_ADDR
	#endif
#endif

#ifdef CAN_USE_EGL_SYNC
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>
	#ifdef CONFIG_MACHINE_PANDORA
	using EGLSyncKHR = void*;
	using EGLTimeKHR = uint64_t;
	#endif
#ifndef EGL_TIMEOUT_EXPIRED
#define EGL_TIMEOUT_EXPIRED 0x30F5
#endif
#ifndef EGL_CONDITION_SATISFIED
#define EGL_CONDITION_SATISFIED 0x30F6
#endif
#ifndef EGL_SYNC_FENCE
#define EGL_SYNC_FENCE 0x30F9
#endif
#ifndef EGL_FOREVER
#define EGL_FOREVER 0xFFFFFFFFFFFFFFFFull
#endif
#define EGLSync EGLSyncKHR
#define EGLTime EGLTimeKHR
	#ifdef EGL_SYNC_NEEDS_PROC_ADDR
	static EGLSync (EGLAPIENTRY *eglCreateSyncFunc)(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list){};
	static EGLBoolean (EGLAPIENTRY *eglDestroySyncFunc)(EGLDisplay dpy, EGLSync sync){};
	static EGLint (EGLAPIENTRY *eglClientWaitSyncFunc)(EGLDisplay dpy, EGLSync sync, EGLint flags, EGLTime timeout){};
	static EGLint (EGLAPIENTRY *eglWaitSyncFunc)(EGLDisplay dpy, EGLSync sync, EGLint flags){};
	#else
	#define eglCreateSyncFunc eglCreateSyncKHR
	#define eglDestroySyncFunc eglDestroySyncKHR
	#define eglClientWaitSyncFunc eglClientWaitSyncKHR
	#define eglWaitSyncFunc eglWaitSyncKHR
	#endif
#endif

#ifndef GL_TIMEOUT_EXPIRED
#define GL_TIMEOUT_EXPIRED 0x911B
#endif

#ifndef GL_CONDITION_SATISFIED
#define GL_CONDITION_SATISFIED 0x911C
#endif

namespace Gfx
{

static constexpr int ON_EXIT_PRIORITY = 200;

static constexpr bool CAN_USE_OPENGL_ES_3 = !Config::MACHINE_IS_PANDORA;

Gfx::GC orientationToGC(uint o)
{
	using namespace Base;
	switch(o)
	{
		case VIEW_ROTATE_0: return Gfx::angleFromDegree(0.);
		case VIEW_ROTATE_90: return Gfx::angleFromDegree(-90.);
		case VIEW_ROTATE_180: return Gfx::angleFromDegree(-180.);
		case VIEW_ROTATE_270: return Gfx::angleFromDegree(90.);
		default: bug_unreachable("o == %d", o); return 0.;
	}
}

void GLRenderer::setupAnisotropicFiltering()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	GLfloat maximumAnisotropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnisotropy);
	logMsg("anisotropic filtering supported, max value: %f", (double)maximumAnisotropy);
	support.hasAnisotropicFiltering = true;
	#endif
}

void GLRenderer::setupMultisample()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	logMsg("multisample antialiasing supported");
	support.hasMultisample = true;
	#endif
}

void GLRenderer::setupMultisampleHints()
{
	#if !defined CONFIG_GFX_OPENGL_ES && !defined __APPLE__
	logMsg("multisample hints supported");
	support.hasMultisampleHints = true;
	//glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
	#endif
}

void GLRenderer::setupNonPow2Textures()
{
	if(!support.textureSizeSupport.nonPow2)
		logMsg("Non-Power-of-2 textures supported");
	support.textureSizeSupport.nonPow2 = true;
}

void GLRenderer::setupNonPow2MipmapTextures()
{
	if(!support.textureSizeSupport.nonPow2CanMipmap)
		logMsg("Non-Power-of-2 textures with mipmaps supported");
	support.textureSizeSupport.nonPow2 = true;
	support.textureSizeSupport.nonPow2CanMipmap = true;
}

void GLRenderer::setupNonPow2MipmapRepeatTextures()
{
	if(!support.textureSizeSupport.nonPow2CanRepeat)
		logMsg("Non-Power-of-2 textures with mipmaps & repeat modes supported");
	support.textureSizeSupport.nonPow2 = true;
	support.textureSizeSupport.nonPow2CanMipmap = true;
	support.textureSizeSupport.nonPow2CanRepeat = true;
}

#ifdef CONFIG_GFX_OPENGL_ES
void GLRenderer::setupBGRPixelSupport()
{
	support.hasBGRPixels = true;
	logMsg("BGR pixel types are supported%s", support.bgrInternalFormat == GL_RGBA ? " (Apple version)" : "");
}
#endif

void GLRenderer::setupFBOFuncs(bool &useFBOFuncs)
{
	useFBOFuncs = true;
	#if defined CONFIG_GFX_OPENGL_ES && CONFIG_GFX_OPENGL_ES_MAJOR_VERSION == 1
	support.generateMipmaps = glGenerateMipmapOES;
	#elif !defined CONFIG_GFX_OPENGL_ES
	support.generateMipmaps = glGenerateMipmap;
	#endif
	logMsg("FBO functions are supported");
}

void GLRenderer::setupVAOFuncs()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	logMsg("using VAOs");
	useStreamVAO = true;
	#endif
}

void GLRenderer::setupTextureSwizzle()
{
	if(support.hasTextureSwizzle)
		return;
	logMsg("using texture swizzling");
	support.hasTextureSwizzle = true;
}

void GLRenderer::setupImmutableTexStorage(bool extSuffix)
{
	if(support.hasImmutableTexStorage)
		return;
	logMsg("using immutable texture storage");
	support.hasImmutableTexStorage = true;
	#ifdef CONFIG_GFX_OPENGL_ES
	const char *procName = extSuffix ? "glTexStorage2DEXT" : "glTexStorage2D";
	support.glTexStorage2D = (typeof(support.glTexStorage2D))Base::GLContext::procAddress(procName);
	#endif
}

void GLRenderer::setupRGFormats()
{
	support.luminanceFormat = GL_RED;
	support.luminanceInternalFormat = GL_R8;
	support.luminanceAlphaFormat = GL_RG;
	support.luminanceAlphaInternalFormat = GL_RG8;
	support.alphaFormat = GL_RED;
	support.alphaInternalFormat = GL_R8;
}

void GLRenderer::setupSamplerObjects()
{
	if(support.hasSamplerObjects)
		return;
	logMsg("using sampler objects");
	support.hasSamplerObjects = true;
	#ifdef CONFIG_GFX_OPENGL_ES
	support.glGenSamplers = (typeof(support.glGenSamplers))Base::GLContext::procAddress("glGenSamplers");
	support.glDeleteSamplers = (typeof(support.glDeleteSamplers))Base::GLContext::procAddress("glDeleteSamplers");
	support.glBindSampler = (typeof(support.glBindSampler))Base::GLContext::procAddress("glBindSampler");
	support.glSamplerParameteri = (typeof(support.glSamplerParameteri))Base::GLContext::procAddress("glSamplerParameteri");
	#endif
}

void GLRenderer::setupPBO()
{
	if(support.hasPBOFuncs)
		return;
	logMsg("using PBOs");
	support.hasPBOFuncs = true;
}

void GLRenderer::setupFenceSync()
{
	if(support.hasSyncFences())
		return;
	logMsg("using sync fences");
	#ifdef CONFIG_GFX_OPENGL_ES
	support.glFenceSync = (typeof(support.glFenceSync))Base::GLContext::procAddress("glFenceSync");
	support.glDeleteSync = (typeof(support.glDeleteSync))Base::GLContext::procAddress("glDeleteSync");
	support.glWaitSync = (typeof(support.glWaitSync))Base::GLContext::procAddress("glWaitSync");
	#else
	support.hasFenceSync = true;
	#endif
}

#ifdef CONFIG_GFX_OPENGL_ES
void GLRenderer::setupAppleFenceSync()
{
	if(support.hasSyncFences())
		return;
	logMsg("Using sync fences (Apple version)");
	support.glFenceSync = (typeof(support.glFenceSync))Base::GLContext::procAddress("glFenceSyncAPPLE");
	support.glDeleteSync = (typeof(support.glDeleteSync))Base::GLContext::procAddress("glDeleteSyncAPPLE");
	support.glWaitSync = (typeof(support.glWaitSync))Base::GLContext::procAddress("glWaitSyncAPPLE");
}
#endif

#ifdef CAN_USE_EGL_SYNC
void GLRenderer::setupEGLFenceSync(bool supportsServerSync)
{
	if(support.hasSyncFences())
		return;
	logMsg("Using sync fences (EGL version)%s", supportsServerSync ? "" : ", only client sync supported");
	#ifdef EGL_SYNC_NEEDS_PROC_ADDR
	eglCreateSyncFunc = (typeof(eglCreateSyncFunc))Base::GLContext::procAddress("eglCreateSyncKHR");
	eglDestroySyncFunc = (typeof(eglDestroySyncFunc))Base::GLContext::procAddress("eglDestroySyncKHR");
	if(supportsServerSync)
		eglWaitSyncFunc = (typeof(eglWaitSyncFunc))Base::GLContext::procAddress("eglWaitSyncKHR");
	else
		eglClientWaitSyncFunc = (typeof(eglClientWaitSyncFunc))Base::GLContext::procAddress("eglClientWaitSyncKHR");
	#endif
	// wrap EGL sync in terms of ARB sync
	support.glFenceSync =
		[](GLenum condition, GLbitfield flags)
		{
			return (GLsync)eglCreateSyncFunc(Base::GLDisplay::getDefault().eglDisplay(), EGL_SYNC_FENCE, nullptr);
		};
	support.glDeleteSync =
		[](GLsync sync)
		{
			eglDestroySyncFunc(Base::GLDisplay::getDefault().eglDisplay(), (EGLSync)sync);
		};
	if(supportsServerSync)
	{
		support.glWaitSync =
		[](GLsync sync, GLbitfield flags, GLuint64 timeout)
		{
			if(eglWaitSyncFunc(Base::GLDisplay::getDefault().eglDisplay(), (EGLSync)sync, 0) == GL_FALSE)
			{
				logErr("error waiting for sync object:%p", sync);
			}
		};
	}
	else
	{
		support.glWaitSync =
			[](GLsync sync, GLbitfield flags, GLuint64 timeout)
			{
				if(eglClientWaitSyncFunc(Base::GLDisplay::getDefault().eglDisplay(), (EGLSync)sync, 0, timeout) == GL_FALSE)
				{
					logErr("error waiting for sync object:%p", sync);
				}
			};
	}
}
#endif

void GLRenderer::setupSpecifyDrawReadBuffers()
{
	#ifdef CONFIG_GFX_OPENGL_ES
	support.glDrawBuffers = (typeof(support.glDrawBuffers))Base::GLContext::procAddress("glDrawBuffers");
	support.glReadBuffer = (typeof(support.glReadBuffer))Base::GLContext::procAddress("glReadBuffer");
	#endif
}

bool DrawContextSupport::hasDrawReadBuffers() const
{
	#ifdef CONFIG_GFX_OPENGL_ES
	return glDrawBuffers;
	#else
	return true;
	#endif
}

bool DrawContextSupport::hasSyncFences() const
{
	#ifdef CONFIG_GFX_OPENGL_ES
	return glFenceSync;
	#else
	return hasFenceSync;
	#endif
}

void GLRenderer::setupUnmapBufferFunc()
{
	#ifdef CONFIG_GFX_OPENGL_ES
	if(!support.glUnmapBuffer)
	{
		if constexpr(Config::envIsAndroid || Config::envIsIOS)
		{
			support.glUnmapBuffer = (DrawContextSupport::UnmapBufferProto)glUnmapBufferOES;
		}
		else
		{
			if constexpr(Config::Gfx::OPENGL_ES)
			{
				support.glUnmapBuffer = (typeof(support.glUnmapBuffer))Base::GLContext::procAddress("glUnmapBufferOES");
			}
			else
			{
				support.glUnmapBuffer = (typeof(support.glUnmapBuffer))Base::GLContext::procAddress("glUnmapBuffer");
			}
		}
	}
	#endif
}

void GLRenderer::checkExtensionString(const char *extStr, bool &useFBOFuncs)
{
	//logMsg("checking %s", extStr);
	if(string_equal(extStr, "GL_ARB_texture_non_power_of_two")
		|| (Config::Gfx::OPENGL_ES && string_equal(extStr, "GL_OES_texture_npot")))
	{
		// allows mipmaps and repeat modes
		setupNonPow2MipmapRepeatTextures();
	}
	#ifdef CONFIG_GFX_OPENGL_DEBUG_CONTEXT
	else if(Config::DEBUG_BUILD && string_equal(extStr, "GL_KHR_debug"))
	{
		support.hasDebugOutput = true;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_ES
	else if(Config::Gfx::OPENGL_ES_MAJOR_VERSION == 1
		&& (string_equal(extStr, "GL_APPLE_texture_2D_limited_npot") || string_equal(extStr, "GL_IMG_texture_npot")))
	{
		// no mipmaps or repeat modes
		setupNonPow2Textures();
	}
	else if(Config::Gfx::OPENGL_ES_MAJOR_VERSION >= 2
		&& !Config::envIsIOS && string_equal(extStr, "GL_NV_texture_npot_2D_mipmap"))
	{
		// no repeat modes
		setupNonPow2MipmapTextures();
	}
	else if(Config::Gfx::OPENGL_ES_MAJOR_VERSION >= 2 && string_equal(extStr, "GL_EXT_unpack_subimage"))
	{
		logMsg("unpacking sub-images supported");
		support.hasUnpackRowLength = true;
	}
	else if(string_equal(extStr, "GL_APPLE_texture_format_BGRA8888"))
	{
		support.bgrInternalFormat = GL_RGBA;
		setupBGRPixelSupport();
	}
	else if(string_equal(extStr, "GL_EXT_texture_format_BGRA8888"))
	{
		setupBGRPixelSupport();
	}
	else if(Config::Gfx::OPENGL_ES_MAJOR_VERSION == 1 && string_equal(extStr, "GL_OES_framebuffer_object"))
	{
		if(!useFBOFuncs)
			setupFBOFuncs(useFBOFuncs);
	}
	else if(string_equal(extStr, "GL_EXT_texture_storage"))
	{
		setupImmutableTexStorage(true);
	}
	#if defined __ANDROID__ || defined __APPLE__
	else if(string_equal(extStr, "GL_APPLE_sync"))
	{
		setupAppleFenceSync();
	}
	#endif
	#if defined __ANDROID__
	else if(string_equal(extStr, "GL_OES_EGL_image"))
	{
		support.hasEGLImages = true;
	}
	else if(Config::Gfx::OPENGL_ES_MAJOR_VERSION >= 2 &&
		string_equal(extStr, "GL_OES_EGL_image_external"))
	{
		support.hasExternalEGLImages = true;
	}
	#endif
	else if(Config::Gfx::OPENGL_ES_MAJOR_VERSION >= 2 && string_equal(extStr, "GL_NV_pixel_buffer_object"))
	{
		setupPBO();
	}
	else if(Config::Gfx::OPENGL_ES_MAJOR_VERSION >= 2 && string_equal(extStr, "GL_NV_map_buffer_range"))
	{
		logMsg("supports map buffer range (NVIDIA)");
		if(!support.glMapBufferRange)
			support.glMapBufferRange = (typeof(support.glMapBufferRange))Base::GLContext::procAddress("glMapBufferRangeNV");
		setupUnmapBufferFunc();
	}
	else if(string_equal(extStr, "GL_EXT_map_buffer_range"))
	{
		logMsg("supports map buffer range");
		if(!support.glMapBufferRange)
			support.glMapBufferRange = (typeof(support.glMapBufferRange))Base::GLContext::procAddress("glMapBufferRangeEXT");
		setupUnmapBufferFunc();
	}
	/*else if(string_equal(extStr, "GL_OES_mapbuffer"))
	{
		// handled in *_map_buffer_range currently
	}*/
	#endif
	#ifndef CONFIG_GFX_OPENGL_ES
	else if(string_equal(extStr, "GL_EXT_texture_filter_anisotropic"))
	{
		setupAnisotropicFiltering();
	}
	else if(string_equal(extStr, "GL_ARB_multisample"))
	{
		setupMultisample();
	}
	else if(string_equal(extStr, "GL_NV_multisample_filter_hint"))
	{
		setupMultisampleHints();
	}
	else if(string_equal(extStr, "GL_EXT_framebuffer_object"))
	{
		#ifndef __APPLE__
		if(!useFBOFuncs)
		{
			setupFBOFuncs(useFBOFuncs);
			support.generateMipmaps = glGenerateMipmapEXT;
		}
		#endif
	}
	else if(string_equal(extStr, "GL_ARB_framebuffer_object"))
	{
		if(!useFBOFuncs)
			setupFBOFuncs(useFBOFuncs);
	}
	else if(string_equal(extStr, "GL_ARB_texture_storage"))
	{
		setupImmutableTexStorage(false);
	}
	else if(string_equal(extStr, "GL_ARB_pixel_buffer_object"))
	{
		setupPBO();
	}
	else if(string_equal(extStr, "GL_ARB_sync"))
	{
		setupFenceSync();
	}
	#endif
}

void GLRenderer::checkFullExtensionString(const char *fullExtStr)
{
	char fullExtStrTemp[strlen(fullExtStr)+1];
	strcpy(fullExtStrTemp, fullExtStr);
	char *savePtr;
	auto extStr = strtok_r(fullExtStrTemp, " ", &savePtr);
	bool useFBOFuncs = false;
	while(extStr)
	{
		checkExtensionString(extStr, useFBOFuncs);
		extStr = strtok_r(nullptr, " ", &savePtr);
	}
}

static int glVersionFromStr(const char *versionStr)
{
	// skip to version number
	while(!isdigit(*versionStr) && *versionStr != '\0')
		versionStr++;
	int major = 1, minor = 0;
	if(sscanf(versionStr, "%d.%d", &major, &minor) != 2)
	{
		logErr("unable to parse GL version string");
	}
	return 10 * major + minor;
}

static Base::GLContextAttributes makeGLContextAttributes(uint majorVersion, uint minorVersion)
{
	Base::GLContextAttributes glAttr;
	if(Config::DEBUG_BUILD)
		glAttr.setDebug(true);
	glAttr.setMajorVersion(majorVersion);
	#ifdef CONFIG_GFX_OPENGL_ES
	glAttr.setOpenGLESAPI(true);
	#else
	glAttr.setMinorVersion(minorVersion);
	#endif
	return glAttr;
}

Base::GLContextAttributes GLRenderer::makeKnownGLContextAttributes()
{
	#ifdef CONFIG_GFX_OPENGL_ES
	if(Config::Gfx::OPENGL_ES_MAJOR_VERSION == 1)
	{
		return makeGLContextAttributes(1, 0);
	}
	else
	{
		assert(glMajorVer);
		return makeGLContextAttributes(glMajorVer, 0);
	}
	#else
	if(Config::Gfx::OPENGL_SHADER_PIPELINE)
	{
		return makeGLContextAttributes(3, 3);
	}
	else
	{
		return makeGLContextAttributes(1, 3);
	}
	#endif
}

Renderer::Renderer() {}

Renderer::Renderer(IG::PixelFormat pixelFormat, Error &err)
{
	Base::GLDisplay dpy{};
	{
		std::error_code ec{};
		dpy = Base::GLDisplay::makeDefault(ec);
		if(ec)
		{
			logErr("error getting GL display");
			err = std::runtime_error("error creating GL display");
			return;
		}
		glDpy = dpy;
		dpy.logInfo();
		#ifdef CONFIG_GFX_OPENGL_ES
		if(!Base::GLContext::bindAPI(Base::GLContext::OPENGL_ES_API))
		{
			logErr("unable to bind GLES API");
		}
		#endif
	}
	if(!pixelFormat)
		pixelFormat = Base::Window::defaultPixelFormat();
	Base::GLBufferConfigAttributes glBuffAttr;
	glBuffAttr.setPixelFormat(pixelFormat);
	#if CONFIG_GFX_OPENGL_ES_MAJOR_VERSION == 1
	auto glAttr = makeGLContextAttributes(1, 0);
	gfxBufferConfig = gfxResourceContext.makeBufferConfig(dpy, glAttr, glBuffAttr);
	std::error_code ec{};
	gfxResourceContext = {dpy, glAttr, gfxBufferConfig, ec};
	#elif CONFIG_GFX_OPENGL_ES_MAJOR_VERSION > 1
	if(CAN_USE_OPENGL_ES_3)
	{
		auto glAttr = makeGLContextAttributes(3, 0);
		gfxBufferConfig = gfxResourceContext.makeBufferConfig(dpy, glAttr, glBuffAttr);
		std::error_code ec{};
		if(gfxBufferConfig)
		{
			gfxResourceContext = {dpy, glAttr, gfxBufferConfig, ec};
			glMajorVer = glAttr.majorVersion();
		}
	}
	if(!gfxResourceContext) // fall back to OpenGL ES 2.0
	{
		auto glAttr = makeGLContextAttributes(2, 0);
		gfxBufferConfig = gfxResourceContext.makeBufferConfig(dpy, glAttr, glBuffAttr);
		std::error_code ec{};
		gfxResourceContext = {dpy, glAttr, gfxBufferConfig, ec};
		glMajorVer = glAttr.majorVersion();
	}
	#else
	if(!Base::GLContext::bindAPI(Base::GLContext::OPENGL_API))
	{
		logErr("unable to bind GL API");
	}
	if(Config::Gfx::OPENGL_SHADER_PIPELINE)
	{
		#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
		support.useFixedFunctionPipeline = false;
		#endif
		auto glAttr = makeGLContextAttributes(3, 3);
		gfxBufferConfig = gfxResourceContext.makeBufferConfig(dpy, glAttr, glBuffAttr);
		std::error_code ec{};
		gfxResourceContext = {dpy, glAttr, gfxBufferConfig, ec};
		if(!gfxResourceContext)
		{
			logMsg("3.3 context not supported");
		}
	}
	if(Config::Gfx::OPENGL_FIXED_FUNCTION_PIPELINE && !gfxResourceContext)
	{
		#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
		support.useFixedFunctionPipeline = true;
		#endif
		auto glAttr = makeGLContextAttributes(1, 3);
		gfxBufferConfig = gfxResourceContext.makeBufferConfig(dpy, glAttr, glBuffAttr);
		std::error_code ec{};
		gfxResourceContext = {dpy, glAttr, gfxBufferConfig, ec};
		if(!gfxResourceContext)
		{
			logMsg("1.3 context not supported");
		}
	}
	#endif
	if(!gfxResourceContext)
	{
		err = std::runtime_error("error creating GL context");
		return;
	}
	err = {};
	mainTask = std::make_unique<GLMainTask>();
	mainTask->start(gfxResourceContext);
}

Renderer::Renderer(Error &err): Renderer(Base::Window::defaultPixelFormat(), err) {}

void Renderer::configureRenderer(ThreadMode threadMode)
{
	runGLTaskSync(
		[this]()
		{
			auto version = (const char*)glGetString(GL_VERSION);
			assert(version);
			auto rendererName = (const char*)glGetString(GL_RENDERER);
			logMsg("version: %s (%s)", version, rendererName);

			int glVer = glVersionFromStr(version);

			bool useFBOFuncs = false;
			#ifndef CONFIG_GFX_OPENGL_ES
			// core functionality
			if(glVer >= 15)
			{
				support.hasVBOFuncs = true;
			}
			if(glVer >= 20)
			{
				setupNonPow2MipmapRepeatTextures();
				setupSpecifyDrawReadBuffers();
			}
			if(glVer >= 21)
			{
				setupPBO();
			}
			if(glVer >= 30)
			{
				if(!support.useFixedFunctionPipeline)
				{
					// must render via VAOs/VBOs in 3.1+ without compatibility context
					setupVAOFuncs();
					setupTextureSwizzle();
					setupRGFormats();
					setupSamplerObjects();
				}
				setupFBOFuncs(useFBOFuncs);
			}
			if(glVer >= 32)
			{
				setupFenceSync();
			}

			// extension functionality
			if(glVer >= 30)
			{
				GLint numExtensions;
				glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
				if(Config::DEBUG_BUILD)
				{
					logMsgNoBreak("extensions: ");
					iterateTimes(numExtensions, i)
					{
						logger_printf(LOG_M, "%s ", (const char*)glGetStringi(GL_EXTENSIONS, i));
					}
					logger_printf(LOG_M, "\n");
				}
				iterateTimes(numExtensions, i)
				{
					checkExtensionString((const char*)glGetStringi(GL_EXTENSIONS, i), useFBOFuncs);
				}
			}
			else
			{
				auto extensions = (const char*)glGetString(GL_EXTENSIONS);
				assert(extensions);
				logMsg("extensions: %s", extensions);
				checkFullExtensionString(extensions);
			}
			#else
			// core functionality
			if(Config::Gfx::OPENGL_ES_MAJOR_VERSION == 1 && glVer >= 11)
			{
				// safe to use VBOs
			}
			if(Config::Gfx::OPENGL_ES_MAJOR_VERSION > 1)
			{
				if(glVer >= 30)
					setupNonPow2MipmapRepeatTextures();
				else
					setupNonPow2Textures();
				setupFBOFuncs(useFBOFuncs);
				if(glVer >= 30)
				{
					support.glMapBufferRange = (typeof(support.glMapBufferRange))Base::GLContext::procAddress("glMapBufferRange");
					support.glUnmapBuffer = (typeof(support.glUnmapBuffer))Base::GLContext::procAddress("glUnmapBuffer");
					setupImmutableTexStorage(false);
					setupTextureSwizzle();
					setupRGFormats();
					setupSamplerObjects();
					setupPBO();
					setupFenceSync();
					if(!Config::envIsIOS)
						setupSpecifyDrawReadBuffers();
					support.hasUnpackRowLength = true;
					support.useLegacyGLSL = false;
				}
			}

			#ifdef CAN_USE_EGL_SYNC
			// check for fence sync via EGL extensions
			bool checkFenceSync = glVer < 30
					&& !Config::MACHINE_IS_PANDORA; // TODO: driver waits for full timeout even if commands complete,
																					// possibly broken glFlush() behavior?
			if(checkFenceSync)
			{
				auto extStr = glDpy.queryExtensions();
				if(strstr(extStr, "EGL_KHR_fence_sync"))
				{
					auto supportsServerSync = strstr(extStr, "EGL_KHR_wait_sync");
					setupEGLFenceSync(supportsServerSync);
				}
			}
			#endif

			// extension functionality
			auto extensions = (const char*)glGetString(GL_EXTENSIONS);
			assert(extensions);
			logMsg("extensions: %s", extensions);
			checkFullExtensionString(extensions);
			#endif // CONFIG_GFX_OPENGL_ES

			GLint texSize;
			glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
			support.textureSizeSupport.maxXSize = support.textureSizeSupport.maxYSize = texSize;
			assert(support.textureSizeSupport.maxXSize > 0 && support.textureSizeSupport.maxYSize > 0);
			logMsg("max texture size is %d", texSize);

			#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
			if(!support.useFixedFunctionPipeline)
			{
				if(Config::DEBUG_BUILD)
					logMsg("shader language version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
			}
			#endif
		});

	if(Config::DEBUG_BUILD && defaultToFullErrorChecks)
	{
		setCorrectnessChecks(true);
		setDebugOutput(true);
	}

	if(!support.hasSyncFences())
		threadMode = ThreadMode::SINGLE;
	if(threadMode == ThreadMode::AUTO)
	{
		useSeparateDrawContext = support.hasSyncFences();
	}
	else
	{
		useSeparateDrawContext = threadMode == ThreadMode::MULTI;
	}
	if(useSeparateDrawContext)
		logMsg("using separate draw context");
	support.isConfigured = true;
}

bool Renderer::isConfigured() const
{
	return support.isConfigured;
}

Renderer Renderer::makeConfiguredRenderer(ThreadMode threadMode, IG::PixelFormat pixelFormat, Error &err)
{
	auto renderer = Renderer{pixelFormat, err};
	if(err)
		return {};
	renderer.configureRenderer(threadMode);
	return renderer;
}

Renderer Renderer::makeConfiguredRenderer(ThreadMode threadMode, Error &err)
{
	return makeConfiguredRenderer(threadMode, Base::Window::defaultPixelFormat(), err);
}

Renderer Renderer::makeConfiguredRenderer(Error &err)
{
	return makeConfiguredRenderer(Gfx::Renderer::ThreadMode::AUTO, err);
}

Renderer::ThreadMode Renderer::threadMode() const
{
	return useSeparateDrawContext ? ThreadMode::MULTI : ThreadMode::SINGLE;
}

bool Renderer::supportsThreadMode() const
{
	return support.hasSyncFences();
}

Base::WindowConfig Renderer::addWindowConfig(Base::WindowConfig config)
{
	assert(isConfigured());
	config.setFormat(gfxBufferConfig.windowFormat(glDpy));
	return config;
}

static void updateSensorStateForWindowOrientations(Base::Window &win)
{
	// activate orientation sensor if doing rotation in software and the main window
	// has multiple valid orientations
	if(Config::SYSTEM_ROTATES_WINDOWS || win != Base::mainWindow())
		return;
	Base::setDeviceOrientationChangeSensor(IG::bitsSet(win.validSoftOrientations()) > 1);
}

void Renderer::initWindow(Base::Window &win, Base::WindowConfig config)
{
	win.init(addWindowConfig(config));
	updateSensorStateForWindowOrientations(win);
}

void Renderer::setWindowValidOrientations(Base::Window &win, uint validO)
{
	if(win != Base::mainWindow())
		return;
	auto oldWinO = win.softOrientation();
	if(win.setValidOrientations(validO) && !Config::SYSTEM_ROTATES_WINDOWS)
	{
		animateProjectionMatrixRotation(orientationToGC(oldWinO), orientationToGC(win.softOrientation()));
	}
	updateSensorStateForWindowOrientations(win);
}

void GLRenderer::addOnExitHandler()
{
	if(onExit)
		return;
	onExit =
		[this](bool backgrounded)
		{
			if(backgrounded)
			{
				runGLTaskSync(
					[]()
					{
						glFinish();
					});
			}
			else
			{
				if(!gfxResourceContext)
					return true;
				mainTask->stop();
				gfxResourceContext.deinit(glDpy);
				glDpy.deinit();
				#ifndef NDEBUG
				contextDestroyed = true;
				#endif
			}
			return true;
		};
	Base::addOnExit(onExit, ON_EXIT_PRIORITY);
}

}
