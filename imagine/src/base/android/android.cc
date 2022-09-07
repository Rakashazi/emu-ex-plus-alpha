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
#include <imagine/fs/FSUtils.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/utility.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/format.hh>
#include "android.hh"
#include "AndroidInputDevice.hh"

namespace IG
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
	initChoreographer(env, baseActivity, baseActivityClass, androidSDK);
	initScreens(env, baseActivity, baseActivityClass, androidSDK, initParams.nActivity);
	initInput(env, baseActivity, baseActivityClass, androidSDK);
	{
		auto aConfig = AConfiguration_new();
		auto freeConfig = IG::scopeGuard([&](){ AConfiguration_delete(aConfig); });
		AConfiguration_fromAssetManager(aConfig, ctx.aAssetManager());
		initInputConfig(aConfig);
	}
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

MutablePixmapView makePixmapView(JNIEnv *env, jobject bitmap, void *pixels, IG::PixelFormat format)
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
	return {{{(int)info.width, (int)info.height}, format}, pixels, {(int)info.stride, MutablePixmapView::Units::BYTE}};
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
		JNI::InstMethod<jstring()> filesDir{env, baseActivity, "filesDir", "()Ljava/lang/String;"};
		return FS::PathString{JNI::StringChars{env, filesDir(env, baseActivity)}};
	}
	else
		return act->internalDataPath;
}

FS::PathString ApplicationContext::storagePath(const char *) const
{
	if(androidSDK() < 19)
		return sharedStoragePath();
	else // Android 4.4+ can use external storage without write permission
		return act->externalDataPath;
}

FS::PathString ApplicationContext::cachePath(const char *) const
{
	auto env = thisThreadJniEnv();
	auto baseActivityCls = (jclass)env->GetObjectClass(baseActivityObject());
	JNI::ClassMethod<jstring()> cacheDir{env, baseActivityCls, "cacheDir", "()Ljava/lang/String;"};
	return FS::PathString{JNI::StringChars{env, cacheDir(env, baseActivityCls)}};
}

FS::PathString ApplicationContext::sharedStoragePath() const
{
	auto env = thisThreadJniEnv();
	return application().sharedStoragePath(env, env->GetObjectClass(baseActivityObject()));
}

FS::PathString AndroidApplication::sharedStoragePath(JNIEnv *env, jclass baseActivityClass) const
{
	JNI::ClassMethod<jstring()> extStorageDir{env, baseActivityClass, "extStorageDir", "()Ljava/lang/String;"};
	return FS::PathString{JNI::StringChars{env, extStorageDir(env, baseActivityClass)}};
}

FS::PathLocation ApplicationContext::sharedStoragePathLocation() const
{
	auto path = sharedStoragePath();
	return {path, "Storage Media", "Media"};
}

FS::PathString AndroidApplication::externalMediaPath(JNIEnv *env, jobject baseActivity) const
{
	JNI::InstMethod<jstring()> extMediaDir{env, baseActivity, "extMediaDir", "()Ljava/lang/String;"};
	return FS::PathString{JNI::StringChars{env, extMediaDir(env, baseActivity)}};
}

FS::PathLocation AndroidApplicationContext::externalMediaPathLocation() const
{
	auto path = application().externalMediaPath(thisThreadJniEnv(), baseActivityObject());
	return {path, "App Media Folder", "Media"};
}

