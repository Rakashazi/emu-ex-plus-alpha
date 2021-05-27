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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Screen.hh>
#include <imagine/base/Timer.hh>
#include <imagine/input/Input.hh>
#include <imagine/fs/FS.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/util/utility.h>
#include <imagine/util/algorithm.h>
#include "android.hh"

namespace Base
{

static JavaVM* jVM{};
static void *mainLibHandle{};
static constexpr bool unloadNativeLibOnDestroy = false;
static NoopThread noopThread{};
static pid_t mainThreadId{};

static void setNativeActivityCallbacks(ANativeActivity *nActivity);

AndroidApplication::AndroidApplication(ApplicationInitParams initParams):
	BaseApplication{initParams.nActivity}
{
	ApplicationContext ctx{initParams.nActivity};
	auto env = ctx.mainThreadJniEnv();
	auto baseActivity = ctx.baseActivityObject();
	auto androidSDK = ctx.androidSDK();
	auto baseActivityClass = (jclass)env->GetObjectClass(baseActivity);
	if(Config::DEBUG_BUILD)
	{
		auto extPath = sharedStoragePath(env, baseActivityClass);
		logger_setLogDirectoryPrefix(extPath.data());
		logMsg("SDK API Level:%d", androidSDK);
		logMsg("internal storage path: %s", ctx.supportPath({}).data());
		logMsg("external storage path: %s", extPath.data());
	}
	initActivity(env, baseActivity, baseActivityClass, androidSDK);
	setNativeActivityCallbacks(initParams.nActivity);
	initChoreographer(screens(), env, baseActivity, baseActivityClass, androidSDK);
	initScreens(env, baseActivity, baseActivityClass, androidSDK, initParams.nActivity);
	initInput(env, baseActivity, baseActivityClass, androidSDK);
	{
		auto aConfig = AConfiguration_new();
		auto freeConfig = IG::scopeGuard([&](){ AConfiguration_delete(aConfig); });
		AConfiguration_fromAssetManager(aConfig, ctx.aAssetManager());
		initInputConfig(aConfig);
	}
}

void AndroidApplicationContext::setApplicationPtr(Application *appPtr)
{
	act->instance = appPtr;
}

Application &AndroidApplicationContext::application() const
{
	assert(act->instance);
	return *static_cast<Application*>(act->instance);
}

IG::PixelFormat makePixelFormatFromAndroidFormat(int32_t androidFormat)
{
	switch(androidFormat)
	{
		case AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM:
		case AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM: return PIXEL_FMT_RGBA8888;
		case AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM: return PIXEL_FMT_RGB565;
		case ANDROID_BITMAP_FORMAT_RGBA_4444: return PIXEL_FMT_RGBA4444;
		case ANDROID_BITMAP_FORMAT_A_8: return PIXEL_FMT_I8;
		default:
		{
			if(androidFormat == ANDROID_BITMAP_FORMAT_NONE)
				return {};
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
	if(res != ANDROID_BITMAP_RESULT_SUCCESS) [[unlikely]]
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

void AndroidApplication::recycleBitmap(JNIEnv *env, jobject bitmap)
{
	jRecycle(env, bitmap);
}

void ApplicationContext::exit(int returnVal)
{
	// TODO: return exit value as activity result
	application().setExitingActivityState();
	auto env = thisThreadJniEnv();
	auto baseActivity = baseActivityObject();
	JNI::InstMethod<void()> jFinish{env, baseActivity, "finish", "()V"};
	jFinish(env, baseActivity);
}

FS::PathString ApplicationContext::assetPath(const char *) const { return {}; }

FS::PathString ApplicationContext::supportPath(const char *) const
{
	if(androidSDK() < 11) // bug in pre-3.0 Android causes paths in ANativeActivity to be null
	{
		//logMsg("ignoring paths from ANativeActivity due to Android 2.3 bug");
		auto env = thisThreadJniEnv();
		auto baseActivity = baseActivityObject();
		JNI::InstMethod<jobject()> filesDir{env, baseActivity, "filesDir", "()Ljava/lang/String;"};
		return JNI::stringCopy<FS::PathString>(env, (jstring)filesDir(env, baseActivity));
	}
	else
		return FS::makePathString(act->internalDataPath);
}

FS::PathString ApplicationContext::cachePath(const char *) const
{
	auto env = thisThreadJniEnv();
	auto baseActivityCls = (jclass)env->GetObjectClass(baseActivityObject());
	JNI::ClassMethod<jobject()> cacheDir{env, baseActivityCls, "cacheDir", "()Ljava/lang/String;"};
	return JNI::stringCopy<FS::PathString>(env, (jstring)cacheDir(env, baseActivityCls));
}

FS::PathString ApplicationContext::sharedStoragePath() const
{
	auto env = thisThreadJniEnv();
	return application().sharedStoragePath(env, env->GetObjectClass(baseActivityObject()));
}

FS::PathString AndroidApplication::sharedStoragePath(JNIEnv *env, jclass baseActivityClass) const
{
	JNI::ClassMethod<jobject()> extStorageDir{env, baseActivityClass, "extStorageDir", "()Ljava/lang/String;"};
	return JNI::stringCopy<FS::PathString>(env, (jstring)extStorageDir(env, baseActivityClass));
}

FS::PathLocation ApplicationContext::sharedStoragePathLocation() const
{
	auto path = sharedStoragePath();
	return {path, FS::makeFileString("Storage Media"), {FS::makeFileString("Media"), strlen(path.data())}};
}

std::vector<FS::PathLocation> ApplicationContext::rootFileLocations() const
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
		auto env = thisThreadJniEnv();
		auto baseActivity = baseActivityObject();
		JNI::InstMethod<jobject()> jNewStorageManagerHelper{env, baseActivity, "storageManagerHelper", "()Lcom/imagine/StorageManagerHelper;"};
		auto storageManagerHelper = jNewStorageManagerHelper(env, baseActivity);
		auto storageManagerHelperCls = env->GetObjectClass(storageManagerHelper);
		JNINativeMethod method[]
		{
			{
				"volumeEnumerated", "(JLjava/lang/String;Ljava/lang/String;)V",
				(void*)(void (*)(JNIEnv*, jobject, jlong, jstring, jstring))
				([](JNIEnv* env, jobject thiz, jlong userData, jstring jName, jstring jPath)
				{
					auto rootLocation = (std::vector<FS::PathLocation>*)userData;
					auto path = JNI::stringCopy<FS::PathString>(env, jPath);
					auto name = JNI::stringCopy<FS::FileString>(env, jName);
					logMsg("volume:%s with path:%s", name.data(), path.data());
					rootLocation->emplace_back(path, name, FS::RootPathInfo{name, strlen(path.data())});
				})
			},
		};
		env->RegisterNatives(storageManagerHelperCls, method, std::size(method));
		JNI::ClassMethod<void(jobject, jlong)> jEnumVolumes{env, storageManagerHelperCls, "enumVolumes", "(Landroid/app/Activity;J)V"};
		jEnumVolumes(env, storageManagerHelperCls, baseActivityObject(), (jlong)&rootLocation);
		return rootLocation;
	}
}

FS::PathString ApplicationContext::libPath(const char *) const
{
	if(androidSDK() < 24)
	{
		auto env = thisThreadJniEnv();
		auto baseActivity = baseActivityObject();
		JNI::InstMethod<jobject()> libDir{env, baseActivity, "libDir", "()Ljava/lang/String;"};
		return JNI::stringCopy<FS::PathString>(env, (jstring)libDir(env, baseActivity));
	}
	return {};
}

static FS::PathString mainSOPath(ApplicationContext ctx)
{
	if(ctx.androidSDK() < 24)
	{
		return FS::makePathStringPrintf("%s/libmain.so", ctx.libPath(nullptr).data());
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

bool ApplicationContext::usesPermission(Permission p) const
{
	if(androidSDK() < 23)
		return false;
	return true;
}

bool ApplicationContext::requestPermission(Permission p)
{
	if(androidSDK() < 23)
		return false;
	auto env = mainThreadJniEnv();
	auto permissionJStr = permissionToJString(env, p);
	if(!permissionJStr)
		return false;
	auto baseActivity = baseActivityObject();
	JNI::InstMethod<jboolean(jobject)> requestPermission{env, baseActivity, "requestPermission", "(Ljava/lang/String;)Z"};
	return requestPermission(env, baseActivity, permissionJStr);
}

JNIEnv *AndroidApplicationContext::mainThreadJniEnv() const
{
	assert(mainThreadId == gettid());
	return act->env;
}

jobject AndroidApplicationContext::baseActivityObject() const
{
	return act->clazz;
}

AAssetManager *AndroidApplicationContext::aAssetManager() const
{
	return act->assetManager;
}

bool AndroidApplication::hasHardwareNavButtons() const
{
	return hasPermanentMenuKey;
}

bool ApplicationContext::hasHardwareNavButtons() const
{
	return application().hasHardwareNavButtons();
}

int32_t AndroidApplicationContext::androidSDK() const
{
	#ifdef ANDROID_COMPAT_API
	static_assert(__ANDROID_API__ == 14, "Compiling with ANDROID_COMPAT_API and API higher than 14");
	return std::max(9, act->sdkVersion);
	#else
	return std::max(__ANDROID_API__, act->sdkVersion);
	#endif
}

void AndroidApplication::setOnSystemOrientationChanged(SystemOrientationChangedDelegate del)
{
	onSystemOrientationChanged = del;
}

bool AndroidApplication::systemAnimatesWindowRotation() const
{
	return osAnimatesRotation;
}

void ApplicationContext::setOnSystemOrientationChanged(SystemOrientationChangedDelegate del)
{
	application().setOnSystemOrientationChanged(del);
}

bool ApplicationContext::systemAnimatesWindowRotation() const
{
	return application().systemAnimatesWindowRotation();
}

void AndroidApplication::setRequestedOrientation(JNIEnv *env, jobject baseActivity, int orientation)
{
	jSetRequestedOrientation(env, baseActivity, orientation);
}

SurfaceRotation AndroidApplication::currentRotation() const
{
	return osRotation;
}

void AndroidApplication::setCurrentRotation(ApplicationContext ctx, SurfaceRotation rotation, bool notify)
{
	auto oldRotation = std::exchange(osRotation, rotation);
	if(notify && onSystemOrientationChanged)
		onSystemOrientationChanged(ctx, oldRotation, rotation);
}

SurfaceRotation AndroidApplication::mainDisplayRotation(JNIEnv *env, jobject baseActivity) const
{
	return (SurfaceRotation)jMainDisplayRotation(env, baseActivity);
}

jobject AndroidApplication::makeFontRenderer(JNIEnv *env, jobject baseActivity)
{
	return jNewFontRenderer(env, baseActivity);
}

uint32_t toAHardwareBufferFormat(IG::PixelFormatID format)
{
	using namespace IG;
	switch(format)
	{
		case PIXEL_RGBA8888: return AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
		case PIXEL_RGB565: return AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM;
		default: return 0;
	}
}

const char *aHardwareBufferFormatStr(uint32_t format)
{
	switch(format)
	{
		case 0: return "None";
		case AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM: return "RGBA8888";
		case AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM: return "RGBX8888";
		case AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM: return "RGB888";
		case AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM: return "RGB565";
	}
	return "Unknown";
}

void AndroidApplication::initActivity(JNIEnv *env, jobject baseActivity, jclass baseActivityClass, int32_t androidSDK)
{
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
	pthread_setspecific(jEnvKey, env);

	// BaseActivity JNI functions
	jSetRequestedOrientation = {env, baseActivityClass, "setRequestedOrientation", "(I)V"};
	jMainDisplayRotation = {env, baseActivityClass, "mainDisplayRotation", "()I"};
	#ifdef CONFIG_RESOURCE_FONT_ANDROID
	jNewFontRenderer = {env, baseActivityClass, "newFontRenderer", "()Lcom/imagine/FontRenderer;"};
	#endif
	{
		JNINativeMethod method[]
		{
			{
				"onContentRectChanged", "(JIIIIII)V",
				(void*)
				+[](JNIEnv* env, jobject thiz, jlong windowAddr, jint x, jint y, jint x2, jint y2, jint winWidth, jint winHeight)
				{
					assumeExpr(windowAddr);
					auto win = (Window*)windowAddr;
					win->setContentRect({{x, y}, {x2, y2}}, {winWidth, winHeight});
				}
			},
			{
				"displayEnumerated", "(JLandroid/view/Display;IFILandroid/util/DisplayMetrics;)V",
				(void*)
				+[](JNIEnv* env, jobject, jlong nActivityAddr, jobject disp, jint id, jfloat refreshRate, jint rotation, jobject metrics)
				{
					ApplicationContext ctx{(ANativeActivity*)nActivityAddr};
					auto &app = ctx.application();
					auto screen = app.findScreen(id);
					if(!screen)
					{
						app.addScreen(ctx, std::make_unique<Screen>(ctx,
							Screen::InitParams{env, disp, metrics, id, refreshRate, (SurfaceRotation)rotation}), false);
						return;
					}
					else
					{
						// already in list, update existing
						screen->updateRefreshRate(refreshRate);
					}
				}
			},
			{
				"inputDeviceEnumerated", "(JILandroid/view/InputDevice;Ljava/lang/String;IIIZ)V",
				(void*)
				+[](JNIEnv* env, jobject, jlong nUserData, jint devID, jobject jDev, jstring jName, jint src, jint kbType, jint jsAxisBits, jboolean isPowerButton)
				{
					auto &app = *((AndroidApplication*)nUserData);
					const char *name = env->GetStringUTFChars(jName, nullptr);
					Input::AndroidInputDevice sysDev{env, jDev, app.nextInputDeviceEnumId(name, devID), devID, src,
						name, kbType, (uint32_t)jsAxisBits, (bool)isPowerButton};
					env->ReleaseStringUTFChars(jName, name);
					auto devPtr = app.addInputDevice(sysDev, false, false);
					// check for special device IDs
					if(devID == -1)
					{
						app.virtualDev = devPtr;
					}
					else if(devID == 0)
					{
						// built-in keyboard is always id 0 according to Android docs
						app.builtinKeyboardDev = devPtr;
					}
				}
			},
			{
				"documentTreeOpened", "(JLjava/lang/String;)V",
				(void*)
				+[](JNIEnv* env, jobject, jlong nUserData, jstring jPath)
				{
					auto &app = *((AndroidApplication*)nUserData);
					auto path = env->GetStringUTFChars(jPath, nullptr);
					app.onSystemPathPicker(path);
					env->ReleaseStringUTFChars(jPath, path);
				}
			},
		};
		env->RegisterNatives(baseActivityClass, method, std::size(method));
	}

	if(androidSDK >= 11)
		osAnimatesRotation = true;
	else
	{
		JNI::ClassMethod<jboolean()> jAnimatesRotation{env, baseActivityClass, "gbAnimatesRotation", "()Z"};
		osAnimatesRotation = jAnimatesRotation(env, baseActivityClass);
	}
	if(!osAnimatesRotation)
	{
		logMsg("app handles rotation animations");
	}

	if(androidSDK >= 14)
	{
		JNI::InstMethod<jboolean()> jHasPermanentMenuKey{env, baseActivityClass, "hasPermanentMenuKey", "()Z"};
		hasPermanentMenuKey = jHasPermanentMenuKey(env, baseActivity);
		if(hasPermanentMenuKey)
		{
			logMsg("device has hardware nav/menu keys");
		}
	}
	else
		hasPermanentMenuKey = 1;

	/*if(unloadNativeLibOnDestroy)
	{
		auto soPath = mainSOPath(appCtx);
		#if __ANDROID_API__ >= 21
		mainLibHandle = dlopen(soPath.data(), RTLD_LAZY | RTLD_NOLOAD);
		#else
		mainLibHandle = dlopen(soPath.data(), RTLD_LAZY);
		dlclose(mainLibHandle);
		#endif
		if(!mainLibHandle)
			logWarn("unable to get native lib handle");
	}*/

	jSetWinFlags = {env, baseActivityClass, "setWinFlags", "(II)V"};
	jWinFlags = {env, baseActivityClass, "winFlags", "()I"};

	if(androidSDK >= 11)
	{
		jSetUIVisibility = {env, baseActivityClass, "setUIVisibility", "(I)V"};
	}

	jRecycle = {env, env->FindClass("android/graphics/Bitmap"), "recycle", "()V"};
}

JNIEnv* AndroidApplication::thisThreadJniEnv() const
{
	auto env = (JNIEnv*)pthread_getspecific(jEnvKey);
	if(!env) [[unlikely]]
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
	return env;
}

JNIEnv* AndroidApplicationContext::thisThreadJniEnv() const
{
	return application().thisThreadJniEnv();
}

void AndroidApplication::setIdleDisplayPowerSave(JNIEnv *env, jobject baseActivity, bool on)
{
	jint keepOn = !on;
	auto keepsScreenOn = userActivityCallback.isArmed() ?
		false : (bool)(jWinFlags(env, baseActivity) & AWINDOW_FLAG_KEEP_SCREEN_ON);
	if(keepOn != keepsScreenOn)
	{
		logMsg("keep screen on: %d", keepOn);
		jSetWinFlags(env, baseActivity, keepOn ? AWINDOW_FLAG_KEEP_SCREEN_ON : 0, AWINDOW_FLAG_KEEP_SCREEN_ON);
	}
	keepScreenOn = keepOn; // cache value for endIdleByUserActivity()
}

void ApplicationContext::setIdleDisplayPowerSave(bool on)
{
	application().setIdleDisplayPowerSave(mainThreadJniEnv(), baseActivityObject(), on);
}

void AndroidApplication::endIdleByUserActivity(ApplicationContext ctx)
{
	if(!keepScreenOn)
	{
		//logMsg("signaling user activity to the OS");
		// quickly toggle KEEP_SCREEN_ON flag to brighten screen,
		// waiting about 20ms before toggling it back off triggers the screen to brighten if it was already dim
		jSetWinFlags(ctx.mainThreadJniEnv(), ctx.baseActivityObject(), AWINDOW_FLAG_KEEP_SCREEN_ON, AWINDOW_FLAG_KEEP_SCREEN_ON);
		userActivityCallback.runIn(IG::Milliseconds(20), {},
			[this, ctx]()
			{
				if(!keepScreenOn)
				{
					jSetWinFlags(ctx.mainThreadJniEnv(), ctx.baseActivityObject(), 0, AWINDOW_FLAG_KEEP_SCREEN_ON);
				}
			});
	}
}

void ApplicationContext::endIdleByUserActivity()
{
	application().endIdleByUserActivity(*this);
}

void AndroidApplication::setStatusBarHidden(JNIEnv *env, jobject baseActivity, bool hidden)
{
	auto statusBarIsHidden = (bool)(jWinFlags(env, baseActivity) & AWINDOW_FLAG_FULLSCREEN);
	if(hidden != statusBarIsHidden)
	{
		logMsg("setting app window fullscreen: %u", hidden);
		jSetWinFlags(env, baseActivity, hidden ? AWINDOW_FLAG_FULLSCREEN : 0, AWINDOW_FLAG_FULLSCREEN);
	}
}

void AndroidApplication::setSysUIStyle(JNIEnv *env, jobject baseActivity, int32_t androidSDK, uint32_t flags)
{
	// Flags mapped directly to SYSTEM_UI_FLAG_*
	// SYS_UI_STYLE_DIM_NAV -> SYSTEM_UI_FLAG_LOW_PROFILE (0)
	// SYS_UI_STYLE_HIDE_NAV -> SYSTEM_UI_FLAG_HIDE_NAVIGATION (1)
	// SYS_UI_STYLE_HIDE_STATUS -> SYSTEM_UI_FLAG_FULLSCREEN (2)
	// SYS_UI_STYLE_NO_FLAGS -> SYSTEM_UI_FLAG_VISIBLE (no bits set)

	// always handle status bar hiding via full-screen window flag
	// even on SDK level >= 11 so our custom view has the correct insets
	setStatusBarHidden(env, baseActivity, flags & SYS_UI_STYLE_HIDE_STATUS);
	if(androidSDK >= 11)
	{
		constexpr uint32_t SYSTEM_UI_FLAG_IMMERSIVE_STICKY = 0x1000;
		// Add SYSTEM_UI_FLAG_IMMERSIVE_STICKY for use with Android 4.4+ if flags contain OS_NAV_STYLE_HIDDEN
		if(flags & SYS_UI_STYLE_HIDE_NAV)
			flags |= SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
		logMsg("setting UI visibility: 0x%X", flags);
		uiVisibilityFlags = flags;
		jSetUIVisibility(env, baseActivity, flags);
	}
}

void ApplicationContext::setSysUIStyle(uint32_t flags)
{
	return application().setSysUIStyle(mainThreadJniEnv(), baseActivityObject(), androidSDK(), flags);
}

bool ApplicationContext::hasTranslucentSysUI() const
{
	return androidSDK() >= 19;
}

bool AndroidApplication::hasFocus() const
{
	return aHasFocus;
}

SustainedPerformanceType AndroidApplicationContext::sustainedPerformanceModeType() const
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

void AndroidApplicationContext::setSustainedPerformanceMode(bool on)
{
	switch(sustainedPerformanceModeType())
	{
		case SustainedPerformanceType::DEVICE:
		{
			logMsg("set sustained performance mode:%s", on ? "on" : "off");
			auto env = mainThreadJniEnv();
			auto baseActivity = baseActivityObject();
			JNI::InstMethod<void(jboolean)> jSetSustainedPerformanceMode{env, baseActivity, "setSustainedPerformanceMode", "(Z)V"};
			jSetSustainedPerformanceMode(env, baseActivity, on);
			return;
		}
		case SustainedPerformanceType::NOOP:
		{
			if(!Config::MACHINE_IS_GENERIC_ARMV7)
				return;
			auto &ctx = *static_cast<ApplicationContext*>(this);
			if(on && ctx.isRunning())
			{
				if(noopThread)
					return;
				ctx.addOnExit(
					[](ApplicationContext, bool)
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

Window *AndroidApplication::deviceWindow() const
{
	if(windows().size()) [[likely]]
		return windows()[0].get();
	return nullptr;
}

void AndroidApplication::onWindowFocusChanged(ApplicationContext ctx, int focused)
{
	aHasFocus = focused;
	logMsg("focus change:%d", focused);
	if(focused && ctx.androidSDK() >= 11)
	{
		// re-apply UI visibility flags
		jSetUIVisibility(ctx.mainThreadJniEnv(), ctx.baseActivityObject(), uiVisibilityFlags);
	}
	for(auto &w : windows())
	{
		w->dispatchFocusChange(focused);
	}
	if(!focused)
		deinitKeyRepeatTimer();
}

void AndroidApplication::onInputQueueCreated(ApplicationContext ctx, AInputQueue *queue)
{
	assert(!inputQueue);
	inputQueue = queue;
	logMsg("made & attached input queue");
	AInputQueue_attachLooper(queue, EventLoop::forThread().nativeObject(), ALOOPER_POLL_CALLBACK,
		[](int, int, void *data)
		{
			auto &app = *((AndroidApplication*)data);
			app.processInput(app.inputQueue);
			return 1;
		}, this);
}

void AndroidApplication::onInputQueueDestroyed(AInputQueue *queue)
{
	logMsg("input queue destroyed");
	inputQueue = {};
	AInputQueue_detachLooper(queue);
}

static void setNativeActivityCallbacks(ANativeActivity *nActivity)
{
	nActivity->callbacks->onDestroy =
		[](ANativeActivity *nActivity)
		{
			ApplicationContext ctx{nActivity};
			ctx.application().removePostedNotifications(ctx.mainThreadJniEnv(), ctx.baseActivityObject());
			jVM = {}; // clear VM pointer to let Android framework detach main thread
			if(mainLibHandle)
			{
				logMsg("closing main lib:%p", mainLibHandle);
				dlclose(mainLibHandle);
			}
			else
			{
				logMsg("exiting process");
				delete static_cast<BaseApplication*>(nActivity->instance);
				::exit(0);
			}
		};
	nActivity->callbacks->onStart =
		[](ANativeActivity *nActivity)
		{
			ApplicationContext ctx{nActivity};
			auto &app = ctx.application();
			logMsg("app started");
			app.setActiveForAllScreens(true);
			if(ctx.androidSDK() >= 11)
			{
				app.setRunningActivityState();
				app.dispatchOnResume(ctx, app.hasFocus());
			}
		};
	nActivity->callbacks->onResume =
		[](ANativeActivity *nActivity)
		{
			ApplicationContext ctx{nActivity};
			auto &app = ctx.application();
			logMsg("app resumed");
			if(ctx.androidSDK() < 11)
			{
				app.setRunningActivityState();
				app.dispatchOnResume(ctx, app.hasFocus());
			}
			app.handleIntent(ctx);
		};
	//nActivity->callbacks->onSaveInstanceState = nullptr; // unused
	nActivity->callbacks->onPause =
		[](ANativeActivity *nActivity)
		{
			ApplicationContext ctx{nActivity};
			auto &app = ctx.application();
			if(ctx.androidSDK() < 11)
			{
				app.setPausedActivityState();
				logMsg("app %s", app.isPaused() ? "paused" : "exiting");
				// App is killable in Android 2.3, run exit handler to save volatile data
				app.dispatchOnExit(ctx, app.isPaused());
			}
			else
				logMsg("app paused");
			app.deinitKeyRepeatTimer();
		};
	nActivity->callbacks->onStop =
		[](ANativeActivity *nActivity)
		{
			ApplicationContext ctx{nActivity};
			auto &app = ctx.application();
			if(ctx.androidSDK() >= 11)
			{
				app.setPausedActivityState();
				logMsg("app %s", app.isPaused() ? "stopped" : "exiting");
				app.dispatchOnExit(ctx, app.isPaused());
			}
			else
				logMsg("app stopped");
			app.setActiveForAllScreens(false);
		};
	nActivity->callbacks->onConfigurationChanged =
		[](ANativeActivity *nActivity)
		{
			ApplicationContext ctx{nActivity};
			auto &app = ctx.application();
			auto aConfig = AConfiguration_new();
			auto freeConfig = IG::scopeGuard([&](){ AConfiguration_delete(aConfig); });
			AConfiguration_fromAssetManager(aConfig, nActivity->assetManager);
			auto rotation = app.mainDisplayRotation(nActivity->env, nActivity->clazz);
			if(rotation != app.currentRotation())
			{
				logMsg("changed OS orientation");
				app.setCurrentRotation(ctx, rotation, true);
			}
			app.updateInputConfig(aConfig);
		};
	nActivity->callbacks->onLowMemory =
		[](ANativeActivity *nActivity)
		{
			ApplicationContext ctx{nActivity};
			ctx.dispatchOnFreeCaches(ctx.isRunning());
		};
	nActivity->callbacks->onWindowFocusChanged =
		[](ANativeActivity *nActivity, int focused)
		{
			ApplicationContext ctx{nActivity};
			ctx.application().onWindowFocusChanged(ctx, focused);
		};
	nActivity->callbacks->onNativeWindowCreated =
		[](ANativeActivity *nActivity, ANativeWindow *nWin)
		{
			ApplicationContext ctx{nActivity};
			auto deviceWindow = ctx.application().deviceWindow();
			if(!deviceWindow) [[unlikely]]
				return;
			deviceWindow->setNativeWindow(ctx, nWin);
		};
	nActivity->callbacks->onNativeWindowDestroyed =
		[](ANativeActivity *nActivity, ANativeWindow *)
		{
			ApplicationContext ctx{nActivity};
			auto deviceWindow = ctx.application().deviceWindow();
			if(!deviceWindow) [[unlikely]]
				return;
			deviceWindow->setNativeWindow(nActivity, nullptr);
		};
	// Note: Surface resizing handled by ContentView callback
	//nActivity->callbacks->onNativeWindowResized = nullptr;
	nActivity->callbacks->onNativeWindowRedrawNeeded =
		[](ANativeActivity *nActivity, ANativeWindow *)
		{
			ApplicationContext ctx{nActivity};
			auto deviceWindow = ctx.application().deviceWindow();
			if(!deviceWindow) [[unlikely]]
				return;
			deviceWindow->systemRequestsRedraw(true);
		};
	nActivity->callbacks->onInputQueueCreated =
		[](ANativeActivity *nActivity, AInputQueue *queue)
		{
			ApplicationContext ctx{nActivity};
			ctx.application().onInputQueueCreated(ctx, queue);
		};
	nActivity->callbacks->onInputQueueDestroyed =
		[](ANativeActivity *nActivity, AInputQueue *queue)
		{
			ApplicationContext ctx{nActivity};
			ctx.application().onInputQueueDestroyed(queue);
		};
	// Note: Content rectangle handled by ContentView callback
	// for correct handling of system window insets
	//nActivity->callbacks->onContentRectChanged = nullptr;
}

}

CLINK void LVISIBLE ANativeActivity_onCreate(ANativeActivity *nActivity, void* savedState, size_t savedStateSize)
{
	using namespace Base;
	if(Config::DEBUG_BUILD)
	{
		mainThreadId = gettid();
		logMsg("called ANativeActivity_onCreate, thread ID %d", mainThreadId);
	}
	jVM = nActivity->vm;
	ApplicationInitParams initParams{nActivity};
	ApplicationContext ctx{nActivity};
	ctx.onInit(initParams);
	if(Config::DEBUG_BUILD)
	{
		if(!ctx.windows().size())
			logWarn("didn't create a window");
	}
}
