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
#include "private.hh"
#include "utils.h"
#include "GLStateCache.hh"

#ifndef GL_KHR_debug
typedef void (GL_APIENTRY *GLDEBUGPROCKHR)(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, const void *);
using GLDEBUGPROC = GLDEBUGPROCKHR;
#define GL_DEBUG_OUTPUT_KHR 0x92E0
#define GL_DEBUG_OUTPUT 0x92E0
#endif

namespace Gfx
{

GLint projectionUniform, modelViewUniform, textureUniform;

bool useAnisotropicFiltering = false;

static bool useMultisample = false;
static bool forceNoMultisample = true;
static bool forceNoMultisampleHint = false;

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

static GLuint streamVAO = 0;
static GLuint streamVBO[6]{};
static uint streamVBOIdx = 0;

bool useTextureSwizzle = false;

bool useUnpackRowLength = !Config::Gfx::OPENGL_ES;

bool useSamplerObjects = !Config::Gfx::OPENGL_ES;
#ifdef CONFIG_GFX_OPENGL_ES
void (* GL_APIENTRY glGenSamplers) (GLsizei count, GLuint* samplers){};
void (* GL_APIENTRY glDeleteSamplers) (GLsizei count, const GLuint* samplers){};
void (* GL_APIENTRY glBindSampler) (GLuint unit, GLuint sampler){};
void (* GL_APIENTRY glSamplerParameteri) (GLuint sampler, GLenum pname, GLint param){};
#endif

GLenum luminanceFormat = GL_LUMINANCE;
GLenum luminanceInternalFormat = GL_LUMINANCE8;
GLenum luminanceAlphaFormat = GL_LUMINANCE_ALPHA;
GLenum luminanceAlphaInternalFormat = GL_LUMINANCE8_ALPHA8;
GLenum alphaFormat = GL_ALPHA;
GLenum alphaInternalFormat = GL_ALPHA8;

bool useImmutableTexStorage = false;
#ifdef CONFIG_GFX_OPENGL_ES
void (* GL_APIENTRY glTexStorage2D) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height){};
#endif

bool usePBO = false;
#ifdef CONFIG_GFX_OPENGL_ES
GLvoid* (* GL_APIENTRY glMapBufferRange) (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access){};
GLboolean (* GL_APIENTRY glUnmapBuffer) (GLenum target){};
#endif

bool useEGLImages = false;
bool useExternalEGLImages = false;

#ifdef CONFIG_GFX_OPENGL_ES
void GL_APIENTRY (*glDebugMessageCallback)(GLDEBUGPROCKHR callback, const void *userParam){};
static constexpr auto DEBUG_OUTPUT = GL_DEBUG_OUTPUT_KHR;
#else
void GL_APIENTRY (*glDebugMessageCallback)(GLDEBUGPROC callback, const void *userParam){};
static constexpr auto DEBUG_OUTPUT = GL_DEBUG_OUTPUT;
#endif
static bool useDebugOutput = false;

bool useLegacyGLSL = Config::Gfx::OPENGL_ES;

static Base::GLBufferConfig gfxBufferConfig;

static void initVBOs()
{
	logMsg("making stream VBO");
	glGenBuffers(sizeofArray(streamVBO), streamVBO);
}

GLuint getVBO()
{
	assert(streamVBO[streamVBOIdx]);
	auto vbo = streamVBO[streamVBOIdx];
	streamVBOIdx = (streamVBOIdx+1) % sizeofArray(streamVBO);
	return vbo;
}

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
	GLfloat maximumAnisotropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnisotropy);
	logMsg("anisotropic filtering supported, max value: %f", (double)maximumAnisotropy);
	useAnisotropicFiltering = 1;
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
	glGenVertexArrays(1, &streamVAO);
	glBindVertexArray(streamVAO);
	#endif
}

static void setupTextureSwizzle()
{
	if(useTextureSwizzle)
		return;
	logMsg("using texture swizzling");
	useTextureSwizzle = true;
}

static void setupImmutableTexStorage(bool extSuffix)
{
	if(useImmutableTexStorage)
		return;
	logMsg("using immutable texture storage");
	useImmutableTexStorage = true;
	#ifdef CONFIG_GFX_OPENGL_ES
	const char *procName = extSuffix ? "glTexStorage2DEXT" : "glTexStorage2D";
	glTexStorage2D = (typeof(glTexStorage2D))Base::GLContext::procAddress(procName);
	#endif
}

static void setupRGFormats()
{
	luminanceFormat = GL_RED;
	luminanceInternalFormat = GL_R8;
	luminanceAlphaFormat = GL_RG;
	luminanceAlphaInternalFormat = GL_RG8;
	alphaFormat = GL_RED;
	alphaInternalFormat = GL_R8;
}

