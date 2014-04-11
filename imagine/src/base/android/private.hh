#pragma once

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

#include <imagine/engine-globals.h>
#include <imagine/util/jni.hh>
#include "privateApi/gralloc.h"
#include <imagine/util/pixel.h>
#include <android/looper.h>
#include <android/asset_manager.h>
#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>

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

// Android Bluetooth
static const ushort MSG_BT_SOCKET_STATUS_DELEGATE = 151;

#endif

EGLDisplay getAndroidEGLDisplay();

bool hasHardKeyboard();
int hardKeyboardState();
int keyboardType();

// Activity thread ALooper
void setupActivityLooper();
ALooper *activityLooper();

AAssetManager *activityAAssetManager();

jobject newFontRenderer(JNIEnv *jEnv);

void restoreOpenGLContext();
void unrefUIGL();
void refUIGL();

void doOnResume(ANativeActivity* activity);
void handleIntent(ANativeActivity* activity);

bool hasLowLatencyAudio();

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
