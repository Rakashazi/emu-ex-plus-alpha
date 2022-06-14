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
#include <imagine/fs/FSDefs.hh>
#include <imagine/util/jni.hh>
#include <pthread.h>
#include <optional>
#include <string>

struct ANativeActivity;
struct AInputQueue;
struct AConfiguration;
struct AInputEvent;

namespace IG::Input
{
class AndroidInputDevice;
}

namespace IG
{

class ApplicationContext;
class FrameTimer;

enum SurfaceRotation : uint8_t;

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
	bool hasHardwareNavButtons() const { return deviceFlags & PERMANENT_MENU_KEY_BIT; }
	constexpr JNI::InstMethod<void()> recycleBitmapMethod() const { return jRecycle; }
	jobject makeFontRenderer(JNIEnv *, jobject baseActivity);
	void setStatusBarHidden(JNIEnv *, jobject baseActivity, bool hidden);
	std::string androidBuildDevice(JNIEnv *env, jclass baseActivityClass) const;
	Window *deviceWindow() const;
	FS::PathString sharedStoragePath(JNIEnv *, jclass baseActivityClass) const;
	void setRequestedOrientation(JNIEnv *, jobject baseActivity, int orientation);
	SurfaceRotation currentRotation() const;
	void setCurrentRotation(ApplicationContext, SurfaceRotation, bool notify = false);
	SurfaceRotation mainDisplayRotation(JNIEnv *, jobject baseActivity) const;
	void setOnSystemOrientationChanged(SystemOrientationChangedDelegate del);
	bool systemAnimatesWindowRotation() const;
	void setIdleDisplayPowerSave(JNIEnv *, jobject baseActivity, bool on);
	void endIdleByUserActivity(ApplicationContext);
	void setSysUIStyle(JNIEnv *, jobject baseActivity, int32_t androidSDK, uint32_t flags);
	bool hasDisplayCutout() const { return deviceFlags & DISPLAY_CUTOUT_BIT; }
	bool hasFocus() const;
	void addNotification(JNIEnv *, jobject baseActivity, const char *onShow, const char *title, const char *message);
	void removePostedNotifications(JNIEnv *, jobject baseActivity);
	void handleIntent(ApplicationContext);
	void openDocumentTreeIntent(JNIEnv *, jobject baseActivity, SystemDocumentPickerDelegate);
	void openDocumentIntent(JNIEnv *, jobject baseActivity, SystemDocumentPickerDelegate);
	void createDocumentIntent(JNIEnv *, jobject baseActivity, SystemDocumentPickerDelegate);
	FrameTimer makeFrameTimer(Screen &);
	bool requestPermission(ApplicationContext, Permission);
	UniqueFileDescriptor openFileUriFd(JNIEnv *, jobject baseActivity, IG::CStringView uri, IODefs::OpenFlags oFlags = {}) const;
	bool fileUriExists(JNIEnv *, jobject baseActivity, IG::CStringView uri) const;
	std::string fileUriFormatLastWriteTimeLocal(JNIEnv *, jobject baseActivity, IG::CStringView uri) const;
	FS::FileString fileUriDisplayName(JNIEnv *, jobject baseActivity, IG::CStringView uri) const;
	bool removeFileUri(JNIEnv *, jobject baseActivity, IG::CStringView uri, bool isDir) const;
	bool renameFileUri(JNIEnv *, jobject baseActivity, IG::CStringView oldUri, IG::CStringView newUri) const;
	bool createDirectoryUri(JNIEnv *, jobject baseActivity, IG::CStringView uri) const;
	void forEachInDirectoryUri(JNIEnv *, jobject baseActivity, IG::CStringView uri, FS::DirectoryEntryDelegate) const;

	// Input system functions
	void onInputQueueCreated(ApplicationContext, AInputQueue *);
	void onInputQueueDestroyed(AInputQueue *);
	void updateInputConfig(AConfiguration *config);
	int hardKeyboardState() const;
	int keyboardType() const;
	bool hasXperiaPlayGamepad() const;
	Input::AndroidInputDevice *addAndroidInputDevice(Input::AndroidInputDevice, bool notify);
	Input::AndroidInputDevice *updateAndroidInputDevice(Input::AndroidInputDevice, bool notify);
	Input::AndroidInputDevice *inputDeviceForId(int id) const;
	std::pair<Input::AndroidInputDevice*, int> inputDeviceForEvent(AInputEvent *);
	void enumInputDevices(JNIEnv *, jobject baseActivity, bool notify);
	bool processInputEvent(AInputEvent*, Window &);
	bool hasTrackball() const;
	void flushSystemInputEvents();
	bool hasPendingInputQueueEvents() const;
	bool hasMultipleInputDeviceSupport() const;

private:
	using DeviceFlags = uint8_t;
	static constexpr DeviceFlags PERMANENT_MENU_KEY_BIT = bit(0);
	static constexpr DeviceFlags DISPLAY_CUTOUT_BIT = bit(1);

