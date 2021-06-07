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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Window.hh>
#include <imagine/util/string.h>
#include <imagine/fs/FS.hh>
#include "internalDefs.hh"
#include "utils.hh"
#ifdef __ANDROID__
#include "../../base/android/android.hh"
#include "android/egl.hh"
#endif
#include <string>

namespace Gfx
{

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
		featuresStr.append(" [BGRA Format]");
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
	if(Config::Gfx::OPENGL_ES >= 2 && support.hasUnpackRowLength)
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
	if(support.hasSrgbWriteControl)
	{
		featuresStr.append(" [sRGB FB Write Control]");
	}
	#ifdef __ANDROID__
	if(support.eglPresentationTimeANDROID)
	{
		featuresStr.append(" [Presentation Time]");
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
EGLImageKHR makeAndroidNativeBufferEGLImage(EGLDisplay dpy, EGLClientBuffer clientBuff, bool srgb)
{
	EGLint eglImgAttrs[]
	{
		EGL_IMAGE_PRESERVED_KHR,
		EGL_TRUE,
		srgb ? EGL_GL_COLORSPACE : EGL_NONE,
		srgb ? EGL_GL_COLORSPACE_SRGB : EGL_NONE,
		EGL_NONE
	};
	return eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
		clientBuff, eglImgAttrs);
}
#endif

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

void GLRenderer::setupBGRPixelSupport()
{
	support.hasBGRPixels = true;
}

void GLRenderer::setupFBOFuncs(bool &useFBOFuncs)
{
	useFBOFuncs = true;
	#if defined CONFIG_GFX_OPENGL_ES && CONFIG_GFX_OPENGL_ES_MAJOR_VERSION == 1
	support.generateMipmaps = glGenerateMipmapOES;
	#elif !defined CONFIG_GFX_OPENGL_ES
	support.generateMipmaps = glGenerateMipmap;
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
	support.glTexStorage2D = (typeof(support.glTexStorage2D))glManager.procAddress(procName);
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
	support.glGenSamplers = (typeof(support.glGenSamplers))glManager.procAddress("glGenSamplers");
	support.glDeleteSamplers = (typeof(support.glDeleteSamplers))glManager.procAddress("glDeleteSamplers");
	support.glBindSampler = (typeof(support.glBindSampler))glManager.procAddress("glBindSampler");
	support.glSamplerParameteri = (typeof(support.glSamplerParameteri))glManager.procAddress("glSamplerParameteri");
	#endif
}

void GLRenderer::setupPBO()
{
	support.hasPBOFuncs = true;
}

void GLRenderer::setupSpecifyDrawReadBuffers()
{
	#ifdef CONFIG_GFX_OPENGL_ES
	//support.glDrawBuffers = (typeof(support.glDrawBuffers))glManager.procAddress("glDrawBuffers");
	//support.glReadBuffer = (typeof(support.glReadBuffer))glManager.procAddress("glReadBuffer");
	#endif
}

bool DrawContextSupport::hasDrawReadBuffers() const
{
	#ifdef CONFIG_GFX_OPENGL_ES
	return false; //glDrawBuffers;
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
			if constexpr((bool)Config::Gfx::OPENGL_ES)
			{
				support.glUnmapBuffer = (typeof(support.glUnmapBuffer))glManager.procAddress("glUnmapBufferOES");
			}
			else
			{
				support.glUnmapBuffer = (typeof(support.glUnmapBuffer))glManager.procAddress("glUnmapBuffer");
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
	support.glBufferStorage = (typeof(support.glBufferStorage))glManager.procAddress("glBufferStorageEXT");
	#else
	support.hasBufferStorage = true;
	#endif
}

void GLRenderer::setupMemoryBarrier()
{
	/*if(support.hasMemoryBarriers())
		return;
	#ifdef CONFIG_GFX_OPENGL_ES
	support.glMemoryBarrier = (typeof(support.glMemoryBarrier))glManager.procAddress("glMemoryBarrier");
	#else
	support.hasMemoryBarrier = true;
	#endif*/
}

void GLRenderer::setupPresentationTime(const char *eglExtenstionStr)
{
	#ifdef __ANDROID__
	if(strstr(eglExtenstionStr, "EGL_ANDROID_presentation_time"))
	{
		glManager.loadSymbol(support.eglPresentationTimeANDROID, "eglPresentationTimeANDROID");
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
		if(mainTask.appContext().androidSDK() < 23)
		{
			support.hasDebugOutput = false;
		}
		#endif
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_ES
	else if(Config::Gfx::OPENGL_ES == 1
		&& (string_equal(extStr, "GL_APPLE_texture_2D_limited_npot") || string_equal(extStr, "GL_IMG_texture_npot")))
	{
		// no mipmaps or repeat modes
		setupNonPow2Textures();
	}
	else if(Config::Gfx::OPENGL_ES >= 2
		&& !Config::envIsIOS && string_equal(extStr, "GL_NV_texture_npot_2D_mipmap"))
	{
		// no repeat modes
		setupNonPow2MipmapTextures();
	}
	else if(Config::Gfx::OPENGL_ES >= 2 && string_equal(extStr, "GL_EXT_unpack_subimage"))
	{
		support.hasUnpackRowLength = true;
	}
	else if((!Config::envIsIOS && string_equal(extStr, "GL_EXT_texture_format_BGRA8888"))
			|| (Config::envIsIOS && string_equal(extStr, "GL_APPLE_texture_format_BGRA8888")))
	{
		setupBGRPixelSupport();
	}
	else if(Config::Gfx::OPENGL_ES == 1 && string_equal(extStr, "GL_OES_framebuffer_object"))
	{
		if(!useFBOFuncs)
			setupFBOFuncs(useFBOFuncs);
	}
	else if(string_equal(extStr, "GL_EXT_texture_storage"))
	{
		setupImmutableTexStorage(true);
	}
	else if(!Config::Base::GL_PLATFORM_EGL && Config::envIsIOS && string_equal(extStr, "GL_APPLE_sync"))
	{
		setupAppleFenceSync();
	}
	else if(Config::envIsAndroid && string_equal(extStr, "GL_OES_EGL_image"))
	{
		support.hasEGLImages = true;
	}
	else if(Config::Gfx::OPENGL_ES >= 2 &&
		Config::Gfx::OPENGL_TEXTURE_TARGET_EXTERNAL &&
		string_equal(extStr, "GL_OES_EGL_image_external"))
	{
		support.hasExternalEGLImages = true;
	}
	#ifdef __ANDROID__
	else if(Config::Gfx::OPENGL_ES >= 2 &&
		string_equal(extStr, "GL_EXT_EGL_image_storage"))
	{
		support.glEGLImageTargetTexStorageEXT = (typeof(support.glEGLImageTargetTexStorageEXT))glManager.procAddress("glEGLImageTargetTexStorageEXT");
	}
	#endif
	else if(Config::Gfx::OPENGL_ES >= 2 && string_equal(extStr, "GL_NV_pixel_buffer_object"))
	{
		setupPBO();
	}
	else if(Config::Gfx::OPENGL_ES >= 2 && string_equal(extStr, "GL_NV_map_buffer_range"))
	{
		if(!support.glMapBufferRange)
			support.glMapBufferRange = (typeof(support.glMapBufferRange))glManager.procAddress("glMapBufferRangeNV");
		setupUnmapBufferFunc();
	}
	else if(string_equal(extStr, "GL_EXT_map_buffer_range"))
	{
		if(!support.glMapBufferRange)
			support.glMapBufferRange = (typeof(support.glMapBufferRange))glManager.procAddress("glMapBufferRangeEXT");
		// Only using ES 3.0 version currently
		//if(!support.glFlushMappedBufferRange)
		//	support.glFlushMappedBufferRange = (typeof(support.glFlushMappedBufferRange))glManager.procAddress("glFlushMappedBufferRangeEXT");
		setupUnmapBufferFunc();
	}
	else if(Config::Gfx::OPENGL_ES >= 2 && string_equal(extStr, "GL_EXT_buffer_storage"))
	{
		setupImmutableBufferStorage();
	}
	/*else if(string_equal(extStr, "GL_OES_mapbuffer"))
	{
		// handled in *_map_buffer_range currently
	}*/
	else if(Config::Gfx::OPENGL_ES >= 2 && string_equal(extStr, "GL_EXT_sRGB_write_control"))
	{
		support.hasSrgbWriteControl = true;
	}
	#endif
	#ifndef CONFIG_GFX_OPENGL_ES
	/*else if(string_equal(extStr, "GL_EXT_texture_filter_anisotropic"))
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
	}*/
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

void Renderer::configureRenderer()
{
	if(Config::DEBUG_BUILD && defaultToFullErrorChecks)
	{
		setCorrectnessChecks(true);
	}
	task().runSync(
		[this](GLTask::TaskContext ctx)
		{
			auto version = (const char*)glGetString(GL_VERSION);
			assert(version);
			auto rendererName = (const char*)glGetString(GL_RENDERER);
			logMsg("version: %s (%s)", version, rendererName);

			int glVer = glVersionFromStr(version);

			#ifdef CONFIG_BASE_GL_PLATFORM_EGL
			if constexpr((bool)Config::Gfx::OPENGL_ES)
			{
				auto extStr = ctx.glDisplay().queryExtensions();
				setupEglFenceSync(extStr);
				setupPresentationTime(extStr);
			}
			#endif

			bool useFBOFuncs = false;
			#ifndef CONFIG_GFX_OPENGL_ES
			// core functionality
			support.useFixedFunctionPipeline = glVer < 33;
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
					//setupVAOFuncs();
					setupTextureSwizzle();
					setupRGFormats();
					setupSamplerObjects();
				}
				setupFBOFuncs(useFBOFuncs);
				support.hasSrgbWriteControl = true;
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
			if(Config::Gfx::OPENGL_ES == 1 && glVer >= 11)
			{
				// safe to use VBOs
			}
			if(Config::Gfx::OPENGL_ES > 1)
			{
				if(glVer >= 30)
					setupNonPow2MipmapRepeatTextures();
				else
					setupNonPow2Textures();
				setupFBOFuncs(useFBOFuncs);
				if(glVer >= 30)
				{
					support.glMapBufferRange = (typeof(support.glMapBufferRange))glManager.procAddress("glMapBufferRange");
					support.glUnmapBuffer = (typeof(support.glUnmapBuffer))glManager.procAddress("glUnmapBuffer");
					support.glFlushMappedBufferRange = (typeof(support.glFlushMappedBufferRange))glManager.procAddress("glFlushMappedBufferRange");
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
			task().runInitialCommandsInGL(ctx, support);
		});
	support.isConfigured = true;
}

bool Renderer::isConfigured() const
{
	return support.isConfigured;
}

const RendererTask &Renderer::task() const
{
	return mainTask;
}

RendererTask &Renderer::task()
{
	return mainTask;
}

static void updateSensorStateForWindowOrientations(Base::Window &win)
{
	// activate orientation sensor if doing rotation in software and the main window
	// has multiple valid orientations
	if(Config::SYSTEM_ROTATES_WINDOWS || !win.isMainWindow())
		return;
	win.appContext().setDeviceOrientationChangeSensor(std::popcount(win.validSoftOrientations()) > 1);
}

void Renderer::setWindowValidOrientations(Base::Window &win, Base::Orientation validO)
{
	if(!win.isMainWindow())
		return;
	auto oldWinO = win.softOrientation();
	if(win.setValidOrientations(validO) && !Config::SYSTEM_ROTATES_WINDOWS)
	{
		animateProjectionMatrixRotation(win, orientationToGC(oldWinO), orientationToGC(win.softOrientation()));
	}
	updateSensorStateForWindowOrientations(win);
}

void GLRenderer::addEventHandlers(Base::ApplicationContext ctx, RendererTask &task)
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	releaseShaderCompilerEvent.attach(
		[&task, ctx]()
		{
			if(!ctx.isRunning())
				return;
			logMsg("automatically releasing shader compiler");
			task.releaseShaderCompiler();
		});
	#endif
	if constexpr(Config::envIsIOS)
		task.setIOSDrawableDelegates();
}

std::optional<Base::GLBufferConfig> GLRenderer::makeGLBufferConfig(Base::ApplicationContext ctx, IG::PixelFormat pixelFormat, const Base::Window *winPtr)
{
	if(!pixelFormat)
	{
		if(winPtr)
			pixelFormat = winPtr->pixelFormat();
		else
			pixelFormat = ctx.defaultWindowPixelFormat();
	}
	Base::GLBufferConfigAttributes glBuffAttr{pixelFormat};
	if constexpr(Config::Gfx::OPENGL_ES >= 2)
	{
		if(auto config = glManager.makeBufferConfig(ctx, glBuffAttr, glAPI, 3);
			config)
		{
			return config;
		}
		// fall back to OpenGL ES 2.0
		return glManager.makeBufferConfig(ctx, glBuffAttr, glAPI, 2);
	}
	else
	{
		// OpenGL ES 1.0 or full OpenGL
		return glManager.makeBufferConfig(ctx, glBuffAttr, glAPI);
	}
}

}
