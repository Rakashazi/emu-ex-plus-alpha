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

#include <imagine/pixmap/PixelFormat.hh>
#include <jni.h>

namespace Base
{

class ApplicationContext;

class NoopThread
{
public:
	constexpr NoopThread() {}
	void start();
	void stop();
	explicit operator bool() { return runFlagAddr; }

private:
	bool *runFlagAddr{};
};

jobject makeSurfaceTexture(ApplicationContext, JNIEnv *, jint texName);
jobject makeSurfaceTexture(ApplicationContext, JNIEnv *, jint texName, jboolean singleBufferMode);
bool releaseSurfaceTextureImage(JNIEnv *env, jobject surfaceTexture);
void updateSurfaceTextureImage(JNIEnv *env, jobject surfaceTexture);
void releaseSurfaceTexture(JNIEnv *env, jobject surfaceTexture);

jobject makeSurface(JNIEnv *env, jobject surfaceTexture);
void releaseSurface(JNIEnv *env, jobject surface);

uint32_t toAHardwareBufferFormat(IG::PixelFormatID);
const char *aHardwareBufferFormatStr(uint32_t format);

enum SurfaceRotation : uint8_t
{
	SURFACE_ROTATION_0 = 0, SURFACE_ROTATION_90 = 1,
	SURFACE_ROTATION_180 = 2, SURFACE_ROTATION_270 = 3
};

static bool surfaceRotationIsStraight(SurfaceRotation o)
{
	return o == SURFACE_ROTATION_0 || o == SURFACE_ROTATION_180;
}

}
