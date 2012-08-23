#pragma once

#ifdef CONFIG_INPUT
	#include <input/interface.h>
#endif

#ifdef CONFIG_BASE_ANDROID
#include <base/android/libhardwarePrivate.h>
#include <base/android/private.hh>
#include <base/android/public.hh>
#endif

#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
	#include <dlfcn.h>
	#if CONFIG_ENV_ANDROID_MINSDK < 9
		#include <base/android/EGLPrivate.h>
		static EGLDisplay eglDisplay = 0;
		static EGLImageKHR (*eglCreateImageKHR)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list) = 0;
		static EGLBoolean (*eglDestroyImageKHR)(EGLDisplay dpy, EGLImageKHR image) = 0;
		static EGLDisplay (*eglGetDisplay)(EGLNativeDisplayType display_id);
		namespace Base
		{
			static EGLDisplay getAndroidEGLDisplay() { return eglDisplay; }
		}
	#else
		#include <EGL/egl.h>
		#define EGL_EGLEXT_PROTOTYPES
		#include <EGL/eglext.h>
		namespace Base
		{
			EGLDisplay getAndroidEGLDisplay();
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
	logMsg("glViewport %d:%d:%d:%d from window %d:%d:%d:%d", win.rect.x, win.h - win.rect.y2, width, height,
			win.rect.x, win.rect.y, win.rect.x2, win.rect.y2);
	glViewport(win.rect.x,
		win.h - win.rect.y2,
		width, height);

	if(width == 0 || height == 0)
	{
		bug_exit("view is invisible");
		return; // view is invisible, do nothing
	}

	glcMatrixMode(GL_PROJECTION);

	//fovy = 45.0;
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
	if(animateOrientationChange)
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
		logMsg("no rotation");
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
}

void resizeDisplay(const Base::Window &win)
{
	Gfx::GfxViewState oldState =
	{
		proj.w, proj.h, proj.aspectRatio,
		viewPixelWidth_, viewPixelHeight_
	};
	resizeGLScene(win);

	//logMsg("calling view space callback %p", viewSpaceCallback.func);
	//callSafe(viewChangeHandler, viewChangeHandlerCtx, &oldState);
	Gfx::onViewChange(&oldState);
}

#ifdef CONFIG_GFX_SOFT_ORIENTATION

#ifdef CONFIG_INPUT
void configureInputForOrientation()
{
	input_xPointerTransform(rotateView == VIEW_ROTATE_0 || rotateView == VIEW_ROTATE_90 ? INPUT_POINTER_NORMAL : INPUT_POINTER_INVERT);
	input_yPointerTransform(rotateView == VIEW_ROTATE_0 || rotateView == VIEW_ROTATE_270 ? INPUT_POINTER_NORMAL : INPUT_POINTER_INVERT);
	input_pointerAxis(rotateView == VIEW_ROTATE_0 || rotateView == VIEW_ROTATE_180 ? INPUT_POINTER_NORMAL : INPUT_POINTER_INVERT);
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

static void checkForAnisotropicFiltering()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	if(!forceNoAnisotropicFiltering && GLEW_EXT_texture_filter_anisotropic)
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnisotropy);
		logMsg("anisotropic filtering supported, max value: %f", maximumAnisotropy);
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

static void checkForAutoMipmapGeneration()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	if(!forceNoAutoMipmapGeneration && GLEW_SGIS_generate_mipmap)
	{
		logMsg("automatic mipmap generation supported");
		useAutoMipmapGeneration = 1;

		//glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
	}
	#else
		useAutoMipmapGeneration = 1;
	#endif
}

static uchar useMultisample = 0;
static uchar forceNoMultisample = 1;
static uchar forceNoMultisampleHint = 0;

static void checkForMultisample()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	if(!forceNoMultisample && GLEW_ARB_multisample)
	{
		logMsg("multisample antialiasing supported");
		useMultisample = 1;
		if(!forceNoMultisampleHint && GLEW_NV_multisample_filter_hint)
		{
			logMsg("multisample hints supported");
			glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
		}
		glcEnable(GL_MULTISAMPLE_ARB);
	}
	#endif
}

static void checkForVertexArrays()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	if(!GLEW_VERSION_1_1)
	{
		logErr("OpenGL 1.1 vertex arrays not supported");
		Base::exit();
	}
	#endif
}

