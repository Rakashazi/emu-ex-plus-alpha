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

#include <imagine/config/defs.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <jni.h>
#include <android/api-level.h>
#include <array>

namespace Base
{

using AndroidPropString = std::array<char, 92>;

enum class SustainedPerformanceType
{
	NONE,
	DEVICE,
	NOOP
};

JNIEnv *jEnvForThread();
uint32_t androidSDK();
bool apkSignatureIsConsistent();
AndroidPropString androidBuildDevice();
bool packageIsInstalled(const char *name);
SustainedPerformanceType sustainedPerformanceModeType();
void setSustainedPerformanceMode(bool on);
IG::PixelFormat makePixelFormatFromAndroidFormat(int32_t androidFormat);
IG::Pixmap makePixmapView(JNIEnv *env, jobject bitmap, void *pixels, IG::PixelFormat format = IG::PIXEL_FMT_NONE);

}

namespace Input
{

void initMOGA(bool notify);
void deinitMOGA();
bool mogaSystemIsActive();
void enumDevices();
bool hasTrackball();

}
