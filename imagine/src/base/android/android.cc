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

#define LOGTAG "Base"
#include <cstdlib>
#include <android/window.h>
#include <android/configuration.h>
#include <android/looper.h>
#include <android/native_activity.h>
#include <android/api-level.h>
#include <dlfcn.h>
#include <imagine/logger/logger.h>
#include <imagine/base/Base.hh>
#include <imagine/base/Timer.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/fd-utils.h>
#include <imagine/util/bits.h>
#include <imagine/util/assume.h>
#include <imagine/util/algorithm.h>
#include "../common/windowPrivate.hh"
#include "../common/basePrivate.hh"
#include "android.hh"
#include "internal.hh"

namespace Base
{

JavaVM* jVM{};
static JNIEnv* jEnv_{};
static JavaInstMethod<void()> jRecycle{};

// activity
jclass jBaseActivityCls{};
jobject jBaseActivity{};
uint appState = APP_PAUSED;
bool aHasFocus = true;
static AConfiguration *aConfig{};
static AAssetManager *assetManager{};
static JavaInstMethod<void(jint)> jSetUIVisibility{};
static JavaInstMethod<jobject()> jNewFontRenderer{};
JavaInstMethod<void(jint)> jSetRequestedOrientation{};
static const char *filesDir{};
static uint aSDK = __ANDROID_API__;
static bool osAnimatesRotation = false;
SurfaceRotation osRotation{};
static SystemOrientationChangedDelegate onSystemOrientationChanged{};
static bool hasPermanentMenuKey = true;
static bool keepScreenOn = false;
static Timer userActivityCallback{};
static uint uiVisibilityFlags = SYS_UI_STYLE_NO_FLAGS;
AInputQueue *inputQueue{};
static void *mainLibHandle{};
static bool unloadNativeLibOnDestroy = false;

// window
JavaInstMethod<void(jint, jint)> jSetWinFlags{};
JavaInstMethod<void(jint)> jSetWinFormat{};
JavaInstMethod<jint()> jWinFormat{}, jWinFlags{};

void recycleBitmap(JNIEnv *env, jobject bitmap)
{
	if(unlikely(!jRecycle))
	{
		jRecycle.setup(env, env->GetObjectClass(bitmap), "recycle", "()V");
	}
	jRecycle(env, bitmap);
}

uint appActivityState() { return appState; }

void exit(int returnVal)
{
	// TODO: return exit value as activity result
	appState = APP_EXITING;
	auto env = jEnv();
	JavaInstMethod<void()> jFinish{env, jBaseActivityCls, "finish", "()V"};
	jFinish(env, jBaseActivity);
}

void abort()
{
	::abort();
}

//void openURL(const char *url) { };

FS::PathString assetPath() { return {}; }

FS::PathString documentsPath()
{
	if(Base::androidSDK() < 11) // bug in pre-3.0 Android causes paths in ANativeActivity to be null
	{
		//logMsg("ignoring paths from ANativeActivity due to Android 2.3 bug");
		auto env = jEnv();
		JavaInstMethod<jobject()> filesDir{env, jBaseActivityCls, "filesDir", "()Ljava/lang/String;"};
		auto filesDirStr = (jstring)filesDir(env, jBaseActivity);
		FS::PathString path{};
		javaStringCopy(env, path, filesDirStr);
		return path;
	}
	else
		return FS::makePathString(filesDir);
}

FS::PathString storagePath()
{
	auto env = jEnv();
	JavaClassMethod<jobject()> extStorageDir{env, jBaseActivityCls, "extStorageDir", "()Ljava/lang/String;"};
	auto extStorageDirStr = (jstring)extStorageDir(env, jBaseActivityCls);
	FS::PathString path{};
	javaStringCopy(env, path, extStorageDirStr);
	return path;
}

FS::PathString libPath()
{
	auto env = jEnv();
	JavaInstMethod<jobject()> libDir{env, jBaseActivityCls, "libDir", "()Ljava/lang/String;"};
	auto libDirStr = (jstring)libDir(env, jBaseActivity);
	FS::PathString path{};
	javaStringCopy(env, path, libDirStr);
	return path;
}

FS::PathString mainSOPath()
{
	auto env = jEnv();
	JavaInstMethod<jobject()> soPath{env, jBaseActivityCls, "mainSOPath", "()Ljava/lang/String;"};
	auto soPathStr = (jstring)soPath(env, jBaseActivity);
	FS::PathString path{};
	javaStringCopy(env, path, soPathStr);
	return path;
}

bool documentsPathIsShared() { return false; }

static jstring permissionToJString(JNIEnv *env, Permission p)
{
	switch(p)
	{
		case Permission::WRITE_EXT_STORAGE: return env->NewStringUTF("android.permission.WRITE_EXTERNAL_STORAGE");
		default: return nullptr;
	}
}

bool usesPermission(Permission p)
{
	if(Base::androidSDK() < 23)
		return false;
	return true;
}

bool requestPermission(Permission p)
{
	if(Base::androidSDK() < 23)
		return false;
	auto env = jEnv();
	auto permissionJStr = permissionToJString(env, p);
	if(!permissionJStr)
		return false;
	JavaInstMethod<jboolean(jobject)> requestPermission{env, jBaseActivityCls, "requestPermission", "(Ljava/lang/String;)Z"};
	return requestPermission(env, jBaseActivity, permissionJStr);
}

AAssetManager *activityAAssetManager()
{
	assert(assetManager);
	return assetManager;
}

bool hasHardwareNavButtons()
{
	return hasPermanentMenuKey;
}

uint androidSDK()
{
	return std::max((uint)__ANDROID_API__, aSDK);
}

void setOnSystemOrientationChanged(SystemOrientationChangedDelegate del)
{
	onSystemOrientationChanged = del;
}

bool Window::systemAnimatesRotation()
{
	return osAnimatesRotation;
}

uint defaultSystemOrientations()
{
	return VIEW_ROTATE_ALL;
}

jobject newFontRenderer(JNIEnv *env)
{
	return jNewFontRenderer(env, jBaseActivity);
}

static void initConfig(AConfiguration* config)
{
	Input::initInputConfig(config);
}

int pixelFormatToDirectAndroidFormat(IG::PixelFormatID format)
{
	using namespace IG;
	switch(format)
	{
		case PIXEL_RGBA8888: return HAL_PIXEL_FORMAT_RGBA_8888;
		case PIXEL_BGRA8888: return HAL_PIXEL_FORMAT_BGRA_8888;
		case PIXEL_RGB888: return HAL_PIXEL_FORMAT_RGB_888;
		case PIXEL_RGB565: return HAL_PIXEL_FORMAT_RGB_565;
		default: return 0;
	}
}

static void activityInit(JNIEnv* env, jobject activity)
{
	// BaseActivity members
	{
		jBaseActivityCls = (jclass)env->NewGlobalRef(env->GetObjectClass(activity));
		jSetRequestedOrientation.setup(env, jBaseActivityCls, "setRequestedOrientation", "(I)V");
		#ifdef CONFIG_RESOURCE_FONT_ANDROID
		jNewFontRenderer.setup(env, jBaseActivityCls, "newFontRenderer", "()Lcom/imagine/FontRenderer;");
		#endif
		{
			JNINativeMethod method[]
			{
				{
					"onContentRectChanged", "(JIIIIII)V",
					(void*)(void (*)(JNIEnv*, jobject, jlong, jint, jint, jint, jint, jint, jint))
					([](JNIEnv* env, jobject thiz, jlong windowAddr, jint x, jint y, jint x2, jint y2, jint winWidth, jint winHeight)
					{
						auto win = windowAddr ? (Window*)windowAddr : deviceWindow();
						androidWindowContentRectChanged(*win, {x, y, x2, y2}, {winWidth, winHeight});
					})
				}
			};
			env->RegisterNatives(jBaseActivityCls, method, IG::size(method));
		}

		if(Config::DEBUG_BUILD)
		{
			logMsg("internal storage path: %s", documentsPath().data());
			logMsg("external storage path: %s", storagePath().data());
		}

		logger_init();
		engineInit();
		logMsg("SDK API Level: %d", aSDK);

		if(Base::androidSDK() >= 11)
			osAnimatesRotation = true;
		else
		{
			JavaClassMethod<jboolean()> jAnimatesRotation{env, jBaseActivityCls, "gbAnimatesRotation", "()Z"};
			osAnimatesRotation = jAnimatesRotation(env, jBaseActivityCls);
		}
		if(!osAnimatesRotation)
		{
			logMsg("app handles rotation animations");
		}

		if(!Config::MACHINE_IS_OUYA)
		{
			if(Base::androidSDK() >= 14)
			{
				JavaInstMethod<jboolean()> jHasPermanentMenuKey{env, jBaseActivityCls, "hasPermanentMenuKey", "()Z"};
				Base::hasPermanentMenuKey = jHasPermanentMenuKey(env, activity);
				if(Base::hasPermanentMenuKey)
				{
					logMsg("device has hardware nav/menu keys");
				}
			}
			else
				Base::hasPermanentMenuKey = 1;
		}

		if(unloadNativeLibOnDestroy)
		{
			auto soPath = mainSOPath();
			#if __ANDROID_API__ >= 21
			mainLibHandle = dlopen(soPath.data(), RTLD_LAZY | RTLD_NOLOAD);
			#else
			mainLibHandle = dlopen(soPath.data(), RTLD_LAZY);
			dlclose(mainLibHandle);
			#endif
			if(!mainLibHandle)
				logWarn("unable to get native lib handle");
		}
	}

	initScreens(env, activity, jBaseActivityCls);
	initFrameTimer(env, activity);

	jSetWinFlags.setup(env, jBaseActivityCls, "setWinFlags", "(II)V");
	jWinFlags.setup(env, jBaseActivityCls, "winFlags", "()I");
	if(Base::androidSDK() < 11)
	{
		jSetWinFormat.setup(env, jBaseActivityCls, "setWinFormat", "(I)V");
		jWinFormat.setup(env, jBaseActivityCls, "winFormat", "()I");
	}

	if(Base::androidSDK() >= 11)
	{
		jSetUIVisibility.setup(env, jBaseActivityCls, "setUIVisibility", "(I)V");
	}
}

JNIEnv* jEnv() { assert(jEnv_); return jEnv_; }

void setIdleDisplayPowerSave(bool on)
{
	auto env = jEnv();
	jint keepOn = !on;
	auto keepsScreenOn = userActivityCallback ? false : (bool)(jWinFlags(env, jBaseActivity) & AWINDOW_FLAG_KEEP_SCREEN_ON);
	if(keepOn != keepsScreenOn)
	{
		logMsg("keep screen on: %d", keepOn);
		jSetWinFlags(env, jBaseActivity, keepOn ? AWINDOW_FLAG_KEEP_SCREEN_ON : 0, AWINDOW_FLAG_KEEP_SCREEN_ON);
	}
	keepScreenOn = keepOn; // cache value for endIdleByUserActivity()
}

void endIdleByUserActivity()
{
	if(!keepScreenOn)
	{
		//logMsg("signaling user activity to the OS");
		auto env = jEnv();
		// quickly toggle KEEP_SCREEN_ON flag to brighten screen,
		// waiting about 20ms before toggling it back off triggers the screen to brighten if it was already dim
		jSetWinFlags(env, jBaseActivity, AWINDOW_FLAG_KEEP_SCREEN_ON, AWINDOW_FLAG_KEEP_SCREEN_ON);
		userActivityCallback.callbackAfterMSec(
			[env]()
			{
				if(!keepScreenOn)
				{
					jSetWinFlags(env, jBaseActivity, 0, AWINDOW_FLAG_KEEP_SCREEN_ON);
				}
			}, 20);
	}
}

static void setStatusBarHidden(JNIEnv *env, bool hidden)
{
	auto statusBarIsHidden = (bool)(jWinFlags(env, jBaseActivity) & AWINDOW_FLAG_FULLSCREEN);
	if(hidden != statusBarIsHidden)
	{
		logMsg("setting app window fullscreen: %u", hidden);
		jSetWinFlags(env, jBaseActivity, hidden ? AWINDOW_FLAG_FULLSCREEN : 0, AWINDOW_FLAG_FULLSCREEN);
	}
}

void setSysUIStyle(uint flags)
{
	// Flags mapped directly to SYSTEM_UI_FLAG_*
	// SYS_UI_STYLE_DIM_NAV -> SYSTEM_UI_FLAG_LOW_PROFILE (0)
	// SYS_UI_STYLE_HIDE_NAV -> SYSTEM_UI_FLAG_HIDE_NAVIGATION (1)
	// SYS_UI_STYLE_HIDE_STATUS -> SYSTEM_UI_FLAG_FULLSCREEN (2)
	// SYS_UI_STYLE_NO_FLAGS -> SYSTEM_UI_FLAG_VISIBLE (no bits set)

	if(Config::MACHINE_IS_OUYA)
	{
		return;
	}
	auto env = jEnv();
	// Using SYSTEM_UI_FLAG_FULLSCREEN has an odd delay when
	// combined with SYSTEM_UI_FLAG_HIDE_NAVIGATION, so we'll
	// set the window flag even on Android 4.1+.
	// TODO: Re-test on Android versions after 4.4 for any change
	//if(androidSDK() < 16)
	{
		// handle status bar hiding via full-screen window flag
		setStatusBarHidden(env, flags & SYS_UI_STYLE_HIDE_STATUS);
	}
	if(androidSDK() >= 11)
	{
		constexpr uint SYSTEM_UI_FLAG_IMMERSIVE_STICKY = 0x1000;
		// Add SYSTEM_UI_FLAG_IMMERSIVE_STICKY for use with Android 4.4+ if flags contain OS_NAV_STYLE_HIDDEN
		if(flags & SYS_UI_STYLE_HIDE_NAV)
			flags |= SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
		logMsg("setting UI visibility: 0x%X", flags);
		uiVisibilityFlags = flags;
		jSetUIVisibility(env, jBaseActivity, flags);
	}
}

bool hasTranslucentSysUI()
{
	return androidSDK() >= 19;
}

static void setNativeActivityCallbacks(ANativeActivity* activity)
{
	activity->callbacks->onDestroy =
		[](ANativeActivity *)
		{
			removePostedNotifications();
			if(mainLibHandle)
			{
				logMsg("closing main lib:%p", mainLibHandle);
				dlclose(mainLibHandle);
			}
			else
			{
				logMsg("exiting process");
				::exit(0);
			}
		};
	activity->callbacks->onStart =
		[](ANativeActivity *activity)
		{
			logMsg("app started");
			appState = APP_RUNNING;
			Screen::setActiveAll(true);
			dispatchOnResume(aHasFocus);
			handleIntent(activity->env, activity->clazz);
		};
	activity->callbacks->onResume =
		[](ANativeActivity *activity)
		{
			logMsg("app resumed");
			appState = APP_RUNNING;
			#ifdef CONFIG_INPUT_ANDROID_MOGA
			Input::onResumeMOGA(jEnv(), true);
			#endif
		};
	//activity->callbacks->onSaveInstanceState = nullptr; // unused
	activity->callbacks->onPause =
		[](ANativeActivity *activity)
		{
			if(Base::androidSDK() < 11)
			{
				if(appIsRunning())
					appState = APP_PAUSED;
				logMsg("app %s", appState == APP_PAUSED ? "paused" : "exiting");
				// App is killable in Android 2.3, run exit handler to save volatile data
				dispatchOnExit(appState == APP_PAUSED);
			}
			else
				logMsg("app paused");
			#ifdef CONFIG_INPUT_ANDROID_MOGA
			Input::onPauseMOGA(activity->env);
			#endif
			Input::deinitKeyRepeatTimer();
		};
	activity->callbacks->onStop =
		[](ANativeActivity *activity)
		{
			if(Base::androidSDK() >= 11)
			{
				if(appIsRunning())
					appState = APP_PAUSED;
				logMsg("app %s", appState == APP_PAUSED ? "stopped" : "exiting");
				dispatchOnExit(appState == APP_PAUSED);
			}
			else
				logMsg("app stopped");
			Screen::setActiveAll(false);
		};
	activity->callbacks->onConfigurationChanged =
		[](ANativeActivity *activity)
		{
			AConfiguration_fromAssetManager(aConfig, activity->assetManager);
			auto rotation = mainScreen().rotation(activity->env);
			if(rotation != osRotation)
			{
				logMsg("changed OS orientation");
				auto oldRotation = osRotation;
				osRotation = rotation;
				if(onSystemOrientationChanged)
					onSystemOrientationChanged(oldRotation, rotation);
			}
			Input::changeInputConfig(aConfig);
		};
	activity->callbacks->onLowMemory =
		[](ANativeActivity *)
		{
			dispatchOnFreeCaches();
		};
	activity->callbacks->onWindowFocusChanged =
		[](ANativeActivity *activity, int focused)
		{
			aHasFocus = focused;
			logMsg("focus change: %d", focused);
			if(focused && Base::androidSDK() >= 11)
			{
				// re-apply UI visibility flags
				jSetUIVisibility(activity->env, activity->clazz, uiVisibilityFlags);
			}
			iterateTimes(Window::windows(), i)
			{
				Window::window(i)->dispatchFocusChange(focused);
			}
			if(!focused)
				Input::deinitKeyRepeatTimer();
		};
	activity->callbacks->onNativeWindowCreated =
		[](ANativeActivity *activity, ANativeWindow *nWin)
		{
			if(Base::androidSDK() < 11)
			{
				// In testing with CM7 on a Droid, the surface is re-created in RGBA8888 upon
				// resuming the app no matter what format was used in ANativeWindow_setBuffersGeometry().
				// Explicitly setting the format here seems to fix the problem (Android driver bug?).
				// In case of a mismatch, the surface is usually destroyed & re-created by the OS after this callback.
				if(Config::DEBUG_BUILD)
				{
					logMsg("setting window format to %d (current %d) after surface creation",
						deviceWindow()->nativePixelFormat(), ANativeWindow_getFormat(nWin));
				}
				jSetWinFormat(activity->env, activity->clazz, deviceWindow()->nativePixelFormat());
			}
			deviceWindow()->setNativeWindow(nWin);
		};
	activity->callbacks->onNativeWindowDestroyed =
		[](ANativeActivity *, ANativeWindow *)
		{
			deviceWindow()->setNativeWindow(nullptr);
		};
	// Note: Surface resizing handled by ContentView callback
	//activity->callbacks->onNativeWindowResized = nullptr;
	activity->callbacks->onNativeWindowRedrawNeeded =
		[](ANativeActivity *, ANativeWindow *)
		{
			androidWindowNeedsRedraw(*deviceWindow());
		};
	activity->callbacks->onInputQueueCreated =
		[](ANativeActivity *, AInputQueue *queue)
		{
			inputQueue = queue;
			logMsg("input queue created & attached");
			AInputQueue_attachLooper(queue, activityLooper(), ALOOPER_POLL_CALLBACK,
				[](int, int, void* data)
				{
					Input::processInput((AInputQueue*)data);
					return 1;
				}, queue);
		};
	activity->callbacks->onInputQueueDestroyed =
		[](ANativeActivity *, AInputQueue *queue)
		{
			logMsg("input queue destroyed");
			inputQueue = nullptr;
			AInputQueue_detachLooper(queue);
		};
	// Note: Content rectangle handled by ContentView callback
	// for correct handling of system window insets
	//activity->callbacks->onContentRectChanged = nullptr;
}

}

CLINK void LVISIBLE ANativeActivity_onCreate(ANativeActivity* activity, void* savedState, size_t savedStateSize)
{
	using namespace Base;
	if(Config::DEBUG_BUILD)
		logMsg("called ANativeActivity_onCreate, thread ID %d", gettid());
	aSDK = activity->sdkVersion;
	initActivityLooper();
	jVM = activity->vm;
	assetManager = activity->assetManager;
	jBaseActivity = activity->clazz;
	jEnv_ = activity->env;
	filesDir = activity->internalDataPath;
	activityInit(jEnv_, activity->clazz);
	setNativeActivityCallbacks(activity);
	Input::init();
	aConfig = AConfiguration_new();
	AConfiguration_fromAssetManager(aConfig, activity->assetManager);
	initConfig(aConfig);
	onInit(0, nullptr);
	if(!Window::windows())
	{
		bug_exit("didn't create a window");
	}
}
