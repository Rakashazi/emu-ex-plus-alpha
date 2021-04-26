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
#include <imagine/base/FrameTimer.hh>
#include <imagine/input/Device.hh>
#include <imagine/input/AxisKeyEmu.hh>
#include <imagine/fs/FSDefs.hh>
#include <imagine/util/jni.hh>
#include <imagine/util/container/ArrayList.hh>
#include <pthread.h>
#include <optional>

struct ANativeActivity;
struct AInputQueue;
struct AConfiguration;
struct AInputEvent;

namespace Input
{

class AndroidInputDevice : public Input::Device
{
public:
	int osId = 0;
	uint32_t joystickAxisAsDpadBits_ = 0, joystickAxisAsDpadBitsDefault_ = 0;
	uint32_t axisBits = 0;
	bool iCadeMode_ = false;
	//static constexpr uint32_t MAX_STICK_AXES = 6; // 6 possible axes defined in key codes
	static constexpr uint32_t MAX_AXES = 10;
	//static_assert(MAX_STICK_AXES <= MAX_AXES, "MAX_AXES must be large enough to hold MAX_STICK_AXES");

	struct Axis
	{
		constexpr Axis() {}
		constexpr Axis(uint8_t id, AxisKeyEmu<float> keyEmu): id{id}, keyEmu{keyEmu} {}
		uint8_t id{};
		AxisKeyEmu<float> keyEmu{};
	};
	StaticArrayList<Axis, MAX_AXES> axis;

	AndroidInputDevice(int osId, uint32_t typeBits, const char *name);
	AndroidInputDevice(JNIEnv* env, jobject aDev, uint32_t enumId, int osId, int src,
		const char *name, int kbType, int axisBits, bool isPowerButton);
	bool operator ==(AndroidInputDevice const& rhs) const;
	void setTypeBits(int bits);
	void setJoystickAxisAsDpadBits(uint32_t axisMask) final;
	uint32_t joystickAxisAsDpadBits() final;
	uint32_t joystickAxisAsDpadBitsDefault() final;
	uint32_t joystickAxisBits() final;
	void setICadeMode(bool on) final;
	bool iCadeMode() const final;
};

}

namespace Base
{

class ApplicationContext;

using AndroidPropString = std::array<char, 92>;

enum SurfaceRotation : uint8_t;

using AndroidInputDeviceContainer = std::vector<std::unique_ptr<Input::AndroidInputDevice>>;

struct TouchState
{
	constexpr TouchState() {}
	int8_t id{-1};
	bool isTouching{false};
};

using TouchStateArray = std::array<TouchState, Config::Input::MAX_POINTERS>;

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
	jclass baseActivityClass() const;
	bool hasHardwareNavButtons() const;
	void recycleBitmap(JNIEnv *, jobject bitmap);
	jobject makeFontRenderer(JNIEnv *, jobject baseActivity);
	void setStatusBarHidden(JNIEnv *, jobject baseActivity, bool hidden);
	AndroidPropString androidBuildDevice(JNIEnv *env) const;
	Window *deviceWindow() const;
	FS::PathString sharedStoragePath(JNIEnv *) const;
	void setRequestedOrientation(JNIEnv *, jobject baseActivity, int orientation);
	SurfaceRotation currentRotation() const;
	void setCurrentRotation(ApplicationContext, SurfaceRotation, bool notify = false);
	SurfaceRotation mainDisplayRotation(JNIEnv *, jobject baseActivity) const;
	void setOnSystemOrientationChanged(SystemOrientationChangedDelegate del);
	bool systemAnimatesWindowRotation() const;
	FrameTimer *frameTimer() const;
	void setIdleDisplayPowerSave(JNIEnv *, jobject baseActivity, bool on);
	void endIdleByUserActivity(ApplicationContext);
	void setSysUIStyle(JNIEnv *, jobject baseActivity, int32_t androidSDK, uint32_t flags);
	bool hasFocus() const;
	void addNotification(JNIEnv *, jobject baseActivity, const char *onShow, const char *title, const char *message);
	void removePostedNotifications(JNIEnv *, jobject baseActivity);
	void handleIntent(ApplicationContext);

