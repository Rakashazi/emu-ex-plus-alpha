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

#define LOGTAG "InputConfig"
#include <imagine/base/Timer.hh>
#include <imagine/base/sharedLibrary.hh>
#include <imagine/base/Application.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/bit.hh>
#include "AndroidInputDevice.hh"
#include <android/configuration.h>
#include <android/input.h>
#include <sys/inotify.h>
#include <optional>

static float (*AMotionEvent_getAxisValueFunc)(const AInputEvent* motion_event, int32_t axis, size_t pointer_index){};
static float (*AMotionEvent_getButtonStateFunc)(const AInputEvent *motion_event){};

#if ANDROID_MIN_API == 9
CLINK float AMotionEvent_getAxisValue(const AInputEvent* motion_event, int32_t axis, size_t pointer_index)
{
	assumeExpr(AMotionEvent_getAxisValueFunc);
	return AMotionEvent_getAxisValueFunc(motion_event, axis, pointer_index);
}

CLINK int32_t AMotionEvent_getButtonState(const AInputEvent *motion_event)
{
	if(!AMotionEvent_getButtonStateFunc)
		return IG::Input::Pointer::LBUTTON;
	return AMotionEvent_getButtonStateFunc(motion_event);
}
#endif

namespace IG::Input
{

static constexpr uint32_t maxJoystickAxisPairs = 4; // 2 sticks + POV hat + L/R Triggers

// Note: values must remain in sync with Java code
static constexpr int DEVICE_ADDED = 0;
static constexpr int DEVICE_CHANGED = 1;
static constexpr int DEVICE_REMOVED = 2;

static AndroidInputDevice makeGenericKeyDevice()
{
	return {-1, virtualDeviceFlags, "Key Input (All Devices)"};
}

// NAVHIDDEN_* mirrors KEYSHIDDEN_*

static const char *hardKeyboardNavStateToStr(int state)
{
	switch(state)
	{
		case ACONFIGURATION_KEYSHIDDEN_ANY: return "undefined";
		case ACONFIGURATION_KEYSHIDDEN_NO: return "no";
		case ACONFIGURATION_KEYSHIDDEN_YES: return "yes";
		case ACONFIGURATION_KEYSHIDDEN_SOFT:  return "soft";
		default: return "unknown";
	}
}

static const char *inputDeviceKeyboardTypeToStr(int type)
{
	switch(type)
	{
		case AINPUT_KEYBOARD_TYPE_NONE: return "None";
		case AINPUT_KEYBOARD_TYPE_NON_ALPHABETIC: return "Non-Alphabetic";
		case AINPUT_KEYBOARD_TYPE_ALPHABETIC: return "Alphabetic";
	}
	return "Unknown";
}

AndroidInputDevice::AndroidInputDevice(int osId, DeviceTypeFlags typeFlags, std::string name):
	Device{osId, Map::SYSTEM, typeFlags, std::move(name)}
{}

AndroidInputDevice::AndroidInputDevice(JNIEnv* env, jobject aDev,
	int osId, int src, std::string devName, int kbType, AxisFlags jsAxisFlags,
	uint32_t vendorProductId, bool isPowerButton):
	Device{osId, Map::SYSTEM, {.miscKeys = true}, std::move(devName)}
{
	typeFlags_.virtualInput = osId == -1;
	typeFlags_.powerButton = isPowerButton;
	if(src & AINPUT_SOURCE_CLASS_POINTER)
	{
		typeFlags_ .touchscreen = isBitMaskSet(src, AINPUT_SOURCE_TOUCHSCREEN);
		typeFlags_ .mouse = isBitMaskSet(src, AINPUT_SOURCE_MOUSE);
	}
	auto &name = name_;
	if(isBitMaskSet(src, AINPUT_SOURCE_GAMEPAD))
	{
		bool isGamepad = true;
		if(Config::MACHINE_IS_GENERIC_ARMV7 && name.contains("-zeus"))
		{
			logMsg("detected Xperia Play gamepad");
			subtype_ = Device::Subtype::XPERIA_PLAY;
		}
		else if((Config::MACHINE_IS_GENERIC_ARMV7 && name == "sii9234_rcp")
			|| name.contains("MHLRCP" ) || name.contains("Button Jack"))
		{
			// sii9234_rcp on Samsung devices like Galaxy S2, may claim to be a gamepad & full keyboard
			// but has only special function keys
			logMsg("ignoring extra device bits");
			src = 0;
			isGamepad = false;
		}
		updateGamepadSubtype(name, vendorProductId);
		typeFlags_ .gamepad = isGamepad;
	}
	if(kbType)
	{
		// Classify full alphabetic keyboards, and also devices with other keyboard
		// types, as long as they are exclusively SOURCE_KEYBOARD
		// (needed for the iCade 8-bitty since it reports a non-alphabetic keyboard type)
		if(kbType == AINPUT_KEYBOARD_TYPE_ALPHABETIC
			|| src == AINPUT_SOURCE_KEYBOARD)
		{
			typeFlags_ .keyboard = true;
			logMsg("has keyboard type: %s", inputDeviceKeyboardTypeToStr(kbType));
		}
	}
	if(IG::isBitMaskSet(src, (int)AINPUT_SOURCE_JOYSTICK))
	{
		typeFlags_.joystick = true;
		logMsg("detected a joystick");
		if(subtype_ == Subtype::NONE
			&& typeFlags_.gamepad
			&& !typeFlags_.keyboard
			&& !typeFlags_.virtualInput)
		{
			logMsg("device looks like a generic gamepad");
			subtype_ = Device::Subtype::GENERIC_GAMEPAD;
		}
		// check joystick axes
		static constexpr AxisId stickAxes[]{AxisId::X, AxisId::Y, AxisId::Z, AxisId::RX, AxisId::RY, AxisId::RZ,
				AxisId::HAT0X, AxisId::HAT0Y, AxisId::RUDDER, AxisId::WHEEL};
		static constexpr AxisFlags stickAxesBits[]{{.x = true}, {.y = true}, {.z = true}, {.rx = true}, {.ry = true}, {.rz = true},
				{.hatX = true}, {.hatY = true}, {.rudder = true}, {.wheel = true}};
		for(auto &axisId : stickAxes)
		{
			bool hasAxis = asInt(jsAxisFlags & stickAxesBits[std::distance(stickAxes, &axisId)]);
			if(!hasAxis)
			{
				continue;
			}
			logMsg("joystick axis:%d", (int)axisId);
			axis.emplace_back(*this, axisId);
			assert(!axis.isFull());
		}
		// check trigger axes
		static constexpr AxisId triggerAxes[]{AxisId::LTRIGGER, AxisId::RTRIGGER, AxisId::GAS, AxisId::BRAKE};
		static constexpr AxisFlags triggerAxesBits[]{{.lTrigger = true}, {.rTrigger = true}, {.gas = true}, {.brake = true}};
		for(auto &axisId : triggerAxes)
		{
			bool hasAxis = asInt(jsAxisFlags & triggerAxesBits[std::distance(triggerAxes, &axisId)]);
			if(!hasAxis)
			{
				continue;
			}
			logMsg("trigger axis:%d", (int)axisId);
			axis.emplace_back(*this, axisId);
			assert(!axis.isFull());
		}
	}
}

bool AndroidInputDevice::operator ==(AndroidInputDevice const& rhs) const
{
	return id() == rhs.id() && name_ == rhs.name_;
}

std::span<Axis> AndroidInputDevice::motionAxes()
{
	return axis;
}

void AndroidInputDevice::setICadeMode(bool on)
{
	logMsg("set iCade mode %s for %s", on ? "on" : "off", name().data());
	iCadeMode_ = on;
}

bool AndroidInputDevice::iCadeMode() const
{
	return iCadeMode_;
}

void AndroidInputDevice::update(const AndroidInputDevice &other)
{
	name_ = other.name_;
	typeFlags_ = other.typeFlags_;
	subtype_ = other.subtype_;
	axis = other.axis;
}

bool Device::anyTypeFlagsPresent(ApplicationContext ctx, DeviceTypeFlags typeFlags)
{
	auto &app = ctx.application();
	if(typeFlags.keyboard)
	{
		if(app.keyboardType() == ACONFIGURATION_KEYBOARD_QWERTY)
		{
			if(app.hardKeyboardState() == ACONFIGURATION_KEYSHIDDEN_YES || app.hardKeyboardState() == ACONFIGURATION_KEYSHIDDEN_SOFT)
			{
				logDMsg("keyboard present, but not in use");
			}
			else
			{
				//logDMsg("keyboard present");
				return true;
			}
		}
		typeFlags.keyboard = false; // ignore keyboards in device list
	}

	if(Config::MACHINE_IS_GENERIC_ARMV7 && app.hasXperiaPlayGamepad() &&
		typeFlags.gamepad && app.hardKeyboardState() != ACONFIGURATION_KEYSHIDDEN_YES)
	{
		logDMsg("Xperia-play gamepad in use");
		return true;
	}

	for(auto &devPtr : app.inputDevices())
	{
		auto &e = *devPtr;
		if((e.isVirtual() && (typeFlags.miscKeys && e.typeFlags().miscKeys)) // virtual devices count as miscKeys flag only
			|| (!e.isVirtual() && asInt(e.typeFlags() & typeFlags)))
		{
			logDMsg("device:%s has bits:0x%X", e.name().data(), std::bit_cast<uint8_t>(typeFlags));
			return true;
		}
	}
	return false;
}

bool hasGetAxisValue()
{
	if constexpr(Config::ENV_ANDROID_MIN_SDK == 9)
		return AMotionEvent_getAxisValueFunc;
	else
		return true;
}

}

