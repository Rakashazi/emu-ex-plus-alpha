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

#include <assert.h>
#include <imagine/gfx/Gfx.hh>
#include <imagine/base/Base.hh>
#include <imagine/base/Window.hh>
#include <imagine/util/string.h>
#include "private.hh"
#include "utils.h"

namespace Gfx
{

void GLRenderer::initVBOs()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	logMsg("making stream VBO");
	glGenBuffers(IG::size(streamVBO), streamVBO);
	#endif
}

GLuint GLRenderer::getVBO()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	assert(streamVBO[streamVBOIdx]);
	auto vbo = streamVBO[streamVBOIdx];
	streamVBOIdx = (streamVBOIdx+1) % IG::size(streamVBO);
	return vbo;
	#else
	return 0;
	#endif
}

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
	glGenVertexArrays(1, &streamVAO);
	glBindVertexArray(streamVAO);
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
	initTexturePBO();
}

void GLRenderer::setupSpecifyDrawReadBuffers()
{
	support.shouldSpecifyDrawReadBuffers = true;
	#ifdef CONFIG_GFX_OPENGL_ES
	support.glDrawBuffers = (typeof(support.glDrawBuffers))Base::GLContext::procAddress("glDrawBuffers");
	support.glReadBuffer = (typeof(support.glReadBuffer))Base::GLContext::procAddress("glReadBuffer");
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
	else if((!Config::envIsIOS && !Config::envIsMacOSX) && Config::DEBUG_BUILD && string_equal(extStr, "GL_KHR_debug"))
	{
		support.hasDebugOutput = true;
	}
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
	#if __ANDROID__
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

Renderer::Renderer() {}

Renderer::Renderer(IG::PixelFormat pixelFormat, Error &err)
{
	{
		std::error_code ec{};
		glDpy = Base::GLDisplay::makeDefault(ec);
		if(ec)
		{
			logErr("error getting GL display");
			err = std::runtime_error("error creating GL display");
			return;
		}
	}
	if(!pixelFormat)
		pixelFormat = Base::Window::defaultPixelFormat();
	Base::GLBufferConfigAttributes glBuffAttr;
	glBuffAttr.setPixelFormat(pixelFormat);
	Base::GLContextAttributes glAttr;
	if(Config::DEBUG_BUILD)
		glAttr.setDebug(true);
	#ifdef CONFIG_GFX_OPENGL_ES
	if(!Base::GLContext::bindAPI(Base::GLContext::OPENGL_ES_API))
	{
		logErr("unable to bind GLES API");
	}
	glAttr.setOpenGLESAPI(true);
	if(Config::Gfx::OPENGL_ES_MAJOR_VERSION == 1)
	{
		gfxBufferConfig = gfxContext.makeBufferConfig(glDpy, glAttr, glBuffAttr);
		std::error_code ec{};
		gfxContext = {glDpy, glAttr, gfxBufferConfig, ec};
	}
	else
	{
		glAttr.setMajorVersion(3);
		gfxBufferConfig = gfxContext.makeBufferConfig(glDpy, glAttr, glBuffAttr);
		std::error_code ec{};
		if(gfxBufferConfig)
		{
			gfxContext = {glDpy, glAttr, gfxBufferConfig, ec};
		}
		if(!gfxContext)
		{
			glAttr.setMajorVersion(2);
			gfxBufferConfig = gfxContext.makeBufferConfig(glDpy, glAttr, glBuffAttr);
			gfxContext = {glDpy, glAttr, gfxBufferConfig, ec};
		}
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
		glAttr.setMajorVersion(3);
		glAttr.setMinorVersion(3);
		gfxBufferConfig = gfxContext.makeBufferConfig(glDpy, glAttr, glBuffAttr);
		std::error_code ec{};
		gfxContext = {glDpy, glAttr, gfxBufferConfig, ec};
		if(!gfxContext)
		{
			logMsg("3.3 context not supported");
		}
	}
	if(Config::Gfx::OPENGL_FIXED_FUNCTION_PIPELINE && !gfxContext)
	{
		#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
		support.useFixedFunctionPipeline = true;
		#endif
		glAttr.setMajorVersion(1);
		glAttr.setMinorVersion(3);
		gfxBufferConfig = gfxContext.makeBufferConfig(glDpy, glAttr, glBuffAttr);
		std::error_code ec{};
		gfxContext = {glDpy, glAttr, gfxBufferConfig, ec};
		if(!gfxContext)
		{
			logMsg("1.3 context not supported");
		}
	}
	#endif
	if(!gfxContext)
	{
		err = std::runtime_error("error creating GL context");
		return;
	}
	err = {};
}

Renderer::Renderer(Error &err): Renderer(Base::Window::defaultPixelFormat(), err) {}

void Renderer::configureRenderer()
{
	verifyCurrentContext();

	if(checkGLErrorsVerbose)
		logMsg("using verbose error checks");
	else if(checkGLErrors)
		logMsg("using error checks");
	
	auto version = (const char*)glGetString(GL_VERSION);
	assert(version);Renderer();
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
			if(!Config::envIsIOS)
				setupSpecifyDrawReadBuffers();
			support.hasUnpackRowLength = true;
			support.useLegacyGLSL = false;
		}
	}

	// extension functionality
	auto extensions = (const char*)glGetString(GL_EXTENSIONS);
	assert(extensions);
	logMsg("extensions: %s", extensions);
	checkFullExtensionString(extensions);
	#endif // CONFIG_GFX_OPENGL_ES

	if(support.hasVBOFuncs)
		initVBOs();
	setClearColor(0., 0., 0.);
	setVisibleGeomFace(FRONT_FACES);

	GLint texSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
	support.textureSizeSupport.maxXSize = support.textureSizeSupport.maxYSize = texSize;
	assert(support.textureSizeSupport.maxXSize > 0 && support.textureSizeSupport.maxYSize > 0);
	logMsg("max texture size is %d", texSize);

	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(support.useFixedFunctionPipeline)
		glcEnableClientState(GL_VERTEX_ARRAY);
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(!support.useFixedFunctionPipeline)
	{
		handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s before shaders", err); });
		if(Config::DEBUG_BUILD)
			logMsg("shader language version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
		initShaders(*this);
	}
	#endif

	support.isConfigured = true;
}

