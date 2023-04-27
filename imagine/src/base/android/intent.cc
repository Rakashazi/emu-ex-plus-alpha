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

namespace IG
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

void ApplicationContext::addNotification(CStringView onShow, CStringView title, CStringView message)
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

void ApplicationContext::addLauncherIcon(CStringView name, CStringView path)
{
	logMsg("adding launcher icon:%s, for location:%s", name.data(), path.data());
	auto env = mainThreadJniEnv();
	auto baseActivity = baseActivityObject();
	JNI::InstMethod<void(jstring, jstring)> jAddViewShortcut{env, baseActivity, "addViewShortcut", "(Ljava/lang/String;Ljava/lang/String;)V"};
	jAddViewShortcut(env, baseActivity, env->NewStringUTF(name), env->NewStringUTF(path));
}

void AndroidApplication::handleIntent(ApplicationContext ctx)
{
	if(!acceptsIntents)
		return;
	auto env = ctx.mainThreadJniEnv();
	auto baseActivity = ctx.baseActivityObject();
	// check for view intents
	JNI::InstMethod<jstring()> jIntentDataPath{env, baseActivity, "intentDataPath", "()Ljava/lang/String;"};
	jstring intentDataPathJStr = jIntentDataPath(env, baseActivity);
	if(intentDataPathJStr)
	{
		const char *intentDataPathStr = env->GetStringUTFChars(intentDataPathJStr, nullptr);
		logMsg("got intent with path: %s", intentDataPathStr);
		onEvent(ctx, InterProcessMessageEvent{intentDataPathStr});
		env->ReleaseStringUTFChars(intentDataPathJStr, intentDataPathStr);
	}
}

void ApplicationContext::openURL(CStringView url) const
{
	auto env = mainThreadJniEnv();
	auto baseActivity = baseActivityObject();
	JNI::InstMethod<void(jstring)> jOpenURL{env, baseActivity, "openURL", "(Ljava/lang/String;)V"};
	jOpenURL(env, baseActivity, env->NewStringUTF(url));
}

bool AndroidApplication::openDocumentTreeIntent(JNIEnv *env, jobject baseActivity, SystemDocumentPickerDelegate del)
{
	onSystemDocumentPicker = del;
	JNI::InstMethod<jboolean(jlong)> jOpenDocumentTree{env, env->GetObjectClass(baseActivity), "openDocumentTree", "(J)Z"};
	return jOpenDocumentTree(env, baseActivity, (jlong)this);
}

bool ApplicationContext::hasSystemPathPicker() const { return androidSDK() >= 21; }

bool ApplicationContext::showSystemPathPicker(SystemDocumentPickerDelegate del)
{
	return application().openDocumentTreeIntent(mainThreadJniEnv(), baseActivityObject(), del);
}

bool AndroidApplication::openDocumentIntent(JNIEnv *env, jobject baseActivity, SystemDocumentPickerDelegate del)
{
	onSystemDocumentPicker = del;
	JNI::InstMethod<jboolean(jlong)> jOpenDocument{env, env->GetObjectClass(baseActivity), "openDocument", "(J)Z"};
	return jOpenDocument(env, baseActivity, (jlong)this);
}

bool ApplicationContext::hasSystemDocumentPicker() const { return androidSDK() >= 19; }

bool ApplicationContext::showSystemDocumentPicker(SystemDocumentPickerDelegate del)
{
	return application().openDocumentIntent(mainThreadJniEnv(), baseActivityObject(), del);
}

bool AndroidApplication::createDocumentIntent(JNIEnv *env, jobject baseActivity, SystemDocumentPickerDelegate del)
{
	onSystemDocumentPicker = del;
	JNI::InstMethod<jboolean(jlong)> jCreateDocument{env, env->GetObjectClass(baseActivity), "createDocument", "(J)Z"};
	return jCreateDocument(env, baseActivity, (jlong)this);
}

bool ApplicationContext::showSystemCreateDocumentPicker(SystemDocumentPickerDelegate del)
{
	return application().createDocumentIntent(mainThreadJniEnv(), baseActivityObject(), del);
}

void AndroidApplication::handleDocumentIntentResult(const char *uri, const char *name)
{
	if(isRunning())
	{
		std::exchange(onSystemDocumentPicker, {})(uri, name);
	}
	else
	{
		// wait until after app resumes before handling result
		addOnResume(
			[uriCopy = strdup(uri), nameCopy = strdup(name)](ApplicationContext ctx, bool focused)
			{
				std::exchange(ctx.application().onSystemDocumentPicker, {})(uriCopy, nameCopy);
				::free(nameCopy);
				::free(uriCopy);
				return false;
			}, APP_ON_RESUME_PRIORITY + 100);
	}
}

void ApplicationContext::setAcceptIPC(bool on, const char *) { application().acceptsIntents = on; }

}
