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
#include <imagine/util/time/sys.hh>
#include "private.hh"
#include "settings.h"
#include "utils.h"
#include "GLStateCache.hh"
#ifdef __ANDROID__
#include "../../base/android/android.hh"
#endif

#ifndef GL_KHR_debug
typedef void (GL_APIENTRY *GLDEBUGPROCKHR)(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, const void *);
#define GL_DEBUG_OUTPUT_KHR 0x92E0
#endif

#ifndef CONFIG_GFX_OPENGL_ES
#define GL_APIENTRY APIENTRY
#endif

namespace Gfx
{

GLint projectionUniform, modelViewUniform, textureUniform;

GLfloat maximumAnisotropy, anisotropy = 0, forceAnisotropy = 0;
bool useAnisotropicFiltering = false;
static bool forceNoAnisotropicFiltering = true;

static bool useMultisample = false;
static bool forceNoMultisample = true;
static bool forceNoMultisampleHint = false;

static bool forceNoNonPow2Textures = false;

#ifdef CONFIG_GFX_OPENGL_ES
bool supportBGRPixels = false;
static bool useBGRPixels = false;
static bool forceNoBGRPixels = true;
bool preferBGRA = false, preferBGR = false;
#else
static bool forceNoBGRPixels = true;
bool supportBGRPixels = !forceNoBGRPixels;
bool preferBGRA = !forceNoBGRPixels, preferBGR = !forceNoBGRPixels;
#endif
GLenum bgrInternalFormat = GL_BGRA;

bool useFBOFuncs = false;
using GenerateMipmapsProto = void (*)(GLenum target);
#if defined CONFIG_GFX_OPENGL_ES && CONFIG_GFX_OPENGL_ES_MAJOR_VERSION > 1
GenerateMipmapsProto generateMipmaps = glGenerateMipmap;
#else
GenerateMipmapsProto generateMipmaps{}; // set via extensions
#endif

bool useVBOFuncs = false;
#if defined CONFIG_GFX_OPENGL_ES || !defined CONFIG_GFX_OPENGL_SHADER_PIPELINE
static bool forceNoVBOFuncs = true;
#else
static bool forceNoVBOFuncs = false;
#endif

static GLuint globalStreamVAO = 0;
GLuint globalStreamVBO[4]{};
uint globalStreamVBOIdx = 0;

bool forceNoTextureSwizzle = false;
bool useTextureSwizzle = false;

bool useUnpackRowLength = !Config::Gfx::OPENGL_ES;

bool useSamplerObjects = !Config::Gfx::OPENGL_ES;
#ifdef CONFIG_GFX_OPENGL_ES
GL_APICALL void (* GL_APIENTRY glGenSamplers) (GLsizei count, GLuint* samplers){};
GL_APICALL void (* GL_APIENTRY glDeleteSamplers) (GLsizei count, const GLuint* samplers){};
GL_APICALL GLboolean (* GL_APIENTRY glIsSampler) (GLuint sampler){};
GL_APICALL void (* GL_APIENTRY glBindSampler) (GLuint unit, GLuint sampler){};
GL_APICALL void (* GL_APIENTRY glSamplerParameteri) (GLuint sampler, GLenum pname, GLint param){};
#endif

GLenum luminanceFormat = GL_LUMINANCE;
GLenum luminanceInternalFormat = GL_LUMINANCE8;
GLenum luminanceAlphaFormat = GL_LUMINANCE_ALPHA;
GLenum luminanceAlphaInternalFormat = GL_LUMINANCE8_ALPHA8;
GLenum alphaFormat = GL_ALPHA;
GLenum alphaInternalFormat = GL_ALPHA8;

bool useImmutableTexStorage = false;
#ifdef CONFIG_GFX_OPENGL_ES
GL_APICALL void (* GL_APIENTRY glTexStorage2D) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height){};
#endif

bool useLegacyGLSL = Config::Gfx::OPENGL_ES;

static Base::GLBufferConfig gfxBufferConfig;