bool Renderer::isConfigured() const
{
	return support.isConfigured;
}

Renderer Renderer::makeConfiguredRenderer(IG::PixelFormat pixelFormat, Error &err)
{
	auto renderer = Renderer{pixelFormat, err};
	if(err)
		return {};
	renderer.bind();
	renderer.configureRenderer();
	return renderer;
}

Renderer Renderer::makeConfiguredRenderer(Error &err)
{
	return makeConfiguredRenderer(Base::Window::defaultPixelFormat(), err);
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

void Renderer::setDebugOutput(bool on)
{
	if(!support.hasDebugOutput)
	{
		return;
	}
	verifyCurrentContext();
	if(!on)
	{
		glDisable(support.DEBUG_OUTPUT);
	}
	else
	{
		if(!support.glDebugMessageCallback)
		{
			auto glDebugMessageCallbackStr =
					Config::Gfx::OPENGL_ES ? "glDebugMessageCallbackKHR" : "glDebugMessageCallback";
			logMsg("enabling debug output with %s", glDebugMessageCallbackStr);
			support.glDebugMessageCallback = (typeof(support.glDebugMessageCallback))Base::GLContext::procAddress(glDebugMessageCallbackStr);
			support.glDebugMessageCallback(
				GL_APIENTRY [](GLenum source, GLenum type, GLuint id,
					GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
				{
					logger_printfn(LOG_D, "GL Debug: %s", message);
				}, nullptr);
		}
		glEnable(support.DEBUG_OUTPUT);
	}
}

}
