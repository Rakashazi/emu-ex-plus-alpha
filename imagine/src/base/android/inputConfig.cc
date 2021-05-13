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
#include "input.hh"
#include <android/configuration.h>
#include <sys/inotify.h>
#include <optional>

#ifdef ANDROID_COMPAT_API
static float (*AMotionEvent_getAxisValueFunc)(const AInputEvent* motion_event, int32_t axis, size_t pointer_index){};

CLINK float AMotionEvent_getAxisValue(const AInputEvent* motion_event, int32_t axis, size_t pointer_index)
{
	assumeExpr(AMotionEvent_getAxisValueFunc);
	return AMotionEvent_getAxisValueFunc(motion_event, axis, pointer_index);
}
#endif

namespace Input
{

static constexpr uint32_t maxJoystickAxisPairs = 4; // 2 sticks + POV hat + L/R Triggers

// Note: values must remain in sync with Java code
static constexpr int DEVICE_ADDED = 0;
static constexpr int DEVICE_CHANGED = 1;
static constexpr int DEVICE_REMOVED = 2;

static AndroidInputDevice makeGenericKeyDevice()
{
	return {-1, Device::TYPE_BIT_VIRTUAL | Device::TYPE_BIT_KEYBOARD | Device::TYPE_BIT_KEY_MISC,
		"Key Input (All Devices)"};
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
		case AInputDevice::KEYBOARD_TYPE_NONE: return "None";
		case AInputDevice::KEYBOARD_TYPE_NON_ALPHABETIC: return "Non-Alphabetic";
		case AInputDevice::KEYBOARD_TYPE_ALPHABETIC: return "Alphabetic";
	}
	return "Unknown";
}

AndroidInputDevice::AndroidInputDevice(int osId, uint32_t typeBits, const char *name, uint32_t axisBits):
	Device{0, Map::SYSTEM, typeBits, name},
	osId{osId},
	axisBits{axisBits}
{}

