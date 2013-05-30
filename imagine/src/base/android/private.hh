#pragma once

#include <engine-globals.h>
#include <util/jni.hh>

#include <base/android/privateApi/gralloc.h>
#include <util/pixel.h>
#if CONFIG_ENV_ANDROID_MINSDK < 9
	#include <base/android/privateApi/EGL.h>
#else
	#include <EGL/egl.h>
	#define EGL_EGLEXT_PROTOTYPES
	#include <EGL/eglext.h>
#endif
#include <android/looper.h>

struct ANativeActivity;
class BluetoothSocket;

namespace Base
{

extern JavaVM* jVM;
JNIEnv* eEnv(); // JNIEnv of main event thread
jobject jniThreadNewGlobalRef(JNIEnv* jEnv, jobject obj);
void jniThreadDeleteGlobalRef(JNIEnv* jEnv, jobject obj);

// BaseActivity JNI
extern jclass jBaseActivityCls;
extern jobject jBaseActivity;

#if CONFIG_ENV_ANDROID_MINSDK >= 9

void postDrawWindowIfNeeded();

// Android Bluetooth
static const ushort MSG_BT_SOCKET_STATUS_DELEGATE = 151;

#endif

EGLDisplay getAndroidEGLDisplay();

bool hasHardKeyboard();
int hardKeyboardState();
int keyboardType();

// Activity thread ALooper
ALooper *activityLooper();

void sendBTSocketData(BluetoothSocket &socket, int len, jbyte *data);

jobject newFontRenderer(JNIEnv *jEnv);

void restoreOpenGLContext();

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

namespace Audio
{
	void updateFocusOnPause();
	void updateFocusOnResume();
}
