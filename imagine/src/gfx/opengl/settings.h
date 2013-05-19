#pragma once

#ifdef CONFIG_INPUT
	#include <input/Input.hh>
#endif

#ifdef CONFIG_BASE_ANDROID
#include <base/android/private.hh>
#include <base/android/public.hh>
namespace Gfx
{
	static void setupAndroidOGLExtensions(const char *extensions, const char *rendererName);
}
#endif

#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
	#include "android/DirectTextureBufferImage.hh"
	#include <dlfcn.h>
	#if CONFIG_ENV_ANDROID_MINSDK < 9
		static EGLDisplay eglDisplay = 0;
		static EGLImageKHR (*eglCreateImageKHR)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list) = 0;
		static EGLBoolean (*eglDestroyImageKHR)(EGLDisplay dpy, EGLImageKHR image) = 0;
		static EGLDisplay (*eglGetDisplay)(EGLNativeDisplayType display_id);
		namespace Base
		{
			EGLDisplay getAndroidEGLDisplay() { return eglDisplay; }
		}
	#endif
#endif

//static int usingFixedAspectRatio = 0;
//static float aspectRatio = 4.0/3.0;

namespace Gfx
{

static int discardFrameBuffer = 0;
static bool useSGIVidSync = 0;

TextureSizeSupport textureSizeSupport =
{
	0, // nonPow2
	1, // nonSquare
	1, // filtering
	0, // nonPow2CanMipmap
	0, 0, // minXSize, minYSize
	0, 0 // maxXSize, maxYSize
};

static float zRange = 1000.0;

static void resizeGLScene(const Base::Window &win)
{
	auto width = win.rect.xSize();
	auto height = win.rect.ySize();
	logMsg("glViewport %d:%d:%d:%d from window %d:%d:%d:%d (%d,%d)", win.rect.x, win.h - win.rect.y2, width, height,
			win.rect.x, win.rect.y, win.rect.x2, win.rect.y2, win.w, win.h);
	glViewport(win.rect.x,
		win.h - win.rect.y2,
		width, height);

	if(width == 0 || height == 0)
	{
		bug_exit("view is invisible");
		return; // view is invisible, do nothing
	}

	glcMatrixMode(GL_PROJECTION);

	GC fovy = M_PI/4.0;

	bool isSideways = rotateView == VIEW_ROTATE_90 || rotateView == VIEW_ROTATE_270;
	Gfx::proj.aspectRatio = isSideways ? (GC)height / (GC)width : (GC)width / (GC)height;
	//glLoadIdentity();
	//gluPerspective(45.0f, viewAspectRatio, 0.1f, 1000.0f);
	Matrix4x4<GC> mat, rMat;
	Gfx::proj.focal = 1.0;
	mat.perspectiveFovLH(fovy, Gfx::proj.aspectRatio, 1.0, zRange);
	/*Gfx::proj.focal = -100;
	mat.perspectiveFrustumWithView(isSideways ? height : width, isSideways ? width : height,
			1, 200, -Gfx::proj.focal);*/
	viewPixelWidth_ = width;
	viewPixelHeight_ = height;
	/*if(isSideways)
	{
		IG::swap(viewPixelWidth, viewPixelHeight);
	}*/
	Gfx::proj.setMatrix(mat, isSideways);
	setupScreenSize();
	if(animateOrientationChange && !projAngleM.isComplete())
	{
		logMsg("animated rotation %f", (double)IG::toDegrees(projAngleM.now));
		rMat.zRotationLH(projAngleM.now);
		mat = Matrix4x4<GC>::mult(mat, rMat);
	}
	else if(rotateView != VIEW_ROTATE_0)
	{
		logMsg("fixed rotation %f", (double)orientationToGC(rotateView));
		rMat.zRotationLH(IG::toRadians(orientationToGC(rotateView)));
		mat = Matrix4x4<GC>::mult(mat, rMat);
	}
	else
	{
		//logMsg("no rotation");
	}
	glLoadMatrixf((GLfloat *)&mat.v[0]);
	//mat.print();
	/*glGetFloatv (GL_PROJECTION_MATRIX, (GLfloat*)&proj);
	logMsg("projection matrix set with fovy %f aspect ratio %f and z range %f - %f", (float)Gfx::proj.fovy, (float)Gfx::proj.aspectRatio, 1.0, (float)zRange);
	logMsg("view space half %f x %f, inverse %f x %f", 1.0f/proj._11, 1.0f/proj._22, proj._11, proj._22);*/
	//gfx_setupSpace(proj._11, proj._22, width, height);
	/*{
		GC x = 0,y = 0,z = Gfx::proj.focal;
		Gfx::proj.project(x,y,z);
		logMsg("projected to %f %f %f",x,y,z);
		x = 0;y = 0;z = 0.5;
		Gfx::proj.unProject(x,y,z);
		logMsg("unprojected to %f %f %f",x,y,z);
	}*/
	glcMatrixMode(GL_MODELVIEW);
}

void resizeDisplay(const Base::Window &win)
{
	Gfx::GfxViewState oldState =
	{
		proj.w, proj.h, proj.aspectRatio,
		viewPixelWidth_, viewPixelHeight_
	};
	resizeGLScene(win);
	Gfx::onViewChange(&oldState);
}

#ifdef CONFIG_GFX_SOFT_ORIENTATION

#ifdef CONFIG_INPUT
void configureInputForOrientation()
{
	using namespace Input;
	xPointerTransform(rotateView == VIEW_ROTATE_0 || rotateView == VIEW_ROTATE_90 ? POINTER_NORMAL : POINTER_INVERT);
	yPointerTransform(rotateView == VIEW_ROTATE_0 || rotateView == VIEW_ROTATE_270 ? POINTER_NORMAL : POINTER_INVERT);
	pointerAxis(rotateView == VIEW_ROTATE_0 || rotateView == VIEW_ROTATE_180 ? POINTER_NORMAL : POINTER_INVERT);
}
#endif

uint setOrientation(uint o)
{
	assert(o == VIEW_ROTATE_0 || o == VIEW_ROTATE_90 || o == VIEW_ROTATE_180 || o == VIEW_ROTATE_270);

	if((validOrientations & o) && rotateView != o)
	{
		logMsg("setting orientation %d", o);
		rotateView = o;
		if(animateOrientationChange)
		{
			projAngleM.initLinear(projAngleM.now, IG::toRadians(orientationToGC(rotateView)), 10);
			Base::displayNeedsUpdate();
		}
		Base::setSystemOrientation(o);
		resizeDisplay(Base::window());
		#ifdef CONFIG_INPUT
			configureInputForOrientation();
		#endif
		return 1;
	}
	else
		return 0;
}
#endif

}