	// Input system functions
	void onInputQueueCreated(ApplicationContext, AInputQueue *);
	void onInputQueueDestroyed(AInputQueue *);
	void updateInputConfig(AConfiguration *config);
	int hardKeyboardState() const;
	int keyboardType() const;
	bool hasXperiaPlayGamepad() const;
	void setEventsUseOSInputMethod(bool on);
	bool eventsUseOSInputMethod() const;
	Input::AndroidInputDevice *addInputDevice(Input::AndroidInputDevice, bool updateExisting, bool notify);
	bool removeInputDevice(int id, bool notify);
	void enumInputDevices(JNIEnv *, bool notify);
	bool processInputEvent(AInputEvent*, Window &);
	bool hasTrackball() const;
	void flushSystemInputEvents();

private:
	jclass jBaseActivityCls{};
	jclass inputDeviceHelperCls{};
	JavaInstMethod<void()> jRecycle{};
	JavaInstMethod<void(jint)> jSetUIVisibility{};
	JavaInstMethod<jobject()> jNewFontRenderer{};
	JavaInstMethod<void(jint)> jSetRequestedOrientation{};
	JavaInstMethod<jint()> jMainDisplayRotation{};
	JavaInstMethod<void(jint, jint)> jSetWinFlags{};
	JavaInstMethod<jint()> jWinFlags{};
	JavaInstMethod<void(jstring, jstring, jstring)> jAddNotification{};
	JavaClassMethod<void(jlong)> jEnumInputDevices{};
	SystemOrientationChangedDelegate onSystemOrientationChanged{};
	Timer userActivityCallback{"userActivityCallback"};
	void (AndroidApplication::*processInput_)(AInputQueue *);
	AInputQueue *inputQueue{};
	AndroidInputDeviceContainer sysInputDev{};
	const Input::AndroidInputDevice *builtinKeyboardDev{};
	const Input::AndroidInputDevice *virtualDev{};
	std::unique_ptr<FrameTimer> frameTimer_{};
	pthread_key_t jEnvKey{};
	uint32_t uiVisibilityFlags{};
	int aHardKeyboardState{};
	int aKeyboardType{};
	int mostRecentKeyEventDevID{-1};
	TouchStateArray m{};
	SurfaceRotation osRotation{};
	bool osAnimatesRotation{};
	bool aHasFocus{true};
	bool hasPermanentMenuKey{true};
	bool keepScreenOn{};
	bool trackballNav{};
	bool sendInputToIME{};

	// InputDeviceListener-based device changes
	jobject inputDeviceListenerHelper{};
	JavaInstMethod<void()> jRegister{};
	JavaInstMethod<void()> jUnregister{};

	// inotify-based device changes
	std::optional<Base::Timer> inputRescanCallback{};
	int inputDevNotifyFd = -1;
	int watch = -1;

	void setHardKeyboardState(int hardKeyboardState);
	void initActivity(JNIEnv *, jobject baseActivity, int32_t androidSDK);
	void initInput(JNIEnv *, jobject baseActivity, int32_t androidSDK);
	void initInputConfig(AConfiguration *config);
	void initFrameTimer(JNIEnv *, jobject baseActivity, int32_t androidSDK, Screen &screen);
	void initScreens(JNIEnv *, jobject baseActivity, int32_t androidSDK, ANativeActivity *);
	void processInput(AInputQueue *);
	void processInputWithGetEvent(AInputQueue *);
	void processInputWithHasEvents(AInputQueue *);
	void processInputCommon(AInputQueue *inputQueue, AInputEvent* event);
};

using ApplicationImpl = AndroidApplication;

}