	JNI::UniqueGlobalRef displayListenerHelper{};
	JNI::InstMethod<void()> jRecycle{};
	JNI::InstMethod<void(jint)> jSetUIVisibility{};
	JNI::InstMethod<jobject()> jNewFontRenderer{};
	JNI::InstMethod<void(jint)> jSetRequestedOrientation{};
	JNI::InstMethod<jint()> jMainDisplayRotation{};
	JNI::InstMethod<void(jint, jint)> jSetWinFlags{};
	JNI::InstMethod<jint()> jWinFlags{};
	JNI::InstMethod<void(jstring, jstring, jstring)> jAddNotification{};
	JNI::InstMethod<void(jlong)> jEnumInputDevices{};
	JNI::InstMethod<jint(jstring, jint)> openUriFd{};
	JNI::InstMethod<jboolean(jstring)> uriExists{};
	JNI::InstMethod<jstring(jstring)> uriLastModified{};
	JNI::InstMethod<jstring(jstring)> uriDisplayName{};
	JNI::InstMethod<jboolean(jstring, jboolean)> deleteUri{};
	JNI::InstMethod<jboolean(jlong, jstring)> listUriFiles{};
	JNI::InstMethod<jboolean(jstring)> createDirUri{};
	JNI::InstMethod<jboolean(jstring, jstring)> renameUri{};
	SystemDocumentPickerDelegate onSystemDocumentPicker{};
	SystemOrientationChangedDelegate onSystemOrientationChanged{};
	Timer userActivityCallback{"userActivityCallback"};
	using ProcessInputFunc = void (AndroidApplication::*)(AInputQueue *);
	IG_UseMemberIf(Config::ENV_ANDROID_MIN_SDK < 12, ProcessInputFunc, processInput_){&AndroidApplication::processInputWithHasEvents};
	AInputQueue *inputQueue{};
	Input::AndroidInputDevice *builtinKeyboardDev{};
	Input::AndroidInputDevice *virtualDev{};
	Choreographer choreographer{};
	pthread_key_t jEnvKey{};
	uint32_t uiVisibilityFlags{};
	int aHardKeyboardState{};
	int aKeyboardType{};
	int mostRecentKeyEventDevID{-1};
	SurfaceRotation osRotation{};
	bool osAnimatesRotation{};
	bool aHasFocus{true};
	DeviceFlags deviceFlags{PERMANENT_MENU_KEY_BIT};
	bool keepScreenOn{};
	bool trackballNav{};

	// InputDeviceListener-based device changes
	JNI::UniqueGlobalRef inputDeviceListenerHelper{};
	JNI::InstMethod<void()> jRegister{};
	JNI::InstMethod<void()> jUnregister{};

	// inotify-based device changes
	std::optional<Timer> inputRescanCallback{};
	int inputDevNotifyFd = -1;
	int watch = -1;

	void setHardKeyboardState(int hardKeyboardState);
	void initActivity(JNIEnv *, jobject baseActivity, jclass baseActivityClass, int32_t androidSDK);
	void initInput(JNIEnv *, jobject baseActivity, jclass baseActivityClass, int32_t androidSDK);
	void initInputConfig(AConfiguration *config);
	void initChoreographer(JNIEnv *, jobject baseActivity, jclass baseActivityClass, int32_t androidSDK);
	void initScreens(JNIEnv *, jobject baseActivity, jclass baseActivityClass, int32_t androidSDK, ANativeActivity *);
	void processInput(AInputQueue *);
	void processInputWithGetEvent(AInputQueue *);
	void processInputWithHasEvents(AInputQueue *);
	void processInputCommon(AInputQueue *inputQueue, AInputEvent* event);
	void handleDocumentIntentResult(const char *uri, const char *name);
};

using ApplicationImpl = AndroidApplication;

}