static void setupSamplerObjects()
{
	if(useSamplerObjects)
		return;
	logMsg("using sampler objects");
	useSamplerObjects = true;
	#ifdef CONFIG_GFX_OPENGL_ES
	glGenSamplers = (typeof(glGenSamplers))Base::GLContext::procAddress("glGenSamplers");
	glDeleteSamplers = (typeof(glDeleteSamplers))Base::GLContext::procAddress("glDeleteSamplers");
	glBindSampler = (typeof(glBindSampler))Base::GLContext::procAddress("glBindSampler");
	glSamplerParameteri = (typeof(glSamplerParameteri))Base::GLContext::procAddress("glSamplerParameteri");
	#endif
}

static void setupPBO()
{
	if(usePBO)
		return;
	logMsg("using PBOs");
	usePBO = true;
	initTexturePBO();
}

static void checkExtensionString(const char *extStr)
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
		useDebugOutput = true;
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
		useUnpackRowLength = true;
	}
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
		setupImmutableTexStorage(true);
	}
	else if(Config::envIsAndroid && string_equal(extStr, "GL_OES_EGL_image"))
	{
		useEGLImages = true;
	}
	else if(Config::envIsAndroid && Config::Gfx::OPENGL_ES_MAJOR_VERSION >= 2 &&
		string_equal(extStr, "GL_OES_EGL_image_external"))
	{
		useExternalEGLImages = true;
	}
	else if(Config::Gfx::OPENGL_ES_MAJOR_VERSION >= 2 && string_equal(extStr, "GL_NV_pixel_buffer_object"))
	{
		setupPBO();
	}
	else if(Config::Gfx::OPENGL_ES_MAJOR_VERSION >= 2 && string_equal(extStr, "GL_NV_map_buffer_range"))
	{
		logMsg("supports map buffer range (NVIDIA)");
		if(!glMapBufferRange)
			glMapBufferRange = (typeof(glMapBufferRange))Base::GLContext::procAddress("glMapBufferRangeNV");
		if(!glUnmapBuffer)
			glUnmapBuffer = glUnmapBufferOES;
	}
	else if(string_equal(extStr, "GL_EXT_map_buffer_range"))
	{
		logMsg("supports map buffer range");
		if(!glMapBufferRange)
			glMapBufferRange = (typeof(glMapBufferRange))Base::GLContext::procAddress("glMapBufferRangeEXT");
		if(!glUnmapBuffer)
			glUnmapBuffer = glUnmapBufferOES;
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
		#ifndef __APPLE__
		if(!useFBOFuncs)
		{
			setupFBOFuncs();
			generateMipmaps = glGenerateMipmapEXT;
		}
		#endif
	}
	else if(string_equal(extStr, "GL_ARB_framebuffer_object"))
	{
		if(!useFBOFuncs)
			setupFBOFuncs();
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
	return init(Base::Window::defaultPixelFormat());
}

