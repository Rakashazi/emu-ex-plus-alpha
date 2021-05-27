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
#include <jni.h>
#include <array>

struct ANativeActivity;
struct AAssetManager;

namespace IG
{
class PixelFormat;
class Pixmap;
}

namespace Base
{

class Application;

using AndroidPropString = std::array<char, 92>;

enum class SustainedPerformanceType
{
	NONE,
	DEVICE,
	NOOP
};

enum SurfaceRotation : uint8_t;

class AndroidApplicationContext
{
public:
	constexpr AndroidApplicationContext() {}
	constexpr AndroidApplicationContext(ANativeActivity *act):act{act} {}
	constexpr ANativeActivity *aNativeActivityPtr() const { return act; }
	void setApplicationPtr(Application*);
	Application &application() const;
	JNIEnv *mainThreadJniEnv() const;
	JNIEnv *thisThreadJniEnv() const;
	int32_t androidSDK() const;
	jobject baseActivityObject() const;
	AAssetManager *aAssetManager() const;
	AndroidPropString androidBuildDevice() const;
	SustainedPerformanceType sustainedPerformanceModeType() const;
	void setSustainedPerformanceMode(bool on);
	bool apkSignatureIsConsistent() const;
	bool packageIsInstalled(const char *name) const;

	// Input system functions
	void enumInputDevices();
	void initMogaInputSystem(bool notify);
	void deinitMogaInputSystem();
	bool mogaInputSystemIsActive() const;
	bool hasTrackball() const;

protected:
	ANativeActivity *act{};
};

IG::PixelFormat makePixelFormatFromAndroidFormat(int32_t androidFormat);
IG::Pixmap makePixmapView(JNIEnv *env, jobject bitmap, void *pixels, IG::PixelFormat format);


using ApplicationContextImpl = AndroidApplicationContext;

}
