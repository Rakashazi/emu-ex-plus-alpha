#pragma once

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

#include <imagine/config/defs.hh>
#include <imagine/base/BaseApplication.hh>
#include <imagine/base/Timer.hh>
#include <imagine/base/android/Choreographer.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/jni.hh>
#include <pthread.h>
#include <optional>
#include <string>
#include <utility>
#include <variant>

struct ANativeActivity;
struct AInputQueue;
struct AConfiguration;
struct AInputEvent;

namespace IG::Input
{
class Device;
}

namespace IG::FS
{
class PathString;
class FileString;
struct DirOpenFlags;
}

namespace IG
{

class ApplicationContext;
class FrameTimer;

struct ApplicationInitParams
{
	ANativeActivity *nActivity;

	constexpr CommandArgs commandArgs() const	{ return {}; }
};

class AndroidApplication : public BaseApplication
{
public:
	AndroidApplication(ApplicationInitParams);
	void onWindowFocusChanged(ApplicationContext, int focused);
	JNIEnv* thisThreadJniEnv() const;
	bool hasHardwareNavButtons() const { return deviceFlags.permanentMenuKey; }
	constexpr JNI::InstMethod<void()> recycleBitmapMethod() const { return jRecycle; }
	jobject makeFontRenderer(JNIEnv *, jobject baseActivity);
	void setStatusBarHidden(JNIEnv *, jobject baseActivity, bool hidden);
	std::string androidBuildDevice(JNIEnv *env, jclass baseActivityClass) const;
	Window *deviceWindow() const;
	FS::PathString sharedStoragePath(JNIEnv *, jclass baseActivityClass) const;
	FS::PathString externalMediaPath(JNIEnv *, jobject baseActivity) const;
	void setRequestedOrientation(JNIEnv *, jobject baseActivity, int orientation);
	Rotation currentRotation() const;
	void setCurrentRotation(ApplicationContext, Rotation, bool notify = false);
	Rotation mainDisplayRotation(JNIEnv *, jobject baseActivity) const;
	void setOnSystemOrientationChanged(SystemOrientationChangedDelegate del);
	bool systemAnimatesWindowRotation() const;
	void setIdleDisplayPowerSave(JNIEnv *, jobject baseActivity, bool on);
	void endIdleByUserActivity(ApplicationContext);
	void setSysUIStyle(JNIEnv *, jobject baseActivity, int32_t androidSDK, SystemUIStyleFlags);
	bool hasDisplayCutout() const { return deviceFlags.displayCutout; }
	bool hasSustainedPerformanceMode() const { return deviceFlags.hasSustainedPerformanceMode; }
	bool hasFocus() const;
	void addNotification(JNIEnv *, jobject baseActivity, const char *onShow, const char *title, const char *message);
	void removePostedNotifications(JNIEnv *, jobject baseActivity);
	void handleIntent(ApplicationContext);
	bool openDocumentTreeIntent(JNIEnv*, ANativeActivity*, jobject baseActivity);
	bool openDocumentIntent(JNIEnv*, ANativeActivity*, jobject baseActivity);
	bool createDocumentIntent(JNIEnv*, ANativeActivity*, jobject baseActivity);
	void emplaceFrameTimer(FrameTimer&, Screen&, bool useVariableTime = {});
	bool requestPermission(ApplicationContext, Permission);
	UniqueFileDescriptor openFileUriFd(JNIEnv *, jobject baseActivity, CStringView uri, OpenFlags oFlags = {}) const;
	bool fileUriExists(JNIEnv *, jobject baseActivity, CStringView uri) const;
	WallClockTimePoint fileUriLastWriteTime(JNIEnv *, jobject baseActivity, CStringView uri) const;
	std::string fileUriFormatLastWriteTimeLocal(JNIEnv *, jobject baseActivity, CStringView uri) const;
	FS::FileString fileUriDisplayName(JNIEnv *, jobject baseActivity, CStringView uri) const;
	FS::file_type fileUriType(JNIEnv*, jobject baseActivity, CStringView uri) const;
	bool removeFileUri(JNIEnv *, jobject baseActivity, CStringView uri, bool isDir) const;
	bool renameFileUri(JNIEnv *, jobject baseActivity, CStringView oldUri, CStringView newUri) const;
	bool createDirectoryUri(JNIEnv *, jobject baseActivity, CStringView uri) const;
	bool forEachInDirectoryUri(JNIEnv *, jobject baseActivity, CStringView uri, DirectoryEntryDelegate,
		FS::DirOpenFlags) const;
	std::string formatDateAndTime(JNIEnv *, jclass baseActivityClass, WallClockTimePoint timeSinceEpoch);

