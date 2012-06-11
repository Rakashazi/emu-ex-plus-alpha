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
#include <errno.h>
#include <util/egl.hh>
#include <logger/interface.h>
#include <engine-globals.h>
#include <base/android/sdk.hh>
#include <base/Base.hh>
#include <base/common/funcs.h>
#include <base/common/PollWaitTimer.hh>
#include <input/android/private.hh>
#include <android/window.h>
#include <dlfcn.h>
#include "private.hh"

void* android_app_entry(void* param);

ANativeWindow* (*ANativeWindow_fromSurfaceTexture)(JNIEnv* env, jobject surfaceTexture) = nullptr;

namespace Base
{

static const EGLint attribs16BPP[] =
{
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	//EGL_DEPTH_SIZE, 24,
	EGL_NONE
};

static const EGLint attribs24BPP[] =
{
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	EGL_BLUE_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_RED_SIZE, 8,
	EGL_NONE
};

static const EGLint attribs32BPP[] =
{
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	EGL_BLUE_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_RED_SIZE, 8,
	EGL_ALPHA_SIZE, 8,
	EGL_NONE
};

struct EGLWindow
{
	EGLDisplay display = EGL_NO_DISPLAY;
	EGLSurface surface = EGL_NO_SURFACE;
	EGLContext context = EGL_NO_CONTEXT;
	EGLConfig config = 0;
	int winFormat = defaultWinFormat;
	bool gotFormat = 0;
	static constexpr int defaultWinFormat = WINDOW_FORMAT_RGB_565;

	constexpr EGLWindow() { }

	void initEGL()
	{
		logMsg("doing EGL init");
		display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		assert(display != EGL_NO_DISPLAY);
		eglInitialize(display, 0, 0);

		if(Base::androidSDK() >= 11)
		{
			logMsg("defaulting to 32-bit color");
			winFormat = WINDOW_FORMAT_RGBA_8888;
		}

		//printEGLConfs(display);

		logMsg("%s (%s), extensions: %s", eglQueryString(display, EGL_VENDOR), eglQueryString(display, EGL_VERSION), eglQueryString(display, EGL_EXTENSIONS));
	}

