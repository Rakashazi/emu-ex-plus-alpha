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
#include <imagine/input/Input.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/logger/logger.h>
#include "android.hh"

namespace Input
{

static InputTextDelegate vKeyboardTextDelegate;
static IG::WindowRect textRect{{8, 200}, {8+304, 200+48}};
static JavaInstMethod<void(jstring, jstring, jint, jint, jint, jint, jint, jlong)> jStartSysTextInput;
static JavaInstMethod<void(jboolean)> jFinishSysTextInput;
static JavaInstMethod<void(jint, jint, jint, jint)> jPlaceSysTextInput;
static
void JNICALL textInputEnded(JNIEnv* env, jobject thiz, jlong userData, jstring jStr, jboolean processText, jboolean isDoingDismiss);

static void setupTextInputJni(JNIEnv* env, jclass baseActivityClass)
{
	using namespace Base;
	if(!jStartSysTextInput)
	{
		logMsg("setting up text input JNI");
		jStartSysTextInput.setup(env, baseActivityClass, "startSysTextInput", "(Ljava/lang/String;Ljava/lang/String;IIIIIJ)V");
		jFinishSysTextInput.setup(env, baseActivityClass, "finishSysTextInput", "(Z)V");
		jPlaceSysTextInput.setup(env, baseActivityClass, "placeSysTextInput", "(IIII)V");

		static JNINativeMethod activityMethods[] =
		{
			{"sysTextInputEnded", "(JLjava/lang/String;ZZ)V", (void *)&textInputEnded}
		};
		env->RegisterNatives(baseActivityClass, activityMethods, std::size(activityMethods));
	}
}

uint32_t startSysTextInput(Base::ApplicationContext ctx, InputTextDelegate callback, const char *initialText, const char *promptText, uint32_t fontSizePixels)
{
	using namespace Base;
	auto env = ctx.mainThreadJniEnv();
	auto &app = ctx.application();
	setupTextInputJni(env, app.baseActivityClass());
	logMsg("starting system text input");
	app.setEventsUseOSInputMethod(true);
	vKeyboardTextDelegate = callback;
	jStartSysTextInput(env, ctx.baseActivityObject(), env->NewStringUTF(initialText), env->NewStringUTF(promptText),
		textRect.x, textRect.y, textRect.xSize(), textRect.ySize(), fontSizePixels, (jlong)&app);
	return 0;
}

void cancelSysTextInput(Base::ApplicationContext ctx)
{
	using namespace Base;
	auto env = ctx.mainThreadJniEnv();
	setupTextInputJni(env, ctx.baseActivityClass());
	vKeyboardTextDelegate = {};
	jFinishSysTextInput(env, ctx.baseActivityObject(), 1);
}

void finishSysTextInput(Base::ApplicationContext ctx)
{
	using namespace Base;
	auto env = ctx.mainThreadJniEnv();
	setupTextInputJni(env, ctx.baseActivityClass());
	jFinishSysTextInput(env, ctx.baseActivityObject(), 0);
}

void placeSysTextInput(Base::ApplicationContext ctx, IG::WindowRect rect)
{
	using namespace Base;
	auto env = ctx.mainThreadJniEnv();
	setupTextInputJni(env, ctx.baseActivityClass());
	textRect = rect;
	logMsg("placing text edit box at %d,%d with size %d,%d", rect.x, rect.y, rect.xSize(), rect.ySize());
	jPlaceSysTextInput(env, ctx.baseActivityObject(), rect.x, rect.y, rect.xSize(), rect.ySize());
}

IG::WindowRect sysTextInputRect(Base::ApplicationContext)
{
	return textRect;
}

static void JNICALL textInputEnded(JNIEnv* env, jobject thiz, jlong nUserData, jstring jStr, jboolean processText, jboolean isDoingDismiss)
{
	if(!processText)
	{
		return;
	}
	auto &app = *((Base::AndroidApplication*)nUserData);
	app.setEventsUseOSInputMethod(false);
	auto delegate = std::exchange(vKeyboardTextDelegate, {});
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