AndroidInputDevice::AndroidInputDevice(JNIEnv* env, jobject aDev, uint32_t enumId,
	int osId, int src, const char *name, int kbType, uint32_t jsAxisBits, bool isPowerButton):
	Device{enumId, Map::SYSTEM, Device::TYPE_BIT_KEY_MISC, name},
	osId{osId}
{
	if(osId == -1)
	{
		type_ |= Device::TYPE_BIT_VIRTUAL;
	}
	if(isPowerButton)
	{
		type_ |= Device::TYPE_BIT_POWER_BUTTON;
	}
	if(src & AInputDevice::SOURCE_CLASS_POINTER)
	{
		if(IG::isBitMaskSet(src, AInputDevice::SOURCE_TOUCHSCREEN))
		{
			type_ |= Device::TYPE_BIT_TOUCHSCREEN;
		}
		if(IG::isBitMaskSet(src, AInputDevice::SOURCE_MOUSE))
		{
			type_ |= Device::TYPE_BIT_MOUSE;
		}
	}
	if(IG::isBitMaskSet(src, AInputDevice::SOURCE_GAMEPAD))
	{
		bool isGamepad = 1;
		if(Config::MACHINE_IS_GENERIC_ARMV7 && strstr(name, "-zeus"))
		{
			logMsg("detected Xperia Play gamepad");
			subtype_ = Device::SUBTYPE_XPERIA_PLAY;
		}
		else if((Config::MACHINE_IS_GENERIC_ARMV7 && string_equal(name, "sii9234_rcp"))
			|| string_equal(name, "MHLRCP" ) || strstr(name, "Button Jack"))
		{
			// sii9234_rcp on Samsung devices like Galaxy S2, may claim to be a gamepad & full keyboard
			// but has only special function keys
			logMsg("ignoring extra device bits");
			src = 0;
			isGamepad = 0;
		}
		else if(string_equal(name, "Sony PLAYSTATION(R)3 Controller"))
		{
			logMsg("detected PS3 gamepad");
			subtype_ = Device::SUBTYPE_PS3_CONTROLLER;
		}
		else if(string_equal(name, "OUYA Game Controller"))
		{
			logMsg("detected OUYA gamepad");
			subtype_ = Device::SUBTYPE_OUYA_CONTROLLER;
		}
		else if(strstr(name, "NVIDIA Controller"))
		{
			logMsg("detected NVidia Shield gamepad");
			subtype_ = Device::SUBTYPE_NVIDIA_SHIELD;
		}
		else if(string_equal(name, "Xbox 360 Wireless Receiver"))
		{
			logMsg("detected wireless 360 gamepad");
			subtype_ = Device::SUBTYPE_XBOX_360_CONTROLLER;
		}
		else if(string_equal(name, "8Bitdo SF30 Pro"))
		{
			logMsg("detected 8Bitdo SF30 Pro");
			subtype_ = Device::SUBTYPE_8BITDO_SF30_PRO;
		}
		else if(string_equal(name, "8BitDo SN30 Pro+"))
		{
			logMsg("detected 8BitDo SN30 Pro+");
			subtype_ = Device::SUBTYPE_8BITDO_SN30_PRO_PLUS;
		}
		else if(string_equal(name, "8BitDo M30 gamepad"))
		{
			logMsg("detected 8BitDo M30 gamepad");
			subtype_ = Device::SUBTYPE_8BITDO_M30_GAMEPAD;
		}
		else
		{
			logMsg("detected a gamepad");
		}
		if(isGamepad)
		{
			type_ |= Device::TYPE_BIT_GAMEPAD;
		}
	}
	if(kbType)
	{
		// Classify full alphabetic keyboards, and also devices with other keyboard
		// types, as long as they are exclusively SOURCE_KEYBOARD
		// (needed for the iCade 8-bitty since it reports a non-alphabetic keyboard type)
		if(kbType == AInputDevice::KEYBOARD_TYPE_ALPHABETIC
			|| src == AInputDevice::SOURCE_KEYBOARD)
		{
			type_ |= Device::TYPE_BIT_KEYBOARD;
			logMsg("has keyboard type: %s", inputDeviceKeyboardTypeToStr(kbType));
		}
	}
	if(IG::isBitMaskSet(src, AInputDevice::SOURCE_JOYSTICK))
	{
		type_ |= Device::TYPE_BIT_JOYSTICK;
		logMsg("detected a joystick");
		if(!subtype_
			&& (type_ & Device::TYPE_BIT_GAMEPAD)
			&& !(type_ & Device::TYPE_BIT_KEYBOARD)
			&& !(type_ & Device::TYPE_BIT_VIRTUAL))
		{
			logMsg("device looks like a generic gamepad");
			subtype_ = Device::SUBTYPE_GENERIC_GAMEPAD;
		}
		axisBits = jsAxisBits;
		// check joystick axes
		{
			constexpr uint8_t stickAxes[]{AXIS_X, AXIS_Y, AXIS_Z, AXIS_RX, AXIS_RY, AXIS_RZ,
					AXIS_HAT_X, AXIS_HAT_Y, AXIS_RUDDER, AXIS_WHEEL};
			constexpr uint32_t stickAxesBits[]{AXIS_BIT_X, AXIS_BIT_Y, AXIS_BIT_Z, AXIS_BIT_RX, AXIS_BIT_RY, AXIS_BIT_RZ,
					AXIS_BIT_HAT_X, AXIS_BIT_HAT_Y, AXIS_BIT_RUDDER, AXIS_BIT_WHEEL};
			uint32_t axisIdx = 0;
			for(auto axisId : stickAxes)
			{
				bool hasAxis = jsAxisBits & stickAxesBits[axisIdx];
				if(!hasAxis)
				{
					axisIdx++;
					continue;
				}
				logMsg("joystick axis: %d", axisId);
				auto size = 2.0f;
				axis.emplace_back(axisId, (AxisKeyEmu<float>){-1.f + size/4.f, 1.f - size/4.f,
					Key(axisToKeycode(axisId)+1), axisToKeycode(axisId), Key(axisToKeycode(axisId)+1), axisToKeycode(axisId)});
				if(axis.isFull())
				{
					logMsg("reached maximum joystick axes");
					break;
				}
				axisIdx++;
			}
		}
		// check trigger axes
		if(!axis.isFull())
		{
			const uint8_t triggerAxes[]{AXIS_LTRIGGER, AXIS_RTRIGGER, AXIS_GAS, AXIS_BRAKE};
			const uint32_t triggerAxesBits[]{AXIS_BIT_LTRIGGER, AXIS_BIT_RTRIGGER, AXIS_BIT_GAS, AXIS_BIT_BRAKE};
			uint32_t axisIdx = 0;
			for(auto axisId : triggerAxes)
			{
				bool hasAxis = jsAxisBits & triggerAxesBits[axisIdx];
				if(!hasAxis)
				{
					axisIdx++;
					continue;
				}
				logMsg("trigger axis: %d", axisId);
				// use unreachable lowLimit value so only highLimit is used
				axis.emplace_back(axisId, (AxisKeyEmu<float>){-1.f, 0.25f,
					0, axisToKeycode(axisId), 0, axisToKeycode(axisId)});
				if(axis.isFull())
				{
					logMsg("reached maximum joystick axes");
					break;
				}
			}
		}
		joystickAxisAsDpadBitsDefault_ = axisBits & (AXIS_BITS_STICK_1 | AXIS_BITS_HAT); // default left analog and POV hat as dpad
		setJoystickAxisAsDpadBits(joystickAxisAsDpadBitsDefault_); // default left analog and POV hat as dpad
	}
}