static Gfx::GC orientationToGC(uint o)
{
	using namespace Base;
	switch(o)
	{
		case VIEW_ROTATE_0: return Gfx::angleFromDegree(0.);
		case VIEW_ROTATE_90: return Gfx::angleFromDegree(-90.);
		case VIEW_ROTATE_180: return Gfx::angleFromDegree(-180.);
		case VIEW_ROTATE_270: return Gfx::angleFromDegree(90.);
		default: bug_branch("%d", o); return 0.;
	}
}

static void setupAnisotropicFiltering()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnisotropy);
	logMsg("anisotropic filtering supported, max value: %f", (double)maximumAnisotropy);
	useAnisotropicFiltering = 1;

	if(forceAnisotropy) // force a specific anisotropy value
		anisotropy = forceAnisotropy;
	else anisotropy = maximumAnisotropy;
	assert(anisotropy <= maximumAnisotropy);
	#endif
}

static void setupMultisample()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	logMsg("multisample antialiasing supported");
	useMultisample = 1;
	glcEnable(GL_MULTISAMPLE);
	#endif
}

static void setupMultisampleHints()
{
	#if !defined CONFIG_GFX_OPENGL_ES && !defined __APPLE__
	logMsg("multisample hints supported");
	glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
	#endif
}

static void setupNonPow2Textures()
{
	if(!Gfx::textureSizeSupport.nonPow2)
		logMsg("Non-Power-of-2 textures supported");
	Gfx::textureSizeSupport.nonPow2 = true;
}

static void setupNonPow2MipmapTextures()
{
	if(!Gfx::textureSizeSupport.nonPow2CanMipmap)
		logMsg("Non-Power-of-2 textures with mipmaps supported");
	Gfx::textureSizeSupport.nonPow2 = true;
	Gfx::textureSizeSupport.nonPow2CanMipmap = true;
}

static void setupNonPow2MipmapRepeatTextures()
{
	if(!Gfx::textureSizeSupport.nonPow2CanRepeat)
		logMsg("Non-Power-of-2 textures with mipmaps & repeat modes supported");
	Gfx::textureSizeSupport.nonPow2 = true;
	Gfx::textureSizeSupport.nonPow2CanMipmap = true;
	Gfx::textureSizeSupport.nonPow2CanRepeat = true;
}

#ifdef CONFIG_GFX_OPENGL_ES
static void setupBGRPixelSupport()
{
	supportBGRPixels = 1;
	useBGRPixels = 1;

	preferBGR = 1;
	preferBGRA = 1;

	logMsg("BGR pixel types are supported%s", bgrInternalFormat == GL_RGBA ? " (Apple version)" : "");
}
#endif

static void setupFBOFuncs()
{
	useFBOFuncs = true;
	#if defined CONFIG_GFX_OPENGL_ES && CONFIG_GFX_OPENGL_ES_MAJOR_VERSION == 1
	generateMipmaps = glGenerateMipmapOES;
	#elif !defined CONFIG_GFX_OPENGL_ES
	generateMipmaps = glGenerateMipmap;
	#endif
	logMsg("FBO functions are supported");
}

static void setupVAOFuncs()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	logMsg("using VAOs");
	glGenVertexArrays(1, &globalStreamVAO);
	glBindVertexArray(globalStreamVAO);
	#endif
}

static void setupVBOFuncs()
{
	logMsg("using VBOs");
	useVBOFuncs = 1;
	glGenBuffers(sizeofArray(globalStreamVBO), globalStreamVBO);
	/*iterateTimes(sizeofArray(globalStreamVBO), i)
	{
		glcBindBuffer(GL_ARRAY_BUFFER, globalStreamVBO[i]);
		//glBufferData(GL_ARRAY_BUFFER, 64, nullptr, GL_DYNAMIC_DRAW);
		glBufferData(GL_ARRAY_BUFFER, 64*32, nullptr, GL_DYNAMIC_DRAW);
	}*/
	//logMsg("created global stream VBO: %d", globalStreamVBO);
}