static void vsyncEnable()
{
	#ifdef CONFIG_BASE_WIN32
		#define WGL_VSYNC_ON_INTERVAL 1
		if (wglewIsSupported("WGL_EXT_swap_control"))
		{
			wglSwapIntervalEXT(WGL_VSYNC_ON_INTERVAL);
			logMsg("vsync enabled via WGL_EXT_swap_control");
		}
		else
		{
			logWarn("WGL_EXT_swap_control is not supported");
		}
	#elif defined(__APPLE__) && !defined(CONFIG_BASE_IOS)
		GLint sync = 1;
		CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &sync);
	#endif
}

static GLfloat maximumAnisotropy, anisotropy = 0, forceAnisotropy = 0;
static bool useAnisotropicFiltering = 0;
static bool forceNoAnisotropicFiltering = 1;

static void checkForAnisotropicFiltering(const char *extensions)
{
	#ifndef CONFIG_GFX_OPENGL_ES
	if(!forceNoAnisotropicFiltering && strstr(extensions, "GL_EXT_texture_filter_anisotropic"))
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnisotropy);
		logMsg("anisotropic filtering supported, max value: %f", (double)maximumAnisotropy);
		useAnisotropicFiltering = 1;

		if(forceAnisotropy) // force a specific anisotropy value
			anisotropy = forceAnisotropy;
		else anisotropy = maximumAnisotropy;
		assert(anisotropy <= maximumAnisotropy);
	}
	#endif
}

static uchar useAutoMipmapGeneration = 0;
static uchar forceNoAutoMipmapGeneration = 0;

static void checkForAutoMipmapGeneration(const char *extensions, const char *version, const char *rendererName)
{
	bool use = 0;
	#ifndef CONFIG_GFX_OPENGL_ES
	use = !forceNoAutoMipmapGeneration && strstr(extensions, "GL_SGIS_generate_mipmap");
	#elif defined CONFIG_BASE_ANDROID
	// Older Android devices may only support OpenGL ES 1.0
	use = !forceNoAutoMipmapGeneration && (Base::androidSDK() >= 10  || strstr(version, "1.1"));
	if(strstr(rendererName, "GC800 Graphics"))
	{
		logMsg("automatic mipmap generation bugged on Vivante, disabling");
		use = 0;
	}
	#else
	use = !forceNoAutoMipmapGeneration;
	#endif
	if(use)
	{
		logMsg("automatic mipmap generation supported");
		useAutoMipmapGeneration = 1;
		//glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
	}
}