void AndroidInputDevice::setJoystickAxisAsDpadBitsDefault(uint32_t axisMask)
{
	joystickAxisAsDpadBitsDefault_ = axisMask;
}

void AndroidInputDevice::setJoystickAxisAsDpadBits(uint32_t axisMask)
{
	if(!axis.size() || joystickAxisAsDpadBits_ == axisMask)
		return;
	joystickAxisAsDpadBits_ = axisMask;
	logMsg("remapping joystick axes for device: %d", osId);
	for(auto &e : axis)
	{
		switch(e.id)
		{
			bcase AXIS_X:
			{
				bool on = axisMask & AXIS_BIT_X;
				//logMsg("axis x as dpad: %s", on ? "yes" : "no");
				e.keyEmu.lowKey = e.keyEmu.lowSysKey = on ? Keycode::LEFT : Keycode::JS1_XAXIS_NEG;
				e.keyEmu.highKey = e.keyEmu.highSysKey = on ? Keycode::RIGHT : Keycode::JS1_XAXIS_POS;
			}
			bcase AXIS_Y:
			{
				bool on = axisMask & AXIS_BIT_Y;
				//logMsg("axis y as dpad: %s", on ? "yes" : "no");
				e.keyEmu.lowKey = e.keyEmu.lowSysKey = on ? Keycode::UP : Keycode::JS1_YAXIS_NEG;
				e.keyEmu.highKey = e.keyEmu.highSysKey = on ? Keycode::DOWN : Keycode::JS1_YAXIS_POS;
			}
			bcase AXIS_Z:
			{
				bool on = axisMask & AXIS_BIT_Z;
				//logMsg("axis z as dpad: %s", on ? "yes" : "no");
				e.keyEmu.lowKey = e.keyEmu.lowSysKey = on ? Keycode::LEFT : Keycode::JS2_XAXIS_NEG;
				e.keyEmu.highKey = e.keyEmu.highSysKey = on ? Keycode::RIGHT : Keycode::JS2_XAXIS_POS;
			}
			bcase AXIS_RZ:
			{
				bool on = axisMask & AXIS_BIT_RZ;
				//logMsg("axis rz as dpad: %s", on ? "yes" : "no");
				e.keyEmu.lowKey = e.keyEmu.lowSysKey = on ? Keycode::UP : Keycode::JS2_YAXIS_NEG;
				e.keyEmu.highKey = e.keyEmu.highSysKey = on ? Keycode::DOWN : Keycode::JS2_YAXIS_POS;
			}
			bcase AXIS_HAT_X:
			{
				bool on = axisMask & AXIS_BIT_HAT_X;
				//logMsg("axis hat x as dpad: %s", on ? "yes" : "no");
				e.keyEmu.lowKey = e.keyEmu.lowSysKey = on ? Keycode::LEFT : Keycode::JS_POV_XAXIS_NEG;
				e.keyEmu.highKey = e.keyEmu.highSysKey = on ? Keycode::RIGHT : Keycode::JS_POV_XAXIS_POS;
			}
			bcase AXIS_HAT_Y:
			{
				bool on = axisMask & AXIS_BIT_HAT_Y;
				//logMsg("axis hat y as dpad: %s", on ? "yes" : "no");
				e.keyEmu.lowKey = e.keyEmu.lowSysKey = on ? Keycode::UP : Keycode::JS_POV_YAXIS_NEG;
				e.keyEmu.highKey = e.keyEmu.highSysKey = on ? Keycode::DOWN : Keycode::JS_POV_YAXIS_POS;
			}
		}
	}
}