CallResult init(IG::PixelFormat pixelFormat)
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

	if(!pixelFormat)
		pixelFormat = Base::Window::defaultPixelFormat();
	int glVer = 0;
	Base::GLBufferConfigAttributes glBuffAttr;
	glBuffAttr.setPixelFormat(pixelFormat);
	Base::GLContextAttributes glAttr;
	if(Config::DEBUG_BUILD)
		glAttr.setDebug(true);
	#ifdef CONFIG_GFX_OPENGL_ES
	if(!Base::GLContext::bindAPI(Base::GLContext::OPENGL_ES_API))
	{
		bug_exit("unable to bind GLES API");
	}
	glAttr.setOpenGLESAPI(true);
	if(Config::Gfx::OPENGL_ES_MAJOR_VERSION == 1)
	{
		gfxBufferConfig = gfxContext.makeBufferConfig(glAttr, glBuffAttr);
		gfxContext.init(glAttr, gfxBufferConfig);
	}
	else
	{
		glAttr.setMajorVersion(3);
		gfxBufferConfig = gfxContext.makeBufferConfig(glAttr, glBuffAttr);
		if(gfxBufferConfig)
		{
			gfxContext.init(glAttr, gfxBufferConfig);
		}
		if(!gfxContext)
		{
			glAttr.setMajorVersion(2);
			gfxBufferConfig = gfxContext.makeBufferConfig(glAttr, glBuffAttr);
			gfxContext.init(glAttr, gfxBufferConfig);
		}
	}
	#else
	if(!Base::GLContext::bindAPI(Base::GLContext::OPENGL_API))
	{
		bug_exit("unable to bind GL API");
	}
	if(Config::Gfx::OPENGL_SHADER_PIPELINE)
	{
		#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
		useFixedFunctionPipeline = false;
		#endif
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
		#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
		useFixedFunctionPipeline = true;
		#endif
		glAttr.setMajorVersion(1);
		glAttr.setMinorVersion(3);
		gfxBufferConfig = gfxContext.makeBufferConfig(glAttr, glBuffAttr);
		if(gfxContext.init(glAttr, gfxBufferConfig) != OK)
		{
			logMsg("1.3 context not supported");
		}
	}
	#endif
	if(!gfxContext)
	{
		bug_exit("can't create context");
		return INVALID_PARAMETER;
	}

	gfxContext.setCurrent(gfxContext, nullptr);

	if(checkGLErrorsVerbose)
		logMsg("using verbose error checks");
	else if(checkGLErrors)
		logMsg("using error checks");
	
	auto version = (const char*)glGetString(GL_VERSION);
	assert(version);
	auto rendererName = (const char*)glGetString(GL_RENDERER);
	logMsg("version: %s (%s)", version, rendererName);
	
	if(!glVer)
		glVer = glVersionFromStr(version);
	
	#ifndef CONFIG_GFX_OPENGL_ES
	// core functionality
	if(glVer >= 15)
	{
		useVBOFuncs = true;
	}
	if(glVer >= 20)
	{
		setupNonPow2MipmapRepeatTextures();
	}
	if(glVer >= 21)
	{
		setupPBO();
	}
	if(glVer >= 30)
	{
		if(!useFixedFunctionPipeline)
		{
			// must render via VAOs/VBOs in 3.1+ without compatibility context
			setupVAOFuncs();
			setupTextureSwizzle();
			setupRGFormats();
			setupSamplerObjects();
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
		// safe to use VBOs
	}
	if(Config::Gfx::OPENGL_ES_MAJOR_VERSION > 1)
	{
		if(glVer >= 30)
			setupNonPow2MipmapRepeatTextures();
		else
			setupNonPow2Textures();
		setupFBOFuncs();
		if(glVer >= 30)
		{
			glMapBufferRange = (typeof(glMapBufferRange))Base::GLContext::procAddress("glMapBufferRange");
			glUnmapBuffer = (typeof(glUnmapBuffer))Base::GLContext::procAddress("glUnmapBuffer");
			setupImmutableTexStorage(false);
			setupTextureSwizzle();
			setupRGFormats();
			setupSamplerObjects();
			setupPBO();
			useUnpackRowLength = true;
			useLegacyGLSL = false;
		}
	}

	// extension functionality
	auto extensions = (const char*)glGetString(GL_EXTENSIONS);
	assert(extensions);
	logMsg("extensions: %s", extensions);
	checkFullExtensionString(extensions);
	#endif // CONFIG_GFX_OPENGL_ES

	if(useVBOFuncs)
		initVBOs();
	setClearColor(0., 0., 0.);
	//glShadeModel(GL_SMOOTH);
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	setVisibleGeomFace(FRONT_FACES);

	GLint texSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
	textureSizeSupport.maxXSize = textureSizeSupport.maxYSize = texSize;
	assert(textureSizeSupport.maxXSize > 0 && textureSizeSupport.maxYSize > 0);
	logMsg("max texture size is %d", texSize);

	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
		glcEnableClientState(GL_VERTEX_ARRAY);
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(!useFixedFunctionPipeline)
	{
		handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s before shaders", err); });
		if(Config::DEBUG_BUILD)
			logMsg("shader language version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
		initShaders();
	}
	#endif
	return OK;
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

void setDebugOutput(bool on)
{
	if(!useDebugOutput)
	{
		return;
	}
	if(!on)
	{
		glDisable(DEBUG_OUTPUT);
	}
	else
	{
		if(!glDebugMessageCallback)
		{
			auto glDebugMessageCallbackStr =
					Config::Gfx::OPENGL_ES ? "glDebugMessageCallbackKHR" : "glDebugMessageCallback";
			logMsg("enabling debug output with %s", glDebugMessageCallbackStr);
			glDebugMessageCallback = (typeof(glDebugMessageCallback))Base::GLContext::procAddress(glDebugMessageCallbackStr);
			glDebugMessageCallback(
				GL_APIENTRY [](GLenum source, GLenum type, GLuint id,
					GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
				{
					logger_printfn(LOG_D, "GL Debug: %s", message);
				}, nullptr);
		}
		glEnable(DEBUG_OUTPUT);
	}
}

}