static uchar useMultisample = 0;
static uchar forceNoMultisample = 1;
static uchar forceNoMultisampleHint = 0;

static void checkForMultisample(const char *extensions)
{
	#ifndef CONFIG_GFX_OPENGL_ES
	if(!forceNoMultisample && strstr(extensions, "GL_ARB_multisample"))
	{
		logMsg("multisample antialiasing supported");
		useMultisample = 1;
		if(!forceNoMultisampleHint && strstr(extensions, "GL_NV_multisample_filter_hint"))
		{
			logMsg("multisample hints supported");
			glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
		}
		glcEnable(GL_MULTISAMPLE_ARB);
	}
	#endif
}

static uchar forceNoNonPow2Textures = 0;

static void checkForNonPow2Textures(const char *extensions, const char *rendererName)
{
	if(forceNoNonPow2Textures)
		return;

	#ifdef CONFIG_BASE_ANDROID
	if(glBrokenNpot)
		return;
	#endif

	#if defined(CONFIG_BASE_PS3)
	if(1)
	#elif defined(CONFIG_GFX_OPENGL_ES) && defined(CONFIG_BASE_IOS)
	if(strstr(extensions, "GL_APPLE_texture_2D_limited_npot"))
	#elif defined(CONFIG_GFX_OPENGL_ES)
	if(strstr(extensions, "GL_IMG_texture_npot")
			|| strstr(extensions, "GL_NV_texture_npot_2D_mipmap") // no repeat modes
			|| strstr(extensions, "GL_APPLE_texture_2D_limited_npot") // no mipmaps or repeat modes
			|| strstr(extensions, "GL_OES_texture_npot")
			|| strstr(extensions, "GL_ARB_texture_non_power_of_two")
			)
	#else
	if(strstr(extensions, "GL_ARB_texture_non_power_of_two"))
	#endif
	{
		#ifdef CONFIG_BASE_ANDROID
		if(string_equal(rendererName, "Adreno 200") ||
				(Base::androidSDK() <= 8 && strstr(rendererName, "Adreno")))
		{
			logWarn("Non-Power-of-2 textures supported but disabled due to buggy driver");
			return;
		}
		#endif

		Gfx::textureSizeSupport.nonPow2 = 1;
		logMsg("Non-Power-of-2 textures are supported");
	}
}

static uchar supportBGRPixels = 0;
static uchar useBGRPixels = 0;
static uchar forceNoBGRPixels = 0;

namespace Gfx
{
bool preferBGRA = 0, preferBGR = 0;

static void checkForBGRPixelSupport(const char *extensions)
{
	#ifdef CONFIG_GFX_OPENGL_ES
		#ifdef CONFIG_BASE_IOS
			if(strstr(extensions, "GL_APPLE_texture_format_BGRA8888") != NULL)
		#elif defined(CONFIG_BASE_PS3)
			if(0)
		#else
			if(strstr(extensions, "GL_EXT_texture_format_BGRA8888") != NULL)
		#endif
	#else
		if(!forceNoBGRPixels)
	#endif
	{
		supportBGRPixels = 1;
		useBGRPixels = 1;

		#if !defined(CONFIG_GFX_OPENGL_ES) || defined(CONFIG_BASE_PS3)
		preferBGR = 1;
		#endif
		preferBGRA = 1;

		logMsg("BGR pixel types are supported");
	}
}

}

static uchar supportCompressedTextures = 0;
static uchar useCompressedTextures = 0;
static uchar forceNoCompressedTextures = 1;

static void checkForCompressedTexturesSupport(bool hasGL1_3)
{
	#ifndef CONFIG_GFX_OPENGL_ES
	if(!forceNoCompressedTextures && hasGL1_3)
	{
		supportCompressedTextures = 1;
		useCompressedTextures = 1;
		logMsg("Compressed textures are supported");
	}
	#endif
}

static uchar useFBOFuncs = 0;
static uchar forceNoFBOFuncs = 1;

static void checkForFBOFuncs(const char *extensions)
{
	#ifndef CONFIG_GFX_OPENGL_ES
	if(!forceNoFBOFuncs && strstr(extensions, "GL_EXT_framebuffer_object"))
	{
		useFBOFuncs = 1;
		logMsg("FBO functions are supported");
	}
	#endif
}

