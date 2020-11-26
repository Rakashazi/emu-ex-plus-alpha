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
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/base/Base.hh>
#include <imagine/base/Window.hh>
#include <imagine/util/string.h>
#include <imagine/fs/FS.hh>
#include "private.hh"
#include "utils.h"
#ifdef __ANDROID__
#include "../../base/android/android.hh"
#endif
#include <string>

namespace Gfx
{

static constexpr bool CAN_USE_OPENGL_ES_3 = !Config::MACHINE_IS_PANDORA;

Gfx::GC orientationToGC(Base::Orientation o)
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

static void printFeatures(DrawContextSupport support)
{
	if(!Config::DEBUG_BUILD)
		return;
	std::string featuresStr{};
	featuresStr.reserve(256);

	featuresStr.append(" [Texture Size:");
	featuresStr.append(string_makePrintf<8>("%u", support.textureSizeSupport.maxXSize).data());
	featuresStr.append("]");
	if(support.textureSizeSupport.nonPow2)
	{
		featuresStr.append(" [NPOT Textures");
		if(support.textureSizeSupport.nonPow2CanRepeat)
			featuresStr.append(" w/ Mipmap+Repeat]");
		else if(support.textureSizeSupport.nonPow2CanMipmap)
			featuresStr.append(" w/ Mipmap]");
		else
			featuresStr.append("]");
	}
	#ifdef CONFIG_GFX_OPENGL_ES
	if(support.hasBGRPixels)
	{
		if(support.bgrInternalFormat == GL_RGBA)
			featuresStr.append(" [BGR Formats (Apple)]");
		else
			featuresStr.append(" [BGR Formats]");
	}
	#endif
	if(support.hasTextureSwizzle)
	{
		featuresStr.append(" [Texture Swizzle]");
	}
	if(support.hasImmutableTexStorage)
	{
		featuresStr.append(" [Immutable Texture Storage]");
	}
	if(support.hasImmutableBufferStorage())
	{
		featuresStr.append(" [Immutable Buffer Storage]");
	}
	if(support.hasMemoryBarriers())
	{
		featuresStr.append(" [Memory Barriers]");
	}
	if(Config::Gfx::OPENGL_ES_MAJOR_VERSION >= 2 && support.hasUnpackRowLength)
	{
		featuresStr.append(" [Unpack Sub-Images]");
	}
	if(support.hasSamplerObjects)
	{
		featuresStr.append(" [Sampler Objects]");
	}
	if(support.hasPBOFuncs)
	{
		featuresStr.append(" [PBOs]");
	}
	if(support.glMapBufferRange)
	{
		featuresStr.append(" [Map Buffer Range]");
	}
	if(support.hasSyncFences())
	{
		if(Config::Base::GL_PLATFORM_EGL)
		{
			if(support.hasServerWaitSync())
				featuresStr.append(" [EGL Sync Fences + Server Sync]");
			else
				featuresStr.append(" [EGL Sync Fences]");
		}
		else
		{
			featuresStr.append(" [Sync Fences]");
		}
	}
	#ifdef __ANDROID__
	if(support.eglPresentationTimeANDROID)
	{
		featuresStr.append(" [Presentation Time]");
	}
	#endif
	#ifndef CONFIG_GFX_OPENGL_ES
	if(support.maximumAnisotropy)
	{
		featuresStr.append(" [Max Anisotropy:");
		featuresStr.append(string_makePrintf<8>("%.1f", support.maximumAnisotropy).data());
		featuresStr.append("]");
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(!support.useFixedFunctionPipeline)
	{
		featuresStr.append(" [GLSL:");
		featuresStr.append((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
		featuresStr.append("]");
	}
	#endif

	logMsg("features:%s", featuresStr.c_str());
}

#ifdef __ANDROID__
EGLImageKHR makeAndroidNativeBufferEGLImage(EGLDisplay dpy, EGLClientBuffer clientBuff)
{
	const EGLint eglImgAttrs[]
	{
		EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
		EGL_NONE, EGL_NONE
	};
	return eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
		clientBuff, eglImgAttrs);
}
#endif

void GLRenderer::setupAnisotropicFiltering()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	GLfloat maximumAnisotropy{};
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnisotropy);
	support.maximumAnisotropy = maximumAnisotropy;
	#endif
}

void GLRenderer::setupMultisample()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	support.hasMultisample = true;
	#endif
}

void GLRenderer::setupMultisampleHints()
{
	#if !defined CONFIG_GFX_OPENGL_ES && !defined __APPLE__
	support.hasMultisampleHints = true;
	//glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
	#endif
}

void GLRenderer::setupNonPow2Textures()
{
	support.textureSizeSupport.nonPow2 = true;
}

