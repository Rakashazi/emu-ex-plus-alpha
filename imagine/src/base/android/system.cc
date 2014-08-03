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
		jDevName.setup(eEnv(), jBaseActivityCls, "devName", "()Ljava/lang/String;");
		auto devName = (jstring)jDevName(eEnv());
		buildDevice = eEnv()->GetStringUTFChars(devName, nullptr);
		logMsg("device name: %s", buildDevice);
		assert(buildDevice);
	}
	return buildDevice;
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
	auto jEnv = eEnv();
	jSigHash.setup(jEnv, jBaseActivityCls, "sigHash", "()I");
	sigMatchesAPK = jSigHash(jEnv, jBaseActivity) == ANDROID_APK_SIGNATURE_HASH;
	#endif
	return sigMatchesAPK;
}

bool packageIsInstalled(const char *name)
{
	auto jEnv = eEnv();
	if(!jPackageIsInstalled)
		jPackageIsInstalled.setup(jEnv, jBaseActivityCls, "packageIsInstalled", "(Ljava/lang/String;)Z");
	return jPackageIsInstalled(jEnv, jBaseActivity, jEnv->NewStringUTF(name));
}

static void initVibration(JNIEnv* jEnv)
{
	if(likely(vibrationSystemIsInit) || Config::MACHINE_IS_OUYA)
		return;
	{
		JavaInstMethod<jobject> jSysVibrator;
		jSysVibrator.setup(jEnv, jBaseActivityCls, "systemVibrator", "()Landroid/os/Vibrator;");
		vibrator = jSysVibrator(jEnv, jBaseActivity);
	}
	vibrationSystemIsInit = true;
	if(!vibrator)
		return;
	logMsg("Vibrator present");
	vibrator = jEnv->NewGlobalRef(vibrator);
	auto vibratorCls = jEnv->FindClass("android/os/Vibrator");
	jVibrate.setup(jEnv, vibratorCls, "vibrate", "(J)V");
}

bool hasVibrator()
{
	initVibration(eEnv());
	return vibrator;
}

void vibrate(uint ms)
{
	initVibration(eEnv());
	if(unlikely(!vibrator))
		return;
	//logDMsg("vibrating for %u ms", ms);
	jVibrate(eEnv(), vibrator, (jlong)ms);
}

}