namespace IG
{

int32_t (*AMotionEvent_getActionButton_)(const AInputEvent* motion_event){};

static bool isXperiaPlayDeviceStr(std::string_view str)
{
	return str.contains("R800") || str == "zeus";
}

bool AndroidApplication::hasMultipleInputDeviceSupport() const
{
	return Config::ENV_ANDROID_MIN_SDK >= 12 || processInput_ == &AndroidApplication::processInputWithGetEvent;
}

void AndroidApplication::initInput(ApplicationContext ctx, JNIEnv *env, jobject baseActivity, jclass baseActivityClass, int32_t androidSDK)
{
	if(androidSDK >= 12)
	{
		processInput_ = &AndroidApplication::processInputWithGetEvent;
		if constexpr(Config::ENV_ANDROID_MIN_SDK == 9)
		{
			// load AMotionEvent_getAxisValue dynamically
			if(!loadSymbol(AMotionEvent_getAxisValueFunc, {}, "AMotionEvent_getAxisValue"))
			{
				bug_unreachable("AMotionEvent_getAxisValue not found even though using SDK level >= 12");
			}
			if(androidSDK >= 14)
			{
				if(!loadSymbol(AMotionEvent_getButtonStateFunc, {}, "AMotionEvent_getButtonState"))
				{
					bug_unreachable("AMotionEvent_getButtonState not found even though using SDK level >= 14");
				}
			}
		}
		if(androidSDK >= 33)
		{
			if(!loadSymbol(AMotionEvent_getActionButton_, {}, "AMotionEvent_getActionButton"))
			{
				bug_unreachable("AMotionEvent_getActionButton not found even though using SDK level >= 33");
			}
		}

		jEnumInputDevices = {env, baseActivityClass, "enumInputDevices", "(J)V"};

		// device change notifications
		if(androidSDK >= 16)
		{
			logMsg("setting up input notifications");
			JNI::InstMethod<jobject(jlong)> jInputDeviceListenerHelper{env, baseActivityClass, "inputDeviceListenerHelper", "(J)Lcom/imagine/InputDeviceListenerHelper;"};
			inputDeviceListenerHelper = {env, jInputDeviceListenerHelper(env, baseActivity, jlong(ctx.aNativeActivityPtr()))};
			auto inputDeviceListenerHelperCls = env->GetObjectClass(inputDeviceListenerHelper);
			jRegister = {env, inputDeviceListenerHelperCls, "register", "()V"};
			jUnregister = {env, inputDeviceListenerHelperCls, "unregister", "()V"};
			JNINativeMethod method[]
			{
				{
					"deviceChanged", "(JIILandroid/view/InputDevice;Ljava/lang/String;IIII)V",
					(void*)
					+[](JNIEnv* env, jobject thiz, jlong nUserData, jint change, jint devID, jobject jDev,
						jstring jName, jint src, jint kbType, jint jsAxisFlags, jint vendorProductId)
					{
						ApplicationContext ctx{reinterpret_cast<ANativeActivity*>(nUserData)};
						auto &app = ctx.application();
						if(change == Input::DEVICE_REMOVED)
						{
							app.removeInputDevice(ctx, Input::Map::SYSTEM, devID, true);
						}
						else // add or update existing
						{
							const char *name = env->GetStringUTFChars(jName, nullptr);
							Input::AndroidInputDevice sysDev{env, jDev, devID,
								src, name, kbType, std::bit_cast<Input::AxisFlags>(jsAxisFlags), (uint32_t)vendorProductId, false};
							env->ReleaseStringUTFChars(jName, name);
							app.updateAndroidInputDevice(ctx, std::move(sysDev), true);
						}
					}
				}
			};
			env->RegisterNatives(inputDeviceListenerHelperCls, method, std::size(method));
			addOnResume([this, env](ApplicationContext ctx, bool)
				{
					enumInputDevices(ctx, env, ctx.baseActivityObject(), true);
					logMsg("registering input device listener");
					jRegister(env, inputDeviceListenerHelper);
					return true;
				}, INPUT_DEVICE_ON_RESUME_PRIORITY);
			addOnExit([this, env](ApplicationContext, bool backgrounded)
				{
					logMsg("unregistering input device listener");
					jUnregister(env, inputDeviceListenerHelper);
					return true;
				}, INPUT_DEVICE_ON_EXIT_PRIORITY);
		}
		else
		{
			logMsg("setting up input notifications with inotify");
			inputDevNotifyFd = inotify_init();
			if(inputDevNotifyFd == -1)
			{
				logErr("couldn't create inotify instance");
			}
			else
			{
				int ret = ALooper_addFd(EventLoop::forThread().nativeObject(), inputDevNotifyFd, ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT,
					[](int fd, int events, void* data)
					{
						logMsg("got inotify event");
						auto &app = *((AndroidApplication*)data);
						if(events == POLLEV_IN)
						{
							char buffer[2048];
							auto size = read(fd, buffer, sizeof(buffer));
							if(app.isRunning())
							{
								app.inputRescanCallback->runIn(IG::Milliseconds(250));
							}
						}
						return 1;
					}, this);
				if(ret != 1)
				{
					logErr("couldn't add inotify fd to looper");
				}
				addOnResume([this, env](ApplicationContext ctx, bool)
					{
						inputRescanCallback.emplace("inputRescanCallback",
							[this, ctx]()
							{
								ctx.enumInputDevices();
							});
						enumInputDevices(ctx, env, ctx.baseActivityObject(), true);
						if(inputDevNotifyFd != -1 && watch == -1)
						{
							logMsg("registering inotify input device listener");
							watch = inotify_add_watch(inputDevNotifyFd, "/dev/input", IN_CREATE | IN_DELETE);
							if(watch == -1)
							{
								logErr("error setting inotify watch");
							}
						}
						return true;
					}, INPUT_DEVICE_ON_RESUME_PRIORITY);
				addOnExit([this, env](ApplicationContext, bool backgrounded)
					{
						if(watch != -1)
						{
							logMsg("unregistering inotify input device listener");
							inotify_rm_watch(inputDevNotifyFd, watch);
							watch = -1;
							inputRescanCallback.reset();
						}
						return true;
					}, INPUT_DEVICE_ON_EXIT_PRIORITY);
			}
		}
	}
	else
	{
		// no multi-input device support
		auto genericKeyDev = Input::makeGenericKeyDevice();
		if(Config::MACHINE_IS_GENERIC_ARMV7)
		{
			auto buildDevice = androidBuildDevice(env, baseActivityClass);
			if(isXperiaPlayDeviceStr(buildDevice))
			{
				logMsg("detected Xperia Play gamepad");
				genericKeyDev.setSubtype(Input::Device::Subtype::XPERIA_PLAY);
			}
			else if(buildDevice == "sholes")
			{
				logMsg("detected Droid/Milestone keyboard");
				genericKeyDev.setSubtype(Input::Device::Subtype::MOTO_DROID_KEYBOARD);
			}
		}
		builtinKeyboardDev = addAndroidInputDevice(ctx, std::move(genericKeyDev), false);
	}
}

void AndroidApplication::initInputConfig(AConfiguration *config)
{
	auto hardKeyboardState = AConfiguration_getKeysHidden(config);
	auto navigationState = AConfiguration_getNavHidden(config);
	auto keyboard = AConfiguration_getKeyboard(config);
	trackballNav = AConfiguration_getNavigation(config) == ACONFIGURATION_NAVIGATION_TRACKBALL;
	if(trackballNav)
		logMsg("detected trackball");

	aHardKeyboardState = hasXperiaPlayGamepad() ? navigationState : hardKeyboardState;
	logMsg("keyboard/nav hidden: %s", Input::hardKeyboardNavStateToStr(aHardKeyboardState));

	aKeyboardType = keyboard;
	if(aKeyboardType != ACONFIGURATION_KEYBOARD_NOKEYS)
		logMsg("keyboard type: %d", aKeyboardType);
}

void AndroidApplication::updateInputConfig(ApplicationContext ctx, AConfiguration *config)
{
	auto hardKeyboardState = AConfiguration_getKeysHidden(config);
	auto navState = AConfiguration_getNavHidden(config);
	//trackballNav = AConfiguration_getNavigation(config) == ACONFIGURATION_NAVIGATION_TRACKBALL;
	logMsg("config change, keyboard: %s, navigation: %s", Input::hardKeyboardNavStateToStr(hardKeyboardState), Input::hardKeyboardNavStateToStr(navState));
	setHardKeyboardState(ctx, hasXperiaPlayGamepad() ? navState : hardKeyboardState);
}

void ApplicationContext::enumInputDevices() const
{
	application().enumInputDevices(*this, mainThreadJniEnv(), baseActivityObject(), true);
}

bool ApplicationContext::hasInputDeviceHotSwap() const { return androidSDK() >= 12; }

void AndroidApplication::enumInputDevices(ApplicationContext ctx, JNIEnv* env, jobject baseActivity, bool notify)
{
	logMsg("doing input device scan");
	removeInputDevices(ctx, Input::Map::SYSTEM);
	jEnumInputDevices(env, baseActivity, jlong(ctx.aNativeActivityPtr()));
	if(!virtualDev)
	{
		logMsg("no \"Virtual\" device id found, adding one");
		virtualDev = addAndroidInputDevice(ctx,
			{-1, Input::virtualDeviceFlags, "Virtual"},
			false);
	}
	if(notify) { onEvent(ctx, Input::DevicesEnumeratedEvent{}); }
}

bool AndroidApplication::hasTrackball() const
{
	return trackballNav;
}

bool AndroidApplicationContext::hasTrackball() const
{
	return application().hasTrackball();
}

int AndroidApplication::hardKeyboardState() const { return aHardKeyboardState; }

void AndroidApplication::setHardKeyboardState(ApplicationContext ctx, int hardKeyboardState)
{
	if(aHardKeyboardState != hardKeyboardState)
	{
		aHardKeyboardState = hardKeyboardState;
		logMsg("hard keyboard hidden: %s", Input::hardKeyboardNavStateToStr(aHardKeyboardState));
		if(builtinKeyboardDev)
		{
			bool shown = aHardKeyboardState == ACONFIGURATION_KEYSHIDDEN_NO;
			auto change = shown ? Input::DeviceChange::shown : Input::DeviceChange::hidden;
			onEvent(ctx, Input::DeviceChangeEvent{*builtinKeyboardDev, change});
		}
	}
}

int AndroidApplication::keyboardType() const { return aKeyboardType; }

bool AndroidApplication::hasXperiaPlayGamepad() const
{
	return builtinKeyboardDev && builtinKeyboardDev->subtype() == Input::Device::Subtype::XPERIA_PLAY;
}

Input::AndroidInputDevice *AndroidApplication::addAndroidInputDevice(ApplicationContext ctx, Input::AndroidInputDevice dev, bool notify)
{
	if(Config::DEBUG_BUILD)
	{
		if(inputDeviceForId(dev.id()))
			logWarn("adding duplicate device ID:%d", dev.id());
	}
	logMsg("added device id:%d (%s) to list", dev.id(), dev.name().data());
	return static_cast<Input::AndroidInputDevice*>(&addInputDevice(ctx, std::make_unique<Input::AndroidInputDevice>(std::move(dev)), notify));
}

Input::AndroidInputDevice *AndroidApplication::updateAndroidInputDevice(ApplicationContext ctx, Input::AndroidInputDevice dev, bool notify)
{
	auto existingDevPtr = inputDeviceForId(dev.id());
	if(!existingDevPtr)
	{
		return addAndroidInputDevice(ctx, std::move(dev), notify);
	}
	else
	{
		logMsg("device id:%d (%s) updated", dev.id(), dev.name().data());
		existingDevPtr->update(std::move(dev));
		if(notify)
			dispatchInputDeviceChange(ctx, *existingDevPtr, Input::DeviceChange::updated);
		return existingDevPtr;
	}
}

}
