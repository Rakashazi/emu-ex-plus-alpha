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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/base/Timer.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>
#include "android.hh"

namespace Base
{

static jobject vibrator{};
static JNI::InstMethod<void(jlong)> jVibrate{};
static bool vibrationSystemIsInit = false;

AndroidPropString AndroidApplication::androidBuildDevice(JNIEnv *env, jclass baseActivityClass) const
{
	JNI::ClassMethod<jobject()> jDevName{env, baseActivityClass, "devName", "()Ljava/lang/String;"};
	auto str = JNI::stringCopy<AndroidPropString>(env, (jstring)jDevName(env, baseActivityClass));
	//logMsg("device name: %s", str.data());
	return str;
}

AndroidPropString AndroidApplicationContext::androidBuildDevice() const
{
	auto env = mainThreadJniEnv();
	return application().androidBuildDevice(env, env->GetObjectClass(baseActivityObject()));
}

bool AndroidApplicationContext::apkSignatureIsConsistent() const
{
	bool sigMatchesAPK = true;
	#ifdef ANDROID_APK_SIGNATURE_HASH
	auto env = mainThreadJniEnv();
	auto baseActivity = baseActivityObject();
	JNI::InstMethod<jint()> jSigHash{env, baseActivity, "sigHash", "()I"};
	sigMatchesAPK = jSigHash(env, baseActivity) == ANDROID_APK_SIGNATURE_HASH;
	#endif
	return sigMatchesAPK;
}

bool AndroidApplicationContext::packageIsInstalled(const char *name) const
{
	auto env = mainThreadJniEnv();
	auto baseActivity = baseActivityObject();
	JNI::InstMethod<jboolean(jstring)> jPackageIsInstalled{env, baseActivity, "packageIsInstalled", "(Ljava/lang/String;)Z"};
	return jPackageIsInstalled(env, baseActivity, env->NewStringUTF(name));
}

static void initVibration(JNIEnv* env, jobject baseActivity)
{
	if(vibrationSystemIsInit) [[likely]]
		return;
	{
		JNI::InstMethod<jobject()> jSysVibrator{env, baseActivity, "systemVibrator", "()Landroid/os/Vibrator;"};
		vibrator = jSysVibrator(env, baseActivity);
	}
	vibrationSystemIsInit = true;
	if(!vibrator)
		return;
	logMsg("Vibrator present");
	vibrator = env->NewGlobalRef(vibrator);
	auto vibratorCls = env->FindClass("android/os/Vibrator");
	jVibrate = {env, vibratorCls, "vibrate", "(J)V"};
}

bool ApplicationContext::hasVibrator()
{
	initVibration(mainThreadJniEnv(), baseActivityObject());
	return vibrator;
}

void ApplicationContext::vibrate(IG::Milliseconds ms)
{
	auto env = mainThreadJniEnv();
	initVibration(env, baseActivityObject());
	if(!vibrator) [[unlikely]]
		return;
	//logDMsg("vibrating for %u ms", ms.count());
	jVibrate(env, vibrator, (jlong)ms.count());
}

void setDeviceOrientationChangedSensor(bool)
{
	// TODO
}

void ApplicationContext::setOnDeviceOrientationChanged(DeviceOrientationChangedDelegate)
{
	// TODO
}

void NoopThread::start()
{
	if(runFlagAddr)
		return;
	IG::makeDetachedThreadSync(
		[this](auto &sem)
		{
			// keep cpu governor busy by running a low priority thread executing no-op instructions
			setpriority(PRIO_PROCESS, gettid(), 19);
			bool run = true;
			runFlagAddr = &run;
			logMsg("started no-op thread");
			sem.notify();
			while(run)
			{
				iterateTimes(16, i)
				{
					asm("nop");
				}
			}
			logMsg("ended no-op thread");
		});
}

void NoopThread::stop()
{
	if(!runFlagAddr)
		return;
	*runFlagAddr = false;
	runFlagAddr = {};
}

void ApplicationContext::exitWithErrorMessageVPrintf(int exitVal, const char *format, va_list args)
{
	std::array<char, 512> msg{};
	auto result = vsnprintf(msg.data(), msg.size(), format, args);
	auto env = mainThreadJniEnv();
	auto baseActivity = baseActivityObject();
	JNI::InstMethod<void(jstring)> jMakeErrorPopup{env, baseActivity, "makeErrorPopup", "(Ljava/lang/String;)V"};
	jMakeErrorPopup(env, baseActivity, env->NewStringUTF(msg.data()));
	auto exitTimer = new Timer{"exitTimer", [=]() { ::exit(exitVal); }};
	exitTimer->runIn(IG::Seconds{3});
}

}

#ifdef ANDROID_COMPAT_API
CLINK int ftruncate64(int fd, off64_t length)
{
	return ftruncate(fd, length);
}
#endif
