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
#include <sys/inotify.h>
#include <imagine/base/Base.hh>
#include <imagine/base/Timer.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/fd-utils.h>
#include "internal.hh"
#include "android.hh"
#include "../../input/private.hh"
#include "AndroidInputDevice.hh"

namespace Input
{

static int aHardKeyboardState = 0, aKeyboardType = ACONFIGURATION_KEYBOARD_NOKEYS, aHasHardKeyboard = 0;
static bool trackballNav = false;
bool handleVolumeKeys = false;
bool allowOSKeyRepeats = true;
bool sendInputToIME = false;
static constexpr uint maxJoystickAxisPairs = 4; // 2 sticks + POV hat + L/R Triggers
static Base::Timer inputRescanCallback;
static Device *builtinKeyboardDev{};
StaticArrayList<AndroidInputDevice*, maxSysInputDevs> sysInputDev;
Device *virtualDev{};
AndroidInputDevice genericKeyDev
{
	-1,
	Device::TYPE_BIT_VIRTUAL | Device::TYPE_BIT_KEYBOARD | Device::TYPE_BIT_KEY_MISC,
	"Key Input (All Devices)"
};
jclass AInputDeviceJ::cls = nullptr;
JavaClassMethod<jobject> AInputDeviceJ::getDeviceIds_, AInputDeviceJ::getDevice_;
JavaInstMethod<jobject> AInputDeviceJ::getName_, AInputDeviceJ::getKeyCharacterMap_, AInputDeviceJ::getMotionRange_, AInputDeviceJ::getDescriptor_;
JavaInstMethod<jint> AInputDeviceJ::getSources_, AInputDeviceJ::getKeyboardType_;

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

bool hasHardKeyboard() { return aHasHardKeyboard; }
int hardKeyboardState() { return aHardKeyboardState; }

static void setHardKeyboardState(int hardKeyboardState)
{
	if(aHardKeyboardState != hardKeyboardState)
	{
		aHardKeyboardState = hardKeyboardState;
		logMsg("hard keyboard hidden: %s", hardKeyboardNavStateToStr(aHardKeyboardState));
		if(builtinKeyboardDev)
		{
			bool shown = aHardKeyboardState == ACONFIGURATION_KEYSHIDDEN_NO;
			Device::Change change{shown ? Device::Change::SHOWN : Device::Change::HIDDEN};
			onDeviceChange.callCopySafe(*builtinKeyboardDev, change);
		}
	}
}

int keyboardType()
{
	return aKeyboardType;
}

bool hasTrackball()
{
	return trackballNav;
}

bool hasXperiaPlayGamepad()
{
	return builtinKeyboardDev && builtinKeyboardDev->subtype() == Device::SUBTYPE_XPERIA_PLAY;
}

void initInputConfig(AConfiguration *config)
{
	auto hardKeyboardState = AConfiguration_getKeysHidden(config);
	auto navigationState = AConfiguration_getNavHidden(config);
	auto keyboard = AConfiguration_getKeyboard(config);
	trackballNav = AConfiguration_getNavigation(config) == ACONFIGURATION_NAVIGATION_TRACKBALL;
	if(trackballNav)
		logMsg("detected trackball");

	aHardKeyboardState = hasXperiaPlayGamepad() ? navigationState : hardKeyboardState;
	logMsg("keyboard/nav hidden: %s", hardKeyboardNavStateToStr(aHardKeyboardState));

	aKeyboardType = keyboard;
	if(aKeyboardType != ACONFIGURATION_KEYBOARD_NOKEYS)
		logMsg("keyboard type: %d", aKeyboardType);
}

void changeInputConfig(AConfiguration *config)
{
	auto hardKeyboardState = AConfiguration_getKeysHidden(config);
	auto navState = AConfiguration_getNavHidden(config);
	auto keyboard = AConfiguration_getKeyboard(config);
	//trackballNav = AConfiguration_getNavigation(config) == ACONFIGURATION_NAVIGATION_TRACKBALL;
	logMsg("config change, keyboard: %s, navigation: %s", hardKeyboardNavStateToStr(hardKeyboardState), hardKeyboardNavStateToStr(navState));
	setHardKeyboardState(hasXperiaPlayGamepad() ? navState : hardKeyboardState);
}

static const char *androidEventEnumToStr(uint e)
{
	switch(e)
	{
		case AMOTION_EVENT_ACTION_DOWN: return "Down";
		case AMOTION_EVENT_ACTION_UP: return "Up";
		case AMOTION_EVENT_ACTION_MOVE: return "Move";
		case AMOTION_EVENT_ACTION_CANCEL: return "Cancel";
		case AMOTION_EVENT_ACTION_POINTER_DOWN: return "PDown";
		case AMOTION_EVENT_ACTION_POINTER_UP: return "PUp";
	}
	return "Unknown";
}

void setKeyRepeat(bool on)
{
	// always accept repeats on Android 3.1+ because 2+ devices pushing
	// the same button is considered a repeat by the OS
	if(Base::androidSDK() < 12)
	{
		logMsg("set key repeat %s", on ? "On" : "Off");
		allowOSKeyRepeats = on;
	}
	setAllowKeyRepeats(on);
}

void setHandleVolumeKeys(bool on)
{
	logMsg("set volume key use %s", on ? "On" : "Off");
	handleVolumeKeys = on;
}

void setEventsUseOSInputMethod(bool on)
{
	logMsg("set IME use %s", on ? "On" : "Off");
	sendInputToIME = on;
}

bool eventsUseOSInputMethod()
{
	return sendInputToIME;
}

static const char *inputDeviceKeyboardTypeToStr(int type)
{
	switch(type)
	{
		case AInputDeviceJ::KEYBOARD_TYPE_NONE: return "None";
		case AInputDeviceJ::KEYBOARD_TYPE_NON_ALPHABETIC: return "Non-Alphabetic";
		case AInputDeviceJ::KEYBOARD_TYPE_ALPHABETIC: return "Alphabetic";
	}
	return "Unknown";
}

AndroidInputDevice::AndroidInputDevice(JNIEnv* env, AInputDeviceJ aDev, uint enumId, int osId, int src, const char *name):
	Device{enumId, Event::MAP_SYSTEM, Device::TYPE_BIT_KEY_MISC, nameStr},
	osId{osId}
{
	string_copy(nameStr, name);
	if(osId == -1)
	{
		type_ |= Device::TYPE_BIT_VIRTUAL;
	}
	if(bit_isMaskSet(src, AInputDeviceJ::SOURCE_GAMEPAD))
	{
		bool isGamepad = 1;
		if(Config::MACHINE_IS_GENERIC_ARMV7 && strstr(name, "-zeus"))
		{
			logMsg("detected Xperia Play gamepad");
			subtype_ = Device::SUBTYPE_XPERIA_PLAY;
		}
		else if(Config::MACHINE_IS_GENERIC_ARMV7 &&
			(string_equal(name, "sii9234_rcp") || string_equal(name, "MHLRCP" ) || strstr(name, "Button Jack")))
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
		else if(Config::MACHINE_IS_GENERIC_ARMV7 && strstr(name, "NVIDIA Controller"))
		{
			logMsg("detected NVidia Shield gamepad");
			subtype_ = Device::SUBTYPE_NVIDIA_SHIELD;
		}
		else if(string_equal(name, "Xbox 360 Wireless Receiver"))
		{
			logMsg("detected wireless 360 gamepad");
			subtype_ = Device::SUBTYPE_XBOX_360_CONTROLLER;
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
	if(bit_isMaskSet(src, AInputDeviceJ::SOURCE_KEYBOARD))
	{
		auto kbType = aDev.getKeyboardType(env);
		// Classify full alphabetic keyboards, and also devices with other keyboard
		// types, as long as they are exclusively SOURCE_KEYBOARD
		// (needed for the iCade 8-bitty since it reports a non-alphabetic keyboard type)
		if(kbType == AInputDeviceJ::KEYBOARD_TYPE_ALPHABETIC
			|| src == AInputDeviceJ::SOURCE_KEYBOARD)
		{
			type_ |= Device::TYPE_BIT_KEYBOARD;
			logMsg("has keyboard type: %s", inputDeviceKeyboardTypeToStr(kbType));
		}
	}
	if(bit_isMaskSet(src, AInputDeviceJ::SOURCE_JOYSTICK))
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

		// check joystick axes
		{
			const uint8 stickAxes[] { AXIS_X, AXIS_Y, AXIS_Z, AXIS_RX, AXIS_RY, AXIS_RZ,
					AXIS_HAT_X, AXIS_HAT_Y, AXIS_RUDDER, AXIS_WHEEL };
			const uint stickAxesBits[] { AXIS_BIT_X, AXIS_BIT_Y, AXIS_BIT_Z, AXIS_BIT_RX, AXIS_BIT_RY, AXIS_BIT_RZ,
					AXIS_BIT_HAT_X, AXIS_BIT_HAT_Y, AXIS_BIT_RUDDER, AXIS_BIT_WHEEL };
			uint axisIdx = 0;
			for(auto axisId : stickAxes)
			{
				auto range = aDev.getMotionRange(env, axisId);
				if(!range)
				{
					axisIdx++;
					continue;
				}
				env->DeleteLocalRef(range);
				logMsg("joystick axis: %d", axisId);
				auto size = 2.0f;
				axis.emplace_back(axisId, (AxisKeyEmu<float>){-1.f + size/4.f, 1.f - size/4.f,
					Key(axisToKeycode(axisId)+1), axisToKeycode(axisId), Key(axisToKeycode(axisId)+1), axisToKeycode(axisId)});
				axisBits |= stickAxesBits[axisIdx];
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
			const uint8 triggerAxes[] { AXIS_LTRIGGER, AXIS_RTRIGGER, AXIS_GAS, AXIS_BRAKE };
			for(auto axisId : triggerAxes)
			{
				auto range = aDev.getMotionRange(env, axisId);
				if(!range)
					continue;
				env->DeleteLocalRef(range);
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
		/*iterateTimes(48, i)
		{
			auto range = dev.getMotionRange(jEnv(), i);
			if(!range)
			{
				continue;
			}
			jEnv()->DeleteLocalRef(range);
			logMsg("has axis: %d", i);
		}*/
	}
}

void AndroidInputDevice::setJoystickAxisAsDpadBits(uint axisMask)
{
	if(Base::androidSDK() >= 12 && joystickAxisAsDpadBits_ != axisMask)
	{
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
}

uint AndroidInputDevice::joystickAxisAsDpadBits()
{
	return joystickAxisAsDpadBits_;
}

bool Device::anyTypeBitsPresent(uint typeBits)
{
	if(typeBits & TYPE_BIT_KEYBOARD)
	{
		if(keyboardType() == ACONFIGURATION_KEYBOARD_QWERTY)
		{
			if(hardKeyboardState() == ACONFIGURATION_KEYSHIDDEN_YES || hardKeyboardState() == ACONFIGURATION_KEYSHIDDEN_SOFT)
			{
				logMsg("keyboard present, but not in use");
			}
			else
			{
				logMsg("keyboard present");
				return true;
			}
		}
		unsetBits(typeBits, TYPE_BIT_KEYBOARD); // ignore keyboards in device list
	}

	if(Config::MACHINE_IS_GENERIC_ARMV7 && hasXperiaPlayGamepad() &&
		(typeBits & TYPE_BIT_GAMEPAD) && hardKeyboardState() != ACONFIGURATION_KEYSHIDDEN_YES)
	{
		logMsg("Xperia-play gamepad in use");
		return true;
	}

	for(auto &devPtr : devList)
	{
		auto &e = *devPtr;
		if((e.isVirtual() && ((typeBits & TYPE_BIT_KEY_MISC) & e.typeBits())) // virtual devices count as TYPE_BIT_KEY_MISC only
				|| (!e.isVirtual() && (e.typeBits() & typeBits)))
		{
			logMsg("device idx %d has bits 0x%X", e.idx, typeBits);
			return true;
		}
	}
	return false;
}

static bool isUsefulDevice(const char *name)
{
	// skip various devices that don't have useful functions
	if(strstr(name, "pwrkey") || strstr(name, "pwrbutton")) // various power keys
	{
		return false;
	}
	return true;
}

static uint nextEnumID(const char *name)
{
	uint enumID = 0;
	// find the next available ID number for devices with this name, starting from 0
	for(auto &e : sysInputDev)
	{
		if(string_equal(e->name(), name) && e->enumId() == enumID)
			enumID++;
	}
	return enumID;
}

static bool deviceIDPresent(int id)
{
	for(auto &e : sysInputDev)
	{
		if(e->osId == id)
		{
			logMsg("device ID %d already present", id);
			return true;
		}
	}
	return false;
}

static void processDevice(JNIEnv* env, int devID, bool setSpecialDevices, bool notify)
{
	auto dev = AInputDeviceJ::getDevice(env, devID);
	jstring jName = dev.getName(env);
	if(!jName)
	{
		logWarn("no name from device id %d", devID);
		env->DeleteLocalRef(dev);
		return;
	}
	const char *name = env->GetStringUTFChars(jName, nullptr);
	jint src = dev.getSources(env);
	bool hasKeys = src & AInputDeviceJ::SOURCE_CLASS_BUTTON;
	logMsg("%d: %s, source %X", devID, name, src);
	if(hasKeys && !devList.isFull() && !sysInputDev.isFull() && isUsefulDevice(name))
	{
		auto sysInput = new AndroidInputDevice(env, dev, nextEnumID(name), devID, src, name);
		sysInputDev.push_back(sysInput);
		addDevice(*sysInput);
		if(setSpecialDevices)
		{
			if(devID == 0) // built-in keyboard is always id 0 according to Android docs
			{
				builtinKeyboardDev = sysInput;
			}
			else if(devID == -1)
			{
				virtualDev = sysInput;
			}
		}
		logMsg("added to list with device id %d", sysInput->enumId());
		if(notify)
		{
			onDeviceChange.callCopySafe(*sysInput, { Device::Change::ADDED });
		}
	}
	env->ReleaseStringUTFChars(jName, name);
	env->DeleteLocalRef(dev);
}

void devicesChanged(JNIEnv* env)
{
	logMsg("rescanning all devices for changes");
	using namespace Base;
	auto jID = AInputDeviceJ::getDeviceIds(env);
	auto id = env->GetIntArrayElements(jID, 0);
	auto ids = env->GetArrayLength(jID);

	// check for and remove device IDs no longer present
	forEachInContainer(sysInputDev, it)
	{
		bool found = false;
		auto dev = *it;
		if(dev->osId == -1)
			continue; // skip the Virtual device
		iterateTimes(ids, i)
		{
			if(dev->osId == id[i])
			{
				found = true;
				break;
			}
		}
		if(!found)
		{
			auto removedDev = *dev;
			removeDevice(*dev);
			it.erase();
			onDeviceChange.callCopySafe(removedDev, { Device::Change::REMOVED });
			delete dev;
		}
	}

	// add new devices
	iterateTimes(ids, i)
	{
		if(deviceIDPresent(id[i]))
			continue;
		processDevice(env, id[i], false, true);
	}

	env->ReleaseIntArrayElements(jID, id, 0);
}

void devicesChanged()
{
	devicesChanged(Base::jEnv());
}

static void setupDevices(JNIEnv* env)
{
	using namespace Base;
	auto jID = AInputDeviceJ::getDeviceIds(env);
	auto id = env->GetIntArrayElements(jID, 0);
	logMsg("scanning input devices");
	iterateTimes(env->GetArrayLength(jID), i)
	{
		auto devID = id[i];
		processDevice(env, devID, true, false);
	}
	env->ReleaseIntArrayElements(jID, id, 0);

	if(!virtualDev)
	{
		if(sysInputDev.isFull() || devList.isFull())
		{
			// remove last device to make room
			auto lastDev = &sysInputDev.back();
			devList.remove(*lastDev);
			delete lastDev;
			sysInputDev.pop_back();
		}
		logMsg("no \"Virtual\" device id found, adding one");
		auto sysInput = new AndroidInputDevice(-1, Device::TYPE_BIT_VIRTUAL | Device::TYPE_BIT_KEYBOARD | Device::TYPE_BIT_KEY_MISC, "Virtual");
		sysInputDev.push_back(sysInput);
		addDevice(*sysInput);
		virtualDev = sysInput;
	}
}

CallResult init()
{
	if(Base::androidSDK() >= 12)
	{
		auto env = Base::jEnv();
		processInput = processInputWithGetEvent;

		// device change notifications
		if(Base::androidSDK() >= 16)
		{
			logMsg("setting up input notifications");
			JavaInstMethod<jobject> jInputDeviceListenerHelper;
			jInputDeviceListenerHelper.setup(env, Base::jBaseActivityCls, "inputDeviceListenerHelper", "()Lcom/imagine/InputDeviceListenerHelper;");
			auto inputDeviceListenerHelper = jInputDeviceListenerHelper(env, Base::jBaseActivity);
			assert(inputDeviceListenerHelper);
			auto inputDeviceListenerHelperCls = env->GetObjectClass(inputDeviceListenerHelper);
			JNINativeMethod method[] =
			{
				{
					"deviceChange", "(II)V",
					(void*)(void JNICALL(*)(JNIEnv*, jobject, jint, jint))
					([](JNIEnv* env, jobject thiz, jint devID, jint change)
					{
						switch(change)
						{
							bcase 0:
								if(deviceIDPresent(devID))
								{
									logMsg("device %d already in device list", devID);
									break;
								}
								processDevice(env, devID, false, true);
							bcase 2:
								logMsg("device %d removed", devID);
								forEachInContainer(sysInputDev, it)
								{
									auto dev = *it;
									if(dev->osId == devID)
									{
										auto removedDev = *dev;
										removeDevice(*dev);
										it.erase();
										onDeviceChange.callCopySafe(removedDev, { Device::Change::REMOVED });
										break;
										delete dev;
									}
								}
						}
					})
				}
			};
			env->RegisterNatives(inputDeviceListenerHelperCls, method, sizeofArray(method));
		}
		else
		{
			logMsg("setting up input notifications with inotify");
			int inputDevNotifyFd = inotify_init();
			if(inputDevNotifyFd >= 0)
			{
				auto watch = inotify_add_watch(inputDevNotifyFd, "/dev/input", IN_CREATE | IN_DELETE );
				int ret = ALooper_addFd(Base::activityLooper(), inputDevNotifyFd, ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT,
					[](int fd, int events, void* data)
					{
						logMsg("got inotify event");
						if(events == Base::POLLEV_IN)
						{
							char buffer[2048];
							auto size = read(fd, buffer, sizeof(buffer));
							inputRescanCallback.callbackAfterMSec(
								[]()
								{
									devicesChanged(Base::jEnv());
								}, 250);
						}
						return 1;
					}, nullptr);
				assert(ret == 1);
			}
			else
			{
				logErr("couldn't create inotify instance");
			}
		}

		AInputDeviceJ::jniInit(env);
		setupDevices(env);
	}
	else
	{
		// no multi-input device support
		if(Config::MACHINE_IS_GENERIC_ARMV7)
		{
			if(Base::isXperiaPlayDeviceStr(Base::androidBuildDevice()))
			{
				logMsg("detected Xperia Play gamepad");
				genericKeyDev.subtype_ = Device::SUBTYPE_XPERIA_PLAY;
			}
			else if(string_equal(Base::androidBuildDevice(), "sholes"))
			{
				logMsg("detected Droid/Milestone keyboard");
				genericKeyDev.subtype_ = Device::SUBTYPE_MOTO_DROID_KEYBOARD;
			}
		}
		addDevice(genericKeyDev);
		builtinKeyboardDev = &genericKeyDev;
	}
	return OK;
}

}
