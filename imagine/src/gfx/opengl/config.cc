#include <imagine/gfx/Gfx.hh>
#include <imagine/base/Base.hh>
#include <assert.h>

#include <imagine/util/time/sys.hh>
static TimeSys startFrameTime;//, halfFrameTime;//, oneFrameTime, firstOneFrameTime;

#include "private.hh"
#include "settings.h"
#include "utils.h"
#include "GLStateCache.hh"
#ifdef __ANDROID__
#include "../../base/android/private.hh"
#endif

//#include "geometry-test.h"

namespace Base
{
	void *glProcAddress(const char *funcName);
}

namespace Gfx
{

GLint projectionUniform, modelViewUniform, textureUniform;

GLfloat maximumAnisotropy, anisotropy = 0, forceAnisotropy = 0;
bool useAnisotropicFiltering = false;
static bool forceNoAnisotropicFiltering = true;

bool useAutoMipmapGeneration = false;
static bool forceNoAutoMipmapGeneration = false;

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

static bool supportCompressedTextures = false;
bool useCompressedTextures = false;
static bool forceNoCompressedTextures = true;

bool useFBOFuncs = false;
bool useFBOFuncsEXT = false;
static bool forceNoFBOFuncs = false;

bool useVBOFuncs = false;
#if defined CONFIG_GFX_OPENGL_ES || !defined CONFIG_GFX_OPENGL_SHADER_PIPELINE
static bool forceNoVBOFuncs = true;
#else
static bool forceNoVBOFuncs = false;
#endif

static GLuint globalStreamVAO = 0;
GLuint globalStreamVBO[4] {0};
uint globalStreamVBOIdx = 0;

bool forceNoTextureSwizzle = false;
bool useTextureSwizzle = false;

bool usingAutoMipmaping()
{
	if(useAutoMipmapGeneration
		#ifndef CONFIG_GFX_OPENGL_ES
		|| useFBOFuncs
		#endif
	  )
		return 1;
	else return 0;
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

static void setupAutoMipmapGeneration()
{
	logMsg("automatic mipmap generation supported");
	useAutoMipmapGeneration = 1;
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
	logMsg("Non-Power-of-2 textures are supported");
	Gfx::textureSizeSupport.nonPow2 = 1;
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

static void setupCompressedTexturesSupport()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	supportCompressedTextures = 1;
	useCompressedTextures = 1;
	logMsg("Compressed textures are supported");
	#endif
}

static void setupFBOFuncs()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	useFBOFuncs = 1;
	logMsg("FBO functions are supported");
	#endif
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

static void checkExtensionString(const char *extStr)
{
	//logMsg("checking %s", extStr);
	if(
		string_equal(extStr, "GL_ARB_texture_non_power_of_two")
		#ifdef CONFIG_GFX_OPENGL_ES
		|| (CONFIG_GFX_OPENGL_ES_MAJOR_VERSION == 1 && string_equal(extStr, "GL_APPLE_texture_2D_limited_npot")) // no mipmaps or repeat modes
			#if !defined __APPLE__
			|| string_equal(extStr, "GL_IMG_texture_npot")
			|| string_equal(extStr, "GL_NV_texture_npot_2D_mipmap") // no repeat modes
			|| string_equal(extStr, "GL_OES_texture_npot") // allows mipmaps and repeat modes
			#endif
		#endif
		)
	{
		if(!forceNoNonPow2Textures && !Gfx::textureSizeSupport.nonPow2)
			setupNonPow2Textures();
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
		if(!forceNoFBOFuncs && !useFBOFuncs)
		{
			setupFBOFuncs();
			useFBOFuncsEXT = true;
		}
	}
	else if(string_equal(extStr, "GL_ARB_framebuffer_object"))
	{
		if(!forceNoFBOFuncs && !useFBOFuncs)
			setupFBOFuncs();
	}
	else if(string_equal(extStr, "GL_ARB_texture_swizzle"))
	{
		setupTextureSwizzle();
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

#ifndef APIENTRY
#define APIENTRY
#endif

#if defined CONFIG_GFX_OPENGL_ES
static void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length, const GLchar *message, const GLvoid *userParam)
#else
static void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length, const GLchar *message, void *userParam)
#endif
{
	logErr("Debug Info: %s", message);
}

CallResult init()
{
	logMsg("running init");

	if(checkGLErrorsVerbose)
		logMsg("using verbose error checks");
	else if(checkGLErrors)
		logMsg("using error checks");

	#if !defined CONFIG_GFX_OPENGL_ES && !defined __APPLE__ && !defined NDEBUG
	glDebugMessageCallback(debugCallback, nullptr);
	glEnable(GL_DEBUG_OUTPUT);
	#endif
	#if defined CONFIG_GFX_OPENGL_ES && defined CONFIG_BASE_X11 & !defined CONFIG_MACHINE_PANDORA && !defined NDEBUG
	auto glDebugMessageCallback = (void (*)(GLDEBUGPROCKHR callback, const void *userParam))Base::glProcAddress("glDebugMessageCallbackKHR");
	if(glDebugMessageCallback)
	{
		logMsg("using KHR_debug");
		glDebugMessageCallback(debugCallback, nullptr);
		glEnable(GL_DEBUG_OUTPUT_KHR);
	}
	#endif
	
	auto version = (const char*)glGetString(GL_VERSION);
	assert(version);
	auto rendererName = (const char*)glGetString(GL_RENDERER);
	logMsg("version: %s (%s)", version, rendererName);
	
	#ifndef CONFIG_GFX_OPENGL_ES
	//glClearDepth(1.0f);
	//glDepthFunc(GL_LEQUAL);
	auto dotPos = strchr(version, '.');
	int majorVer = dotPos[-1]-'0' , minorVer = dotPos[1]-'0';
	const bool hasGL3_3 = false;//(majorVer > 3) || (majorVer == 3 && minorVer >= 3);
	const bool hasGL3_2 = (majorVer > 3) || (majorVer == 3 && minorVer >= 2);
	const bool hasGL3_1 = (majorVer > 3) || (majorVer == 3 && minorVer >= 1);
	const bool hasGL3_0 = majorVer >= 3;
	const bool hasGL2_0 = majorVer >= 2;
	const bool hasGL1_5 = (majorVer > 1) || (majorVer == 1 && minorVer >= 5);
	const bool hasGL1_4 = hasGL1_5 || (majorVer == 1 && minorVer >= 4);
	const bool hasGL1_3 = hasGL1_4 || (majorVer == 1 && minorVer >= 3);
	const bool hasGL1_2 = hasGL1_3 || (majorVer == 1 && minorVer >= 2);
	assert(hasGL1_2); // needed for CLAMP_TO_EDGE

	if(hasGL3_0)
	{
		#if defined CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE && defined CONFIG_GFX_OPENGL_SHADER_PIPELINE
		useFixedFunctionPipeline = false;
		#endif
	}
	#else
		#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
		const bool hasGLES1_1 = !Config::envIsAndroid || (!Config::MACHINE_IS_GENERIC_ARM || strstr(version, "1.1"));
		const bool hasGLES2_0 = false;
		#else
		const bool hasGLES1_1 = true;
		const bool hasGLES2_0 = true;
		#endif
	#endif
	
	#ifndef CONFIG_GFX_OPENGL_ES
	// core functionality
	if(hasGL1_3)
	{
		if(!forceNoCompressedTextures)
			setupCompressedTexturesSupport();
	}
	if(hasGL1_5)
	{
		if(!forceNoVBOFuncs)
			setupVBOFuncs();
	}
	if(hasGL2_0)
	{
		if(!forceNoNonPow2Textures)
			setupNonPow2Textures();
	}
	if(hasGL3_0)
	{
		if(!useFixedFunctionPipeline)
		{
			// must render via VAOs/VBOs in 3.1+ without compatibility context
			setupVAOFuncs();
			if(!useVBOFuncs)
				setupVBOFuncs();
		}
		if(!forceNoAutoMipmapGeneration)
			setupAutoMipmapGeneration();
		if(!forceNoFBOFuncs)
			setupFBOFuncs();
	}
	if(hasGL3_3)
	{
		setupTextureSwizzle();
	}

	// extension functionality
	if(hasGL3_0)
	{
		GLint numExtensions;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
		#ifndef NDEBUG
		logMsgNoBreak("extensions: ");
		iterateTimes(numExtensions, i)
		{
			logger_printf(LOG_M, "%s ", (const char*)glGetStringi(GL_EXTENSIONS, i));
		}
		logger_printf(LOG_M, "\n");
		#endif
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
	if(hasGLES1_1)
	{
		// TODO: re-test with OpenGL ES 2.0 renderer
		/*if(Config::MACHINE_IS_GENERIC_ARMV7)
		{
			if(strstr(rendererName, "GC800 Graphics"))
			{
				logMsg("automatic mipmap generation bugged on Vivante, disabling");
				forceNoAutoMipmapGeneration = true;
			}
			else if(strstr(rendererName, "Adreno (TM) 220"))
			{
				logMsg("automatic mipmap generation bugged some Adreno 220 drivers, disabling");
				forceNoAutoMipmapGeneration = true;
			}
		}*/
		if(!forceNoAutoMipmapGeneration)
			setupAutoMipmapGeneration();
		if(!forceNoVBOFuncs)
			setupVBOFuncs();
	}
	if(hasGLES2_0)
	{
		if(!forceNoNonPow2Textures)
			setupNonPow2Textures();
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

	vsyncEnable();

	/*#ifdef CONFIG_GFX_OPENGL_ES
	if(strstr(extensions, "GL_EXT_discard_framebuffer") != NULL)
	{
		discardFrameBuffer = 1;
		logMsg("framebuffer discarding supported");
	}
	#endif*/

	#if defined __ANDROID__
	//checkForDrawTexture(extensions, rendererName);
	setupAndroidOGLExtensions(extensions, rendererName);
	#endif

	/*#ifdef CONFIG_GFX_OPENGL_GLX
	if(GLXEW_SGI_video_sync)
	{
		useSGIVidSync = 1;
		glXGetVideoSyncSGI(&gfx_frameTime);
	}
	else
	#endif
	{
		//logMsg("no video sync counter, using system time");
		startFrameTime.setTimeNow();
		//halfFrameTime.setUSecs(16000/2);
		//oneFrameTime.setUSecs(16666);
		//firstOneFrameTime.setUSecs(19500);
		//gfx_frameClockTime = 0;
	}*/
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

#ifdef CONFIG_BASE_ANDROID
static void setupAndroidOGLExtensions(const char *extensions, const char *rendererName)
{
	// TODO: make direct texture setup optional
	if(Base::androidSDK() < 14)
		directTextureConf.checkForEGLImageKHR(extensions, rendererName);
	#ifdef CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES
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
			// TODO: should be re-tested with OpenGL ES 2.0 context
			logWarn("enabling SurfaceTexture GL_TEXTURE_2D binding hack");
			surfaceTextureConf.texture2dBindingHack = 1;
		}
	}
	#endif
}
#endif

}
