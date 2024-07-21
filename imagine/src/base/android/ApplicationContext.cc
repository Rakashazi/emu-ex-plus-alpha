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

#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/fs/FSUtils.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>
#include "android.hh"
#include <sys/resource.h>

namespace IG
{

constexpr SystemLogger log{"AppCtx"};
static NoopThread noopThread;

FS::PathString ApplicationContext::assetPath(const char *) const { return {}; }

FS::PathString ApplicationContext::supportPath(const char *) const
{
	if(androidSDK() < 11) // bug in pre-3.0 Android causes paths in ANativeActivity to be null
	{
		//log.info("ignoring paths from ANativeActivity due to Android 2.3 bug");
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

FS::PathLocation ApplicationContext::sharedStoragePathLocation() const
{
	auto path = sharedStoragePath();
	return {path, "Storage Media", "Media"};
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
		log.info("enumerating storage volumes");
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
				([](JNIEnv* env, jobject, jlong userData, jstring jName, jstring jPath)
				{
					auto rootLocation = (std::vector<FS::PathLocation>*)userData;
					FS::PathString path{JNI::StringChars(env, jPath)};
					FS::FileString name{JNI::StringChars(env, jName)};
					log.info("volume:{} with path:{}", name, path);
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

FileIO ApplicationContext::openFileUri(CStringView uri, OpenFlags openFlags) const
{
	if(androidSDK() < 19 || !isUri(uri))
		return {uri, openFlags};
	return {application().openFileUriFd(thisThreadJniEnv(), baseActivityObject(), uri, openFlags), openFlags};
}

UniqueFileDescriptor ApplicationContext::openFileUriFd(CStringView uri, OpenFlags openFlags) const
{
	if(androidSDK() < 19 || !isUri(uri))
		return PosixIO{uri, openFlags}.releaseFd();
	return application().openFileUriFd(thisThreadJniEnv(), baseActivityObject(), uri, openFlags);
}

bool ApplicationContext::fileUriExists(CStringView uri) const
{
	if(androidSDK() < 19 || !isUri(uri))
		return FS::exists(uri);
	return application().fileUriExists(thisThreadJniEnv(), baseActivityObject(), uri);
}

WallClockTimePoint ApplicationContext::fileUriLastWriteTime(CStringView uri) const
{
	if(androidSDK() < 19 || !isUri(uri))
		return FS::status(uri).lastWriteTime();
	return application().fileUriLastWriteTime(thisThreadJniEnv(), baseActivityObject(), uri);
}

std::string ApplicationContext::fileUriFormatLastWriteTimeLocal(CStringView uri) const
{
	if(androidSDK() < 19 || !isUri(uri))
		return FS::formatLastWriteTimeLocal(*this, uri);
	return application().fileUriFormatLastWriteTimeLocal(thisThreadJniEnv(), baseActivityObject(), uri);
}

FS::FileString ApplicationContext::fileUriDisplayName(CStringView uri) const
{
	if(androidSDK() < 19 || !isUri(uri))
		return FS::displayName(uri);
	return application().fileUriDisplayName(thisThreadJniEnv(), baseActivityObject(), uri);
}

FS::file_type ApplicationContext::fileUriType(CStringView uri) const
{
	if(androidSDK() < 19 || !isUri(uri))
		return FS::status(uri).type();
	return application().fileUriType(thisThreadJniEnv(), baseActivityObject(), uri);
}

bool ApplicationContext::removeFileUri(CStringView uri) const
{
	if(androidSDK() < 19 || !isUri(uri))
		return FS::remove(uri);
	return application().removeFileUri(thisThreadJniEnv(), baseActivityObject(), uri, false);
}

bool ApplicationContext::renameFileUri(CStringView oldUri, CStringView newUri) const
{
	if(androidSDK() < 24 || !isUri(oldUri))
		return FS::rename(oldUri, newUri);
	return application().renameFileUri(thisThreadJniEnv(), baseActivityObject(), oldUri, newUri);
}

bool ApplicationContext::createDirectoryUri(CStringView uri) const
{
	if(androidSDK() < 21 || !isUri(uri))
		return FS::create_directory(uri);
	return application().createDirectoryUri(thisThreadJniEnv(), baseActivityObject(), uri);
}

bool ApplicationContext::removeDirectoryUri(CStringView uri) const
{
	if(androidSDK() < 19 || !isUri(uri))
		return FS::remove(uri);
	return application().removeFileUri(thisThreadJniEnv(), baseActivityObject(), uri, true);
}

bool ApplicationContext::forEachInDirectoryUri(CStringView uri, DirectoryEntryDelegate del,
	FS::DirOpenFlags flags) const
{
	if(androidSDK() < 21 || !isUri(uri))
	{
		return forEachInDirectory(uri, del, flags);
	}
	return application().forEachInDirectoryUri(thisThreadJniEnv(), baseActivityObject(), uri, del, flags);
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

bool ApplicationContext::usesPermission(Permission) const
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
	static_assert(__ANDROID_API__ <= 21, "Compiling with ANDROID_COMPAT_API and API higher than 21");
	#endif
	return std::max(ANDROID_MIN_API, act->sdkVersion);
}

void ApplicationContext::setOnSystemOrientationChanged(SystemOrientationChangedDelegate del)
{
	application().setOnSystemOrientationChanged(del);
}

bool ApplicationContext::systemAnimatesWindowRotation() const
{
	return application().systemAnimatesWindowRotation();
}

JNIEnv* AndroidApplicationContext::thisThreadJniEnv() const
{
	return application().thisThreadJniEnv();
}

void ApplicationContext::setIdleDisplayPowerSave(bool on)
{
	application().setIdleDisplayPowerSave(mainThreadJniEnv(), baseActivityObject(), on);
}

void ApplicationContext::endIdleByUserActivity()
{
	application().endIdleByUserActivity(*this);
}

void ApplicationContext::setSysUIStyle(SystemUIStyleFlags flags)
{
	return application().setSysUIStyle(mainThreadJniEnv(), baseActivityObject(), androidSDK(), flags);
}

bool ApplicationContext::hasTranslucentSysUI() const
{
	return androidSDK() >= 19;
}

bool ApplicationContext::hasDisplayCutout() const { return application().hasDisplayCutout(); }

bool ApplicationContext::hasSustainedPerformanceMode() const { return androidSDK() >= 24 && application().hasSustainedPerformanceMode(); }

void ApplicationContext::setSustainedPerformanceMode(bool on)
{
	if(!hasSustainedPerformanceMode())
		return;
	log.info("set sustained performance mode:{}", on);
	auto env = mainThreadJniEnv();
	auto baseActivity = baseActivityObject();
	JNI::InstMethod<void(jboolean)> jSetSustainedPerformanceMode{env, baseActivity, "setSustainedPerformanceMode", "(Z)V"};
	jSetSustainedPerformanceMode(env, baseActivity, on);
}

void AndroidApplicationContext::setNoopThreadActive(bool on)
{
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

std::string AndroidApplicationContext::androidBuildDevice() const
{
	auto env = mainThreadJniEnv();
	return application().androidBuildDevice(env, env->GetObjectClass(baseActivityObject()));
}

bool ApplicationContext::packageIsInstalled(CStringView name) const
{
	auto env = mainThreadJniEnv();
	auto baseActivity = baseActivityObject();
	JNI::InstMethod<jboolean(jstring)> jPackageIsInstalled{env, baseActivity, "packageIsInstalled", "(Ljava/lang/String;)Z"};
	return jPackageIsInstalled(env, baseActivity, env->NewStringUTF(name));
}

void ApplicationContext::exitWithMessage(int exitVal, const char *msg)
{
	auto env = mainThreadJniEnv();
	auto baseActivity = baseActivityObject();
	JNI::InstMethod<void(jstring)> jMakeErrorPopup{env, baseActivity, "makeErrorPopup", "(Ljava/lang/String;)V"};
	jMakeErrorPopup(env, baseActivity, env->NewStringUTF(msg));
	auto exitTimer = new Timer{{.debugLabel = "exitTimer"}, [=]{ ::exit(exitVal); }};
	exitTimer->runIn(Seconds{3});
}

void ApplicationContext::setOnDeviceOrientationChanged(DeviceOrientationChangedDelegate)
{
	// TODO
}

void ApplicationContext::flushSystemInputEvents()
{
	application().flushSystemInputEvents();
}

void ApplicationContext::addNotification(CStringView onShow, CStringView title, CStringView message)
{
	return application().addNotification(mainThreadJniEnv(), baseActivityObject(), onShow, title, message);
}

void ApplicationContext::addLauncherIcon(CStringView name, CStringView path)
{
	log.info("adding launcher icon:{}, for location:{}", name, path);
	auto env = mainThreadJniEnv();
	auto baseActivity = baseActivityObject();
	JNI::InstMethod<void(jstring, jstring)> jAddViewShortcut{env, baseActivity, "addViewShortcut", "(Ljava/lang/String;Ljava/lang/String;)V"};
	jAddViewShortcut(env, baseActivity, env->NewStringUTF(name), env->NewStringUTF(path));
}

void ApplicationContext::openURL(CStringView url) const
{
	auto env = mainThreadJniEnv();
	auto baseActivity = baseActivityObject();
	JNI::InstMethod<void(jstring)> jOpenURL{env, baseActivity, "openURL", "(Ljava/lang/String;)V"};
	jOpenURL(env, baseActivity, env->NewStringUTF(url));
}

bool ApplicationContext::hasSystemPathPicker() const { return androidSDK() >= 21; }

bool ApplicationContext::showSystemPathPicker()
{
	return application().openDocumentTreeIntent(mainThreadJniEnv(), act, baseActivityObject());
}

bool ApplicationContext::hasSystemDocumentPicker() const { return androidSDK() >= 19; }

bool ApplicationContext::showSystemDocumentPicker()
{
	return application().openDocumentIntent(mainThreadJniEnv(), act, baseActivityObject());
}

bool ApplicationContext::showSystemCreateDocumentPicker()
{
	return application().createDocumentIntent(mainThreadJniEnv(), act, baseActivityObject());
}

void ApplicationContext::setAcceptIPC(bool on, const char *) { application().acceptsIntents = on; }

void NoopThread::start()
{
	if(isRunning.load(std::memory_order_relaxed))
		return;
	isRunning.store(true, std::memory_order_relaxed);
	makeDetachedThread(
		[this]()
		{
			// keep cpu governor busy by running a low priority thread executing no-op instructions
			setpriority(PRIO_PROCESS, 0, 19);
			log.info("started no-op thread");
			while(isRunning.load(std::memory_order_relaxed))
			{
				for([[maybe_unused]] auto i : iotaCount(16))
				{
					asm("nop");
				}
			}
			log.info("ended no-op thread");
		});
}

void NoopThread::stop()
{
	if(!isRunning.load(std::memory_order_relaxed))
		return;
	isRunning.store(false, std::memory_order_relaxed);
}

}
