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

#define thisModuleName "base:android"
#include <stdlib.h>
#include <sys/epoll.h>
#include <engine-globals.h>
#include <logger/interface.h>
#include "private.hh"
#include <base/Base.hh>
#include <base/common/funcs.h>
#include <semaphore.h>
#include "ndkCompat.h"

#include "common.hh"

#ifndef CONFIG_INPUT
namespace Input
{

static jboolean JNICALL touchEvent(JNIEnv *env, jobject thiz, jint action, jint x, jint y, jint pid) { return 0; }
static jboolean JNICALL trackballEvent(JNIEnv *env, jobject thiz, jint action, jfloat x, jfloat y) { return 0; }
static jboolean JNICALL keyEvent(JNIEnv *env, jobject thiz, jint key, jint down, jboolean metaState) { return 0; }

}
#else
#include <input/android/private.hh>
#endif

namespace Base
{

// JNI methods/variables to access Java component
static jclass jMessageCls = 0, jGLViewCls = 0, jHandlerCls = 0;
static jobject jHandler = 0, glView = 0;
#ifdef CONFIG_GFX_SOFT_ORIENTATION
static JavaClassMethod jSetAutoOrientation;
#endif
static JavaInstMethod jSetKeepScreenOn, jPostDelayed, jRemoveCallbacks; /*, jPost, jHasMessages*/
JavaInstMethod jShowIme, jHideIme;
static JavaClassMethod jSwapBuffers;
static jfieldID jAllowKeyRepeatsId, jHandleVolumeKeysId, glViewId;
static jobject jTimerCallbackRunnable;
static const ushort MSG_FD_EVENTS = 1;
static int ePoll;
static ThreadPThread epollThread;
static sem_t ePollWaitSem;
uint appState = APP_RUNNING; //TODO: should start as APP_PAUSED, but BaseActivitiy needs some changes

#ifdef CONFIG_BLUEZ

JavaClassMethod jObtain;
JavaInstMethod jSendToTarget;
static jfieldID msgHandlerId;
jobject msgHandler;

#endif

JavaVM* jVM = 0;

/*void statusBarHidden(uint hidden)
{
	jclass cls = (*jEnv)->FindClass(jEnv, "com/imagine/BaseActivity");
	if(cls == NULL)
	{
		logMsg("can't find class");
	}
	jmethodID mid = (*jEnv)->GetStaticMethodID(jEnv, cls, "setStatusBar", "(B)V");
	if (mid == 0)
	{
		logMsg("can't find method");
		return;
	}
	(*jEnv)->CallStaticVoidMethod(jEnv, cls, mid, (char)hidden);
}*/

void setIdleDisplayPowerSave(bool on)
{
	jint keepOn = !on;
	logMsg("keep screen on: %d", keepOn);
	jEnv->CallVoidMethod(glView, jSetKeepScreenOn.m, (jboolean)keepOn);
}

void setOSNavigationStyle(uint flags) { }

bool hasSurfaceTexture() { return 0; }

static TimerCallbackFunc timerCallbackFunc = 0;
static void *timerCallbackFuncCtx = 0;
void setTimerCallback(TimerCallbackFunc f, void *ctx, int ms)
{
	if(timerCallbackFunc)
	{
		logMsg("canceling callback");
		timerCallbackFunc = 0;
		jEnv->CallBooleanMethod(jHandler, jRemoveCallbacks.m, jTimerCallbackRunnable);
	}
	if(!f)
		return;
	logMsg("setting callback to run in %d ms", ms);
	timerCallbackFunc = f;
	timerCallbackFuncCtx = ctx;
	jEnv->CallBooleanMethod(jHandler, jPostDelayed.m, jTimerCallbackRunnable, (jlong)ms);
}

static jboolean JNICALL timerCallback(JNIEnv*  env, jobject thiz, jboolean isPaused)
{
	if(timerCallbackFunc)
	{
		if(isPaused)
			logMsg("running callback with app paused");
		else
			logMsg("running callback");
		timerCallbackFunc(timerCallbackFuncCtx);
		timerCallbackFunc = 0;
		return !isPaused && gfxUpdate;
	}
	return 0;
}

static void JNICALL envConfig(JNIEnv*  env, jobject thiz, jstring filesPath, jstring eStoragePath,
	float xdpi, float ydpi, jint orientation, jint refreshRate, jint apiLevel, jint hardKeyboardState,
	jint navigationState, jstring devName, jstring apkPath, jobject sysVibrator)
{
	assert(thiz);
	jBaseActivity = env->NewGlobalRef(thiz);
	filesDir = env->GetStringUTFChars(filesPath, 0);
	eStoreDir = env->GetStringUTFChars(eStoragePath, 0);
	appPath = env->GetStringUTFChars(apkPath, 0);
	logMsg("apk @ %s", appPath);

	osOrientation = orientation;
	refreshRate_ = refreshRate;
	logMsg("refresh rate: %d", refreshRate);

	logMsg("set screen DPI size %f,%f", (double)xdpi, (double)ydpi);
	// Note: DPI values must come in un-rotated from DisplayMetrics
	androidXDPI = xDPI = xdpi;
	androidYDPI = yDPI = ydpi;

	setSDK(apiLevel);

	const char *devNameStr = env->GetStringUTFChars(devName, 0);
	logMsg("device name: %s", devNameStr);
	setDeviceType(devNameStr);
	env->ReleaseStringUTFChars(devName, devNameStr);

	aHardKeyboardState = (devType == DEV_TYPE_XPERIA_PLAY) ? navigationState : hardKeyboardState;
	logMsg("keyboard/nav hidden: %s", hardKeyboardNavStateToStr(aHardKeyboardState));

	if(sysVibrator)
	{
		logMsg("has Vibration support");
		vibrator = jEnv->NewGlobalRef(sysVibrator);
		setupVibration(jEnv);
	}
}

static void JNICALL nativeInit(JNIEnv*  env, jobject thiz, jint w, jint h)
{
	logMsg("native code init");
	assert(jVM);
	assert(jBaseActivity);
	#ifdef CONFIG_BLUEZ
	msgHandler = env->NewGlobalRef(env->GetStaticObjectField(jBaseActivityCls, msgHandlerId));
	//logMsg("msgHandler %p", msgHandler);
	#endif
	glView = env->NewGlobalRef(env->GetStaticObjectField(jBaseActivityCls, glViewId));
	jfieldID timerCallbackRunnableId = env->GetStaticFieldID(jBaseActivityCls, "timerCallbackRunnable", "Ljava/lang/Runnable;");
	assert(timerCallbackRunnableId);
	jTimerCallbackRunnable = env->NewGlobalRef(env->GetStaticObjectField(jBaseActivityCls, timerCallbackRunnableId));
	//logMsg("jTimerCallbackRunnable %p", jTimerCallbackRunnable);
	jfieldID handlerID = env->GetStaticFieldID(jGLViewCls, "handler", "Landroid/os/Handler;");
	assert(handlerID);
	jHandler = env->NewGlobalRef(env->GetStaticObjectField(jGLViewCls, handlerID));
	doOrExit(logger_init());
	initialScreenSizeSetup(w, h);
	engineInit();
	logMsg("done init");
	//logMsg("doc files path: %s", filesDir);
	//logMsg("external storage path: %s", eStoreDir);
	return;
}

static void JNICALL nativeResize(JNIEnv*  env, jobject thiz, jint w, jint h)
{
	//logMsg("resize event");
	mainWin.rect.x2 = w;
	mainWin.rect.y2 = h;
	resizeEvent(w, h);
	//logMsg("done resize");
}

static jboolean JNICALL nativeRender(JNIEnv*  env, jobject thiz)
{
	//logMsg("doing render");
	runEngine();
	//if(ret) logMsg("frame rendered");
	return gfxUpdate;
}

void openGLUpdateScreen()
{
	//if(!gfxUpdate) logMsg("sleeping after this frame");
	jEnv->CallStaticVoidMethod(jSwapBuffers.c, jSwapBuffers.m, gfxUpdate);
}

#ifdef CONFIG_BLUETOOTH
void sendMessageToMain(ThreadPThread &thread, int type, int shortArg, int intArg, int intArg2)
{
	assert(thread.jEnv);
	jobject jMsg = thread.jEnv->CallStaticObjectMethod(jObtain.c, jObtain.m, msgHandler, shortArg + (type << 16), intArg, intArg2);
	thread.jEnv->CallVoidMethod(jMsg, jSendToTarget.m);
	thread.jEnv->DeleteLocalRef(jMsg);
}

static int runEpoll(ThreadPThread &thread)
{
	for(;;)
	{
		struct epoll_event event[8];
		// TODO: handle -1 return
		//logMsg("doing epoll_wait()");
		int events = epoll_wait(ePoll, event, sizeofArray(event), -1);
		//logMsg("%d events ready", events);
		iterateTimes(events, i)
		{
			auto e = (PollEventDelegate*)event[i].data.ptr;
			// TODO: pointer->int won't work on 64-bit, but 64-bit Android 2.2 systems probably won't exist
			//logMsg("fd handler %p has events %X, sending to main", e, event[i].events);
			sendMessageToMain(thread, MSG_FD_EVENTS, i == 0 ? events : 0, (int)e, event[i].events);
		}
		// wait until main thread hanldes all events
		sem_wait(&ePollWaitSem);
	}
	bug_exit("Shouldn't exit epoll loop");
	/*logMsg("closing epoll & exiting thread");
	close(ePoll);
	ePoll = 0;*/
	return 0;
}

static void setupEpoll()
{
	if(!ePoll)
	{
		logMsg("creating epoll & thread");
		ePoll = epoll_create(8);
		assert(ePoll);
		sem_init(&ePollWaitSem, 0, 0);
		epollThread.create(1, runEpoll, 0);
	}
}

void addPollEvent2(int fd, PollEventDelegate &handler, uint events)
{
	setupEpoll();
	logMsg("adding fd %d to epoll", fd);
	struct epoll_event ev = { 0 };
	ev.data.ptr = &handler;
	ev.events = /*EPOLLET|*/events;
	epoll_ctl(ePoll, EPOLL_CTL_ADD, fd, &ev);
}

void modPollEvent(int fd, PollEventDelegate &handler, uint events)
{
	struct epoll_event ev = { 0 };
	ev.data.ptr = &handler;
	ev.events = /*EPOLLET|*/events;
	assert(ePoll);
	epoll_ctl(ePoll, EPOLL_CTL_MOD, fd, &ev);
}

void removePollEvent(int fd)
{
	logMsg("removing fd %d from epoll", fd);
	epoll_ctl(ePoll, EPOLL_CTL_DEL, fd, nullptr);
}
#endif

static void JNICALL configChange(JNIEnv*  env, jobject thiz, jint hardKeyboardState, jint navState, jint orientation)
{
	logMsg("config change, keyboard: %s, navigation: %s", hardKeyboardNavStateToStr(hardKeyboardState), hardKeyboardNavStateToStr(navState));
	setHardKeyboardState((devType == DEV_TYPE_XPERIA_PLAY) ? navState : hardKeyboardState);
	setOrientationOS(orientation);
}

static void JNICALL appPaused(JNIEnv*  env, jobject thiz)
{
	logMsg("backgrounding");
	displayNeedsUpdate(); // display needs a redraw upon resume
	appState = APP_PAUSED;
	onExit(1);
}

static void JNICALL appResumed(JNIEnv*  env, jobject thiz, jboolean hasFocus)
{
	logMsg("resumed");
	appState = APP_RUNNING;
	onResume(hasFocus);
}

static jboolean JNICALL appFocus(JNIEnv*  env, jobject thiz, jboolean hasFocus)
{
	logMsg("focus change: %d", (int)hasFocus);
	var_copy(prevGfxUpdateState, gfxUpdate);
	onFocusChange(hasFocus);
	return appState == APP_RUNNING && prevGfxUpdateState == 0 && gfxUpdate;
}

static jboolean JNICALL handleAndroidMsg(JNIEnv *env, jobject thiz, jint arg1, jint arg2, jint arg3)
{
	var_copy(prevGfxUpdateState, gfxUpdate);
	int type = arg1 >> 16, shortArg = arg1 & 0xFFFF;
	switch(type)
	{
		bcase MSG_FD_EVENTS:
		{
			static int totalEvents = 0;
			if(shortArg)
			{
				// first event, set the total to process
				totalEvents = shortArg;
			}
			auto e = (PollEventDelegate*)arg2;
			//logMsg("got fd handler %p with events %X from thread", e, arg3);
			e->invoke(arg3);
			totalEvents--;
			assert(totalEvents >= 0);
			if(totalEvents == 0) // last event in group handled, tell epoll thread to continue
				sem_post(&ePollWaitSem);
		}
		bdefault:
			processAppMsg(type, shortArg, arg2, arg3);
	}
	return prevGfxUpdateState == 0 && gfxUpdate;
}

static jboolean JNICALL layoutChange(JNIEnv* env, jobject thiz, jint height)
{
	assert(height >= 0);
	if(visibleScreenY == (uint)height)
		return 0;
	logMsg("layout change, view height %d", height);
	visibleScreenY = height;
	if(!glView)
		return 0;
	var_copy(prevGfxUpdateState, gfxUpdate);
	resizeEvent(mainWin.rect.x2, visibleScreenY);
	return prevGfxUpdateState == 0 && gfxUpdate;
}

}

