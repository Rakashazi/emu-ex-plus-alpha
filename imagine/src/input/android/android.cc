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
#include <input/interface.h>
#include <input/common/common.h>
#include <util/jni.hh>
#include <base/android/ndkCompat.h>

namespace Base
{
extern fbool gfxUpdate;
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
	var_copy(prevGfxUpdateState, gfxUpdate);
	if(!handleTouchEvent(action, x, y, pid))
		return 0;
	return prevGfxUpdateState == 0 && gfxUpdate;
}

jboolean JNICALL trackballEvent(JNIEnv *env, jobject thiz, jint action, jfloat x, jfloat y)
{
	var_copy(prevGfxUpdateState, gfxUpdate);
	handleTrackballEvent(action, x, y);
	return prevGfxUpdateState == 0 && gfxUpdate;
}

jboolean JNICALL keyEvent(JNIEnv *env, jobject thiz, jint key, jint down)
{
	var_copy(prevGfxUpdateState, gfxUpdate);
	handleKeyEvent(key, down);
	return prevGfxUpdateState == 0 && gfxUpdate;
}

CallResult init()
{
	commonInit();
	return OK;
}

}

#undef thisModuleName
