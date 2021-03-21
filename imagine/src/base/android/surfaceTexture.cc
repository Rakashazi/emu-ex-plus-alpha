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

#define LOGTAG "SurfaceTex"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/util/jni.hh>
#include <imagine/logger/logger.h>
#include "android.hh"

namespace Base
{

static jclass jSurfaceCls{}, jSurfaceTextureCls{};
static JavaInstMethod<void(jobject)> jSurface{};
static JavaInstMethod<void()> jSurfaceRelease{};
static JavaInstMethod<void(jint)> jSurfaceTexture{};
static JavaInstMethod<void(jint, jboolean)> jSurfaceTexture2{};
static JavaInstMethod<void()> jUpdateTexImage{};
static JavaInstMethod<void()> jReleaseTexImage{};
static JavaInstMethod<void()> jSurfaceTextureRelease{};

static void initSurfaceTextureJNI(ApplicationContext app, JNIEnv *env)
{
	assert(app.androidSDK() >= 14);
	if(likely(jSurfaceTextureCls))
		return;
	jSurfaceTextureCls = (jclass)env->NewGlobalRef(env->FindClass("android/graphics/SurfaceTexture"));
	jSurfaceTexture.setup(env, jSurfaceTextureCls, "<init>", "(I)V");
	if(app.androidSDK() >= 19)
	{
		jSurfaceTexture2.setup(env, jSurfaceTextureCls, "<init>", "(IZ)V");
		jReleaseTexImage.setup(env, jSurfaceTextureCls, "releaseTexImage", "()V");
	}
	jUpdateTexImage.setup(env, jSurfaceTextureCls, "updateTexImage", "()V");
	jSurfaceTextureRelease.setup(env, jSurfaceTextureCls, "release", "()V");
}

static void initSurfaceJNI(JNIEnv *env)
{
	if(likely(jSurfaceCls))
		return;
	jSurfaceCls = (jclass)env->NewGlobalRef(env->FindClass("android/view/Surface"));
	jSurface.setup(env, jSurfaceCls, "<init>", "(Landroid/graphics/SurfaceTexture;)V");
	jSurfaceRelease.setup(env, jSurfaceCls, "release", "()V");
}

jobject makeSurfaceTexture(ApplicationContext app, JNIEnv *env, jint texName)
{
	if(app.androidSDK() < 14)
		return nullptr;
	initSurfaceTextureJNI(app, env);
	return env->NewObject(jSurfaceTextureCls, jSurfaceTexture.method, texName);
}

jobject makeSurfaceTexture(ApplicationContext app, JNIEnv *env, jint texName, jboolean singleBufferMode)
{
	if(!singleBufferMode)
		return makeSurfaceTexture(app, env, texName);
	if(app.androidSDK() < 19)
		return nullptr;
	initSurfaceTextureJNI(app, env);
	return env->NewObject(jSurfaceTextureCls, jSurfaceTexture2.method, texName, singleBufferMode);
}

bool releaseSurfaceTextureImage(JNIEnv *env, jobject surfaceTexture)
{
	assumeExpr(jReleaseTexImage);
	jReleaseTexImage(env, surfaceTexture);
	if(unlikely(env->ExceptionCheck()))
	{
		logErr("exception in releaseTexImage()");
		env->ExceptionClear();
		return false;
	}
	return true;
}

void updateSurfaceTextureImage(JNIEnv *env, jobject surfaceTexture)
{
	jUpdateTexImage(env, surfaceTexture);
}

void releaseSurfaceTexture(JNIEnv *env, jobject surfaceTexture)
{
	jSurfaceTextureRelease(env, surfaceTexture);
}

jobject makeSurface(JNIEnv *env, jobject surfaceTexture)
{
	initSurfaceJNI(env);
	return env->NewObject(jSurfaceCls, jSurface.method, surfaceTexture);
}

void releaseSurface(JNIEnv *env, jobject surface)
{
	jSurfaceRelease(env, surface);
}

}