	// Input system functions
	void onInputQueueCreated(ApplicationContext, AInputQueue *);
	void onInputQueueDestroyed(AInputQueue *);
	void updateInputConfig(ApplicationContext, AConfiguration *config);
	int hardKeyboardState() const;
	int keyboardType() const;
	bool hasXperiaPlayGamepad() const;
	Input::Device *addAndroidInputDevice(ApplicationContext, std::unique_ptr<Input::Device>, bool notify);
	Input::Device *updateAndroidInputDevice(ApplicationContext, std::unique_ptr<Input::Device>, bool notify);
	Input::Device *inputDeviceForId(int id) const;
	std::pair<Input::Device*, int> inputDeviceForEvent(AInputEvent *);
	void enumInputDevices(ApplicationContext ctx, JNIEnv *, jobject baseActivity, bool notify);
	bool processInputEvent(AInputEvent*, Input::Device *, int devId, Window &);
	bool hasTrackball() const;
	void flushSystemInputEvents();
	bool hasPendingInputQueueEvents() const;
	bool hasMultipleInputDeviceSupport() const;

private:
	struct DeviceFlags
	{
		uint8_t
		permanentMenuKey:1{},
		displayCutout:1{},
		handleRotationAnimation:1{},
		hasSustainedPerformanceMode:1{};
	};

	struct InputDeviceListenerImpl
	{
		JNI::UniqueGlobalRef listenerHelper;
		JNI::InstMethod<void()> jRegister;
		JNI::InstMethod<void()> jUnregister;
	};

	struct INotifyImpl
	{
		Timer rescanTimer;
		int fd = -1;
		int watch = -1;
	};

	JNI::UniqueGlobalRef displayListenerHelper;
	JNI::InstMethod<void()> jRecycle;
	JNI::InstMethod<void(jint)> jSetUIVisibility;
	JNI::InstMethod<jobject()> jNewFontRenderer;
	JNI::InstMethod<void(jint)> jSetRequestedOrientation;
	JNI::InstMethod<jint()> jMainDisplayRotation;
	JNI::InstMethod<void(jint, jint)> jSetWinFlags;
	JNI::InstMethod<jint()> jWinFlags;
	JNI::InstMethod<void(jstring, jstring, jstring)> jAddNotification;
	JNI::InstMethod<void(jlong)> jEnumInputDevices;
	JNI::InstMethod<jint(jstring, jint)> openUriFd;
	JNI::InstMethod<jboolean(jstring)> uriExists;
	JNI::InstMethod<jstring(jstring)> uriLastModified;
	JNI::InstMethod<jlong(jstring)> uriLastModifiedTime;
	JNI::InstMethod<jstring(jstring)> uriDisplayName;
	JNI::InstMethod<jint(jstring)> uriFileType;
	JNI::InstMethod<jboolean(jstring, jboolean)> deleteUri;
	JNI::InstMethod<jboolean(jlong, jstring)> listUriFiles;
	JNI::InstMethod<jboolean(jstring)> createDirUri;
	JNI::InstMethod<jboolean(jstring, jstring)> renameUri;
	JNI::ClassMethod<jstring(jlong)> jFormatDateTime;
	std::variant<InputDeviceListenerImpl, INotifyImpl> inputDeviceChangeImpl;
	SystemOrientationChangedDelegate onSystemOrientationChanged;
	Timer userActivityCallback;
	using ProcessInputFunc = void (AndroidApplication::*)(AInputQueue *);
	ConditionalMember<Config::ENV_ANDROID_MIN_SDK < 12, ProcessInputFunc> processInput_{&AndroidApplication::processInputWithHasEvents};
	AInputQueue *inputQueue{};
	Input::Device *builtinKeyboardDev{};
	Input::Device *virtualDev{};
	Choreographer choreographer{};
	uint32_t uiVisibilityFlags{};
	int aHardKeyboardState{};
	int aKeyboardType{};
	int mostRecentKeyEventDevID{-1};
	Rotation osRotation{};
	bool aHasFocus{true};
	DeviceFlags deviceFlags{.permanentMenuKey = true};
	bool keepScreenOn{};
	bool trackballNav{};
public:
	bool acceptsIntents{};

private:
	void setHardKeyboardState(ApplicationContext ctx, int hardKeyboardState);
	void initActivity(JNIEnv *, jobject baseActivity, jclass baseActivityClass, int32_t androidSDK);
	void initInput(ApplicationContext ctx, JNIEnv *, jobject baseActivity, jclass baseActivityClass, int32_t androidSDK);
	void initInputConfig(AConfiguration *config);
	void initChoreographer(JNIEnv *, jobject baseActivity, jclass baseActivityClass, int32_t androidSDK);
	void initScreens(JNIEnv *, jobject baseActivity, jclass baseActivityClass, int32_t androidSDK, ANativeActivity *);
	void processInput(AInputQueue *);
	void processInputWithGetEvent(AInputQueue *);
	void processInputWithHasEvents(AInputQueue *);
	void processInputCommon(AInputQueue *inputQueue, AInputEvent* event);
	void handleDocumentIntentResult(ApplicationContext, const char* uri, const char* name);
};

using ApplicationImpl = AndroidApplication;

}