static uchar forceNoNonPow2Textures = 0;

static void checkForNonPow2Textures()
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
	if(GLEW_ARB_texture_non_power_of_two)
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

#if defined(CONFIG_BASE_ANDROID)
	// Android is missing GL_BGRA in ES 1 headers
	#define GL_BGRA 0x80E1
#elif defined(CONFIG_GFX_OPENGL_ES) && defined(CONFIG_BASE_X11)
	// Mesa uses GL_BGRA_EXT
	#define GL_BGRA GL_BGRA_EXT
#endif

static void checkForBGRPixelSupport()
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
		if(!forceNoBGRPixels && GLEW_VERSION_1_2)
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

static uchar useTextureClampToEdge = 0;
static uchar forceNoTextureClampToEdge = 0;

static void checkForTextureClampToEdge()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	if(!forceNoTextureClampToEdge && GLEW_VERSION_1_2)
	{
		useTextureClampToEdge = 1;
		logMsg("Texture clamp to edge mode supported");
	}
	#else
	useTextureClampToEdge = 1;
	#endif
}

static uchar supportCompressedTextures = 0;
static uchar useCompressedTextures = 0;
static uchar forceNoCompressedTextures = 1;

static void checkForCompressedTexturesSupport()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	if(!forceNoCompressedTextures && GLEW_VERSION_1_3)
	{
		supportCompressedTextures = 1;
		useCompressedTextures = 1;
		logMsg("Compressed textures are supported");
	}
	#endif
}

static uchar useFBOFuncs = 0;
static uchar forceNoFBOFuncs = 1;

static void checkForFBOFuncs()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	if(!forceNoFBOFuncs && GLEW_EXT_framebuffer_object)
	{
		useFBOFuncs = 1;
		logMsg("FBO functions are supported");
	}
	#endif
}

static uchar useVBOFuncs = 0;
static uchar forceNoVBOFuncs = 1;

static void checkForVBO()
{
	if(!forceNoVBOFuncs &&
	#ifndef CONFIG_GFX_OPENGL_ES
		GLEW_VERSION_1_5
	#else
		strstr(version, " 1.0") == NULL // make sure OpenGL-ES is not 1.0
	#endif
	)
	{
		useVBOFuncs = 1;
		logMsg("VBOs are supported");
	}
}

#if defined(CONFIG_GFX_OPENGL_ES) && !defined(CONFIG_BASE_PS3)

static bool useDrawTex = 0;
static bool forceNoDrawTex = 0;

static void checkForDrawTexture()
{
	// Limited usefulness due to no 90deg rotation support
	if(!forceNoDrawTex && strstr(extensions, "GL_OES_draw_texture") != 0)
	{
		useDrawTex = 1;
		logMsg("Draw Texture supported");
	}
}

#endif

#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE

struct DirectTextureGfxBufferImage: public TextureGfxBufferImage
{
	constexpr DirectTextureGfxBufferImage() { }
	Pixmap eglPixmap;
	android_native_buffer_t eglBuf;
	EGLImageKHR eglImg = EGL_NO_IMAGE_KHR;

	static bool testSupport(const char **errorStr);
	bool init(Pixmap &pix, uint texRef, uint usedX, uint usedY, const char **errorStr = nullptr);
	void write(Pixmap &p, uint hints);
	Pixmap *lock(uint x, uint y, uint xlen, uint ylen);
	void unlock();
	void deinit();
};

static void dummyIncRef(struct android_native_base_t* base)
{
	logMsg("called incRef");
}

static void dummyDecRef(struct android_native_base_t* base)
{
	logMsg("called decRef");
}

static void setupAndroidNativeBuffer(android_native_buffer_t &eglBuf, int x, int y, int format, int usage)
{
	mem_zero(eglBuf);
	eglBuf.common.magic = ANDROID_NATIVE_BUFFER_MAGIC;
	eglBuf.common.version = sizeof(android_native_buffer_t);
	//memset(eglBuf.common.reserved, 0, sizeof(eglBuf.common.reserved));
	eglBuf.common.incRef = dummyIncRef;
	eglBuf.common.decRef = dummyDecRef;
	//memset(eglBuf.reserved, 0, sizeof(eglBuf.reserved));
	//memset(eglBuf.reserved_proc, 0, sizeof(eglBuf.reserved_proc));
	eglBuf.width = x;
	eglBuf.height = y;
	//eglBuf.stride = 0;
	eglBuf.format = format;
	//eglBuf.handle = 0;
	eglBuf.usage = usage;
}

