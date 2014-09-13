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

#define LOGTAG "Base"
#include <sys/resource.h>
#include <imagine/base/Base.hh>
#include <imagine/logger/logger.h>
#include "internal.hh"
#include "android.hh"

namespace Base
{

static const char *buildDevice = nullptr;
static jobject vibrator = nullptr;
static JavaInstMethod<void> jVibrate;
static bool vibrationSystemIsInit = false;
static JavaInstMethod<jboolean> jPackageIsInstalled;

const char *androidBuildDevice()
{
	if(unlikely(!buildDevice))
	{
		JavaClassMethod<jobject> jDevName;
		jDevName.setup(jEnv(), jBaseActivityCls, "devName", "()Ljava/lang/String;");
		auto devName = (jstring)jDevName(jEnv());
		buildDevice = jEnv()->GetStringUTFChars(devName, nullptr);
		logMsg("device name: %s", buildDevice);
		assert(buildDevice);
	}
	return buildDevice;
}

bool isXperiaPlayDeviceStr(const char *str)
{
	return strstr(str, "R800") || string_equal(str, "zeus");
}

int processPriority()
{
	return getpriority(PRIO_PROCESS, 0);
}

void setProcessPriority(int nice)
{
	assert(nice > -20);
	logMsg("setting process nice level: %d", nice);
	setpriority(PRIO_PROCESS, 0, nice);
}

bool apkSignatureIsConsistent()
{
	bool sigMatchesAPK = true;
	#ifdef ANDROID_APK_SIGNATURE_HASH
	JavaInstMethod<jint> jSigHash;
	auto env = jEnv();
	jSigHash.setup(env, jBaseActivityCls, "sigHash", "()I");
	sigMatchesAPK = jSigHash(env, jBaseActivity) == ANDROID_APK_SIGNATURE_HASH;
	#endif
	return sigMatchesAPK;
}

bool packageIsInstalled(const char *name)
{
	auto env = jEnv();
	if(!jPackageIsInstalled)
		jPackageIsInstalled.setup(env, jBaseActivityCls, "packageIsInstalled", "(Ljava/lang/String;)Z");
	return jPackageIsInstalled(env, jBaseActivity, env->NewStringUTF(name));
}

static void initVibration(JNIEnv* env)
{
	if(likely(vibrationSystemIsInit) || Config::MACHINE_IS_OUYA)
		return;
	{
		JavaInstMethod<jobject> jSysVibrator;
		jSysVibrator.setup(env, jBaseActivityCls, "systemVibrator", "()Landroid/os/Vibrator;");
		vibrator = jSysVibrator(env, jBaseActivity);
	}
	vibrationSystemIsInit = true;
	if(!vibrator)
		return;
	logMsg("Vibrator present");
	vibrator = env->NewGlobalRef(vibrator);
	auto vibratorCls = env->FindClass("android/os/Vibrator");
	jVibrate.setup(env, vibratorCls, "vibrate", "(J)V");
}

#if !defined CONFIG_MACHINE_OUYA
bool hasVibrator()
{
	initVibration(jEnv());
	return vibrator;
}

void vibrate(uint ms)
{
	initVibration(jEnv());
	if(unlikely(!vibrator))
		return;
	//logDMsg("vibrating for %u ms", ms);
	jVibrate(jEnv(), vibrator, (jlong)ms);
}
#endif

void setDeviceOrientationChangedSensor(bool on)
{
	// TODO
}

void setOnDeviceOrientationChanged(DeviceOrientationChangedDelegate del)
{
	// TODO
}

}