static uchar useVBOFuncs = 0;
static uchar forceNoVBOFuncs = 1;

static void checkForVBO(const char *version, bool hasGL1_5)
{
	if(!forceNoVBOFuncs &&
	#ifndef CONFIG_GFX_OPENGL_ES
		hasGL1_5
	#else
		strstr(version, " 1.0") == NULL // make sure OpenGL-ES is not 1.0
	#endif
	)
	{
		useVBOFuncs = 1;
		logMsg("VBOs are supported");
	}
}

#if defined CONFIG_BASE_ANDROID && defined CONFIG_GFX_OPENGL_USE_DRAW_TEXTURE

static bool useDrawTex = 0;
static bool forceNoDrawTex = 0;

static void checkForDrawTexture(const char *extensions, const char *rendererName)
{
	// Limited usefulness due to no 90deg rotation support,
	// only use on Android since OS takes care of screen orientation,
	// but there are lots of GPUs that produce blank out even though
	// they "support" this extension so don't actually use for now
	if(!forceNoDrawTex && strstr(extensions, "GL_OES_draw_texture"))
	{
		if(strstr(rendererName, "NVIDIA") || string_equal(rendererName, "VideoCore IV HW"))
		{
			// completely blank output on Tegra & VideoCore
			logMsg("ignoring reported Draw Texture extension due to driver bugs");
			return;
		}
		if(Base::androidSDK() >= 14 && strstr(rendererName, "Adreno"))
		{
			// blank output if source is SurfaceTexture
			logMsg("ignoring reported Draw Texture extension due to driver bug with SurfaceTexture");
			return;
		}
		useDrawTex = 1;
		logMsg("Draw Texture supported");
	}
}

#endif

#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE

