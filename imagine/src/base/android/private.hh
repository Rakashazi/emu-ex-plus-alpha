#pragma once

#include <engine-globals.h>
#include <util/jni.hh>

#if CONFIG_ENV_ANDROID_MINSDK >= 9
	#include "nativeGlue.hh"
#endif

#include <base/android/privateApi/gralloc.h>
#include <util/pixel.h>
#if CONFIG_ENV_ANDROID_MINSDK < 9
	#include <base/android/privateApi/EGL.h>
#else
	#include <EGL/egl.h>
	#define EGL_EGLEXT_PROTOTYPES
	#include <EGL/eglext.h>
#endif

namespace Base
{

extern JavaVM* jVM;
//extern JNIEnv* jEnv; // JNIEnv of main thread
JNIEnv* aEnv(); // JNIEnv of activity thread
JNIEnv* eEnv(); // JNIEnv of main event thread

#if CONFIG_ENV_ANDROID_MINSDK >= 9
// NativeActivity Instance
android_app *appInstance();
#endif

// BaseActivity JNI
extern jclass jBaseActivityCls;
extern jobject jBaseActivity;
extern JavaInstMethod<void> postUIThread;
extern JavaInstMethod<void> jShowIme, jHideIme;

#if CONFIG_ENV_ANDROID_MINSDK >= 9

// Dispatch a command from native glue
void onAppCmd(android_app* app, uint32 cmd);

// Check if EGL surface is valid
bool windowIsDrawable();

// Init the SDK level
void setSDK(uint sdk);

// Init JNI variables from native glue Activity thread
void jniInit(JNIEnv *jEnv, jobject inst);

// Android Bluetooth
static const ushort MSG_BT_SOCKET_STATUS_DELEGATE = 151;

#endif

// EditText-based Input
void sendTextEntryEnded(const char *str, jstring jStr);

EGLDisplay getAndroidEGLDisplay();

}

struct ANativeWindow;

namespace Gfx
{

// SurfaceTexture JNI
struct AndroidSurfaceTextureConfig
{
	constexpr AndroidSurfaceTextureConfig() { }
	jclass jSurfaceCls = nullptr, jSurfaceTextureCls = nullptr;
	JavaInstMethod<void> jSurface, jSurfaceRelease,
		jSurfaceTexture, jUpdateTexImage, jSurfaceTextureRelease/*, jSetDefaultBufferSize*/;
	bool use = 0, whiteListed = 1;
	bool texture2dBindingHack = 0;
	// Extra dlsym function from libandroid.so
	//ANativeWindow* (*ANativeWindow_fromSurfaceTexture)(JNIEnv* env, jobject surfaceTexture) = nullptr;

	void init(JNIEnv *jEnv);
	void deinit();

	bool isSupported()
	{
		return jSurfaceTextureCls;
	}

	void setUse(bool on)
	{
		if(isSupported())
			use = on;
	}
};

extern AndroidSurfaceTextureConfig surfaceTextureConf;

#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE

struct AndroidDirectTextureConfig
{
	bool useEGLImageKHR = 0, whitelistedEGLImageKHR = 0;
	const char *errorStr = "";
private:
	gralloc_module_t const *grallocMod = nullptr;
	alloc_device_t *allocDev = nullptr;

public:
	constexpr AndroidDirectTextureConfig() { }
	bool isSupported() const { return grallocMod; }
	void checkForEGLImageKHR(const char *extensions, const char *rendererName);
	bool setupEGLImageKHR(const char *extensions);
	int allocBuffer(android_native_buffer_t &eglBuf);
	int lockBuffer(android_native_buffer_t &eglBuf, int usage, int l, int t, int w, int h, void *&data);
	int unlockBuffer(android_native_buffer_t &eglBuf);
	int freeBuffer(android_native_buffer_t &eglBuf);
};

extern AndroidDirectTextureConfig directTextureConf;

#endif

static int pixelFormatToDirectAndroidFormat(const PixelFormatDesc &format)
{
	switch(format.id)
	{
		case PIXEL_RGBA8888: return HAL_PIXEL_FORMAT_RGBA_8888;
		case PIXEL_BGRA8888: return HAL_PIXEL_FORMAT_BGRA_8888;
		//case PIXEL_RGB888: return HAL_PIXEL_FORMAT_RGB_888;
		case PIXEL_RGB565: return HAL_PIXEL_FORMAT_RGB_565;
		case PIXEL_ARGB1555: return HAL_PIXEL_FORMAT_RGBA_5551;
		case PIXEL_ARGB4444: return HAL_PIXEL_FORMAT_RGBA_4444;
		//case PIXEL_I8: return GGL_PIXEL_FORMAT_L_8;
		//case PIXEL_IA88: return GGL_PIXEL_FORMAT_LA_88;
		default: return GGL_PIXEL_FORMAT_NONE;
	}
}

}