std::vector<FS::PathLocation> ApplicationContext::rootFileLocations() const
{
	if(androidSDK() >= 30)
	{
		// When using scoped storage on Android 11+, provide the app's media directory since it's
		// the only file location that's globally readable/writable
		return {externalMediaPathLocation()};
	}
	if(androidSDK() < 14)
	{
		return {sharedStoragePathLocation()};
	}
	else if(androidSDK() < 24)
	{
		FS::PathString storageDevicesPath{"/storage"};
		return
			{
				sharedStoragePathLocation(),
				{storageDevicesPath, "Storage Devices", "Storage"}
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
					FS::PathString path{JNI::StringChars(env, jPath)};
					FS::FileString name{JNI::StringChars(env, jName)};
					logMsg("volume:%s with path:%s", name.data(), path.data());
					rootLocation->emplace_back(path, name);
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
		JNI::InstMethod<jstring()> libDir{env, baseActivity, "libDir", "()Ljava/lang/String;"};
		return FS::PathString{JNI::StringChars{env, libDir(env, baseActivity)}};
	}
	return {};
}

UniqueFileDescriptor AndroidApplication::openFileUriFd(JNIEnv *env, jobject baseActivity, CStringView uri, OpenFlagsMask openFlags) const
{
	int fd = openUriFd(env, baseActivity, env->NewStringUTF(uri), (jint)openFlags);
	if(fd == -1)
	{
		if constexpr(Config::DEBUG_BUILD)
			logErr("error opening URI:%s", uri.data());
		if(to_underlying(openFlags & OpenFlagsMask::TEST))
			return -1;
		else
			throw std::system_error{ENOENT, std::system_category(), uri};
	}
	logMsg("opened fd:%d file URI:%s", fd, uri.data());
	return fd;
}

FileIO ApplicationContext::openFileUri(CStringView uri, IOAccessHint access, OpenFlagsMask openFlags) const
{
	if(androidSDK() < 19 || !IG::isUri(uri))
		return {uri, access, openFlags};
	return {application().openFileUriFd(thisThreadJniEnv(), baseActivityObject(), uri, openFlags), access, openFlags};
}

UniqueFileDescriptor ApplicationContext::openFileUriFd(CStringView uri, OpenFlagsMask openFlags) const
{
	if(androidSDK() < 19 || !IG::isUri(uri))
		return PosixIO{uri, openFlags}.releaseFd();
	return application().openFileUriFd(thisThreadJniEnv(), baseActivityObject(), uri, openFlags);
}

bool AndroidApplication::fileUriExists(JNIEnv *env, jobject baseActivity, IG::CStringView uri) const
{
	bool exists = uriExists(env, baseActivity, env->NewStringUTF(uri));
	logMsg("URI %s:%s", exists ? "exists" : "doesn't exist", uri.data());
	return exists;
}

bool ApplicationContext::fileUriExists(IG::CStringView uri) const
{
	if(androidSDK() < 19 || !IG::isUri(uri))
		return FS::exists(uri);
	return application().fileUriExists(thisThreadJniEnv(), baseActivityObject(), uri);
}

std::string AndroidApplication::fileUriFormatLastWriteTimeLocal(JNIEnv *env, jobject baseActivity, IG::CStringView uri) const
{
	//logMsg("getting modification time for URI:%s", uri.data());
	return std::string{JNI::StringChars{env, uriLastModified(env, baseActivity, env->NewStringUTF(uri))}};
}

std::string ApplicationContext::fileUriFormatLastWriteTimeLocal(IG::CStringView uri) const
{
	if(androidSDK() < 19 || !IG::isUri(uri))
		return FS::formatLastWriteTimeLocal(uri);
	return application().fileUriFormatLastWriteTimeLocal(thisThreadJniEnv(), baseActivityObject(), uri);
}

FS::FileString AndroidApplication::fileUriDisplayName(JNIEnv *env, jobject baseActivity, IG::CStringView uri) const
{
	//logMsg("getting display name for URI:%s", uri.data());
	return FS::FileString{JNI::StringChars{env, uriDisplayName(env, baseActivity, env->NewStringUTF(uri))}};
}

FS::FileString ApplicationContext::fileUriDisplayName(IG::CStringView uri) const
{
	if(androidSDK() < 19 || !IG::isUri(uri))
		return FS::displayName(uri);
	return application().fileUriDisplayName(thisThreadJniEnv(), baseActivityObject(), uri);
}

bool AndroidApplication::removeFileUri(JNIEnv *env, jobject baseActivity, IG::CStringView uri, bool isDir) const
{
	logMsg("removing %s URI:%s", isDir ? "directory" : "file", uri.data());
	return deleteUri(env, baseActivity, env->NewStringUTF(uri), isDir);
}

bool ApplicationContext::removeFileUri(IG::CStringView uri) const
{
	if(androidSDK() < 19 || !IG::isUri(uri))
		return FS::remove(uri);
	return application().removeFileUri(thisThreadJniEnv(), baseActivityObject(), uri, false);
}

bool AndroidApplication::renameFileUri(JNIEnv *env, jobject baseActivity, IG::CStringView oldUri, IG::CStringView newUri) const
{
	logMsg("renaming file URI:%s -> %s", oldUri.data(), newUri.data());
	return renameUri(env, baseActivity, env->NewStringUTF(oldUri), env->NewStringUTF(newUri));
}

bool ApplicationContext::renameFileUri(IG::CStringView oldUri, IG::CStringView newUri) const
{
	if(androidSDK() < 24 || !IG::isUri(oldUri))
		return FS::rename(oldUri, newUri);
	return application().renameFileUri(thisThreadJniEnv(), baseActivityObject(), oldUri, newUri);
}

bool AndroidApplication::createDirectoryUri(JNIEnv *env, jobject baseActivity, IG::CStringView uri) const
{
	logMsg("creating directory URI:%s", uri.data());
	return createDirUri(env, baseActivity, env->NewStringUTF(uri));
}

bool ApplicationContext::createDirectoryUri(IG::CStringView uri) const
{
	if(androidSDK() < 21 || !IG::isUri(uri))
		return FS::create_directory(uri);
	return application().createDirectoryUri(thisThreadJniEnv(), baseActivityObject(), uri);
}

bool ApplicationContext::removeDirectoryUri(IG::CStringView uri) const
{
	if(androidSDK() < 19 || !IG::isUri(uri))
		return FS::remove(uri);
	return application().removeFileUri(thisThreadJniEnv(), baseActivityObject(), uri, true);
}

void AndroidApplication::forEachInDirectoryUri(JNIEnv *env, jobject baseActivity, CStringView uri, DirectoryEntryDelegate del) const
{
	logMsg("listing directory URI:%s", uri.data());
	if(!listUriFiles(env, baseActivity, (jlong)&del, env->NewStringUTF(uri)))
	{
		throw std::system_error{ENOENT, std::system_category(), uri};
	}
}

void ApplicationContext::forEachInDirectoryUri(CStringView uri, DirectoryEntryDelegate del) const
{
	if(androidSDK() < 21 || !IG::isUri(uri))
	{
		return forEachInDirectory(uri, del);
	}
	application().forEachInDirectoryUri(thisThreadJniEnv(), baseActivityObject(), uri, del);
}

static FS::PathString mainSOPath(ApplicationContext ctx)
{
	if(ctx.androidSDK() < 24)
	{
		return FS::pathString(ctx.libPath(nullptr), "libmain.so");
	}
	return "libmain.so";
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
	if(androidSDK() < 23 || androidSDK() >= 30)
		return false;
	return true;
}

bool ApplicationContext::permissionIsRestricted(Permission p) const
{
	return p == Permission::WRITE_EXT_STORAGE ? androidSDK() >= 30 : false;
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
	JNI::InstMethod<jboolean(jstring)> requestPermission{env, baseActivity, "requestPermission", "(Ljava/lang/String;)Z"};
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

bool ApplicationContext::hasHardwareNavButtons() const
{
	return application().hasHardwareNavButtons();
}

int32_t ApplicationContext::androidSDK() const
{
	#ifdef ANDROID_COMPAT_API
	static_assert(__ANDROID_API__ <= 19, "Compiling with ANDROID_COMPAT_API and API higher than 19");
	#endif
	return std::max(ANDROID_MIN_API, act->sdkVersion);
}

void AndroidApplication::setOnSystemOrientationChanged(SystemOrientationChangedDelegate del)
{
	onSystemOrientationChanged = del;
}

bool AndroidApplication::systemAnimatesWindowRotation() const
{
	if(Config::MACHINE_IS_GENERIC_ARMV7)
		return !(deviceFlags & HANDLE_ROTATION_ANIMATION_BIT);
	else
		return true;
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

uint32_t toAHardwareBufferFormat(IG::PixelFormatID format)
{
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
				logDMsg("detaching JNI thread:%d", IG::thisThreadId());
			}
			jVM->DetachCurrentThread();
		});
	pthread_setspecific(jEnvKey, env);

	// BaseActivity JNI functions
	jSetRequestedOrientation = {env, baseActivityClass, "setRequestedOrientation", "(I)V"};
	jMainDisplayRotation = {env, baseActivityClass, "mainDisplayRotation", "()I"};
	jNewFontRenderer = {env, baseActivityClass, "newFontRenderer", "()Lcom/imagine/FontRenderer;"};
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
							Screen::InitParams{env, disp, metrics, id, refreshRate, (Rotation)rotation}), false);
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
				"inputDeviceEnumerated", "(JILandroid/view/InputDevice;Ljava/lang/String;IIIIZ)V",
				(void*)
				+[](JNIEnv* env, jobject, jlong nUserData, jint devID, jobject jDev, jstring jName, jint src,
					jint kbType, jint jsAxisBits, jint vendorProductId, jboolean isPowerButton)
				{
					auto &app = *((AndroidApplication*)nUserData);
					const char *name = env->GetStringUTFChars(jName, nullptr);
					Input::AndroidInputDevice sysDev{env, jDev, devID, src,
						name, kbType, (uint32_t)jsAxisBits, (uint32_t)vendorProductId, (bool)isPowerButton};
					env->ReleaseStringUTFChars(jName, name);
					auto devPtr = app.updateAndroidInputDevice(std::move(sysDev), false);
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
					auto &app = *((AndroidApplication*)nUserData);
					auto uri = JNI::StringChars{env, jUri};
					auto name = JNI::StringChars{env, jDisplayName};
					logMsg("picked URI:%s name:%s", uri.data(), name.data());
					app.handleDocumentIntentResult(uri, name);
				}
			},
			{
				"uriFileListed", "(JLjava/lang/String;Ljava/lang/String;Z)Z",
				(void*)
				+[](JNIEnv* env, jobject thiz, jlong userData, jstring jUri, jstring name, jboolean isDir)
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
		deviceFlags = jDeviceFlags(env, baseActivity);
		if(deviceFlags & PERMANENT_MENU_KEY_BIT)
		{
			logMsg("device has hardware nav/menu keys");
		}
		if(deviceFlags & DISPLAY_CUTOUT_BIT)
		{
			logMsg("device has display cutout");
		}
		if(deviceFlags & HANDLE_ROTATION_ANIMATION_BIT)
		{
			logMsg("app handles rotation animations");
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
			logWarn("unable to get native lib handle");
	}*/

	jSetWinFlags = {env, baseActivityClass, "setWinFlags", "(II)V"};
	jWinFlags = {env, baseActivityClass, "winFlags", "()I"};

	if(androidSDK >= 11)
	{
		jSetUIVisibility = {env, baseActivityClass, "setUIVisibility", "(I)V"};
	}

	jRecycle = {env, env->FindClass("android/graphics/Bitmap"), "recycle", "()V"};

	if(androidSDK >= 19) // Storage Access Framework support
	{
		openUriFd = {env, baseActivity, "openUriFd", "(Ljava/lang/String;I)I"};
		uriExists = {env, baseActivity, "uriExists", "(Ljava/lang/String;)Z"};
		uriLastModified = {env, baseActivity, "uriLastModified", "(Ljava/lang/String;)Ljava/lang/String;"};
		uriDisplayName = {env, baseActivity, "uriDisplayName", "(Ljava/lang/String;)Ljava/lang/String;"};
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
}

JNIEnv* AndroidApplication::thisThreadJniEnv() const
{
	auto env = (JNIEnv*)pthread_getspecific(jEnvKey);
	if(!env) [[unlikely]]
	{
		if(Config::DEBUG_BUILD)
		{
			logDMsg("attaching JNI thread:%d", IG::thisThreadId());
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

bool ApplicationContext::hasDisplayCutout() const { return application().hasDisplayCutout(); }

bool AndroidApplication::hasFocus() const
{
	return aHasFocus;
}

SustainedPerformanceType AndroidApplicationContext::sustainedPerformanceModeType() const
{
	int sdk = static_cast<const ApplicationContext*>(this)->androidSDK();
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

SensorValues ApplicationContext::remapSensorValuesForDeviceRotation(SensorValues v) const
{
	switch(application().currentRotation())
	{
		case Rotation::ANY:
		case Rotation::UP: return v;
		case Rotation::RIGHT: return {-v[1], v[0], v[2]};
		case Rotation::DOWN: return {v[0], -v[1], v[2]};
		case Rotation::LEFT: return {v[1], v[0], v[2]};
	}
	bug_unreachable("invalid Rotation");
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
	using namespace IG;
	if(Config::DEBUG_BUILD)
	{
		mainThreadId = gettid();
		logMsg("called ANativeActivity_onCreate, thread ID %d", mainThreadId);
	}
	jVM = nActivity->vm;
	ApplicationInitParams initParams{nActivity};
	ApplicationContext ctx{nActivity};
	ctx.dispatchOnInit(initParams);
	if(Config::DEBUG_BUILD)
	{
		if(!ctx.windows().size())
			logWarn("didn't create a window");
	}
}
