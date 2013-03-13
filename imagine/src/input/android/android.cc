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

#define thisModuleName "input:android"
#include <base/android/sdk.hh>
#include <input/common/common.h>
#include <util/jni.hh>
#include <base/android/ndkCompat.h>
#include <base/android/private.hh>

namespace Base
{
extern bool gfxUpdate;
}

#include "common.hh"

namespace Input
{

using namespace Base;

jboolean JNICALL touchEvent(JNIEnv *env, jobject thiz, jint action, jint x, jint y, jint pid)
{
	#if CONFIG_ENV_ANDROID_MINSDK == 4
		pid = 0; // no multi-touch
	#endif
	auto prevGfxUpdateState = gfxUpdate;
	if(!handleTouchEvent(action, x, y, pid))
		return 0;
	return prevGfxUpdateState == 0 && gfxUpdate;
}

jboolean JNICALL trackballEvent(JNIEnv *env, jobject thiz, jint action, jfloat x, jfloat y)
{
	auto prevGfxUpdateState = gfxUpdate;
	handleTrackballEvent(action, x, y);
	return prevGfxUpdateState == 0 && gfxUpdate;
}

jboolean JNICALL keyEvent(JNIEnv *env, jobject thiz, jint key, jint down, jboolean metaState)
{
	auto prevGfxUpdateState = gfxUpdate;
	handleKeyEvent(key, down, 0, metaState);
	return prevGfxUpdateState == 0 && gfxUpdate;
}

//void showSoftInput()
//{
//	using namespace Base;
//	logMsg("showing soft input");
//	jShowIme(aEnv(), jBaseActivity, 0);
//}
//
//void hideSoftInput()
//{
//	using namespace Base;
//	logMsg("hiding soft input");
//	jHideIme(aEnv(), jBaseActivity, 0);
//}

CallResult init()
{
	return OK;
}

static jboolean JNICALL textInputEnded(JNIEnv* env, jobject thiz, jstring jStr)
{
	if(vKeyboardTextDelegate.hasCallback())
	{
		auto prevGfxUpdateState = gfxUpdate;
		if(jStr)
		{
			const char *str = env->GetStringUTFChars(jStr, 0);
			logMsg("running text entry callback with text: %s", str);
			vKeyboardTextDelegate.invoke(str);
			env->ReleaseStringUTFChars(jStr, str);
		}
		else
		{
			logMsg("canceled text entry callback");
			vKeyboardTextDelegate.invoke(nullptr);
		}
		return prevGfxUpdateState == 0 && gfxUpdate;
	}
	else
	{
		vKeyboardTextDelegate.clear();
		return 0;
	}
}

}
