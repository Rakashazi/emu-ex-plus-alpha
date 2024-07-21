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

#include <cstdlib>
#include <android/window.h>
#include <android/configuration.h>
#include <android/looper.h>
#include <android/native_activity.h>
#include <android/api-level.h>
#include <android/bitmap.h>
#include <dlfcn.h>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Screen.hh>
#include <imagine/base/Timer.hh>
#include <imagine/fs/FS.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/utility.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/format.hh>
#include "android.hh"
#include <imagine/base/android/AndroidInputDevice.hh>
#include <imagine/logger/logger.h>

namespace IG
{

constexpr SystemLogger log{"App"};
static JavaVM* jVM{};
static void *mainLibHandle{};
[[maybe_unused]] constexpr bool unloadNativeLibOnDestroy{};
pid_t mainThreadId{};
pthread_key_t jEnvPThreadKey{};

static void setNativeActivityCallbacks(ANativeActivity *nActivity);

AndroidApplication::AndroidApplication(ApplicationInitParams initParams):
	BaseApplication{({initParams.nActivity->instance = this; initParams.nActivity;})},
	userActivityCallback
	{
		{.debugLabel = "userActivityCallback"},
		[this, ctx = ApplicationContext{initParams.nActivity}]
		{
			if(!keepScreenOn)
			{
				jSetWinFlags(ctx.mainThreadJniEnv(), ctx.baseActivityObject(), 0, AWINDOW_FLAG_KEEP_SCREEN_ON);
			}
		}
	}
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
		log.info("SDK API Level:{}", androidSDK);
		log.info("internal storage path:{}", ctx.supportPath({}));
		log.info("external storage path:{}", extPath);
	}
	initActivity(env, baseActivity, baseActivityClass, androidSDK);
	setNativeActivityCallbacks(initParams.nActivity);
	initChoreographer(env, baseActivity, baseActivityClass, androidSDK);
	initScreens(env, baseActivity, baseActivityClass, androidSDK, initParams.nActivity);
	initInput(ctx, env, baseActivity, baseActivityClass, androidSDK);
	{
		auto aConfig = AConfiguration_new();
		auto freeConfig = scopeGuard([&](){ AConfiguration_delete(aConfig); });
		AConfiguration_fromAssetManager(aConfig, ctx.aAssetManager());
		initInputConfig(aConfig);
	}
}

PixelFormat makePixelFormatFromAndroidFormat(int32_t androidFormat)
{
	switch(androidFormat)
	{
		case AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM:
		case AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM: return PixelFmtRGBA8888;
		case AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM: return PixelFmtRGB565;
		case ANDROID_BITMAP_FORMAT_RGBA_4444: return PixelFmtRGBA4444;
		case ANDROID_BITMAP_FORMAT_A_8: return PixelFmtI8;
		default:
		{
			log.error("unhandled format");
			return PixelFmtI8;
		}
	}
}

MutablePixmapView makePixmapView(JNIEnv *env, jobject bitmap, void *pixels, PixelFormat format)
{
	AndroidBitmapInfo info;
	auto res = AndroidBitmap_getInfo(env, bitmap, &info);
	if(res != ANDROID_BITMAP_RESULT_SUCCESS) [[unlikely]]
	{
		log.info("error getting bitmap info");
		return {};
	}
	//log.info("android bitmap info:size {}x{}, stride:{}", info.width, info.height, info.stride);
	if(format == PixelFmtUnset)
	{
		// use format from bitmap info
		format = makePixelFormatFromAndroidFormat(info.format);
	}
	return {{{(int)info.width, (int)info.height}, format}, pixels, {(int)info.stride, MutablePixmapView::Units::BYTE}};
}

void ApplicationContext::exit(int)
{
	// TODO: return exit value as activity result
	application().setExitingActivityState();
	auto env = thisThreadJniEnv();
	auto baseActivity = baseActivityObject();
	JNI::InstMethod<void()> jFinish{env, baseActivity, "finish", "()V"};
	jFinish(env, baseActivity);
}