namespace Gfx
{

void AndroidDirectTextureConfig::checkForEGLImageKHR(const char *extensions, const char *rendererName)
{
	if(strstr(rendererName, "NVIDIA") // disable on Tegra, unneeded and causes lock-ups currently
		|| string_equal(rendererName, "VideoCore IV HW")) // seems to crash Samsung Galaxy Y on eglCreateImageKHR, maybe other devices
	{
		logMsg("force-disabling EGLImageKHR due to GPU");
		errorStr = "Unsupported GPU";
	}
	else
	{
		if(!setupEGLImageKHR(extensions))
		{
			logWarn("can't use EGLImageKHR: %s", errorStr);
			return;
		}
		if(strstr(rendererName, "SGX 530")) // enable on PowerVR SGX 530, though it should work on other models
		{
			logMsg("enabling by default on white-listed hardware");
			useEGLImageKHR = whitelistedEGLImageKHR = 1;
		}
	}
}

bool AndroidDirectTextureConfig::setupEGLImageKHR(const char *extensions)
{
	#ifdef NDEBUG
	bool verbose = 0;
	#else
	bool verbose = 1;
	#endif
	static const char *basicEGLErrorStr = "Unsupported libEGL";
	static const char *basicLibhardwareErrorStr = "Unsupported libhardware";

	logMsg("attempting to setup EGLImageKHR support");

	if(strstr(extensions, "GL_OES_EGL_image") == 0)
	{
		errorStr = "No OES_EGL_image extension";
		return 0;
	}

	/*if(strstr(extensions, "GL_OES_EGL_image_external") || strstr(rendererName, "Adreno"))
	{
		logMsg("uses GL_OES_EGL_image_external");
		directTextureTarget = GL_TEXTURE_EXTERNAL_OES;
	}*/

	void *libegl = 0;

	#if CONFIG_ENV_ANDROID_MINSDK < 9
	if((libegl = dlopen("/system/lib/libEGL.so", RTLD_LOCAL | RTLD_LAZY)) == 0)
	{
		errorStr = verbose ? "Can't load libEGL.so" : basicEGLErrorStr;
		goto FAIL;
	}

	//char const *(*eglQueryString)(EGLDisplay, EGLint) = (char const *(*)(EGLDisplay, EGLint))dlsym(libegl, "eglQueryString");
	//logMsg("EGL Extensions: %s", eglQueryString((EGLDisplay)1, EGL_EXTENSIONS));

	//logMsg("eglCreateImageKHR @ %p", eglCreateImageKHR);
	if((eglCreateImageKHR = (EGLImageKHR(*)(EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLint *))dlsym(libegl, "eglCreateImageKHR"))
		== 0)
	{

		errorStr = verbose ? "Can't find eglCreateImageKHR" : basicEGLErrorStr;
		goto FAIL;
	}

	//logMsg("eglDestroyImageKHR @ %p", eglCreateImageKHR);
	if((eglDestroyImageKHR = (EGLBoolean(*)(EGLDisplay, EGLImageKHR))dlsym(libegl, "eglDestroyImageKHR")) == 0)
	{
		errorStr = verbose ? "Can't find eglDestroyImageKHR" : basicEGLErrorStr;
		goto FAIL;
	}
	/*eglGetCurrentDisplay = (EGLDisplay(*)())dlsym(libegl, "eglGetCurrentDisplay");
	logMsg("eglGetCurrentDisplay @ %p", eglGetCurrentDisplay);*/


	//logMsg("eglGetDisplay @ %p", eglGetDisplay);
	if((eglGetDisplay = (EGLDisplay(*)(EGLNativeDisplayType))dlsym(libegl, "eglGetDisplay")) == 0)
	{
		errorStr = verbose ? "Can't find eglGetDisplay" : basicEGLErrorStr;
		goto FAIL;
	}


	if((eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY)
	{
		errorStr = verbose ? "Failed to get EGL display" : basicEGLErrorStr;
		goto FAIL;
	}
	logMsg("got EGL display: %d", (int)eglDisplay);
	#endif

	if(libhardware_dl() != OK)
	{
		errorStr = verbose ? "Incompatible libhardware.so" : basicLibhardwareErrorStr;
		goto FAIL;
	}

	if(hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (hw_module_t const**)&grallocMod) != 0)
	{
		errorStr = verbose ? "Can't load gralloc module" : basicLibhardwareErrorStr;
		goto FAIL;
	}

	gralloc_open((const hw_module_t*)grallocMod, &allocDev);
	if(!allocDev)
	{
		errorStr = verbose ? "Can't load allocator device" : basicLibhardwareErrorStr;
		goto FAIL;
	}
	logMsg("alloc device @ %p", allocDev);

	if(!DirectTextureBufferImage::testSupport(&errorStr))
	{
		goto FAIL;
	}

	logMsg("EGLImageKHR works");
	return 1;

	FAIL:
	if(libegl)
	{
		dlclose(libegl);
		libegl = nullptr;
	}
	grallocMod = nullptr;
	//TODO: free allocDev if needed

	return 0;
}

int AndroidDirectTextureConfig::allocBuffer(android_native_buffer_t &eglBuf)
{
	assert(allocDev);
	return allocDev->alloc(allocDev, eglBuf.width, eglBuf.height, eglBuf.format,
		eglBuf.usage, &eglBuf.handle, &eglBuf.stride);
}

int AndroidDirectTextureConfig::lockBuffer(android_native_buffer_t &eglBuf, int usage, int l, int t, int w, int h, void *&data)
{
	assert(grallocMod);
	return grallocMod->lock(grallocMod, eglBuf.handle, usage, l, t, w, h, &data);
}

int AndroidDirectTextureConfig::unlockBuffer(android_native_buffer_t &eglBuf)
{
	return grallocMod->unlock(grallocMod, eglBuf.handle);
}

int AndroidDirectTextureConfig::freeBuffer(android_native_buffer_t &eglBuf)
{
	if(allocDev->free)
		return allocDev->free(allocDev, eglBuf.handle);
	else
	{
		logWarn("no android native buffer free()");
		return 0;
	}
}

AndroidDirectTextureConfig directTextureConf;

bool supportsAndroidDirectTexture() { return directTextureConf.isSupported(); }
bool supportsAndroidDirectTextureWhitelisted() { return directTextureConf.whitelistedEGLImageKHR; }
const char* androidDirectTextureError() { return directTextureConf.errorStr; }

bool useAndroidDirectTexture()
{
	return supportsAndroidDirectTexture() ? directTextureConf.useEGLImageKHR : 0;
}

void setUseAndroidDirectTexture(bool on)
{
	if(supportsAndroidDirectTexture())
		directTextureConf.useEGLImageKHR = on;
}

}

#endif


static uchar usingMipmaping()
{
	if(useAutoMipmapGeneration
		#ifndef CONFIG_GFX_OPENGL_ES
		|| useFBOFuncs
		#endif
	  )
		return 1;
	else return 0;
}

namespace Gfx
{

void setDither(uint on)
{
	if(on)
		glcEnable(GL_DITHER);
	else
	{
		logMsg("disabling dithering");
		glcDisable(GL_DITHER);
	}
}

uint dither()
{
	return glcIsEnabled(GL_DITHER);
}

}