uint32_t AndroidInputDevice::joystickAxisAsDpadBits()
{
	return joystickAxisAsDpadBits_;
}

bool AndroidInputDevice::operator ==(AndroidInputDevice const& rhs) const
{
	return osId == rhs.osId && string_equal(name(), rhs.name());
}

void AndroidInputDevice::setTypeBits(int bits) { type_ = bits; }

uint32_t AndroidInputDevice::joystickAxisAsDpadBitsDefault() { return joystickAxisAsDpadBitsDefault_; };

uint32_t AndroidInputDevice::joystickAxisBits() { return axisBits; };

void AndroidInputDevice::setICadeMode(bool on)
{
	logMsg("set iCade mode %s for %s", on ? "on" : "off", name());
	iCadeMode_ = on;
}

bool AndroidInputDevice::iCadeMode() const
{
	return iCadeMode_;
}

bool Device::anyTypeBitsPresent(Base::ApplicationContext ctx, uint32_t typeBits)
{
	auto &app = ctx.application();
	if(typeBits & TYPE_BIT_KEYBOARD)
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
		typeBits = IG::clearBits(typeBits, TYPE_BIT_KEYBOARD); // ignore keyboards in device list
	}

	if(Config::MACHINE_IS_GENERIC_ARMV7 && app.hasXperiaPlayGamepad() &&
		(typeBits & TYPE_BIT_GAMEPAD) && app.hardKeyboardState() != ACONFIGURATION_KEYSHIDDEN_YES)
	{
		logDMsg("Xperia-play gamepad in use");
		return true;
	}

	for(auto &devPtr : app.systemInputDevices())
	{
		auto &e = *devPtr;
		if((e.isVirtual() && ((typeBits & TYPE_BIT_KEY_MISC) & e.typeBits())) // virtual devices count as TYPE_BIT_KEY_MISC only
				|| (!e.isVirtual() && (e.typeBits() & typeBits)))
		{
			logDMsg("device idx %d has bits 0x%X", e.idx, typeBits);
			return true;
		}
	}
	return false;
}

bool hasGetAxisValue()
{
	#ifdef ANDROID_COMPAT_API
	return AMotionEvent_getAxisValueFunc;
	#else
	return true;
	#endif
}

}