FS::PathString AndroidApplication::sharedStoragePath(JNIEnv *env, jclass baseActivityClass) const
{
	JNI::ClassMethod<jstring()> extStorageDir{env, baseActivityClass, "extStorageDir", "()Ljava/lang/String;"};
	return FS::PathString{JNI::StringChars{env, extStorageDir(env, baseActivityClass)}};
}

FS::PathString AndroidApplication::externalMediaPath(JNIEnv *env, jobject baseActivity) const
{
	JNI::InstMethod<jstring()> extMediaDir{env, baseActivity, "extMediaDir", "()Ljava/lang/String;"};
	return FS::PathString{JNI::StringChars{env, extMediaDir(env, baseActivity)}};
}

UniqueFileDescriptor AndroidApplication::openFileUriFd(JNIEnv *env, jobject baseActivity, CStringView uri, OpenFlags openFlags) const
{
	int fd = openUriFd(env, baseActivity, env->NewStringUTF(uri), jint(std::bit_cast<OpenFlags::BitSetClassInt>(openFlags)));
	if(fd == -1)
	{
		if constexpr(Config::DEBUG_BUILD)
			log.error("error opening URI:{}", uri);
		if(openFlags.test)
			return -1;
		else
			throw std::system_error{ENOENT, std::generic_category(), uri};
	}
	log.info("opened fd:{} file URI:{}", fd, uri);
	return fd;
}

bool AndroidApplication::fileUriExists(JNIEnv *env, jobject baseActivity, CStringView uri) const
{
	bool exists = uriExists(env, baseActivity, env->NewStringUTF(uri));
	log.info("URI {}:{}", exists ? "exists" : "doesn't exist", uri);
	return exists;
}

WallClockTimePoint AndroidApplication::fileUriLastWriteTime(JNIEnv *env, jobject baseActivity, CStringView uri) const
{
	return WallClockTimePoint{Milliseconds{uriLastModifiedTime(env, baseActivity, env->NewStringUTF(uri))}};
}

std::string AndroidApplication::fileUriFormatLastWriteTimeLocal(JNIEnv *env, jobject baseActivity, CStringView uri) const
{
	//log.info("getting modification time for URI:{}", uri);
	return std::string{JNI::StringChars{env, uriLastModified(env, baseActivity, env->NewStringUTF(uri))}};
}

FS::FileString AndroidApplication::fileUriDisplayName(JNIEnv *env, jobject baseActivity, CStringView uri) const
{
	//log.info("getting display name for URI:{}", uri);
	return FS::FileString{JNI::StringChars{env, uriDisplayName(env, baseActivity, env->NewStringUTF(uri))}};
}

FS::file_type AndroidApplication::fileUriType(JNIEnv* env, jobject baseActivity, CStringView uri) const
{
	return FS::file_type(uriFileType(env, baseActivity, env->NewStringUTF(uri)));
}


bool AndroidApplication::removeFileUri(JNIEnv *env, jobject baseActivity, CStringView uri, bool isDir) const
{
	log.info("removing {} URI:{}", isDir ? "directory" : "file", uri);
	return deleteUri(env, baseActivity, env->NewStringUTF(uri), isDir);
}

bool AndroidApplication::renameFileUri(JNIEnv *env, jobject baseActivity, CStringView oldUri, CStringView newUri) const
{
	log.info("renaming file URI:{} -> {}", oldUri, newUri);
	return renameUri(env, baseActivity, env->NewStringUTF(oldUri), env->NewStringUTF(newUri));
}

bool AndroidApplication::createDirectoryUri(JNIEnv *env, jobject baseActivity, CStringView uri) const
{
	log.info("creating directory URI:{}", uri);
	return createDirUri(env, baseActivity, env->NewStringUTF(uri));
}

bool AndroidApplication::forEachInDirectoryUri(JNIEnv *env, jobject baseActivity,
	CStringView uri, DirectoryEntryDelegate del, FS::DirOpenFlags flags) const
{
	log.info("listing directory URI:{}", uri);
	if(!listUriFiles(env, baseActivity, (jlong)&del, env->NewStringUTF(uri)))
	{
		if(flags.test)
			return false;
		else
			throw std::system_error{ENOENT, std::generic_category(), uri};
	}
	return true;
}

