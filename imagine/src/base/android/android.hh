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

#include <imagine/base/android/android.hh>
#include <imagine/util/jni.hh>
#include <imagine/util/pixel.h>
#include <android/looper.h>
#include <android/asset_manager.h>
#include "privateApi/GraphicBuffer.hh"

class BluetoothSocket;
struct ANativeWindow;

namespace Base
{

extern JavaVM* jVM;
JNIEnv* jEnv(); // JNIEnv of activity thread

// BaseActivity JNI
extern jclass jBaseActivityCls;
extern jobject jBaseActivity;

// Activity thread ALooper
ALooper *activityLooper();

AAssetManager *activityAAssetManager();

jobject newFontRenderer(JNIEnv *env);

bool hasLowLatencyAudio();

jobject makeSurfaceTexture(JNIEnv *env, jint texName);
jobject makeSurfaceTexture(JNIEnv *env, jint texName, jboolean singleBufferMode);
void releaseSurfaceTextureImage(JNIEnv *env, jobject surfaceTexture);
void updateSurfaceTextureImage(JNIEnv *env, jobject surfaceTexture);
void releaseSurfaceTexture(JNIEnv *env, jobject surfaceTexture);

jobject makeSurface(JNIEnv *env, jobject surfaceTexture);
void releaseSurface(JNIEnv *env, jobject surface);

static int pixelFormatToDirectAndroidFormat(const PixelFormatDesc &format)
{
	switch(format.id)
	{
		case PIXEL_RGBA8888: return HAL_PIXEL_FORMAT_RGBA_8888;
		case PIXEL_BGRA8888: return HAL_PIXEL_FORMAT_BGRA_8888;
		case PIXEL_RGB888: return HAL_PIXEL_FORMAT_RGB_888;
		case PIXEL_RGB565: return HAL_PIXEL_FORMAT_RGB_565;
		default: return 0;
	}
}

}

namespace Audio
{
	void updateFocusOnPause();
	void updateFocusOnResume();
}