struct AndroidDirectTextureConfig
{
	bool useEGLImageKHR = 0, whitelistedEGLImageKHR = 0;
	const char *errorStr = "";
	static const EGLint eglImgAttrs[];//= { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE, EGL_NONE };
	static const GLenum directTextureTarget;// = GL_TEXTURE_2D;
private:
	int (*hw_get_module)(const char *id, const struct hw_module_t **module) = nullptr;
	gralloc_module_t const *grallocMod = nullptr;
	alloc_device_t *allocDev = nullptr;

public:
	constexpr AndroidDirectTextureConfig() { }

	bool isSupported() const { return grallocMod; }

	void checkForEGLImageKHR(const char *rendererName)
	{
		if(strstr(rendererName, "NVIDIA") // disable on Tegra, unneeded and causes lock-ups currently
			|| string_equal(rendererName, "VideoCore IV HW")) // seems to crash Samsung Galaxy Y on eglCreateImageKHR, maybe other devices
		{
			logMsg("force-disabling EGLImageKHR due to GPU");
			errorStr = "Unsupported GPU";
		}
		else
		{
			if(strstr(rendererName, "SGX 530")) // enable on PowerVR SGX 530, though it should work on other models
			{
				logMsg("white-listed for EGLImageKHR");
				whitelistedEGLImageKHR = 1;
			}

			if(!enableEGLImageKHR())
			{
				logWarn("can't use EGLImageKHR: %s", errorStr);
			}
		}
	}

	bool enableEGLImageKHR()
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

		void *libegl = 0, *libhardware = 0;

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

		if((libhardware = dlopen("/system/lib/libhardware.so", RTLD_LOCAL | RTLD_LAZY)) == 0)
		{
			errorStr = verbose ? "Can't load libhardware.so" : basicLibhardwareErrorStr;
			goto FAIL;
		}

		//logMsg("hw_get_module @ %p", hw_get_module);
		if((hw_get_module = (int(*)(const char *, const struct hw_module_t **))dlsym(libhardware, "hw_get_module")) == 0)
		{
			errorStr = verbose ? "Can't find hw_get_module" : basicLibhardwareErrorStr;
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

		if(!DirectTextureGfxBufferImage::testSupport(&errorStr))
		{
			goto FAIL;
		}

		useEGLImageKHR = 1;
		logMsg("using EGLImageKHR");
		return 1;

		FAIL:
		if(libegl)
		{
			dlclose(libegl);
			libegl = nullptr;
		}
		if(libhardware)
		{
			dlclose(libhardware);
			libhardware = nullptr;
		}
		grallocMod = nullptr;
		//TODO: free allocDev if needed

		return 0;
	}

	int allocBuffer(android_native_buffer_t &eglBuf)
	{
		assert(allocDev);
		return allocDev->alloc(allocDev, eglBuf.width, eglBuf.height, eglBuf.format,
			eglBuf.usage, &eglBuf.handle, &eglBuf.stride);
	}

	int lockBuffer(android_native_buffer_t &eglBuf, int usage, int l, int t, int w, int h, void *&data)
	{
		assert(grallocMod);
		return grallocMod->lock(grallocMod, eglBuf.handle, usage, l, t, w, h, &data);
	}

	int unlockBuffer(android_native_buffer_t &eglBuf)
	{
		return grallocMod->unlock(grallocMod, eglBuf.handle);
	}

	int freeBuffer(android_native_buffer_t &eglBuf)
	{
		if(allocDev->free)
			return allocDev->free(allocDev, eglBuf.handle);
		else
		{
			logWarn("no android native buffer free()");
			return 0;
		}
	}
};

const EGLint AndroidDirectTextureConfig::eglImgAttrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE, EGL_NONE };
const GLenum AndroidDirectTextureConfig::directTextureTarget = GL_TEXTURE_2D;

static AndroidDirectTextureConfig directTextureConf;

namespace Gfx
{

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