CLINK JNIEXPORT jint JNICALL LVISIBLE JNI_OnLoad(JavaVM *vm, void*)
{
	using namespace Base;
	logMsg("in JNI_OnLoad");
	jVM = vm;
	int getEnvRet = vm->GetEnv((void**) &jEnv, JNI_VERSION_1_6);
	assert(getEnvRet == JNI_OK);

	// BaseActivity members
	jBaseActivityCls = (jclass)jEnv->NewGlobalRef(jEnv->FindClass("com/imagine/BaseActivity"));
	jSetRequestedOrientation.setup(jEnv, jBaseActivityCls, "setRequestedOrientation", "(I)V");
	jAddNotification.setup(jEnv, jBaseActivityCls, "addNotification", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
	jRemoveNotification.setup(jEnv, jBaseActivityCls, "removeNotification", "()V");
	jShowIme.setup(jEnv, jBaseActivityCls, "showIme", "(I)V");
	jHideIme.setup(jEnv, jBaseActivityCls, "hideIme", "(I)V");
	jAllowKeyRepeatsId = jEnv->GetStaticFieldID(jBaseActivityCls, "allowKeyRepeats", "Z");
	jHandleVolumeKeysId = jEnv->GetStaticFieldID(jBaseActivityCls, "handleVolumeKeys", "Z");
	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	jSetAutoOrientation.setup(jEnv, jBaseActivityCls, "setAutoOrientation", "(ZI)V");
	#endif
	glViewId = jEnv->GetStaticFieldID(jBaseActivityCls, "glView", "Lcom/imagine/GLView;");
	assert(glViewId);

	// Handler members
	jHandlerCls = (jclass)jEnv->NewGlobalRef(jEnv->FindClass("android/os/Handler"));
	jPostDelayed.setup(jEnv, jHandlerCls, "postDelayed", "(Ljava/lang/Runnable;J)Z");
	jRemoveCallbacks.setup(jEnv, jHandlerCls, "removeCallbacks", "(Ljava/lang/Runnable;)V");
	//jPost.setup(env, jHandlerCls, "post", "(Ljava/lang/Runnable;)Z");
	//jHasMessages.setup(env, jHandlerCls, "hasMessages", "(I)Z");

	// GLView members
	jGLViewCls = (jclass)jEnv->NewGlobalRef(jEnv->FindClass("com/imagine/GLView"));
	jSwapBuffers.setup(jEnv, jGLViewCls, "swapBuffers", "()V");
	jSetKeepScreenOn.setup(jEnv, jGLViewCls, "setKeepScreenOn", "(Z)V");
	//jFinish.setup(env, jBaseActivityCls, "finish", "()V");

	#ifdef CONFIG_BLUEZ
	jMessageCls = (jclass)jEnv->NewGlobalRef(jEnv->FindClass("android/os/Message"));
	jObtain.setup(jEnv, jMessageCls, "obtain", "(Landroid/os/Handler;III)Landroid/os/Message;");
	jSendToTarget.setup(jEnv, jMessageCls, "sendToTarget", "()V");
	msgHandlerId = jEnv->GetStaticFieldID(jBaseActivityCls, "msgHandler", "Landroid/os/Handler;");
	//logMsg("msgHandlerId %d", (int)msgHandlerId);
	#endif

	static JNINativeMethod activityMethods[] =
	{
	    {"timerCallback", "(Z)Z", (void *)&timerCallback},
	    {"appPaused", "()V", (void *)&appPaused},
	    {"appResumed", "(Z)V", (void *)&appResumed},
	    {"appFocus", "(Z)Z", (void *)&appFocus},
	    {"configChange", "(III)V", (void *)&configChange},
	    {"envConfig", "(Ljava/lang/String;Ljava/lang/String;FFIIIIILjava/lang/String;Ljava/lang/String;Landroid/os/Vibrator;)V", (void *)&envConfig},
	    {"handleAndroidMsg", "(III)Z", (void *)&handleAndroidMsg},
	    {"keyEvent", "(IIZ)Z", (void *)&Input::keyEvent},
	    {"layoutChange", "(I)Z", (void *)&Base::layoutChange},
	};

	jEnv->RegisterNatives(jBaseActivityCls, activityMethods, sizeofArray(activityMethods));

	static JNINativeMethod glViewMethods[] =
	{
		{"nativeInit", "(II)V", (void *)&nativeInit},
		{"nativeResize", "(II)V", (void *)&nativeResize},
		{"nativeRender", "()Z", (void *)&nativeRender},
		{"touchEvent", "(IIII)Z", (void *)&Input::touchEvent},
		{"trackballEvent", "(IFF)Z", (void *)&Input::trackballEvent},
	};

	jEnv->RegisterNatives(jGLViewCls, glViewMethods, sizeofArray(glViewMethods));

	return JNI_VERSION_1_6;
}

namespace Input
{
using namespace Base;

void setKeyRepeat(bool on)
{
	logMsg("set key repeat %s", on ? "On" : "Off");
	jEnv->SetStaticBooleanField(jBaseActivityCls, jAllowKeyRepeatsId, on ? 1 : 0);
}

void setHandleVolumeKeys(bool on)
{
	logMsg("set volume key use %s", on ? "On" : "Off");
	jEnv->SetStaticBooleanField(jBaseActivityCls, jHandleVolumeKeysId, on ? 1 : 0);
}

}



#undef thisModuleName