void GLRenderer::setupNonPow2MipmapTextures()
{
	support.textureSizeSupport.nonPow2 = true;
	support.textureSizeSupport.nonPow2CanMipmap = true;
}

void GLRenderer::setupNonPow2MipmapRepeatTextures()
{
	support.textureSizeSupport.nonPow2 = true;
	support.textureSizeSupport.nonPow2CanMipmap = true;
	support.textureSizeSupport.nonPow2CanRepeat = true;
}

#ifdef CONFIG_GFX_OPENGL_ES
void GLRenderer::setupBGRPixelSupport()
{
	support.hasBGRPixels = true;
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
}

void GLRenderer::setupVAOFuncs()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	useStreamVAO = true;
	#endif
}

void GLRenderer::setupTextureSwizzle()
{
	support.hasTextureSwizzle = true;
}

void GLRenderer::setupImmutableTexStorage(bool extSuffix)
{
	if(support.hasImmutableTexStorage)
		return;
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
	support.hasPBOFuncs = true;
}

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

#ifdef __ANDROID__
bool DrawContextSupport::hasEGLTextureStorage() const
{
	return glEGLImageTargetTexStorageEXT;
}
#endif

bool DrawContextSupport::hasImmutableBufferStorage() const
{
	#ifdef CONFIG_GFX_OPENGL_ES
	return glBufferStorage;
	#else
	return hasBufferStorage;
	#endif
}

