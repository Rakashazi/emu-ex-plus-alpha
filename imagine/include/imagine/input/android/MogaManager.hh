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

#include <imagine/base/ApplicationContext.hh>
#include <imagine/util/jni.hh>

namespace IG::Input
{

class AndroidInputDevice;

class MogaManager
{
public:
	MogaManager(ApplicationContext, bool notify);
	~MogaManager();
	explicit operator bool() const { return mogaHelper; }

private:
	JNI::UniqueGlobalRef mogaHelper{};
	JNI::InstMethod<jint(jint)> jMOGAGetState{};
	JNI::InstMethod<void()> jMOGAOnPause{}, jMOGAOnResume{}, jMOGAExit{};
	Device *mogaDev{};
	OnExit onExit;

	static std::unique_ptr<Device> makeMOGADevice(const char *name);
	void initMOGAJNIAndDevice(JNIEnv *, jobject mogaHelper);
	void onResumeMOGA(JNIEnv *, bool notify);
	void updateMOGAState(JNIEnv *env, bool connected, bool notify);
	ApplicationContext appContext() const;
};

}
