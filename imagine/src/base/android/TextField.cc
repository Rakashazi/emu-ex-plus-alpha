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
#include <imagine/input/TextField.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/logger/logger.h>

namespace IG::Input
{

static JNI::InstMethod<jobject(jstring, jstring, jint, jint, jint, jint, jint, jlong)> jNewTextEntry{};
static JNI::InstMethod<void(jboolean)> jFinishTextInput{};
static JNI::InstMethod<void(jint, jint, jint, jint)> jPlaceTextInput{};

static void setupBaseActivityJni(JNIEnv* env, jobject baseActivity)
{
	if(jNewTextEntry) [[likely]]
		return;
	jNewTextEntry = {env, baseActivity, "newTextEntry", "(Ljava/lang/String;Ljava/lang/String;IIIIIJ)Lcom/imagine/TextEntry;"};
}

AndroidTextField::AndroidTextField(ApplicationContext ctx, TextFieldDelegate del, CStringView initialText, CStringView promptText, int fontSizePixels):
	ctx{ctx},
	textDelegate{del}
{
	auto env = ctx.mainThreadJniEnv();
	auto baseActivity = ctx.baseActivityObject();
	setupBaseActivityJni(env, baseActivity);
	logMsg("starting system text input");
	jTextEntry = {env, jNewTextEntry(env, baseActivity, env->NewStringUTF(initialText), env->NewStringUTF(promptText),
		textRect.x, textRect.y, textRect.xSize(), textRect.ySize(), fontSizePixels, (jlong)this)};
	setupTextEntryJni(env, jTextEntry);
}

AndroidTextField::~AndroidTextField()
{
	static_cast<TextField*>(this)->cancel();
}

void AndroidTextField::setupTextEntryJni(JNIEnv* env, jobject textEntry)
{
	if(jFinishTextInput) [[likely]]
		return;
	logMsg("setting up text input JNI");
	auto textEntryClass = env->GetObjectClass(textEntry);
	jFinishTextInput = {env, textEntryClass, "finish", "(Z)V"};
	jPlaceTextInput = {env, textEntryClass, "place", "(IIII)V"};
	JNINativeMethod methods[] =
	{
		{
			"textInputEnded", "(JLjava/lang/String;ZZ)V",
			(void*)+[](JNIEnv* env, jobject, jlong nUserData, jstring jStr, jboolean processText, [[maybe_unused]] jboolean isDoingDismiss)
			{
				if(!processText)
				{
					return;
				}
				auto &this_ = *((AndroidTextField*)nUserData);
				auto delegate = std::exchange(this_.textDelegate, {});
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
	};
	env->RegisterNatives(textEntryClass, methods, std::size(methods));
}

void TextField::cancel()
{
	if(!jTextEntry)
		return;
	textDelegate = {};
	auto env = ctx.mainThreadJniEnv();
	jFinishTextInput(env, jTextEntry, 1);
	jTextEntry.reset();
}

void TextField::finish()
{
	if(!jTextEntry)
		return;
	auto env = ctx.mainThreadJniEnv();
	jFinishTextInput(env, jTextEntry, 0);
	jTextEntry.reset();
}

void TextField::place(WindowRect rect)
{
	if(!jTextEntry)
		return;
	textRect = rect;
	logMsg("placing text edit box at %d,%d with size %d,%d", rect.x, rect.y, rect.xSize(), rect.ySize());
	jPlaceTextInput(ctx.mainThreadJniEnv(), jTextEntry, rect.x, rect.y, rect.xSize(), rect.ySize());
}

WindowRect TextField::windowRect() const
{
	return textRect;
}

}