static void setupTextureSwizzle()
{
	if(forceNoTextureSwizzle || useTextureSwizzle)
		return;
	logMsg("using texture swizzling");
	useTextureSwizzle = true;
}

static void setupImmutableTexStorage()
{
	if(useImmutableTexStorage)
		return;
	logMsg("using immutable texture storage");
	useImmutableTexStorage = true;
	#ifdef CONFIG_GFX_OPENGL_ES
	glTexStorage2D = (typeof(glTexStorage2D))Base::GLContext::procAddress("glTexStorage2D");
	#endif
}

static void GL_APIENTRY debugCallback(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	logErr("Debug Info: %s", message);
}

static void checkExtensionString(const char *extStr)
{
	//logMsg("checking %s", extStr);
	if(string_equal(extStr, "GL_ARB_texture_non_power_of_two")
		|| (Config::Gfx::OPENGL_ES && string_equal(extStr, "GL_OES_texture_npot")))
	{
		 // allows mipmaps and repeat modes
		if(!forceNoNonPow2Textures)
			setupNonPow2MipmapRepeatTextures();
	}
	else if(Config::Gfx::OPENGL_ES && !Config::envIsIOS && string_equal(extStr, "GL_NV_texture_npot_2D_mipmap"))
	{
		// no repeat modes
		if(!forceNoNonPow2Textures)
			setupNonPow2MipmapTextures();
	}
	else if(Config::Gfx::OPENGL_ES_MAJOR_VERSION == 1
		&& (string_equal(extStr, "GL_APPLE_texture_2D_limited_npot") || string_equal(extStr, "GL_IMG_texture_npot")))
	{
		// no mipmaps or repeat modes
		if(!forceNoNonPow2Textures)
			setupNonPow2Textures();
	}
	else if(Config::Gfx::OPENGL_ES_MAJOR_VERSION > 1 && string_equal(extStr, "GL_EXT_unpack_subimage"))
	{
		logMsg("unpacking sub-images supported");
		useUnpackRowLength = true;
	}
	else if((!Config::envIsIOS && !Config::envIsMacOSX) && Config::DEBUG_BUILD && string_equal(extStr, "GL_KHR_debug"))
	{
		#ifdef CONFIG_GFX_OPENGL_ES
		using DebugMessageCallbackProto = void (*)(GLDEBUGPROCKHR callback, const void *userParam);
		const char *glDebugMessageCallbackStr = "glDebugMessageCallbackKHR";
		const auto DEBUG_OUTPUT = GL_DEBUG_OUTPUT_KHR;
		#else
		using DebugMessageCallbackProto = void (*)(GLDEBUGPROC callback, const void *userParam);
		const char *glDebugMessageCallbackStr = "glDebugMessageCallback";
		const auto DEBUG_OUTPUT = GL_DEBUG_OUTPUT;
		#endif
		logMsg("enabling debug output");
		auto glDebugMessageCallback = (DebugMessageCallbackProto)Base::GLContext::procAddress(glDebugMessageCallbackStr);
		glDebugMessageCallback(debugCallback, nullptr);
		glEnable(DEBUG_OUTPUT);
	}
	#ifdef CONFIG_GFX_OPENGL_ES
	else if(string_equal(extStr, "GL_APPLE_texture_format_BGRA8888"))
	{
		if(!forceNoBGRPixels)
		{
			bgrInternalFormat = GL_RGBA;
			setupBGRPixelSupport();
		}
	}
	else if(string_equal(extStr, "GL_EXT_texture_format_BGRA8888"))
	{
		if(!forceNoBGRPixels)
			setupBGRPixelSupport();
	}
	else if(Config::Gfx::OPENGL_ES_MAJOR_VERSION == 1 && string_equal(extStr, "GL_OES_framebuffer_object"))
	{
		if(!useFBOFuncs)
			setupFBOFuncs();
	}
	else if(string_equal(extStr, "GL_EXT_texture_storage"))
	{
		setupImmutableTexStorage();
	}
	#endif
	#ifndef CONFIG_GFX_OPENGL_ES
	else if(string_equal(extStr, "GL_EXT_texture_filter_anisotropic"))
	{
		if(!forceNoAnisotropicFiltering)
			setupAnisotropicFiltering();
	}
	else if(string_equal(extStr, "GL_ARB_multisample"))
	{
		if(!forceNoMultisample && !useMultisample)
			setupMultisample();
	}
	else if(string_equal(extStr, "GL_NV_multisample_filter_hint"))
	{
		if(!forceNoMultisample && !forceNoMultisampleHint)
			setupMultisampleHints();
	}
	else if(string_equal(extStr, "GL_EXT_framebuffer_object"))
	{
		if(!useFBOFuncs)
		{
			setupFBOFuncs();
			generateMipmaps = glGenerateMipmapEXT;
		}
	}
	else if(string_equal(extStr, "GL_ARB_framebuffer_object"))
	{
		if(!useFBOFuncs)
			setupFBOFuncs();
	}
	else if(string_equal(extStr, "GL_ARB_texture_storage"))
	{
		setupImmutableTexStorage();
	}
	#endif
}