	void init(ANativeWindow *win)
	{
		assert(display != EGL_NO_DISPLAY);
		int currFormat = ANativeWindow_getFormat(win);
		if(currFormat != winFormat)
		{
			logMsg("changing window format from %d to %d", currFormat, winFormat);
			ANativeWindow_setBuffersGeometry(win, 0, 0, winFormat);
		}

		if(!gotFormat)
		{
			const EGLint *attribs = attribs16BPP;

			switch(winFormat)
			{
				//bcase WINDOW_FORMAT_RGBX_8888: attribs = attribs24BPP;
				bcase WINDOW_FORMAT_RGBA_8888: attribs = attribs32BPP;
			}

			//EGLConfig config;
			EGLint configs = 0;
			eglChooseConfig(display, attribs, &config, 1, &configs);
			#ifndef NDEBUG
			printEGLConf(display, config);
			#endif
			//EGLint format;
			//eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
			gotFormat = 1;
		}

		if(context == EGL_NO_CONTEXT)
		{
			logMsg("creating GL context");
			context = eglCreateContext(display, config, 0, 0);
		}

		assert(surface == EGL_NO_SURFACE);
		logMsg("creating window surface");
		surface = eglCreateWindowSurface(display, config, win, 0);

		if(eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
		{
			logErr("error in eglMakeCurrent");
			abort();
		}
		logMsg("window size: %d,%d", ANativeWindow_getWidth(win), ANativeWindow_getHeight(win));

		/*if(eglSwapInterval(display, 1) != EGL_TRUE)
		{
			logErr("error in eglSwapInterval");
		}*/
	}

	void destroySurface()
	{
		if(isDrawable())
		{
			logMsg("destroying window surface");
			eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			eglDestroySurface(display, surface);
			surface = EGL_NO_SURFACE;
		}
	}

	bool verifyContext()
	{
		return eglGetCurrentContext() != EGL_NO_CONTEXT;
	}

	bool isDrawable()
	{
		return surface != EGL_NO_SURFACE;
	}

	void swap()
	{
		eglSwapBuffers(display, surface);
	}
} eglWin;

}

DLList<PollWaitTimer*>::Node PollWaitTimer::timerListNode[4];
DLList<PollWaitTimer*> PollWaitTimer::timerList(timerListNode);

#include "common.hh"

namespace Base
{

static fbool engineIsInit = 0;
static JavaInstMethod jGetRotation;
static jobject jDpy;
jclass jSurfaceTextureCls;
JavaInstMethod postUIThread, jSurfaceTexture, jUpdateTexImage, jSurfaceTextureRelease/*, jSetDefaultBufferSize*/;
static PollWaitTimer timerCallback;
static bool aHasFocus = 1;

// Public implementation

uint appState = APP_PAUSED;

void sendMessageToMain(ThreadPThread &, int type, int shortArg, int intArg, int intArg2)
{
	assert(appInstance()->msgwrite);
	assert(type != MSG_INPUT); // currently all code should use main event loop for input events
	uint16 shortArg16 = shortArg;
	int msg[3] = { (shortArg16 << 16) | type, intArg, intArg2 };
	logMsg("sending msg type %d with args %d %d %d", msg[0] & 0xFFFF, msg[0] >> 16, msg[1], msg[2]);
	if(::write(appInstance()->msgwrite, &msg, sizeof(msg)) != sizeof(msg))
	{
		logErr("unable to write message to pipe: %s", strerror(errno));
	}
}

void setIdleDisplayPowerSave(bool on)
{
	jint keepOn = !on;
	logMsg("keep screen on: %d", keepOn);
	jEnv->CallVoidMethod(jBaseActivity, postUIThread.m, 0, keepOn);
}

void setOSNavigationStyle(uint flags)
{
	// Flags mapped directly
	// OS_NAV_STYLE_DIM -> SYSTEM_UI_FLAG_LOW_PROFILE (1)
	// OS_NAV_STYLE_HIDDEN -> SYSTEM_UI_FLAG_HIDE_NAVIGATION (2)
	// 0 -> SYSTEM_UI_FLAG_VISIBLE
	jEnv->CallVoidMethod(jBaseActivity, postUIThread.m, 1, flags);
}

void setTimerCallback(TimerCallbackFunc f, void *ctx, int ms)
{
	timerCallback.setCallback(f, ctx, ms);
}

void addPollEvent2(int fd, PollEventDelegate &handler, uint events)
{
	logMsg("adding fd %d to looper", fd);
	assert(appInstance()->looper);
	int ret = ALooper_addFd(appInstance()->looper, fd, LOOPER_ID_USER, events, 0, &handler);
	assert(ret == 1);
}

void modPollEvent(int fd, PollEventDelegate &handler, uint events)
{
	addPollEvent2(fd, handler, events);
}

void removePollEvent(int fd)
{
	logMsg("removing fd %d from looper", fd);
	int ret = ALooper_removeFd(appInstance()->looper, fd);
	assert(ret != -1);
}

void destroyPollEvent(int fd)
{
	logMsg("removing fd %d from looper", fd);
	ALooper_removeFd(appInstance()->looper, fd);
}

void openGLUpdateScreen()
{
	/*TimeSys preTime;
	preTime.setTimeNow();*/
	eglWin.swap();
	/*TimeSys postTime;
	postTime.setTimeNow();
	logMsg("swap took %f", double(postTime - preTime));*/
}

// Private implementation

EGLDisplay getAndroidEGLDisplay()
{
	assert(eglWin.display != EGL_NO_DISPLAY);
	return eglWin.display;
}

bool windowIsDrawable()
{
	return eglWin.isDrawable();
}

// runs from activity thread, do not use jEnv
static void JNICALL jEnvConfig(JNIEnv* env, jobject thiz, jfloat xdpi, jfloat ydpi, jint refreshRate, jobject dpy,
		jstring devName, jstring filesPath, jstring eStoragePath, jstring apkPath, jobject sysVibrator,
		jboolean hasPermanentMenuKey)
{
	logMsg("doing java env config");
	logMsg("set screen DPI size %f,%f", (double)xdpi, (double)ydpi);
	// DPI values must come in un-rotated from DisplayMetrics
	androidXDPI = xDPI = xdpi;
	androidYDPI = yDPI = ydpi;

	jDpy = env->NewGlobalRef(dpy);
	refreshRate_ = refreshRate;
	logMsg("refresh rate: %d", refreshRate);

	const char *devNameStr = env->GetStringUTFChars(devName, 0);
	logMsg("device name: %s", devNameStr);
	setDeviceType(devNameStr);
	env->ReleaseStringUTFChars(devName, devNameStr);

	filesDir = env->GetStringUTFChars(filesPath, 0);
	eStoreDir = env->GetStringUTFChars(eStoragePath, 0);
	appPath = env->GetStringUTFChars(apkPath, 0);
	logMsg("apk @ %s", appPath);

	if(sysVibrator)
	{
		logMsg("Vibrator object present");
		vibrator = env->NewGlobalRef(sysVibrator);
		setupVibration(env);
	}

	Base::hasPermanentMenuKey = hasPermanentMenuKey;
	if(!hasPermanentMenuKey)
	{
		logMsg("device has software nav buttons");
	}

	var_copy(act, appInstance());
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&act->thread, &attr, android_app_entry, act);