bool DrawContextSupport::hasMemoryBarriers() const
{
	return false;
	/*#ifdef CONFIG_GFX_OPENGL_ES
	return glMemoryBarrier;
	#else
	return hasMemoryBarrier;
	#endif*/
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

void GLRenderer::setupImmutableBufferStorage()
{
	if(support.hasImmutableBufferStorage())
		return;
	#ifdef CONFIG_GFX_OPENGL_ES
	support.glBufferStorage = (typeof(support.glBufferStorage))Base::GLContext::procAddress("glBufferStorageEXT");
	#else
	support.hasBufferStorage = true;
	#endif
}

void GLRenderer::setupMemoryBarrier()
{
	/*if(support.hasMemoryBarriers())
		return;
	#ifdef CONFIG_GFX_OPENGL_ES
	support.glMemoryBarrier = (typeof(support.glMemoryBarrier))Base::GLContext::procAddress("glMemoryBarrier");
	#else
	support.hasMemoryBarrier = true;
	#endif*/
}

void GLRenderer::setupPresentationTime(const char *eglExtenstionStr)
{
	#ifdef __ANDROID__
	if(strstr(eglExtenstionStr, "EGL_ANDROID_presentation_time"))
	{
		Base::GLContext::loadSymbol(support.eglPresentationTimeANDROID, "eglPresentationTimeANDROID");
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
		#ifdef __ANDROID__
		// older GPU drivers like Tegra 3 can crash when using debug output,
		// only enable on recent Android version to be safe
		if(Base::androidSDK() < 23)
		{
			support.hasDebugOutput = false;
		}
		#endif
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
	else if(!Config::Base::GL_PLATFORM_EGL && string_equal(extStr, "GL_APPLE_sync"))
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
	else if(Config::Gfx::OPENGL_ES_MAJOR_VERSION >= 2 &&
		string_equal(extStr, "GL_EXT_EGL_image_storage"))
	{
		support.glEGLImageTargetTexStorageEXT = (typeof(support.glEGLImageTargetTexStorageEXT))Base::GLContext::procAddress("glEGLImageTargetTexStorageEXT");
	}
	#endif
	else if(Config::Gfx::OPENGL_ES_MAJOR_VERSION >= 2 && string_equal(extStr, "GL_NV_pixel_buffer_object"))
	{
		setupPBO();
	}
	else if(Config::Gfx::OPENGL_ES_MAJOR_VERSION >= 2 && string_equal(extStr, "GL_NV_map_buffer_range"))
	{
		if(!support.glMapBufferRange)
			support.glMapBufferRange = (typeof(support.glMapBufferRange))Base::GLContext::procAddress("glMapBufferRangeNV");
		setupUnmapBufferFunc();
	}
	else if(string_equal(extStr, "GL_EXT_map_buffer_range"))
	{
		if(!support.glMapBufferRange)
			support.glMapBufferRange = (typeof(support.glMapBufferRange))Base::GLContext::procAddress("glMapBufferRangeEXT");
		// Only using ES 3.0 version currently
		//if(!support.glFlushMappedBufferRange)
		//	support.glFlushMappedBufferRange = (typeof(support.glFlushMappedBufferRange))Base::GLContext::procAddress("glFlushMappedBufferRangeEXT");
		setupUnmapBufferFunc();
	}
	else if(Config::Gfx::OPENGL_ES_MAJOR_VERSION >= 2 && string_equal(extStr, "GL_EXT_buffer_storage"))
	{
		setupImmutableBufferStorage();
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
	else if(!Config::Base::GL_PLATFORM_EGL && string_equal(extStr, "GL_ARB_sync"))
	{
		setupFenceSync();
	}
	else if(string_equal(extStr, "GL_ARB_buffer_storage"))
	{
		setupImmutableBufferStorage();
	}
	else if(string_equal(extStr, "GL_ARB_shader_image_load_store"))
	{
		setupMemoryBarrier();
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

static Base::GLContextAttributes makeGLContextAttributes(uint32_t majorVersion, uint32_t minorVersion)
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

void GLRenderer::finishContextCreation(Base::GLContext ctx)
{
	#if CONFIG_GFX_OPENGL_ES_MAJOR_VERSION > 1
	if(Config::envIsAndroid && Config::MACHINE == Config::Machine::GENERIC_ARMV7 && glMajorVer == 2)
	{
		// Vivante "GC1000 core" GPU (Samsung Galaxy S3 Mini, Galaxy Tab 3), possibly others, will fail
		// setting context in render thread with EGL_BAD_ACCESS unless it's first set in the creation thread,
		// exact cause unknown and is most likely a driver bug
		logDMsg("toggling newly created context current on this thread to avoid driver issues");
		ctx.setCurrent(glDpy, ctx, {});
		ctx.setCurrent(glDpy, {}, {});
	}
	#endif
}

GLRenderer::GLRenderer(Init):
	releaseShaderCompilerEvent{"GLRenderer::releaseShaderCompilerEvent"}
{}

GLRenderer::~GLRenderer()
{
	deinit();
}

void GLRenderer::deinit()
{
	glDpy.deinit();
}

Renderer::Renderer(RendererConfig config, Error &err): GLRenderer{Init{}}
{
	auto [ec, dpy] = Base::GLDisplay::makeDefault(glAPI);
	if(ec)
	{
		logErr("error getting GL display");
		err = std::runtime_error("error creating GL display");
		return;
	}
	glDpy = dpy;
	dpy.logInfo();
	{
		auto [mainContext, ec] = makeGLContext(dpy, config.pixelFormat());
		if(!mainContext)
		{
			err = std::runtime_error("error creating GL context");
			return;
		}
		constexpr int DRAW_THREAD_PRIORITY = -4;
		mainTask = std::make_unique<RendererTask>("Main GL Context Messages", mainContext, DRAW_THREAD_PRIORITY);
	}
	addEventHandlers(*mainTask);
}

Renderer::Renderer(Error &err): Renderer({Base::Window::defaultPixelFormat()}, err) {}

Renderer::Renderer(Renderer &&o)
{
	*this = std::move(o);
}

Renderer &Renderer::operator=(Renderer &&o)
{
	deinit();
	GLRenderer::operator=(std::move(o));
	if(mainTask)
		mainTask->setRenderer(this);
	o.glDpy = {};
	return *this;
}

std::pair<Base::GLContext, IG::ErrorCode> GLRenderer::makeGLContext(Base::GLDisplay dpy, IG::PixelFormat pixelFormat)
{
	IG::ErrorCode ec{};
	if(!pixelFormat)
		pixelFormat = Base::Window::defaultPixelFormat();
	Base::GLBufferConfigAttributes glBuffAttr;
	glBuffAttr.setPixelFormat(pixelFormat);
	#if CONFIG_GFX_OPENGL_ES_MAJOR_VERSION == 1
	auto glAttr = makeGLContextAttributes(1, 0);
	auto [found, config] = glCtx.makeBufferConfig(dpy, glAttr, glBuffAttr);
	assert(found);
	gfxBufferConfig = config;
	Base::GLContext glCtx{dpy, glAttr, gfxBufferConfig, ec};
	#elif CONFIG_GFX_OPENGL_ES_MAJOR_VERSION > 1
	Base::GLContext glCtx{};
	if(CAN_USE_OPENGL_ES_3)
	{
		auto glAttr = makeGLContextAttributes(3, 0);
		auto [found, config] = glCtx.makeBufferConfig(dpy, glAttr, glBuffAttr);
		if(found)
		{
			gfxBufferConfig = config;
			glCtx = {dpy, glAttr, gfxBufferConfig, ec};
			glMajorVer = glAttr.majorVersion();
		}
	}
	if(!glCtx) // fall back to OpenGL ES 2.0
	{
		auto glAttr = makeGLContextAttributes(2, 0);
		auto [found, config] = glCtx.makeBufferConfig(dpy, glAttr, glBuffAttr);
		assert(found);
		gfxBufferConfig = config;
		glCtx = {dpy, glAttr, gfxBufferConfig, ec};
		glMajorVer = glAttr.majorVersion();
	}
	#else
	Base::GLContext glCtx{};
	if(Config::Gfx::OPENGL_SHADER_PIPELINE)
	{
		#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
		support.useFixedFunctionPipeline = false;
		#endif
		auto glAttr = makeGLContextAttributes(3, 3);
		auto [found, config] = glCtx.makeBufferConfig(dpy, glAttr, glBuffAttr);
		assert(found);
		gfxBufferConfig = config;
		glCtx = {dpy, glAttr, gfxBufferConfig, ec};
		if(!glCtx)
		{
			logMsg("3.3 context not supported");
		}
	}
	if(Config::Gfx::OPENGL_FIXED_FUNCTION_PIPELINE && !glCtx)
	{
		#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
		support.useFixedFunctionPipeline = true;
		#endif
		auto glAttr = makeGLContextAttributes(1, 3);
		auto [found, config] = glCtx.makeBufferConfig(dpy, glAttr, glBuffAttr);
		assert(found);
		gfxBufferConfig = config;
		glCtx = {dpy, glAttr, gfxBufferConfig, ec};
		if(!glCtx)
		{
			logMsg("1.3 context not supported");
		}
	}
	#endif
	finishContextCreation(glCtx);
	return {glCtx, ec};
}

std::pair<Base::GLContext, IG::ErrorCode> GLRenderer::makeGLContextWithKnownConfig(Base::GLDisplay dpy, Base::GLContext shareContext)
{
	assert(mainTask->glContext());
	IG::ErrorCode ec{};
	Base::GLContext glCtx{dpy, makeKnownGLContextAttributes(), gfxBufferConfig, shareContext, ec};
	finishContextCreation(glCtx);
	return {glCtx, ec};
}

void Renderer::configureRenderer()
{
	task().runSync(
		[this]()
		{
			auto version = (const char*)glGetString(GL_VERSION);
			assert(version);
			auto rendererName = (const char*)glGetString(GL_RENDERER);
			logMsg("version: %s (%s)", version, rendererName);

			int glVer = glVersionFromStr(version);

			#ifdef CONFIG_BASE_GL_PLATFORM_EGL
			if constexpr(Config::Gfx::OPENGL_ES)
			{
				auto extStr = glDpy.queryExtensions();
				setupEglFenceSync(extStr);
				setupPresentationTime(extStr);
			}
			#endif

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
			if(glVer >= 32 && !Config::Base::GL_PLATFORM_EGL)
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
					support.glFlushMappedBufferRange = (typeof(support.glFlushMappedBufferRange))Base::GLContext::procAddress("glFlushMappedBufferRange");
					setupImmutableTexStorage(false);
					setupTextureSwizzle();
					setupRGFormats();
					setupSamplerObjects();
					setupPBO();
					if(!Config::Base::GL_PLATFORM_EGL)
						setupFenceSync();
					if(!Config::envIsIOS)
						setupSpecifyDrawReadBuffers();
					support.hasUnpackRowLength = true;
					support.useLegacyGLSL = false;
				}
				if(glVer >= 31)
				{
					setupMemoryBarrier();
				}
			}

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

			printFeatures(support);
		});
	if(Config::DEBUG_BUILD && defaultToFullErrorChecks)
	{
		setCorrectnessChecks(true);
		setDebugOutput(true);
	}
	support.isConfigured = true;
}

bool Renderer::isConfigured() const
{
	return support.isConfigured;
}

RendererTask &Renderer::task() const
{
	return *mainTask;
}

std::pair<Renderer, Error> Renderer::makeConfiguredRenderer(RendererConfig config)
{
	if(config.pixelFormat() == PIXEL_FMT_NONE)
		config.setPixelFormat(Base::Window::defaultPixelFormat());
	Error err;
	auto renderer = Renderer{config, err};
	if(err)
		return {std::move(renderer), err};
	renderer.configureRenderer();
	return {std::move(renderer), Error{}};
}

Base::WindowConfig Renderer::addWindowConfig(Base::WindowConfig config) const
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

void Renderer::setWindowValidOrientations(Base::Window &win, Base::Orientation validO)
{
	if(win != Base::mainWindow())
		return;
	auto oldWinO = win.softOrientation();
	if(win.setValidOrientations(validO) && !Config::SYSTEM_ROTATES_WINDOWS)
	{
		animateProjectionMatrixRotation(win, orientationToGC(oldWinO), orientationToGC(win.softOrientation()));
	}
	updateSensorStateForWindowOrientations(win);
}

void GLRenderer::addEventHandlers(RendererTask &task)
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	releaseShaderCompilerEvent.attach(
		[&task]()
		{
			if(!Base::appIsRunning())
				return;
			logMsg("automatically releasing shader compiler");
			task.releaseShaderCompiler();
		});
	#endif
	if constexpr(Config::envIsIOS)
		task.setIOSDrawableDelegates();
}

}
