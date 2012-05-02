#pragma once

#include <util/jni.hh>

#if CONFIG_ENV_ANDROID_MINSDK >= 9
	#include "nativeGlue.hh"
#endif

namespace Base
{

extern JNIEnv* jEnv; // JNIEnv of main thread

// BaseActivity JNI
extern jclass jBaseActivityCls;
extern jobject jBaseActivity;

// SurfaceTexture JNI
extern jclass jSurfaceTextureCls;
extern JavaInstMethod jSurfaceTexture, jUpdateTexImage, jSurfaceTextureRelease/*, jSetDefaultBufferSize*/;

#if CONFIG_ENV_ANDROID_MINSDK >= 9

// Dispatch a command from native glue
void onAppCmd(android_app* app, uint32 cmd);

// Check if EGL surface is valid
bool windowIsDrawable();

// Init the SDK level
void setSDK(uint sdk);

// Init JNI variables from native glue Activity thread
void jniInit(JNIEnv *jEnv, jobject inst);

#endif

}

// Extra dlsym function for SurfaceTexture
struct ANativeWindow;
extern ANativeWindow* (*ANativeWindow_fromSurfaceTexture)(JNIEnv* env, jobject surfaceTexture);
