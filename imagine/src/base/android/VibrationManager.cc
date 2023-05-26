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

#define LOGTAG "Vibration"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/VibrationManager.hh>
#include <imagine/logger/logger.h>

namespace IG
{

AndroidVibrationManager::AndroidVibrationManager(ApplicationContext ctx)
{
	auto env = ctx.mainThreadJniEnv();
	auto baseActivity = ctx.baseActivityObject();
	JNI::InstMethod<jobject()> jSysVibrator{env, baseActivity, "systemVibrator", "()Landroid/os/Vibrator;"};
	auto sysVibrator = jSysVibrator(env, baseActivity);
	if(!sysVibrator)
		return;
	logMsg("vibrator present");
	vibrator = {env, sysVibrator};
	jVibrate = {env, env->FindClass("android/os/Vibrator"), "vibrate", "(J)V"};
}

bool VibrationManager::hasVibrator() const
{
	return vibrator;
}

void VibrationManager::vibrate(Milliseconds ms)
{
	if(!vibrator) [[unlikely]]
		return;
	//logDMsg("vibrating for %u ms", ms.count());
	jVibrate(vibrator.jniEnv(), vibrator, (jlong)ms.count());
}

}
