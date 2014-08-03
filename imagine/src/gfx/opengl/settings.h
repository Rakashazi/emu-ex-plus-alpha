#pragma once

#include "private.hh"

#ifdef __ANDROID__
#include "../../base/android/android.hh"
#include <imagine/base/android/public.hh>
#include "android/DirectTextureBufferImage.hh"
#include <dlfcn.h>
namespace Gfx
{
	static void setupAndroidOGLExtensions(const char *extensions, const char *rendererName);
}
#endif

namespace Gfx
{

static Viewport currViewport;
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

static uint maxOpenGLMajorVersionSupport()
{
	#ifdef CONFIG_GFX_OPENGL_ES
		#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
		return 2;
		#else
		return 1;
		#endif
	#else
		#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
		return 3;
		#else
		return 1;
		#endif
	#endif
}

void setViewport(const Viewport &v)
{
	auto inGLFormat = v.inGLFormat();
	//logMsg("set GL viewport %d:%d:%d:%d", inGLFormat.x, inGLFormat.y, inGLFormat.x2, inGLFormat.y2);
	assert(inGLFormat.x2 && inGLFormat.y2);
	glViewport(inGLFormat.x, inGLFormat.y, inGLFormat.x2, inGLFormat.y2);
	currViewport = v;
}

const Viewport &viewport()
{
	return currViewport;
}

Viewport Viewport::makeFromRect(const IG::WindowRect &fullRect, const IG::WindowRect &fullRealRect, const IG::WindowRect &rect)
{
	Viewport v;
	v.rect = rect;
	v.w = rect.xSize();
	v.h = rect.ySize();
	float wScaler = v.w / (float)fullRect.xSize();
	float hScaler = v.h / (float)fullRect.ySize();
	v.wMM = 1;
	v.hMM = 1;
	#ifdef __ANDROID__
	v.wSMM = 1;
	v.hSMM = 1;
	#endif
	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	v.rotateView = 0;
	#endif
	// glViewport() needs flipped Y and relative size
	v.relYFlipViewport = {v.realBounds().x, fullRealRect.ySize() - v.realBounds().y2, v.realWidth(), v.realHeight()};
	//logMsg("transformed for GL %d:%d:%d:%d", v.relYFlipViewport.x, v.relYFlipViewport.y, v.relYFlipViewport.x2, v.relYFlipViewport.y2);
	return v;
}

Viewport Viewport::makeFromWindow(const Base::Window &win, const IG::WindowRect &rect)
{
	Viewport v;
	v.rect = rect;
	v.w = rect.xSize();
	v.h = rect.ySize();
	float wScaler = v.w / (float)win.width();
	float hScaler = v.h / (float)win.height();
	v.wMM = win.widthMM() * wScaler;
	v.hMM = win.heightMM() * hScaler;
	#ifdef __ANDROID__
	v.wSMM = win.widthSMM() * wScaler;
	v.hSMM = win.heightSMM() * hScaler;
	#endif
	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	v.rotateView = win.rotateView;
	#endif
	//logMsg("made viewport %d:%d:%d:%d from window %d:%d",
	//	v.rect.x, v.rect.y, v.rect.x2, v.rect.y2,
	//	win.width(), win.height());

	// glViewport() needs flipped Y and relative size
	v.relYFlipViewport = {v.realBounds().x, win.realHeight() - v.realBounds().y2, v.realWidth(), v.realHeight()};
	//logMsg("transformed for GL %d:%d:%d:%d", v.relYFlipViewport.x, v.relYFlipViewport.y, v.relYFlipViewport.x2, v.relYFlipViewport.y2);
	return v;
}

#if defined __ANDROID__ && defined CONFIG_GFX_OPENGL_USE_DRAW_TEXTURE

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

#if defined __ANDROID__
void AndroidDirectTextureConfig::checkForEGLImageKHR(const char *extensions, const char *rendererName)
{
	if((Config::MACHINE_IS_GENERIC_ARMV7 &&  strstr(rendererName, "NVIDIA")) // disable on Tegra, unneeded and causes lock-ups currently
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
		if(Config::MACHINE_IS_GENERIC_ARMV7 && strstr(rendererName, "SGX 530")) // enable on PowerVR SGX 530, though it should work on other models
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

#endif

}