inline FS::PathString mainSOPath(ApplicationContext ctx)
{
	if(ctx.androidSDK() < 24)
	{
		return FS::pathString(ctx.libPath(nullptr), "libmain.so");
	}
	return "libmain.so";
}

std::string AndroidApplication::formatDateAndTime(JNIEnv *env, jclass baseActivityClass, WallClockTimePoint time)
{
	if(!hasTime(time))
		return {};
	return std::string{JNI::StringChars{env, jFormatDateTime(env, baseActivityClass,
		duration_cast<Milliseconds>(time.time_since_epoch()).count())}};
}

std::string ApplicationContext::formatDateAndTime(WallClockTimePoint time)
{
	auto env = thisThreadJniEnv();
	return application().formatDateAndTime(env,
		(jclass)env->GetObjectClass(baseActivityObject()), time);
}

void AndroidApplication::setOnSystemOrientationChanged(SystemOrientationChangedDelegate del)
{
	onSystemOrientationChanged = del;
}

bool AndroidApplication::systemAnimatesWindowRotation() const
{
	if(Config::MACHINE_IS_GENERIC_ARMV7)
		return !deviceFlags.handleRotationAnimation;
	else
		return true;
}

void AndroidApplication::setRequestedOrientation(JNIEnv *env, jobject baseActivity, int orientation)
{
	jSetRequestedOrientation(env, baseActivity, orientation);
}

Rotation AndroidApplication::currentRotation() const
{
	return osRotation;
}

void AndroidApplication::setCurrentRotation(ApplicationContext ctx, Rotation rotation, bool notify)
{
	auto oldRotation = std::exchange(osRotation, rotation);
	if(notify && onSystemOrientationChanged)
		onSystemOrientationChanged(ctx, oldRotation, rotation);
}

Rotation AndroidApplication::mainDisplayRotation(JNIEnv *env, jobject baseActivity) const
{
	// verify Surface.ROTATION_* maps to Rotation
	static_assert(to_underlying(Rotation::UP) == 0);
	static_assert(to_underlying(Rotation::RIGHT) == 1);
	static_assert(to_underlying(Rotation::DOWN) == 2);
	static_assert(to_underlying(Rotation::LEFT) == 3);
	return (Rotation)jMainDisplayRotation(env, baseActivity);
}

jobject AndroidApplication::makeFontRenderer(JNIEnv *env, jobject baseActivity)
{
	return jNewFontRenderer(env, baseActivity);
}

uint32_t toAHardwareBufferFormat(PixelFormatId format)
{
	switch(format)
	{
		case PixelFormatId::RGBA8888: return AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
		case PixelFormatId::RGB565: return AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM;
		default: return 0;
	}
}

const char *aHardwareBufferFormatStr(uint32_t format)
{
	switch(format)
	{
		case 0: return "Unset";
		case AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM: return "RGBA8888";
		case AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM: return "RGBX8888";
		case AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM: return "RGB888";
		case AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM: return "RGB565";
	}
	return "Unknown";
}

