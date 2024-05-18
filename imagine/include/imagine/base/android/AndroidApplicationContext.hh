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
#include <android/native_activity.h>
#include <jni.h>
#include <string>

struct ANativeActivity;
struct AAssetManager;

namespace IG::FS
{
struct PathLocation;
}

namespace IG
{

class Application;

using NativeDisplayConnection = void*;

class AndroidApplicationContext
{
public:
	constexpr AndroidApplicationContext() = default;
	constexpr AndroidApplicationContext(ANativeActivity *act):act{act} {}
	constexpr ANativeActivity *aNativeActivityPtr() const { return act; }
	Application &application() const { return *static_cast<Application*>(act->instance); }
	JNIEnv *mainThreadJniEnv() const;
	JNIEnv *thisThreadJniEnv() const;
	jobject baseActivityObject() const;
	AAssetManager *aAssetManager() const;
	std::string androidBuildDevice() const;
	void setNoopThreadActive(bool on);
	FS::PathLocation externalMediaPathLocation() const;

	// Input system functions
	bool hasTrackball() const;

protected:
	ANativeActivity *act{};
};

using ApplicationContextImpl = AndroidApplicationContext;

}