static void checkFullExtensionString(const char *fullExtStr)
{
	char fullExtStrTemp[strlen(fullExtStr)+1];
	strcpy(fullExtStrTemp, fullExtStr);
	char *savePtr;
	auto extStr = strtok_r(fullExtStrTemp, " ", &savePtr);
	while(extStr)
	{
		checkExtensionString(extStr);
		extStr = strtok_r(nullptr, " ", &savePtr);
	}
}

static int glVersionFromStr(const char *versionStr)
{
	// skip to version number
	while(!isdigit(*versionStr) && *versionStr != '\0')
		versionStr++;
	int major, minor;
	if(sscanf(versionStr, "%d.%d", &major, &minor) != 2)
	{
		bug_exit("unable to parse GL version string");
	}
	return 10 * major + minor;
}

CallResult init()
{
	return init(defaultColorBits());
}

CallResult init(uint colorBits)
{
	logMsg("running init");

	if(!Config::SYSTEM_ROTATES_WINDOWS)
	{
		Base::setOnDeviceOrientationChanged(
			[](uint newO)
			{
				auto &win = Base::mainWindow();
				auto oldWinO = win.softOrientation();
				if(win.requestOrientationChange(newO))
				{
					Gfx::animateProjectionMatrixRotation(Gfx::orientationToGC(oldWinO), Gfx::orientationToGC(newO));
				}
			});
	}
	else if(Config::SYSTEM_ROTATES_WINDOWS && !Base::Window::systemAnimatesRotation())
	{
		Base::setOnSystemOrientationChanged(
			[](uint oldO, uint newO) // TODO: parameters need proper type definitions in API
			{
				const Angle orientationDiffTable[4][4]
				{
					{0, angleFromDegree(90), angleFromDegree(-180), angleFromDegree(-90)},
					{angleFromDegree(-90), 0, angleFromDegree(90), angleFromDegree(-180)},
					{angleFromDegree(-180), angleFromDegree(-90), 0, angleFromDegree(90)},
					{angleFromDegree(90), angleFromDegree(-180), angleFromDegree(-90), 0},
				};
				auto rotAngle = orientationDiffTable[oldO][newO];
				logMsg("animating from %d degrees", (int)angleToDegree(rotAngle));
				Gfx::animateProjectionMatrixRotation(rotAngle, 0.);
			});
	}

	Base::setOnGLDrawableChanged(
		[](Base::Window *newDrawable)
		{
			// update the cached current window
			if(currWin != newDrawable)
			{
				logMsg("current window changed by event to %p", newDrawable);
				currWin = newDrawable;
			}
		});

	Base::GLBufferConfigAttributes glBuffAttr;
	glBuffAttr.setPreferredColorBits(colorBits);
	Base::GLContextAttributes glAttr;
	if(Config::DEBUG_BUILD)
		glAttr.setDebug(true);
	#if defined CONFIG_GFX_OPENGL_ES
	if(!Base::GLContext::bindAPI(Base::GLContext::OPENGL_ES_API))
	{
		bug_exit("unable to bind GLES API");
	}
	glAttr.setOpenGLESAPI(true);
	if(Config::Gfx::OPENGL_SHADER_PIPELINE)
		glAttr.setMajorVersion(2);
	gfxBufferConfig = gfxContext.makeBufferConfig(glAttr, glBuffAttr);
	gfxContext.init(glAttr, gfxBufferConfig);
	#else
	if(!Base::GLContext::bindAPI(Base::GLContext::OPENGL_API))
	{
		bug_exit("unable to bind GL API");
	}
	if(Config::Gfx::OPENGL_SHADER_PIPELINE)
	{
		useFixedFunctionPipeline = false;
		glAttr.setMajorVersion(3);
		glAttr.setMinorVersion(3);
		gfxBufferConfig = gfxContext.makeBufferConfig(glAttr, glBuffAttr);
		if(gfxContext.init(glAttr, gfxBufferConfig) != OK)
		{
			logMsg("3.3 context not supported");
		}
	}
	if(Config::Gfx::OPENGL_FIXED_FUNCTION_PIPELINE && !gfxContext)
	{
		useFixedFunctionPipeline = true;
		glAttr.setMajorVersion(1);
		glAttr.setMinorVersion(3);
		gfxBufferConfig = gfxContext.makeBufferConfig(glAttr, glBuffAttr);
		if(gfxContext.init(glAttr, gfxBufferConfig) != OK)
		{
			logMsg("1.3 context not supported");
		}
	}
	if(!gfxContext)
	{
		bug_exit("can't create context");
		return INVALID_PARAMETER;
	}
	#endif

	gfxContext.setCurrent(gfxContext, nullptr);

	if(checkGLErrorsVerbose)
		logMsg("using verbose error checks");
	else if(checkGLErrors)
		logMsg("using error checks");
	
	auto version = (const char*)glGetString(GL_VERSION);
	assert(version);
	auto rendererName = (const char*)glGetString(GL_RENDERER);
	logMsg("version: %s (%s)", version, rendererName);
	
	int glVer = glVersionFromStr(version);
	
	#ifndef CONFIG_GFX_OPENGL_ES
	// core functionality
	if(glVer >= 15)
	{
		if(!forceNoVBOFuncs)
			setupVBOFuncs();
	}
	if(glVer >= 20)
	{
		if(!forceNoNonPow2Textures)
			setupNonPow2MipmapRepeatTextures();
	}
	if(glVer >= 30)
	{
		if(!useFixedFunctionPipeline)
		{
			// must render via VAOs/VBOs in 3.1+ without compatibility context
			setupVAOFuncs();
			if(!useVBOFuncs)
				setupVBOFuncs();
			setupTextureSwizzle();
			luminanceFormat = GL_RED;
			luminanceInternalFormat = GL_R8;
			luminanceAlphaFormat = GL_RG;
			luminanceAlphaInternalFormat = GL_RG8;
			alphaFormat = GL_RED;
			alphaInternalFormat = GL_R8;
			useSamplerObjects = true;
		}
		setupFBOFuncs();
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
			checkExtensionString((const char*)glGetStringi(GL_EXTENSIONS, i));
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
		if(!forceNoVBOFuncs)
			setupVBOFuncs();
	}
	if(Config::Gfx::OPENGL_ES_MAJOR_VERSION > 1)
	{
		if(!forceNoNonPow2Textures)
		{
			if(glVer >= 30)
				setupNonPow2MipmapRepeatTextures();
			else
				setupNonPow2Textures();
		}
		setupFBOFuncs();
	}

	// extension functionality
	auto extensions = (const char*)glGetString(GL_EXTENSIONS);
	assert(extensions);
	logMsg("extensions: %s", extensions);
	checkFullExtensionString(extensions);
	#endif // CONFIG_GFX_OPENGL_ES

	setClearColor(0., 0., 0.);
	//glShadeModel(GL_SMOOTH);
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	setVisibleGeomFace(FRONT_FACES);

	GLint texSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
	textureSizeSupport.maxXSize = textureSizeSupport.maxYSize = texSize;
	assert(textureSizeSupport.maxXSize > 0 && textureSizeSupport.maxYSize > 0);
	logMsg("max texture size is %d", texSize);

	#if defined __ANDROID__
	setupAndroidOGLExtensions(extensions, rendererName);
	#endif

	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
		glcEnableClientState(GL_VERTEX_ARRAY);
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(!useFixedFunctionPipeline)
	{
		handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s before shaders", err); });
		logMsg("shader language version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
		initShaders();
	}
	#endif
	return OK;
}

uint defaultColorBits()
{
	return Base::GLBufferConfigAttributes::defaultColorBits();
}

Base::WindowConfig makeWindowConfig()
{
	Base::WindowConfig config;
	setWindowConfig(config);
	return config;
}

void setWindowConfig(Base::WindowConfig &config)
{
	config.setGLConfig(gfxBufferConfig);
}

static void updateSensorStateForWindowOrientations(Base::Window &win)
{
	// activate orientation sensor if doing rotation in software and the main window
	// has multiple valid orientations
	if(Config::SYSTEM_ROTATES_WINDOWS || win != Base::mainWindow())
		return;
	Base::setDeviceOrientationChangeSensor(bit_numSet(win.validSoftOrientations()) > 1);
}

void initWindow(Base::Window &win, Base::WindowConfig config)
{
	setWindowConfig(config);
	win.init(config);
	if(!Config::SYSTEM_ROTATES_WINDOWS && win == Base::mainWindow())
		Gfx::setProjectionMatrixRotation(Gfx::orientationToGC(win.softOrientation()));
	updateSensorStateForWindowOrientations(win);
}

void setWindowValidOrientations(Base::Window &win, uint validO)
{
	if(win != Base::mainWindow())
		return;
	auto oldWinO = win.softOrientation();
	if(win.setValidOrientations(validO) && !Config::SYSTEM_ROTATES_WINDOWS)
	{
		Gfx::animateProjectionMatrixRotation(Gfx::orientationToGC(oldWinO), Gfx::orientationToGC(win.softOrientation()));
	}
	updateSensorStateForWindowOrientations(win);
}

#ifdef __ANDROID__
static void setupAndroidOGLExtensions(const char *extensions, const char *rendererName)
{
	// TODO: make direct texture setup optional
	if(Base::androidSDK() < 14)
		directTextureConf.checkForEGLImageKHR(extensions, rendererName);
	if(surfaceTextureConf.isSupported())
	{
		if(!strstr(extensions, "GL_OES_EGL_image_external"))
		{
			logWarn("SurfaceTexture is supported but OpenGL extension missing, disabling");
			surfaceTextureConf.deinit();
		}
		else if(strstr(rendererName, "GC1000")) // Texture binding issues on Samsung Galaxy Tab 3 7.0 on Android 4.1
		{
			logWarn("buggy SurfaceTexture implementation on Vivante GC1000, disabling by default");
			surfaceTextureConf.use = surfaceTextureConf.whiteListed = 0;
		}
		else if(strstr(rendererName, "Adreno"))
		{
			if(strstr(rendererName, "200")) // Textures may stop updating on HTC EVO 4G (supersonic) on Android 4.1
			{
				logWarn("buggy SurfaceTexture implementation on Adreno 200, disabling by default");
				surfaceTextureConf.use = surfaceTextureConf.whiteListed = 0;
			}

			// When deleting a SurfaceTexture, Adreno 225 on Android 4.0 will unbind
			// the current GL_TEXTURE_2D texture, even though its state shouldn't change.
			// This hack will fix-up the GL state cache manually when that happens.
			// TODO: remove
			//logWarn("enabling SurfaceTexture GL_TEXTURE_2D binding hack");
			//surfaceTextureConf.texture2dBindingHack = 1;
		}
	}
}
#endif

}