namespace Base
{

static bool isXperiaPlayDeviceStr(const char *str)
{
	return strstr(str, "R800") || string_equal(str, "zeus");
}

void AndroidApplication::initInput(JNIEnv *env, jobject baseActivity, jclass baseActivityClass, int32_t androidSDK)
{
	processInput_ = androidSDK >= 12 ? &AndroidApplication::processInputWithGetEvent : &AndroidApplication::processInputWithHasEvents;
	if(androidSDK >= 12)
	{
		#ifdef ANDROID_COMPAT_API
		// load AMotionEvent_getAxisValue dynamically
		if(!Base::loadSymbol(AMotionEvent_getAxisValueFunc, {}, "AMotionEvent_getAxisValue"))
		{
			logWarn("AMotionEvent_getAxisValue not found even though using SDK level >= 12");
		}
		#endif

		jEnumInputDevices = {env, baseActivityClass, "enumInputDevices", "(J)V"};

		// device change notifications
		if(androidSDK >= 16)
		{
			logMsg("setting up input notifications");
			JNI::InstMethod<jobject(jlong)> jInputDeviceListenerHelper{env, baseActivityClass, "inputDeviceListenerHelper", "(J)Lcom/imagine/InputDeviceListenerHelper;"};
			inputDeviceListenerHelper = {env, jInputDeviceListenerHelper(env, baseActivity, (jlong)this)};
			auto inputDeviceListenerHelperCls = env->GetObjectClass(inputDeviceListenerHelper);
			jRegister = {env, inputDeviceListenerHelperCls, "register", "()V"};
			jUnregister = {env, inputDeviceListenerHelperCls, "unregister", "()V"};
			JNINativeMethod method[]
			{
				{
					"deviceChanged", "(JIILandroid/view/InputDevice;Ljava/lang/String;III)V",
					(void*)(void (*)(JNIEnv*, jobject, jlong, jint, jint, jobject, jstring, jint, jint, jint))
					([](JNIEnv* env, jobject thiz, jlong nUserData, jint change, jint devID, jobject jDev,
							jstring jName, jint src, jint kbType, jint jsAxisBits)
					{
						auto &app = *((AndroidApplication*)nUserData);
						if(change == Input::DEVICE_REMOVED)
						{
							app.removeInputDevice(devID, true);
						}
						else // add or update existing
						{
							const char *name = env->GetStringUTFChars(jName, nullptr);
							Input::AndroidInputDevice sysDev{env, jDev, app.nextInputDeviceEnumId(name, devID), devID,
								src, name, kbType, (uint32_t)jsAxisBits, false};
							env->ReleaseStringUTFChars(jName, name);
							app.addInputDevice(sysDev, change == Input::DEVICE_CHANGED, true);
						}
					})
				}
			};
			env->RegisterNatives(inputDeviceListenerHelperCls, method, std::size(method));
			addOnResume([this, env](Base::ApplicationContext ctx, bool)
				{
					enumInputDevices(env, ctx.baseActivityObject(), true);
					logMsg("registering input device listener");
					jRegister(env, inputDeviceListenerHelper);
					return true;
				}, Base::INPUT_DEVICE_ON_RESUME_PRIORITY);
			addOnExit([this, env](Base::ApplicationContext, bool backgrounded)
				{
					logMsg("unregistering input device listener");
					jUnregister(env, inputDeviceListenerHelper);
					return true;
				}, Base::INPUT_DEVICE_ON_EXIT_PRIORITY);
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
				int ret = ALooper_addFd(Base::EventLoop::forThread().nativeObject(), inputDevNotifyFd, ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT,
					[](int fd, int events, void* data)
					{
						logMsg("got inotify event");
						auto &app = *((AndroidApplication*)data);
						if(events == Base::POLLEV_IN)
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
				addOnResume([this, env](Base::ApplicationContext ctx, bool)
					{
						inputRescanCallback.emplace("inputRescanCallback",
							[this, ctx]()
							{
								IG::copySelf(ctx).enumInputDevices();
							});
						enumInputDevices(env, ctx.baseActivityObject(), true);
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
					}, Base::INPUT_DEVICE_ON_RESUME_PRIORITY);
				addOnExit([this, env](Base::ApplicationContext, bool backgrounded)
					{
						if(watch != -1)
						{
							logMsg("unregistering inotify input device listener");
							inotify_rm_watch(inputDevNotifyFd, watch);
							watch = -1;
							inputRescanCallback.reset();
						}
						return true;
					}, Base::INPUT_DEVICE_ON_EXIT_PRIORITY);
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
			if(isXperiaPlayDeviceStr(buildDevice.data()))
			{
				logMsg("detected Xperia Play gamepad");
				genericKeyDev.subtype_ = Input::Device::SUBTYPE_XPERIA_PLAY;
			}
			else if(string_equal(buildDevice.data(), "sholes"))
			{
				logMsg("detected Droid/Milestone keyboard");
				genericKeyDev.subtype_ = Input::Device::SUBTYPE_MOTO_DROID_KEYBOARD;
			}
		}
		sysInputDev.reserve(1);
		builtinKeyboardDev = addInputDevice(genericKeyDev, false, false);
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

void AndroidApplication::updateInputConfig(AConfiguration *config)
{
	auto hardKeyboardState = AConfiguration_getKeysHidden(config);
	auto navState = AConfiguration_getNavHidden(config);
	auto keyboard = AConfiguration_getKeyboard(config);
	//trackballNav = AConfiguration_getNavigation(config) == ACONFIGURATION_NAVIGATION_TRACKBALL;
	logMsg("config change, keyboard: %s, navigation: %s", Input::hardKeyboardNavStateToStr(hardKeyboardState), Input::hardKeyboardNavStateToStr(navState));
	setHardKeyboardState(hasXperiaPlayGamepad() ? navState : hardKeyboardState);
}

void AndroidApplicationContext::enumInputDevices()
{
	application().enumInputDevices(mainThreadJniEnv(), baseActivityObject(), true);
}

void AndroidApplication::enumInputDevices(JNIEnv* env, jobject baseActivity, bool notify)
{
	logMsg("doing input device scan");
	while(sysInputDev.size())
	{
		removeSystemInputDevice(*sysInputDev.back());
		sysInputDev.pop_back();
	}
	jEnumInputDevices(env, baseActivity, (jlong)this);
	if(!virtualDev)
	{
		logMsg("no \"Virtual\" device id found, adding one");
		Input::AndroidInputDevice vDev{-1, Input::Device::TYPE_BIT_VIRTUAL | Input::Device::TYPE_BIT_KEYBOARD | Input::Device::TYPE_BIT_KEY_MISC, "Virtual"};
		virtualDev = addInputDevice(vDev, false, false);
	}
	if(notify)
	{
		onInputDevicesEnumerated.callCopySafe();
	}
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

void AndroidApplication::setHardKeyboardState(int hardKeyboardState)
{
	if(aHardKeyboardState != hardKeyboardState)
	{
		aHardKeyboardState = hardKeyboardState;
		logMsg("hard keyboard hidden: %s", Input::hardKeyboardNavStateToStr(aHardKeyboardState));
		if(builtinKeyboardDev)
		{
			bool shown = aHardKeyboardState == ACONFIGURATION_KEYSHIDDEN_NO;
			Input::DeviceAction change{shown ? Input::DeviceAction::SHOWN : Input::DeviceAction::HIDDEN};
			dispatchInputDeviceChange(*builtinKeyboardDev, change);
		}
	}
}

int AndroidApplication::keyboardType() const { return aKeyboardType; }

bool AndroidApplication::hasXperiaPlayGamepad() const
{
	return builtinKeyboardDev && builtinKeyboardDev->subtype() == Input::Device::SUBTYPE_XPERIA_PLAY;
}

void AndroidApplication::setEventsUseOSInputMethod(bool on)
{
	logMsg("set IME use %s", on ? "On" : "Off");
	sendInputToIME = on;
}

bool AndroidApplication::eventsUseOSInputMethod() const
{
	return sendInputToIME;
}


Input::AndroidInputDevice *AndroidApplication::addInputDevice(Input::AndroidInputDevice dev, bool updateExisting, bool notify)
{
	int id = dev.systemId();
	auto existingIt = std::find_if(sysInputDev.cbegin(), sysInputDev.cend(),
		[=](const auto &e) { return e->systemId() == id; });
	if(existingIt == sysInputDev.end())
	{
		auto &devPtr = sysInputDev.emplace_back(std::make_unique<Input::AndroidInputDevice>(std::move(dev)));
		logMsg("added device id:%d (%s) to list", id, dev.name());
		addSystemInputDevice(*devPtr, notify);
		return devPtr.get();
	}
	else
	{
		if(!updateExisting)
		{
			logMsg("device id:%d (%s) already in list", id, dev.name());
			return {};
		}
		else
		{
			logMsg("device id:%d (%s) updated", id, dev.name());
			auto devPtr = existingIt->get();
			*devPtr = dev;
			if(notify)
				dispatchInputDeviceChange(*devPtr, {Input::DeviceAction::CHANGED});
			return devPtr;
		}
	}
}

bool AndroidApplication::removeInputDevice(int id, bool notify)
{
	if(auto removedDev = IG::moveOutIf(sysInputDev, [&](const auto &e){ return e->systemId() == id; });
		removedDev)
	{
		logMsg("removed device id:%d from list", id);
		removeSystemInputDevice(*removedDev, notify);
		return true;
	}
	logMsg("device id:%d not in list", id);
	return false;
}

uint32_t AndroidApplication::nextInputDeviceEnumId(const char *name, int devID)
{
	uint32_t enumID = 0;
	// find the next available ID number for devices with this name, starting from 0
	for(auto &e : sysInputDev)
	{
		if(!string_equal(e->name(), name))
			continue;
		if(e->systemId() == devID) // device already has an enumID
			return e->enumId();
		if(e->enumId() == enumID)
			enumID++;
	}
	return enumID;
}

}
