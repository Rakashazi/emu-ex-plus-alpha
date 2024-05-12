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

#include <imagine/pixmap/Pixmap.hh>
#include <jni.h>
#include <sys/types.h>
#include <atomic>

namespace IG
{

class ApplicationContext;

class NoopThread
{
public:
	constexpr NoopThread() {}
	void start();
	void stop();
	explicit operator bool() { return isRunning.load(std::memory_order_relaxed); }

private:
	std::atomic_bool isRunning{};
};

extern pid_t mainThreadId;

jobject makeSurfaceTexture(ApplicationContext, JNIEnv *, jint texName);
jobject makeSurfaceTexture(ApplicationContext, JNIEnv *, jint texName, jboolean singleBufferMode);
bool releaseSurfaceTextureImage(JNIEnv *env, jobject surfaceTexture);
void updateSurfaceTextureImage(JNIEnv *env, jobject surfaceTexture);
void releaseSurfaceTexture(JNIEnv *env, jobject surfaceTexture);

jobject makeSurface(JNIEnv *env, jobject surfaceTexture);
void releaseSurface(JNIEnv *env, jobject surface);

uint32_t toAHardwareBufferFormat(PixelFormatId);
const char *aHardwareBufferFormatStr(uint32_t format);

PixelFormat makePixelFormatFromAndroidFormat(int32_t androidFormat);
MutablePixmapView makePixmapView(JNIEnv *env, jobject bitmap, void *pixels, PixelFormat format);

}
