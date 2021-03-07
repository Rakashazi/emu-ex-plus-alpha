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
#include <android/bitmap.h>
#include <android/hardware_buffer.h>
#include <dlfcn.h>
#include <imagine/logger/logger.h>
#include <imagine/base/Base.hh>
#include <imagine/base/Screen.hh>
#include <imagine/base/Timer.hh>
#include <imagine/fs/FS.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/util/fd-utils.h>
#include <imagine/util/bits.h>
#include <imagine/util/utility.h>
#include <imagine/util/algorithm.h>
#include "../common/windowPrivate.hh"
#include "../common/basePrivate.hh"
#include "android.hh"
#include "internal.hh"

namespace Base
{

static JavaVM* jVM{};
static pthread_key_t jEnvKey{};
static JavaInstMethod<void()> jRecycle{};

// activity
jclass jBaseActivityCls{};
jobject jBaseActivity{};
static bool aHasFocus = true;
static AAssetManager *assetManager{};
static JavaInstMethod<void(jint)> jSetUIVisibility{};
static JavaInstMethod<jobject()> jNewFontRenderer{};
JavaInstMethod<void(jint)> jSetRequestedOrientation{};
static JavaInstMethod<jint()> jMainDisplayRotation{};
static const char *filesDir{};
static uint32_t aSDK = __ANDROID_API__;
static bool osAnimatesRotation = false;
SurfaceRotation osRotation{};
static SystemOrientationChangedDelegate onSystemOrientationChanged{};
static bool hasPermanentMenuKey = true;
static bool keepScreenOn = false;
static Timer userActivityCallback{"userActivityCallback"};
static uint32_t uiVisibilityFlags = SYS_UI_STYLE_NO_FLAGS;
AInputQueue *inputQueue{};
static void *mainLibHandle{};
static bool unloadNativeLibOnDestroy = false;
static NoopThread noopThread{};

// window
JavaInstMethod<void(jint, jint)> jSetWinFlags{};
JavaInstMethod<void(jint)> jSetWinFormat{};
JavaInstMethod<jint()> jWinFormat{}, jWinFlags{};

IG::PixelFormat makePixelFormatFromAndroidFormat(int32_t androidFormat)
{
	switch(androidFormat)
	{
		case ANDROID_BITMAP_FORMAT_RGBA_8888: return PIXEL_FMT_RGBA8888;
		case ANDROID_BITMAP_FORMAT_RGB_565: return PIXEL_FMT_RGB565;
		case ANDROID_BITMAP_FORMAT_RGBA_4444: return PIXEL_FMT_RGBA4444;
		case ANDROID_BITMAP_FORMAT_A_8: return PIXEL_FMT_I8;
		default:
		{
			if(androidFormat == ANDROID_BITMAP_FORMAT_NONE)
				logWarn("format wasn't provided");
			else
				logErr("unhandled format");
			return PIXEL_FMT_RGBA8888;
		}
	}
}

IG::Pixmap makePixmapView(JNIEnv *env, jobject bitmap, void *pixels, IG::PixelFormat format)
{
	AndroidBitmapInfo info;
	auto res = AndroidBitmap_getInfo(env, bitmap, &info);
	if(unlikely(res != ANDROID_BITMAP_RESULT_SUCCESS))
	{
		logMsg("error getting bitmap info");
		return {};
	}
	//logMsg("android bitmap info:size %ux%u, stride:%u", info.width, info.height, info.stride);
	if(format == PIXEL_FMT_NONE)
	{
		// use format from bitmap info
		format = makePixelFormatFromAndroidFormat(info.format);
	}
	return {{{(int)info.width, (int)info.height}, format}, pixels, {info.stride, IG::Pixmap::BYTE_UNITS}};
}

void recycleBitmap(JNIEnv *env, jobject bitmap)
{
	if(unlikely(!jRecycle))
	{
		jRecycle.setup(env, env->GetObjectClass(bitmap), "recycle", "()V");
	}
	jRecycle(env, bitmap);
}

void exit(int returnVal)
{
	// TODO: return exit value as activity result
	setExitingActivityState();
	auto env = jEnvForThread();
	JavaInstMethod<void()> jFinish{env, jBaseActivityCls, "finish", "()V"};
	jFinish(env, jBaseActivity);
}

void abort()
{
	::abort();
}

FS::PathString assetPath(const char *) { return {}; }

FS::PathString supportPath(const char *)
{
	if(Base::androidSDK() < 11) // bug in pre-3.0 Android causes paths in ANativeActivity to be null
	{
		//logMsg("ignoring paths from ANativeActivity due to Android 2.3 bug");
		auto env = jEnvForThread();
		JavaInstMethod<jobject()> filesDir{env, jBaseActivityCls, "filesDir", "()Ljava/lang/String;"};
		return javaStringCopy<FS::PathString>(env, (jstring)filesDir(env, jBaseActivity));
	}
	else
		return FS::makePathString(filesDir);
}

FS::PathString cachePath(const char *)
{
	auto env = jEnvForThread();
	JavaClassMethod<jobject()> cacheDir{env, jBaseActivityCls, "cacheDir", "()Ljava/lang/String;"};
	return javaStringCopy<FS::PathString>(env, (jstring)cacheDir(env, jBaseActivityCls));
}

FS::PathString sharedStoragePath()
{
	auto env = jEnvForThread();
	JavaClassMethod<jobject()> extStorageDir{env, jBaseActivityCls, "extStorageDir", "()Ljava/lang/String;"};
	return javaStringCopy<FS::PathString>(env, (jstring)extStorageDir(env, jBaseActivityCls));
}

FS::PathLocation sharedStoragePathLocation()
{
	auto path = sharedStoragePath();
	return {path, FS::makeFileString("Storage Media"), {FS::makeFileString("Media"), strlen(path.data())}};
}

std::vector<FS::PathLocation> rootFileLocations()
{
	if(androidSDK() < 14)
	{
		return
			{
				sharedStoragePathLocation(),
			};
	}
	else if(androidSDK() < 24)
	{
		FS::PathString storageDevicesPath{"/storage"};
		return
			{
				sharedStoragePathLocation(),
				{storageDevicesPath, FS::makeFileString("Storage Devices"), {FS::makeFileString("Storage"), strlen(storageDevicesPath.data())}}
			};
	}
	else
	{
		std::vector<FS::PathLocation> rootLocation{sharedStoragePathLocation()};
		logMsg("enumerating storage volumes");
		auto env = jEnvForThread();
		JavaInstMethod<jobject()> jNewStorageManagerHelper{env, Base::jBaseActivityCls, "storageManagerHelper", "()Lcom/imagine/StorageManagerHelper;"};
		auto storageManagerHelper = jNewStorageManagerHelper(env, Base::jBaseActivity);
		auto storageManagerHelperCls = env->GetObjectClass(storageManagerHelper);
		JNINativeMethod method[]
		{
			{
				"volumeEnumerated", "(JLjava/lang/String;Ljava/lang/String;)V",
				(void*)(void (*)(JNIEnv*, jobject, jlong, jstring, jstring))
				([](JNIEnv* env, jobject thiz, jlong userData, jstring jName, jstring jPath)
				{
					auto rootLocation = (std::vector<FS::PathLocation>*)userData;
					auto path = javaStringCopy<FS::PathString>(env, jPath);
					auto name = javaStringCopy<FS::FileString>(env, jName);
					logMsg("volume:%s with path:%s", name.data(), path.data());
					rootLocation->emplace_back(path, name, FS::RootPathInfo{name, strlen(path.data())});
				})
			},
		};
		env->RegisterNatives(storageManagerHelperCls, method, std::size(method));
		JavaClassMethod<void(jobject, jlong)> jEnumVolumes{env, storageManagerHelperCls, "enumVolumes", "(Landroid/app/Activity;J)V"};
		jEnumVolumes(env, storageManagerHelperCls, jBaseActivity, (jlong)&rootLocation);
		return rootLocation;
	}
}

FS::PathString libPath(const char *)
{
	if(Base::androidSDK() < 24)
	{
		auto env = jEnvForThread();
		JavaInstMethod<jobject()> libDir{env, jBaseActivityCls, "libDir", "()Ljava/lang/String;"};
		return javaStringCopy<FS::PathString>(env, (jstring)libDir(env, jBaseActivity));
	}
	return {};
}

static FS::PathString mainSOPath()
{
	if(Base::androidSDK() < 24)
	{
		return FS::makePathStringPrintf("%s/libmain.so", libPath(nullptr).data());
	}
	return FS::makePathString("libmain.so");
}

static jstring permissionToJString(JNIEnv *env, Permission p)
{
	switch(p)
	{
		case Permission::WRITE_EXT_STORAGE: return env->NewStringUTF("android.permission.WRITE_EXTERNAL_STORAGE");
		case Permission::COARSE_LOCATION: return env->NewStringUTF("android.permission.ACCESS_COARSE_LOCATION");
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
	auto env = jEnvForThread();
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

uint32_t androidSDK()
{
	#ifdef ANDROID_COMPAT_API
	static_assert(__ANDROID_API__ == 14, "Compiling with ANDROID_COMPAT_API and API higher than 14");
	return std::max(9u, aSDK);
	#else
	return std::max((uint32_t)__ANDROID_API__, aSDK);
	#endif
}

void setOnSystemOrientationChanged(SystemOrientationChangedDelegate del)
{
	onSystemOrientationChanged = del;
}

bool Window::systemAnimatesRotation()
{
	return osAnimatesRotation;
}

Orientation defaultSystemOrientations()
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

uint32_t toAHardwareBufferFormat(IG::PixelFormatID format)
{
	using namespace IG;
	switch(format)
	{
		case PIXEL_RGBA8888: return AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
		case PIXEL_RGBX8888: return AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM;
		case PIXEL_RGB888: return AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM;
		case PIXEL_RGB565: return AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM;
		default: return 0;
	}
}

static void activityInit(JNIEnv* env, jobject activity)
{
	// BaseActivity members
	{
		jBaseActivityCls = (jclass)env->NewGlobalRef(env->GetObjectClass(activity));
		jSetRequestedOrientation = {env, jBaseActivityCls, "setRequestedOrientation", "(I)V"};
		jMainDisplayRotation = {env, jBaseActivityCls, "mainDisplayRotation", "()I"};
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
						if(likely(win))
							win->setContentRect({x, y, x2, y2}, {winWidth, winHeight});
					})
				}
			};
			env->RegisterNatives(jBaseActivityCls, method, std::size(method));
		}

		if(Config::DEBUG_BUILD)
		{
			logMsg("internal storage path: %s", supportPath({}).data());
			logMsg("external storage path: %s", sharedStoragePath().data());
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
	initFrameTimer(env, activity, Base::mainScreen());

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

JNIEnv* jEnvForThread()
{
	auto env = (JNIEnv*)pthread_getspecific(jEnvKey);
	if(unlikely(!env))
	{
		if(Config::DEBUG_BUILD)
		{
			logDMsg("attaching JNI thread:0x%lx", IG::thisThreadID<long>());
		}
		assumeExpr(jVM);
		if(jVM->AttachCurrentThread(&env, nullptr) != 0)
		{
			logErr("error attaching JNI thread");
			return nullptr;
		}
		pthread_setspecific(jEnvKey, env);
	}
	assumeExpr(env);
	return env;
}

void setIdleDisplayPowerSave(bool on)
{
	auto env = jEnvForThread();
	jint keepOn = !on;
	auto keepsScreenOn = userActivityCallback.isArmed() ? false : (bool)(jWinFlags(env, jBaseActivity) & AWINDOW_FLAG_KEEP_SCREEN_ON);
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
		auto env = jEnvForThread();
		// quickly toggle KEEP_SCREEN_ON flag to brighten screen,
		// waiting about 20ms before toggling it back off triggers the screen to brighten if it was already dim
		jSetWinFlags(env, jBaseActivity, AWINDOW_FLAG_KEEP_SCREEN_ON, AWINDOW_FLAG_KEEP_SCREEN_ON);
		userActivityCallback.runIn(IG::Milliseconds(20), {},
			[env]()
			{
				if(!keepScreenOn)
				{
					jSetWinFlags(env, jBaseActivity, 0, AWINDOW_FLAG_KEEP_SCREEN_ON);
				}
			});
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

void setSysUIStyle(uint32_t flags)
{
	// Flags mapped directly to SYSTEM_UI_FLAG_*
	// SYS_UI_STYLE_DIM_NAV -> SYSTEM_UI_FLAG_LOW_PROFILE (0)
	// SYS_UI_STYLE_HIDE_NAV -> SYSTEM_UI_FLAG_HIDE_NAVIGATION (1)
	// SYS_UI_STYLE_HIDE_STATUS -> SYSTEM_UI_FLAG_FULLSCREEN (2)
	// SYS_UI_STYLE_NO_FLAGS -> SYSTEM_UI_FLAG_VISIBLE (no bits set)

	auto env = jEnvForThread();
	// always handle status bar hiding via full-screen window flag
	// even on SDK level >= 11 so our custom view has the correct insets
	setStatusBarHidden(env, flags & SYS_UI_STYLE_HIDE_STATUS);
	if(androidSDK() >= 11)
	{
		constexpr uint32_t SYSTEM_UI_FLAG_IMMERSIVE_STICKY = 0x1000;
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

SustainedPerformanceType sustainedPerformanceModeType()
{
	int sdk = androidSDK();
	if(sdk >= 24)
	{
		return SustainedPerformanceType::DEVICE;
	}
	if(Config::MACHINE_IS_GENERIC_ARMV7 && sdk >= 16)
	{
		return SustainedPerformanceType::NOOP;
	}
	return SustainedPerformanceType::NONE;
}

void setSustainedPerformanceMode(bool on)
{
	switch(sustainedPerformanceModeType())
	{
		case SustainedPerformanceType::DEVICE:
		{
			logMsg("set sustained performance mode:%s", on ? "on" : "off");
			auto env = jEnvForThread();
			JavaInstMethod<void(jboolean)> jSetSustainedPerformanceMode{env, jBaseActivityCls, "setSustainedPerformanceMode", "(Z)V"};
			jSetSustainedPerformanceMode(env, jBaseActivity, on);
			return;
		}
		case SustainedPerformanceType::NOOP:
		{
			if(!Config::MACHINE_IS_GENERIC_ARMV7)
				return;
			if(on && appIsRunning())
			{
				if(noopThread)
					return;
				addOnExit(
					[](bool)
					{
						noopThread.stop();
						return false;
					}, -1000);
				noopThread.start();
			}
			else
			{
				noopThread.stop();
			}
			return;
		}
		default:
			return;
	}
}

static void setNativeActivityCallbacks(ANativeActivity* activity)
{
	activity->callbacks->onDestroy =
		[](ANativeActivity *)
		{
			removePostedNotifications();
			jVM = {}; // clear VM pointer to let Android framework detach main thread
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
			Screen::setActiveAll(true);
			if(Base::androidSDK() >= 11)
			{
				setRunningActivityState();
				dispatchOnResume(aHasFocus);
			}
		};
	activity->callbacks->onResume =
		[](ANativeActivity *activity)
		{
			logMsg("app resumed");
			if(Base::androidSDK() < 11)
			{
				setRunningActivityState();
				dispatchOnResume(aHasFocus);
			}
			handleIntent(activity->env, activity->clazz);
		};
	//activity->callbacks->onSaveInstanceState = nullptr; // unused
	activity->callbacks->onPause =
		[](ANativeActivity *activity)
		{
			if(Base::androidSDK() < 11)
			{
				setPausedActivityState();
				logMsg("app %s", appIsPaused() ? "paused" : "exiting");
				// App is killable in Android 2.3, run exit handler to save volatile data
				dispatchOnExit(appIsPaused());
			}
			else
				logMsg("app paused");
			Input::deinitKeyRepeatTimer();
		};
	activity->callbacks->onStop =
		[](ANativeActivity *activity)
		{
			if(Base::androidSDK() >= 11)
			{
				setPausedActivityState();
				logMsg("app %s", appIsPaused() ? "stopped" : "exiting");
				dispatchOnExit(appIsPaused());
			}
			else
				logMsg("app stopped");
			Screen::setActiveAll(false);
		};
	activity->callbacks->onConfigurationChanged =
		[](ANativeActivity *activity)
		{
			auto aConfig = AConfiguration_new();
			auto freeConfig = IG::scopeGuard([&](){ AConfiguration_delete(aConfig); });
			AConfiguration_fromAssetManager(aConfig, activity->assetManager);
			auto rotation = (SurfaceRotation)jMainDisplayRotation(activity->env, activity->clazz);
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
			dispatchOnFreeCaches(appIsRunning());
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
			if(unlikely(!deviceWindow()))
				return;
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
			if(unlikely(!deviceWindow()))
				return;
			deviceWindow()->setNativeWindow(nullptr);
		};
	// Note: Surface resizing handled by ContentView callback
	//activity->callbacks->onNativeWindowResized = nullptr;
	activity->callbacks->onNativeWindowRedrawNeeded =
		[](ANativeActivity *, ANativeWindow *)
		{
			if(unlikely(!deviceWindow()))
				return;
			androidWindowNeedsRedraw(*deviceWindow(), true);
		};
	activity->callbacks->onInputQueueCreated =
		[](ANativeActivity *, AInputQueue *queue)
		{
			inputQueue = queue;
			logMsg("made & attached input queue");
			AInputQueue_attachLooper(queue, EventLoop::forThread().nativeObject(), ALOOPER_POLL_CALLBACK,
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
	jVM = activity->vm;
	assetManager = activity->assetManager;
	jBaseActivity = activity->clazz;
	pthread_key_create(&jEnvKey,
		[](void *)
		{
			if(!jVM)
				return;
			if(Config::DEBUG_BUILD)
			{
				logDMsg("detaching JNI thread:0x%lx", IG::thisThreadID<long>());
			}
			jVM->DetachCurrentThread();
		});
	pthread_setspecific(jEnvKey, activity->env);
	filesDir = activity->internalDataPath;
	activityInit(activity->env, activity->clazz);
	setNativeActivityCallbacks(activity);
	Input::init(activity->env);
	{
		auto aConfig = AConfiguration_new();
		auto freeConfig = IG::scopeGuard([&](){ AConfiguration_delete(aConfig); });
		AConfiguration_fromAssetManager(aConfig, activity->assetManager);
		initConfig(aConfig);
	}
	onInit(0, nullptr);
	if(Config::DEBUG_BUILD && !Window::windows())
	{
		logWarn("didn't create a window");
	}
}