void AndroidApplication::initActivity(JNIEnv *env, jobject baseActivity, jclass baseActivityClass, int32_t androidSDK)
{
	pthread_key_create(&jEnvPThreadKey,
		[](void *)
		{
			if(!jVM)
				return;
			if(Config::DEBUG_BUILD)
			{
				log.debug("detaching JNI thread:{}", thisThreadId());
			}
			jVM->DetachCurrentThread();
		});
	pthread_setspecific(jEnvPThreadKey, env);

	// BaseActivity JNI functions
	jSetRequestedOrientation = {env, baseActivityClass, "setRequestedOrientation", "(I)V"};
	jMainDisplayRotation = {env, baseActivityClass, "mainDisplayRotation", "()I"};
	jFormatDateTime = {env, baseActivityClass, "formatDateTime", "(J)Ljava/lang/String;"};
	jNewFontRenderer = {env, baseActivityClass, "newFontRenderer", "()Lcom/imagine/FontRenderer;"};
	jSetWinFlags = {env, baseActivityClass, "setWinFlags", "(II)V"};
	jWinFlags = {env, baseActivityClass, "winFlags", "()I"};
	if(androidSDK >= 11) { jSetUIVisibility = {env, baseActivityClass, "setUIVisibility", "(I)V"}; }
	jRecycle = {env, env->FindClass("android/graphics/Bitmap"), "recycle", "()V"};

	if(androidSDK >= 19) // Storage Access Framework support
	{
		openUriFd = {env, baseActivity, "openUriFd", "(Ljava/lang/String;I)I"};
		uriExists = {env, baseActivity, "uriExists", "(Ljava/lang/String;)Z"};
		uriLastModified = {env, baseActivity, "uriLastModified", "(Ljava/lang/String;)Ljava/lang/String;"};
		uriLastModifiedTime = {env, baseActivity, "uriLastModifiedTime", "(Ljava/lang/String;)J"};
		uriDisplayName = {env, baseActivity, "uriDisplayName", "(Ljava/lang/String;)Ljava/lang/String;"};
		uriFileType = {env, baseActivity, "uriFileType", "(Ljava/lang/String;)I"};
		deleteUri = {env, baseActivity, "deleteUri", "(Ljava/lang/String;Z)Z"};
		if(androidSDK >= 21)
		{
			listUriFiles = {env, baseActivity, "listUriFiles", "(JLjava/lang/String;)Z"};
			createDirUri = {env, baseActivity, "createDirUri", "(Ljava/lang/String;)Z"};
		}
		if(androidSDK >= 24)
		{
			renameUri = {env, baseActivity, "renameUri", "(Ljava/lang/String;Ljava/lang/String;)Z"};
		}
	}

	{
		static constexpr JNINativeMethod method[]
		{
			{
				"onContentRectChanged", "(JIIIIII)V",
				(void*)
				+[](JNIEnv*, jobject, jlong windowAddr, jint x, jint y, jint x2, jint y2, jint winWidth, jint winHeight)
				{
					assumeExpr(windowAddr);
					auto win = (Window*)windowAddr;
					win->setContentRect({{x, y}, {x2, y2}}, {winWidth, winHeight});
				}
			},
			{
				"displayEnumerated", "(JLandroid/view/Display;IFJILandroid/util/DisplayMetrics;)V",
				(void*)
				+[](JNIEnv* env, jobject, jlong nActivityAddr, jobject disp, jint id, jfloat refreshRate, jlong presentationDeadline, jint rotation, jobject metrics)
				{
					ApplicationContext ctx{(ANativeActivity*)nActivityAddr};
					auto &app = ctx.application();
					auto screen = app.findScreen(id);
					if(!screen)
					{
						app.addScreen(ctx, std::make_unique<Screen>(ctx,
							Screen::InitParams{env, disp, metrics, id, refreshRate, Nanoseconds{presentationDeadline}, Rotation(rotation)}), false);
						return;
					}
					else
					{
						// already in list, update existing
						screen->updateFrameRate(refreshRate);
					}
				}
			},
			{
				"inputDeviceEnumerated", "(JILandroid/view/InputDevice;Ljava/lang/String;IIIIZ)V",
				(void*)
				+[](JNIEnv* env, jobject, jlong nUserData, jint devID, [[maybe_unused]] jobject jDev, jstring jName, jint src,
					jint kbType, jint jsAxisFlags, jint vendorProductId, jboolean isPowerButton)
				{
					ApplicationContext ctx{reinterpret_cast<ANativeActivity*>(nUserData)};
					auto &app = ctx.application();
					const char *name = env->GetStringUTFChars(jName, nullptr);
					auto sysDev = std::make_unique<Input::Device>(std::in_place_type<Input::AndroidInputDevice>, devID, src,
						name, kbType, std::bit_cast<Input::AxisFlags>(jsAxisFlags), (uint32_t)vendorProductId, (bool)isPowerButton);
					env->ReleaseStringUTFChars(jName, name);
					auto devPtr = app.updateAndroidInputDevice(ctx, std::move(sysDev), false);
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
				"uriPicked", "(JLjava/lang/String;Ljava/lang/String;)V",
				(void*)
				+[](JNIEnv* env, jobject, jlong nUserData, jstring jUri, jstring jDisplayName)
				{
					ApplicationContext ctx{reinterpret_cast<ANativeActivity*>(nUserData)};
					auto &app = ctx.application();
					auto uri = JNI::StringChars{env, jUri};
					auto name = JNI::StringChars{env, jDisplayName};
					log.info("picked URI:{} name:{}", uri.data(), name.data());
					app.handleDocumentIntentResult(ctx, uri, name);
				}
			},
			{
				"uriFileListed", "(JLjava/lang/String;Ljava/lang/String;Z)Z",
				(void*)
				+[](JNIEnv* env, jobject, jlong userData, jstring jUri, jstring name, jboolean isDir)
				{
					auto &del = *((DirectoryEntryDelegate*)userData);
					auto type = isDir ? FS::file_type::directory : FS::file_type::regular;
					return del(FS::directory_entry{JNI::StringChars{env, jUri}, JNI::StringChars{env, name}, type});
				}
			},
		};
		env->RegisterNatives(baseActivityClass, method, std::size(method));
	}

	if(androidSDK >= 14)
	{
		JNI::InstMethod<jint()> jDeviceFlags{env, baseActivityClass, "deviceFlags", "()I"};
		deviceFlags = std::bit_cast<DeviceFlags>(uint8_t(jDeviceFlags(env, baseActivity)));
		if(deviceFlags.permanentMenuKey)
		{
			log.info("device has hardware nav/menu keys");
		}
		if(deviceFlags.displayCutout)
		{
			log.info("device has display cutout");
		}
		if(deviceFlags.handleRotationAnimation)
		{
			log.info("app handles rotation animations");
		}
	}

	/*if(unloadNativeLibOnDestroy)
	{
		auto soPath = mainSOPath(appCtx);
		#if ANDROID_MIN_API >= 21
		mainLibHandle = dlopen(soPath.data(), RTLD_LAZY | RTLD_NOLOAD);
		#else
		mainLibHandle = dlopen(soPath.data(), RTLD_LAZY);
		dlclose(mainLibHandle);
		#endif
		if(!mainLibHandle)
			log.warn("unable to get native lib handle");
	}*/
}

JNIEnv* AndroidApplication::thisThreadJniEnv() const
{
	auto env = (JNIEnv*)pthread_getspecific(jEnvPThreadKey);
	if(!env) [[unlikely]]
	{
		if(Config::DEBUG_BUILD)
		{
			log.debug("attaching JNI thread:{}", thisThreadId());
		}
		assumeExpr(jVM);
		if(jVM->AttachCurrentThread(&env, nullptr) != 0)
		{
			log.error("error attaching JNI thread");
			return nullptr;
		}
		pthread_setspecific(jEnvPThreadKey, env);
	}
	return env;
}

void AndroidApplication::setIdleDisplayPowerSave(JNIEnv *env, jobject baseActivity, bool on)
{
	jint keepOn = !on;
	auto keepsScreenOn = userActivityCallback.isArmed() ?
		false : (bool)(jWinFlags(env, baseActivity) & AWINDOW_FLAG_KEEP_SCREEN_ON);
	if(keepOn != keepsScreenOn)
	{
		log.info("keep screen on:{}", (bool)keepOn);
		jSetWinFlags(env, baseActivity, keepOn ? AWINDOW_FLAG_KEEP_SCREEN_ON : 0, AWINDOW_FLAG_KEEP_SCREEN_ON);
	}
	keepScreenOn = keepOn; // cache value for endIdleByUserActivity()
}

void AndroidApplication::endIdleByUserActivity(ApplicationContext ctx)
{
	if(!keepScreenOn)
	{
		//log.info("signaling user activity to the OS");
		// quickly toggle KEEP_SCREEN_ON flag to brighten screen,
		// waiting about 20ms before toggling it back off triggers the screen to brighten if it was already dim
		jSetWinFlags(ctx.mainThreadJniEnv(), ctx.baseActivityObject(), AWINDOW_FLAG_KEEP_SCREEN_ON, AWINDOW_FLAG_KEEP_SCREEN_ON);
		userActivityCallback.runIn(Milliseconds(20));
	}
}

void AndroidApplication::setStatusBarHidden(JNIEnv *env, jobject baseActivity, bool hidden)
{
	auto statusBarIsHidden = (bool)(jWinFlags(env, baseActivity) & AWINDOW_FLAG_FULLSCREEN);
	if(hidden != statusBarIsHidden)
	{
		log.info("setting app window fullscreen: {}", hidden);
		jSetWinFlags(env, baseActivity, hidden ? AWINDOW_FLAG_FULLSCREEN : 0, AWINDOW_FLAG_FULLSCREEN);
	}
}

void AndroidApplication::setSysUIStyle(JNIEnv *env, jobject baseActivity, int32_t androidSDK, SystemUIStyleFlags flags)
{
	// Flags mapped directly to SYSTEM_UI_FLAG_*
	// dimNavigation -> SYSTEM_UI_FLAG_LOW_PROFILE (0)
	// hideNavigation -> SYSTEM_UI_FLAG_HIDE_NAVIGATION (1)
	// hideStatus -> SYSTEM_UI_FLAG_FULLSCREEN (2)
	// (unset) -> SYSTEM_UI_FLAG_VISIBLE (no bits set)

	// always handle status bar hiding via full-screen window flag
	// even on SDK level >= 11 so our custom view has the correct insets
	setStatusBarHidden(env, baseActivity, flags.hideStatus);
	if(androidSDK >= 11)
	{
		constexpr uint32_t SYSTEM_UI_FLAG_IMMERSIVE_STICKY = 0x1000;
		uint32_t nativeFlags = std::bit_cast<uint8_t>(flags);
		// Add SYSTEM_UI_FLAG_IMMERSIVE_STICKY for use with Android 4.4+ if flags contain OS_NAV_STYLE_HIDDEN
		if(flags.hideNavigation)
			nativeFlags |= SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
		log.info("setting UI visibility:0x{:X}", nativeFlags);
		uiVisibilityFlags = nativeFlags;
		jSetUIVisibility(env, baseActivity, nativeFlags);
	}
}

bool AndroidApplication::hasFocus() const
{
	return aHasFocus;
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
	log.info("focus change:{}", focused);
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

void AndroidApplication::onInputQueueCreated(ApplicationContext, AInputQueue* queue)
{
	assert(!inputQueue);
	inputQueue = queue;
	log.info("made & attached input queue");
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
	log.info("input queue destroyed");
	inputQueue = {};
	AInputQueue_detachLooper(queue);
}

std::string AndroidApplication::androidBuildDevice(JNIEnv *env, jclass baseActivityClass) const
{
	JNI::ClassMethod<jstring()> jDevName{env, baseActivityClass, "devName", "()Ljava/lang/String;"};
	return std::string{JNI::StringChars{env, jDevName(env, baseActivityClass)}};
}

void AndroidApplication::addNotification(JNIEnv *env, jobject baseActivity, const char *onShow, const char *title, const char *message)
{
	log.info("adding notificaion icon");
	if(!jAddNotification) [[unlikely]]
	{
		jAddNotification = {env, baseActivity, "addNotification", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V"};
	}
	jAddNotification(env, baseActivity, env->NewStringUTF(onShow), env->NewStringUTF(title), env->NewStringUTF(message));
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
		log.info("got intent with path:{}", intentDataPathStr);
		onEvent(ctx, DocumentPickerEvent{intentDataPathStr, ctx.fileUriDisplayName(intentDataPathStr)});
		env->ReleaseStringUTFChars(intentDataPathJStr, intentDataPathStr);
	}
}

bool AndroidApplication::openDocumentTreeIntent(JNIEnv* env, ANativeActivity* nActivity, jobject baseActivity)
{
	JNI::InstMethod<jboolean(jlong)> jOpenDocumentTree{env, env->GetObjectClass(baseActivity), "openDocumentTree", "(J)Z"};
	return jOpenDocumentTree(env, baseActivity, (jlong)nActivity);
}

bool AndroidApplication::openDocumentIntent(JNIEnv* env, ANativeActivity* nActivity, jobject baseActivity)
{
	JNI::InstMethod<jboolean(jlong)> jOpenDocument{env, env->GetObjectClass(baseActivity), "openDocument", "(J)Z"};
	return jOpenDocument(env, baseActivity, (jlong)nActivity);
}

bool AndroidApplication::createDocumentIntent(JNIEnv* env, ANativeActivity* nActivity, jobject baseActivity)
{
	JNI::InstMethod<jboolean(jlong)> jCreateDocument{env, env->GetObjectClass(baseActivity), "createDocument", "(J)Z"};
	return jCreateDocument(env, baseActivity, (jlong)nActivity);
}

void AndroidApplication::handleDocumentIntentResult(ApplicationContext ctx, const char *uri, const char *name)
{
	if(isRunning())
	{
		onEvent(ctx, DocumentPickerEvent{uri, name});
	}
	else
	{
		// wait until after app resumes before handling result
		addOnResume(
			[uriCopy = strdup(uri), nameCopy = strdup(name)](ApplicationContext ctx, [[maybe_unused]] bool focused)
			{
				ctx.application().onEvent(ctx, DocumentPickerEvent{uriCopy, nameCopy});
				::free(nameCopy);
				::free(uriCopy);
				return false;
			}, APP_ON_RESUME_PRIORITY + 100);
	}
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
				log.info("closing main lib:{}", mainLibHandle);
				dlclose(mainLibHandle);
			}
			else
			{
				log.info("exiting process");
				delete static_cast<BaseApplication*>(nActivity->instance);
				::exit(0);
			}
		};
	nActivity->callbacks->onStart =
		[](ANativeActivity *nActivity)
		{
			ApplicationContext ctx{nActivity};
			auto &app = ctx.application();
			log.info("app started");
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
			log.info("app resumed");
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
				log.info("app {}", app.isPaused() ? "paused" : "exiting");
				// App is killable in Android 2.3, run exit handler to save volatile data
				app.dispatchOnExit(ctx, app.isPaused());
			}
			else
				log.info("app paused");
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
				log.info("app {}", app.isPaused() ? "stopped" : "exiting");
				app.dispatchOnExit(ctx, app.isPaused());
			}
			else
				log.info("app stopped");
			app.setActiveForAllScreens(false);
		};
	nActivity->callbacks->onConfigurationChanged =
		[](ANativeActivity *nActivity)
		{
			ApplicationContext ctx{nActivity};
			auto &app = ctx.application();
			auto aConfig = AConfiguration_new();
			auto freeConfig = scopeGuard([&](){ AConfiguration_delete(aConfig); });
			AConfiguration_fromAssetManager(aConfig, nActivity->assetManager);
			auto rotation = app.mainDisplayRotation(nActivity->env, nActivity->clazz);
			if(rotation != app.currentRotation())
			{
				log.info("changed OS orientation");
				app.setCurrentRotation(ctx, rotation, true);
			}
			app.updateInputConfig(ctx, aConfig);
		};
	nActivity->callbacks->onLowMemory =
		[](ANativeActivity *nActivity)
		{
			ApplicationContext ctx{nActivity};
			ctx.application().onEvent(ctx, FreeCachesEvent{ctx.isRunning()});
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

CLINK void LVISIBLE ANativeActivity_onCreate(ANativeActivity* nActivity, [[maybe_unused]] void* savedState, [[maybe_unused]] size_t savedStateSize)
{
	using namespace IG;
	if(Config::DEBUG_BUILD)
	{
		mainThreadId = gettid();
		IG::log.info("called ANativeActivity_onCreate, thread ID:{}", mainThreadId);
	}
	jVM = nActivity->vm;
	ApplicationInitParams initParams{nActivity};
	ApplicationContext ctx{nActivity};
	ctx.dispatchOnInit(initParams);
	if(Config::DEBUG_BUILD)
	{
		if(!ctx.windows().size())
			IG::log.warn("didn't create a window");
	}
}
