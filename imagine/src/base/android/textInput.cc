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

#define LOGTAG "TextInput"
#include <imagine/base/Base.hh>
#include <imagine/input/Input.hh>
#include <imagine/logger/logger.h>
#include "internal.hh"
#include "android.hh"

namespace Input
{

static InputTextDelegate vKeyboardTextDelegate;
static IG::WindowRect textRect(8, 200, 8+304, 200+48);
static JavaInstMethod<void> jStartSysTextInput, jFinishSysTextInput, jPlaceSysTextInput;
static
void JNICALL textInputEnded(JNIEnv* env, jobject thiz, jstring jStr, jboolean processText, jboolean isDoingDismiss);

static void setupTextInputJni(JNIEnv* env)
{
	using namespace Base;
	if(!jStartSysTextInput.m)
	{
		logMsg("setting up text input JNI");
		jStartSysTextInput.setup(env, jBaseActivityCls, "startSysTextInput", "(Ljava/lang/String;Ljava/lang/String;IIIII)V");
		jFinishSysTextInput.setup(env, jBaseActivityCls, "finishSysTextInput", "(Z)V");
		jPlaceSysTextInput.setup(env, jBaseActivityCls, "placeSysTextInput", "(IIII)V");

		static JNINativeMethod activityMethods[] =
		{
			{"sysTextInputEnded", "(Ljava/lang/String;ZZ)V", (void *)&textInputEnded}
		};
		env->RegisterNatives(jBaseActivityCls, activityMethods, sizeofArray(activityMethods));
	}
}

uint startSysTextInput(InputTextDelegate callback, const char *initialText, const char *promptText, uint fontSizePixels)
{
	using namespace Base;
	auto env = jEnv();
	setupTextInputJni(env);
	logMsg("starting system text input");
	setEventsUseOSInputMethod(true);
	vKeyboardTextDelegate = callback;
	jStartSysTextInput(env, jBaseActivity, env->NewStringUTF(initialText), env->NewStringUTF(promptText),
		textRect.x, textRect.y, textRect.xSize(), textRect.ySize(), fontSizePixels);
	return 0;
}

void cancelSysTextInput()
{
	using namespace Base;
	auto env = jEnv();
	setupTextInputJni(env);
	vKeyboardTextDelegate = {};
	jFinishSysTextInput(env, jBaseActivity, 1);
}

void finishSysTextInput()
{
	using namespace Base;
	auto env = jEnv();
	setupTextInputJni(env);
	jFinishSysTextInput(env, jBaseActivity, 0);
}

void placeSysTextInput(IG::WindowRect rect)
{
	using namespace Base;
	auto env = jEnv();
	setupTextInputJni(env);
	textRect = rect;
	logMsg("placing text edit box at %d,%d with size %d,%d", rect.x, rect.y, rect.xSize(), rect.ySize());
	jPlaceSysTextInput(env, jBaseActivity, rect.x, rect.y, rect.xSize(), rect.ySize());
}

IG::WindowRect sysTextInputRect()
{
	return textRect;
}

static void JNICALL textInputEnded(JNIEnv* env, jobject thiz, jstring jStr, jboolean processText, jboolean isDoingDismiss)
{
	if(!processText)
	{
		return;
	}
	setEventsUseOSInputMethod(false);
	auto delegate = moveAndClear(vKeyboardTextDelegate);
	if(delegate)
	{
		if(jStr)
		{
			const char *str = env->GetStringUTFChars(jStr, nullptr);
			logMsg("running text entry callback with text: %s", str);
			delegate(str);
			env->ReleaseStringUTFChars(jStr, str);
		}
		else
		{
			logMsg("canceled text entry callback");
			delegate(nullptr);
		}
	}
	else
	{
		logMsg("text entry has no callback");
	}
}

}