	// Wait for thread to start.
	pthread_mutex_lock(&act->mutex);
	while (!act->running)
	{
		pthread_cond_wait(&act->cond, &act->mutex);
	}
	pthread_mutex_unlock(&act->mutex);
}

static void envConfig(int orientation, int hardKeyboardState, int navigationState)
{
	logMsg("doing native env config, starting orientation %d", orientation);
	osOrientation = orientation;

	aHardKeyboardState = (devType == DEV_TYPE_XPERIA_PLAY) ? navigationState : hardKeyboardState;
	logMsg("keyboard/nav hidden: %s", hardKeyboardNavStateToStr(aHardKeyboardState));
}

static void nativeInit(jint w, jint h)
{
	doOrExit(logger_init());
	initialScreenSizeSetup(w, h);
	engineInit();
	logMsg("done init");
	engineIsInit = 1;
}

static bool appFocus(bool hasFocus)
{
	aHasFocus = hasFocus;
	logMsg("focus change: %d", (int)hasFocus);
	var_copy(prevGfxUpdateState, gfxUpdate);
	if(engineIsInit)
		onFocusChange(hasFocus);
	return appState == APP_RUNNING && prevGfxUpdateState == 0 && gfxUpdate;
}

static void configChange(struct android_app* app, jint hardKeyboardState, jint navState, jint orientation)
{
	logMsg("config change, keyboard: %s, navigation: %s", hardKeyboardNavStateToStr(hardKeyboardState), hardKeyboardNavStateToStr(navState));
	setHardKeyboardState((devType == DEV_TYPE_XPERIA_PLAY) ? navState : hardKeyboardState);

	if(setOrientationOS(orientation) && androidSDK() < 11)
	{
		// hack for some Android 2.3 devices that won't return the correct window size
		// in the next redraw event unless a certain amount of frames are rendered
		if(appState == APP_RUNNING && eglWin.isDrawable())
		{
			iterateTimes(3, i)
			{
				int w = ANativeWindow_getWidth(app->window);
				int h = ANativeWindow_getHeight(app->window);
				mainWin.rect.x2 = w; mainWin.rect.y2 = h;
				resizeEvent(w, h);
				gfxUpdate = 1;
				runEngine();
			}
		}
	}
}

static void appPaused()
{
	logMsg("app paused");
	appState = APP_PAUSED;
	if(engineIsInit)
	{
		onExit(1);
	}
}

static void appResumed()
{
	logMsg("app resumed");
	appState = APP_RUNNING;
	if(engineIsInit)
	{
		onResume(aHasFocus);
		displayNeedsUpdate();
	}
}

void onAppCmd(struct android_app* app, uint32 cmd)
{
	uint cmdType = cmd & 0xFFFF;
	switch (cmdType)
	{
		bcase APP_CMD_START:
		logMsg("app started");
		bcase APP_CMD_SAVE_STATE:
		//logMsg("got APP_CMD_SAVE_STATE");
		/*app->savedState = malloc(sizeof(struct saved_state));
		*((struct saved_state*)app->savedState) = engine->state;
		app->savedStateSize = sizeof(struct saved_state);*/
		bcase APP_CMD_INIT_WINDOW:
		if(app->window)
		{
			eglWin.init(app->window);
			int w = ANativeWindow_getWidth(app->window);
			int h = ANativeWindow_getHeight(app->window);
			mainWin.rect.x2 = w; mainWin.rect.y2 = h;
			logMsg("window init, size %d,%d", w, h);
			if(!engineIsInit)
				nativeInit(w, h);
			else
				resizeEvent(w, h);
		}
		bcase APP_CMD_TERM_WINDOW:
		//logMsg("got APP_CMD_TERM_WINDOW");
		pthread_mutex_lock(&app->mutex);
		eglWin.destroySurface();
		pthread_cond_signal(&app->cond);
		pthread_mutex_unlock(&app->mutex);
		bcase APP_CMD_GAINED_FOCUS:
		if(app->window)
		{
			gfxUpdate = 1;
			int w = ANativeWindow_getWidth(app->window);
			int h = ANativeWindow_getHeight(app->window);
			mainWin.rect.x2 = w; mainWin.rect.y2 = h;
			logMsg("window gained focus, size %d,%d", w, h);
			resizeEvent(w, h);
		}
		appFocus(1);
		bcase APP_CMD_LOST_FOCUS:
		appFocus(0);
		bcase APP_CMD_WINDOW_REDRAW_NEEDED:
		{
			logMsg("window redraw needed");
			if(eglWin.isDrawable())
			{
				gfxUpdate = 1;
				runEngine();
			}
		}
		bcase APP_CMD_WINDOW_RESIZED:
		{
			int w = ANativeWindow_getWidth(app->window);
			int h = ANativeWindow_getHeight(app->window);
			mainWin.rect.x2 = w; mainWin.rect.y2 = h;
			logMsg("window resize to %d,%d", w, h);
			resizeEvent(w, h);
			if(eglWin.isDrawable())
			{
				gfxUpdate = 1;
				runEngine();
			}
		}
		bcase APP_CMD_CONTENT_RECT_CHANGED:
		{
			var_ref(rect, app->contentRect);
			int w = ANativeWindow_getWidth(app->window);
			int h = ANativeWindow_getHeight(app->window);
			mainWin.rect.x2 = w; mainWin.rect.y2 = h;
			logMsg("content rect changed to %d,%d %d,%d, window size %d,%d", rect.left, rect.top, rect.right, rect.bottom, w, h);
			resizeEvent(w, h);
			if(eglWin.isDrawable())
			{
				gfxUpdate = 1;
				runEngine();
			}
		}
		bcase APP_CMD_LAYOUT_CHANGED:
		{
			//logMsg("layout change, visible height %d", visibleScreenY);
			resizeEvent(newXSize, visibleScreenY);
			if(appState == APP_RUNNING && eglWin.isDrawable())
			{
				gfxUpdate = 1;
				runEngine();
			}
		}
		bcase APP_CMD_CONFIG_CHANGED:
		configChange(app, AConfiguration_getKeysHidden(app->config),
				AConfiguration_getNavHidden(app->config),
				jEnv->CallIntMethod(jDpy, jGetRotation.m));
		bcase APP_CMD_PAUSE:
		appPaused();
		bcase APP_CMD_RESUME:
		appResumed();
		bcase APP_CMD_STOP:
		logMsg("app stopped");
		bcase APP_CMD_DESTROY:
		logMsg("app destroyed");
		app->activity->vm->DetachCurrentThread();
		::exit(0);
		bcase APP_CMD_INPUT_CHANGED:
		bdefault:
		if(cmdType >= MSG_START)
		{
			uint32 arg[2];
			read(app->msgread, arg, sizeof(arg));
			logMsg("got msg type %d with args %d %d %d", cmd & 0xFFFF, cmd >> 16, arg[0], arg[1]);
			Base::processAppMsg(cmdType, cmd >> 16, arg[0], arg[1]);
		}
		else
			logWarn("got unknown cmd %d", cmd);
		break;
	}
}

static int getPollTimeout()
{
	// When waiting for events:
	// 1. If rendering, don't block
	// 2. Else if a timer is active, block for its remaining time
	// 3. Else block until next event
	int pollTimeout = gfxUpdate ? 0 :
		PollWaitTimer::hasCallbacks() ? PollWaitTimer::getNextCallback()->calcPollWaitForFunc() :
		-1;
	if(pollTimeout >= 2000)
		logMsg("will poll for at most %d ms", pollTimeout);
	/*if(pollTimeout == -1)
		logMsg("will poll for next event");*/
	return pollTimeout;
}

bool hasSurfaceTexture()
{
	return ANativeWindow_fromSurfaceTexture != nullptr;
}

void disableSurfaceTexture()
{
	ANativeWindow_fromSurfaceTexture = nullptr;
}

static void JNICALL layoutChange(JNIEnv* env, jobject thiz, jint height)
{
	assert(height >= 0);
	if(visibleScreenY == (uint)height)
		return;
	logMsg("layout change, view height %d", height);
	visibleScreenY = height;
	if(!engineIsInit)
		return;
	uint32 cmd = APP_CMD_LAYOUT_CHANGED;
	write(appInstance()->msgwrite, &cmd, sizeof(cmd));
}

void jniInit(JNIEnv *jEnv, jobject inst) // uses JNIEnv from Activity thread
{
	using namespace Base;
	logMsg("doing pre-app JNI setup");

	// get class loader instance from Activity
	jclass jNativeActivityCls = jEnv->FindClass("android/app/NativeActivity");
	assert(jNativeActivityCls);
	jmethodID jGetClassLoaderID = jEnv->GetMethodID(jNativeActivityCls, "getClassLoader", "()Ljava/lang/ClassLoader;");
	assert(jGetClassLoaderID);
	jobject jClsLoader = jEnv->CallObjectMethod(inst, jGetClassLoaderID);
	assert(jClsLoader);

	jclass jClsLoaderCls = jEnv->FindClass("java/lang/ClassLoader");
	assert(jClsLoaderCls);
	jmethodID jLoadClassID = jEnv->GetMethodID(jClsLoaderCls, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
	assert(jLoadClassID);

	// BaseActivity members
	jstring baseActivityStr = jEnv->NewStringUTF("com/imagine/BaseActivity");
	jBaseActivityCls = (jclass)jEnv->NewGlobalRef(jEnv->CallObjectMethod(jClsLoader, jLoadClassID, baseActivityStr));
	jSetRequestedOrientation.setup(jEnv, jBaseActivityCls, "setRequestedOrientation", "(I)V");
	jAddNotification.setup(jEnv, jBaseActivityCls, "addNotification", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
	jRemoveNotification.setup(jEnv, jBaseActivityCls, "removeNotification", "()V");
	postUIThread.setup(jEnv, jBaseActivityCls, "postUIThread", "(II)V");

	if(Base::androidSDK() >= 14)
	{
		//logMsg("setting up SurfaceView JNI");
		// Surface members
		//jSurfaceCls = jEnv->FindClass("android/view/Surface");
		//jSurface.setup(jEnv, jSurfaceCls, "<init>", "(Landroid/graphics/SurfaceTexture;)V");
		//jSurfaceRelease.setup(jEnv, jSurfaceCls, "release", "()V");

		// SurfaceTexture members
		jSurfaceTextureCls = (jclass)jEnv->NewGlobalRef(jEnv->FindClass("android/graphics/SurfaceTexture"));
		jSurfaceTexture.setup(jEnv, jSurfaceTextureCls, "<init>", "(I)V");
		//jSetDefaultBufferSize.setup(jEnv, jSurfaceTextureCls, "setDefaultBufferSize", "(II)V");
		jUpdateTexImage.setup(jEnv, jSurfaceTextureCls, "updateTexImage", "()V");
		jSurfaceTextureRelease.setup(jEnv, jSurfaceTextureCls, "release", "()V");
	}

	// Display members
	jclass jDisplayCls = jEnv->FindClass("android/view/Display");
	jGetRotation.setup(jEnv, jDisplayCls, "getRotation", "()I");

	static JNINativeMethod activityMethods[] =
	{
	    {"jEnvConfig", "(FFILandroid/view/Display;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Landroid/os/Vibrator;Z)V", (void *)&Base::jEnvConfig},
	    {"layoutChange", "(I)V", (void *)&Base::layoutChange},
	};

	jEnv->RegisterNatives(jBaseActivityCls, activityMethods, sizeofArray(activityMethods));
}

}

static void dlLoadFuncs()
{
	void *libandroid = 0;

	if(Base::androidSDK() < 12) // no functions from dlopen needed before Android 3.1 (SDK 12)
	{
		return;
	}

	if((libandroid = dlopen("/system/lib/libandroid.so", RTLD_LOCAL | RTLD_LAZY)) == 0)
	{
		logWarn("unable to dlopen libandroid.so");
		return;
	}

	if(Base::androidSDK() >= 14)
	{
		if((ANativeWindow_fromSurfaceTexture = (ANativeWindow* (*)(JNIEnv*, jobject))dlsym(libandroid, "ANativeWindow_fromSurfaceTexture"))
				== 0)
		{
			logWarn("ANativeWindow_fromSurfaceTexture not found");
		}
	}

	#ifdef CONFIG_INPUT_ANDROID
	Input::dlLoadAndroidFuncs(libandroid);
	#endif
}

void android_main(struct android_app* state)
{
	using namespace Base;
	assert(!engineIsInit); // catch accidental activity restarts
	logMsg("started native thread");
	dlLoadFuncs();
	{
		var_copy(act, state->activity);
		var_copy(config, state->config);
		jBaseActivity = act->clazz;
		if(act->vm->AttachCurrentThread(&jEnv, 0) != 0)
		{
			bug_exit("error attaching jEnv to thread");
		}
		envConfig(jEnv->CallIntMethod(jDpy, jGetRotation.m),
			AConfiguration_getKeysHidden(config), AConfiguration_getNavHidden(config));
	}
	eglWin.initEGL();

	/*TimeSys realTime;
	realTime.setTimeNow();*/
	for(;;)
	{
		int ident, events, fd;
		PollEventDelegate* source;
		//logMsg("entering looper");
		while((ident=ALooper_pollAll(getPollTimeout(), &fd, &events, (void**)&source)) >= 0)
		{
			//logMsg("out of looper with event id %d", ident);
			switch(ident)
			{
				bcase LOOPER_ID_MAIN: process_cmd(state);
				bcase LOOPER_ID_INPUT: if(likely(engineIsInit)) process_input(state);
				bdefault: // LOOPER_ID_USER
					assert(source);
					source->invoke(events);
			}
		}
		if(ident == -4)
			logMsg("out of looper with error");

		PollWaitTimer::processCallbacks();

		if(!gfxUpdate)
			logMsg("out of event loop without gfxUpdate, returned %d", ident);

		if(unlikely(appState != APP_RUNNING))
		{
			if(gfxUpdate)
				logMsg("app gfx update halted");
			gfxUpdate = 0; // halt screen updates
		}
		else
		{
			if(!eglWin.isDrawable())
				logMsg("drawing without EGL surface");
			/*TimeSys prevTime = realTime;
			realTime.setTimeNow();
			logMsg("%f since last screen update", double(realTime - prevTime));*/
			runEngine();
		}
	}
}

#undef thisModuleName
