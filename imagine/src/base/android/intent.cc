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

#define LOGTAG "Intent"
#include <android/native_activity.h>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/logger/logger.h>

namespace Base
{

void AndroidApplication::addNotification(JNIEnv *env, jobject baseActivity, const char *onShow, const char *title, const char *message)
{
	logMsg("adding notificaion icon");
	if(!jAddNotification) [[unlikely]]
	{
		jAddNotification = {env, baseActivity, "addNotification", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V"};
	}
	jAddNotification(env, baseActivity, env->NewStringUTF(onShow), env->NewStringUTF(title), env->NewStringUTF(message));
}

void ApplicationContext::addNotification(const char *onShow, const char *title, const char *message)
{
	return application().addNotification(mainThreadJniEnv(), baseActivityObject(), onShow, title, message);
}

void AndroidApplication::removePostedNotifications(JNIEnv *env, jobject baseActivity)
{
	// check if notification functions were used at some point
	// and remove the posted notification
	if(!jAddNotification)
		return;
	JNI::InstMethod<void()> jRemoveNotification{env, baseActivity, "removeNotification", "()V"};
	jRemoveNotification(env, baseActivity);
}

void ApplicationContext::addLauncherIcon(const char *name, const char *path)
{
	logMsg("adding launcher icon: %s, for path: %s", name, path);
	auto env = mainThreadJniEnv();
	auto baseActivity = baseActivityObject();
	JNI::InstMethod<void(jstring, jstring)> jAddViewShortcut{env, baseActivity, "addViewShortcut", "(Ljava/lang/String;Ljava/lang/String;)V"};
	jAddViewShortcut(env, baseActivity, env->NewStringUTF(name), env->NewStringUTF(path));
}

void AndroidApplication::handleIntent(ApplicationContext ctx)
{
	if(!hasOnInterProcessMessage())
		return;
	auto env = ctx.mainThreadJniEnv();
	auto baseActivity = ctx.baseActivityObject();
	// check for view intents
	JNI::InstMethod<jobject()> jIntentDataPath{env, baseActivity, "intentDataPath", "()Ljava/lang/String;"};
	jstring intentDataPathJStr = (jstring)jIntentDataPath(env, baseActivity);
	if(intentDataPathJStr)
	{
		const char *intentDataPathStr = env->GetStringUTFChars(intentDataPathJStr, nullptr);
		logMsg("got intent with path: %s", intentDataPathStr);
		ctx.dispatchOnInterProcessMessage(intentDataPathStr);
		env->ReleaseStringUTFChars(intentDataPathJStr, intentDataPathStr);
	}
}

void ApplicationContext::openURL(const char *url) const
{
	auto env = mainThreadJniEnv();
	auto baseActivity = baseActivityObject();
	JNI::InstMethod<void(jstring)> jOpenURL{env, baseActivity, "openURL", "(Ljava/lang/String;)V"};
	jOpenURL(env, baseActivity, env->NewStringUTF(url));
}

void AndroidApplication::openDocumentTreeIntent(JNIEnv *env, jobject baseActivity, SystemPathPickerDelegate del)
{
	onSystemPathPicker = del;
	JNI::InstMethod<void(jlong)> jOpenDocumentTree{env, env->GetObjectClass(baseActivity), "openDocumentTree", "(J)V"};
	jOpenDocumentTree(env, baseActivity, (jlong)this);
}

bool ApplicationContext::hasSystemPathPicker() const { return androidSDK() >= 30; }

void ApplicationContext::showSystemPathPicker(SystemPathPickerDelegate del)
{
	application().openDocumentTreeIntent(mainThreadJniEnv(), baseActivityObject(), del);
}

}
