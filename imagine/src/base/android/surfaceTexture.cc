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

namespace IG
{

static jclass jSurfaceCls{}, jSurfaceTextureCls{};
static JNI::InstMethod<void(jobject)> jSurface{};
static JNI::InstMethod<void()> jSurfaceRelease{};
static JNI::InstMethod<void(jint)> jSurfaceTexture{};
static JNI::InstMethod<void(jint, jboolean)> jSurfaceTexture2{};
static JNI::InstMethod<void()> jUpdateTexImage{};
static JNI::InstMethod<void()> jReleaseTexImage{};
static JNI::InstMethod<void()> jSurfaceTextureRelease{};

static void initSurfaceTextureJNI(ApplicationContext ctx, JNIEnv *env)
{
	assert(ctx.androidSDK() >= 14);
	if(jSurfaceTextureCls) [[likely]]
		return;
	jSurfaceTextureCls = (jclass)env->NewGlobalRef(env->FindClass("android/graphics/SurfaceTexture"));
	jSurfaceTexture = {env, jSurfaceTextureCls, "<init>", "(I)V"};
	if(ctx.androidSDK() >= 19)
	{
		jSurfaceTexture2 = {env, jSurfaceTextureCls, "<init>", "(IZ)V"};
		jReleaseTexImage = {env, jSurfaceTextureCls, "releaseTexImage", "()V"};
	}
	jUpdateTexImage = {env, jSurfaceTextureCls, "updateTexImage", "()V"};
	jSurfaceTextureRelease = {env, jSurfaceTextureCls, "release", "()V"};
}

static void initSurfaceJNI(JNIEnv *env)
{
	if(jSurfaceCls) [[likely]]
		return;
	jSurfaceCls = (jclass)env->NewGlobalRef(env->FindClass("android/view/Surface"));
	jSurface = {env, jSurfaceCls, "<init>", "(Landroid/graphics/SurfaceTexture;)V"};
	jSurfaceRelease = {env, jSurfaceCls, "release", "()V"};
}

jobject makeSurfaceTexture(ApplicationContext ctx, JNIEnv *env, jint texName)
{
	if(ctx.androidSDK() < 14)
		return nullptr;
	initSurfaceTextureJNI(ctx, env);
	return env->NewObject(jSurfaceTextureCls, jSurfaceTexture.method, texName);
}

jobject makeSurfaceTexture(ApplicationContext ctx, JNIEnv *env, jint texName, jboolean singleBufferMode)
{
	if(!singleBufferMode)
		return makeSurfaceTexture(ctx, env, texName);
	if(ctx.androidSDK() < 19)
		return nullptr;
	initSurfaceTextureJNI(ctx, env);
	return env->NewObject(jSurfaceTextureCls, jSurfaceTexture2.method, texName, singleBufferMode);
}

bool releaseSurfaceTextureImage(JNIEnv *env, jobject surfaceTexture)
{
	assumeExpr(jReleaseTexImage);
	jReleaseTexImage(env, surfaceTexture);
	if(env->ExceptionCheck()) [[unlikely]]
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
