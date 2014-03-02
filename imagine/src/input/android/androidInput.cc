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

#define LOGTAG "InputAndroid"
#include <base/android/sdk.hh>
#include <input/common/common.h>
#include <input/AxisKeyEmu.hh>
#include <base/android/private.hh>
#include <input/android/private.hh>
#include <input/android/AndroidInputDevice.hh>
#include <base/Timer.hh>
#include <config/machine.hh>
#include <util/jni.hh>
#include <util/collection/ArrayList.hh>
#include <util/fd-utils.h>
#include <util/bits.h>
#include <android/input.h>
#include <android/configuration.h>
#include <dlfcn.h>
#include <sys/inotify.h>

#include "common.hh"

namespace Input
{

#if CONFIG_ENV_ANDROID_MINSDK < 12
using AMotionEvent_getAxisValueProto = float (__NDK_FPABI__ *)(const AInputEvent* motion_event, int32_t axis, size_t pointer_index);
static AMotionEvent_getAxisValueProto AMotionEvent_getAxisValue = nullptr;
static bool hasGetAxisValue()
{
	return likely(AMotionEvent_getAxisValue);
}
#else
static bool hasGetAxisValue()
{
	return true;
}
#endif
static Base::Timer inputRescanCallback;
static bool handleVolumeKeys = false;
static bool allowOSKeyRepeats = true;
static const int AINPUT_SOURCE_JOYSTICK = 0x01000010;
static const uint maxJoystickAxisPairs = 4; // 2 sticks + POV hat + L/R Triggers
static const uint maxSysInputDevs = MAX_DEVS;
StaticArrayList<AndroidInputDevice*, maxSysInputDevs> sysInputDev;
static Device *virtualDev = nullptr;
static AndroidInputDevice genericKeyDev { -1,
	Device::TYPE_BIT_VIRTUAL | Device::TYPE_BIT_KEYBOARD | Device::TYPE_BIT_KEY_MISC, "Key Input (All Devices)" };
jclass AInputDeviceJ::cls = nullptr;
JavaClassMethod<jobject> AInputDeviceJ::getDeviceIds_, AInputDeviceJ::getDevice_;
JavaInstMethod<jobject> AInputDeviceJ::getName_, AInputDeviceJ::getKeyCharacterMap_, AInputDeviceJ::getMotionRange_, AInputDeviceJ::getDescriptor_;
JavaInstMethod<jint> AInputDeviceJ::getSources_, AInputDeviceJ::getKeyboardType_;
//jclass AInputDeviceMotionRangeJ::cls = nullptr;
//JavaInstMethod<jfloat> AInputDeviceMotionRangeJ::getMin_, AInputDeviceMotionRangeJ::getMax_;
bool sendInputToIME = false;

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

AndroidInputDevice::AndroidInputDevice(JNIEnv* jEnv, AInputDeviceJ aDev, uint enumId, int osId, int src, const char *name):
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
		auto kbType = aDev.getKeyboardType(jEnv);
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
				auto range = aDev.getMotionRange(jEnv, axisId);
				if(!range)
				{
					axisIdx++;
					continue;
				}
				jEnv->DeleteLocalRef(range);
				logMsg("joystick axis: %d", axisId);
				auto size = 2.0f;
				axis.emplace_back(axisId, (AxisKeyEmu<float>){-1.f + size/4.f, 1.f - size/4.f, Key(axisToKeycode(axisId)+1), axisToKeycode(axisId)});
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
				auto range = aDev.getMotionRange(jEnv, axisId);
				if(!range)
					continue;
				jEnv->DeleteLocalRef(range);
				logMsg("trigger axis: %d", axisId);
				// use unreachable lowLimit value so only highLimit is used
				axis.emplace_back(axisId, (AxisKeyEmu<float>){-1.f, 0.25f, 0, axisToKeycode(axisId)});
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
			auto range = dev.getMotionRange(eEnv(), i);
			if(!range)
			{
				continue;
			}
			eEnv()->DeleteLocalRef(range);
			logMsg("has axis: %d", i);
		}*/
	}
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
	allowKeyRepeats = on;
	if(!on)
	{
		deinitKeyRepeatTimer();
	}
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

static const char* aInputSourceToStr(uint source)
{
	switch(source)
	{
		case AINPUT_SOURCE_UNKNOWN: return "Unknown";
		case AINPUT_SOURCE_KEYBOARD: return "Keyboard";
		case AINPUT_SOURCE_DPAD: return "DPad";
		case AINPUT_SOURCE_TOUCHSCREEN: return "Touchscreen";
		case AINPUT_SOURCE_MOUSE: return "Mouse";
		case AINPUT_SOURCE_TRACKBALL: return "Trackball";
		case AINPUT_SOURCE_TOUCHPAD: return "Touchpad";
		case AINPUT_SOURCE_JOYSTICK: return "Joystick";
		case AINPUT_SOURCE_ANY: return "Any";
		default:  return "Unhandled value";
	}
}

static void mapKeycodesForSpecialDevices(const Input::Device &dev, int32_t &keyCode, int32_t &metaState, AInputEvent *event)
{
	switch(dev.subtype())
	{
		bcase Device::SUBTYPE_XPERIA_PLAY:
		{
			if(Config::MACHINE_IS_GENERIC_ARMV7 && unlikely(keyCode == (int)Keycode::ESCAPE && (metaState & AMETA_ALT_ON)))
			{
				keyCode = Keycode::GAME_B;
			}
		}
		bcase Device::SUBTYPE_XBOX_360_CONTROLLER:
		{
			if(keyCode)
				break;
			// map d-pad on wireless controller adapter
			auto scanCode = AKeyEvent_getScanCode(event);
			switch(scanCode)
			{
				bcase 704: keyCode = Keycode::LEFT;
				bcase 705: keyCode = Keycode::RIGHT;
				bcase 706: keyCode = Keycode::UP;
				bcase 707: keyCode = Keycode::DOWN;
			}
		}
		bdefault: break;
	}
}

static const Device *deviceForInputId(int osId)
{
	if(Base::androidSDK() < 12)
	{
		// no multi-input device support
		return &genericKeyDev;
	}
	for(auto &e : sysInputDev)
	{
		if(e->osId == osId)
		{
			return e;
		}
	}
	return nullptr;
}

static AndroidInputDevice *sysDeviceForInputId(int osId)
{
	for(auto &e : sysInputDev)
	{
		if(e->osId == osId)
		{
			return e;
		}
	}
	return nullptr;
}

int32_t onInputEvent(AInputEvent* event, Base::Window &win)
{
	auto type = AInputEvent_getType(event);
	auto source = AInputEvent_getSource(event);
	switch(type)
	{
		case AINPUT_EVENT_TYPE_MOTION:
		{
			int eventAction = AMotionEvent_getAction(event);
			//logMsg("get motion event action %d", eventAction);
			bool isTouch = false;
			switch(source)
			{
				case AINPUT_SOURCE_TRACKBALL:
				{
					//logMsg("from trackball");
					handleTrackballEvent(eventAction, AMotionEvent_getX(event, 0), AMotionEvent_getY(event, 0), AMotionEvent_getEventTime(event));
					return 1;
				}
				case AINPUT_SOURCE_TOUCHPAD: // TODO
				{
					//logMsg("from touchpad");
					return 0;
				}
				case AINPUT_SOURCE_TOUCHSCREEN:
				{
					isTouch = true;
					// fall-through to case AINPUT_SOURCE_MOUSE
				}
				case AINPUT_SOURCE_MOUSE:
				{
					//logMsg("from touchscreen or mouse");
					uint action = eventAction & AMOTION_EVENT_ACTION_MASK;
					if(action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL)
					{
						// touch gesture ended
						handleTouchEvent(AMOTION_EVENT_ACTION_UP,
								AMotionEvent_getX(event, 0),
								AMotionEvent_getY(event, 0),
								AMotionEvent_getPointerId(event, 0),
								AMotionEvent_getEventTime(event), isTouch);
						return 1;
					}
					uint actionPIdx = eventAction >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
					int pointers = AMotionEvent_getPointerCount(event);
					iterateTimes(pointers, i)
					{
						int pAction = action;
						// a pointer not performing the action just needs its position updated
						if(actionPIdx != i)
						{
							//logMsg("non-action pointer idx %d", i);
							pAction = AMOTION_EVENT_ACTION_MOVE;
						}
						handleTouchEvent(pAction,
							AMotionEvent_getX(event, i),
							AMotionEvent_getY(event, i),
							AMotionEvent_getPointerId(event, i),
							AMotionEvent_getEventTime(event), isTouch);
					}
					return 1;
				}
				case AINPUT_SOURCE_JOYSTICK: // Joystick
				{
					auto dev = sysDeviceForInputId(AInputEvent_getDeviceId(event));
					if(unlikely(!dev))
					{
						logWarn("discarding joystick input from unknown device ID: %d", AInputEvent_getDeviceId(event));
						return 0;
					}
					auto enumID = dev->enumId();
					//logMsg("Joystick input from %s", dev->name());

					if(hasGetAxisValue())
					{
						for(auto &axis : dev->axis)
						{
							auto pos = AMotionEvent_getAxisValue(event, axis.id, 0);
							//logMsg("axis %d with value: %f", axis.id, (double)pos);
							axis.keyEmu.dispatch(pos, enumID, Event::MAP_SYSTEM, *dev, win);
						}
					}
					else
					{
						// no getAxisValue, can only use 2 axis values (X and Y)
						iterateTimes(std::min(dev->axis.size(), (uint)2), i)
						{
							auto pos = i ? AMotionEvent_getY(event, 0) : AMotionEvent_getX(event, 0);
							dev->axis[i].keyEmu.dispatch(pos, enumID, Event::MAP_SYSTEM, *dev, win);
						}
					}
					return 1;
				}
				default:
				{
					//logWarn("from other source: %s, %dx%d", aInputSourceToStr(source), (int)AMotionEvent_getX(event, 0), (int)AMotionEvent_getY(event, 0));
					return 0;
				}
			}
		}
		bcase AINPUT_EVENT_TYPE_KEY:
		{
			cancelKeyRepeatTimer();
			auto keyCode = AKeyEvent_getKeyCode(event);
			//logMsg("key event, code: %d id: %d source: 0x%X repeat: %d action: %d", keyCode, AInputEvent_getDeviceId(event), source, AKeyEvent_getRepeatCount(event), AKeyEvent_getAction(event));
			if(!handleVolumeKeys &&
				(keyCode == (int)Keycode::VOL_UP || keyCode == (int)Keycode::VOL_DOWN))
			{
				return 0;
			}

			if(allowOSKeyRepeats || AKeyEvent_getRepeatCount(event) == 0)
			{
				auto dev = deviceForInputId(AInputEvent_getDeviceId(event));
				if(unlikely(!dev))
				{
					assert(virtualDev);
					//logWarn("re-mapping unknown device ID %d to Virtual", AInputEvent_getDeviceId(event));
					dev = virtualDev;
				}
				auto metaState = AKeyEvent_getMetaState(event);
				mapKeycodesForSpecialDevices(*dev, keyCode, metaState, event);
				if(unlikely(!keyCode)) // ignore "unknown" key codes
				{
					return 0;
				}
				handleKeyEvent(keyCode, AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_UP ? 0 : 1, dev->enumId(), metaState & AMETA_SHIFT_ON, AKeyEvent_getEventTime(event), *dev);
			}
			return 1;
		}
	}
	logWarn("unhandled input event type %d", type);
	return 0;
}

static void JNICALL textInputEnded(JNIEnv* env, jobject thiz, jstring jStr, jboolean processText, jboolean isDoingDismiss)
{
	if(isDoingDismiss)
		Base::unrefUIGL();
	if(!processText)
	{
		return;
	}
	setEventsUseOSInputMethod(false);
	auto delegate = moveAndClear(vKeyboardTextDelegate);
	if(delegate)
	{
		Base::restoreOpenGLContext();
		if(jStr)
		{
			const char *str = env->GetStringUTFChars(jStr, nullptr);
			logMsg("running text entry callback with text: %s", str);
			delegate(str);
			env->ReleaseStringUTFChars(jStr, str);
		}
		else
		{
			logMsg("canceled text entry callback");
			delegate(nullptr);
		}
	}
	else
	{
		logMsg("text entry has no callback");
	}
}

bool dlLoadAndroidFuncs(void *libandroid)
{
	#if CONFIG_ENV_ANDROID_MINSDK < 12
	if(Base::androidSDK() < 12)
	{
		return 0;
	}
	// load AMotionEvent_getAxisValue dynamically
	if((AMotionEvent_getAxisValue = (AMotionEvent_getAxisValueProto)dlsym(libandroid, "AMotionEvent_getAxisValue")) == nullptr)
	{
		bug_exit("AMotionEvent_getAxisValue not found even though using SDK %d", Base::androidSDK());
		return 0;
	}
	#endif
	return 1;
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

static bool remapAxisKeyEmu(Device &dev, AxisKeyEmu<float> &keyEmu, const Key (&matchKey)[4], const Key (&setKey)[4])
{
	bool didRemap = false;
	forEachDInArray(matchKey, match)
	{
		if(keyEmu.lowKey == match)
		{
			keyEmu.lowKey = setKey[match_i];
			logMsg("set low key from %s to %s", dev.keyName(match), dev.keyName(keyEmu.lowKey));
			didRemap = true;
		}
		if(keyEmu.highKey == match)
		{
			keyEmu.highKey = setKey[match_i];
			logMsg("set high key from %s to %s", dev.keyName(match), dev.keyName(keyEmu.highKey));
			didRemap = true;
		}
	}
	return didRemap;
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
					e.keyEmu.lowKey = on ? Keycode::LEFT : Keycode::JS1_XAXIS_NEG;
					e.keyEmu.highKey = on ? Keycode::RIGHT : Keycode::JS1_XAXIS_POS;
				}
				bcase AXIS_Y:
				{
					bool on = axisMask & AXIS_BIT_Y;
					//logMsg("axis y as dpad: %s", on ? "yes" : "no");
					e.keyEmu.lowKey = on ? Keycode::UP : Keycode::JS1_YAXIS_NEG;
					e.keyEmu.highKey = on ? Keycode::DOWN : Keycode::JS1_YAXIS_POS;
				}
				bcase AXIS_Z:
				{
					bool on = axisMask & AXIS_BIT_Z;
					//logMsg("axis z as dpad: %s", on ? "yes" : "no");
					e.keyEmu.lowKey = on ? Keycode::LEFT : Keycode::JS2_XAXIS_NEG;
					e.keyEmu.highKey = on ? Keycode::RIGHT : Keycode::JS2_XAXIS_POS;
				}
				bcase AXIS_RZ:
				{
					bool on = axisMask & AXIS_BIT_RZ;
					//logMsg("axis rz as dpad: %s", on ? "yes" : "no");
					e.keyEmu.lowKey = on ? Keycode::UP : Keycode::JS2_YAXIS_NEG;
					e.keyEmu.highKey = on ? Keycode::DOWN : Keycode::JS2_YAXIS_POS;
				}
				bcase AXIS_HAT_X:
				{
					bool on = axisMask & AXIS_BIT_HAT_X;
					//logMsg("axis hat x as dpad: %s", on ? "yes" : "no");
					e.keyEmu.lowKey = on ? Keycode::LEFT : Keycode::JS_POV_XAXIS_NEG;
					e.keyEmu.highKey = on ? Keycode::RIGHT : Keycode::JS_POV_XAXIS_POS;
				}
				bcase AXIS_HAT_Y:
				{
					bool on = axisMask & AXIS_BIT_HAT_Y;
					//logMsg("axis hat y as dpad: %s", on ? "yes" : "no");
					e.keyEmu.lowKey = on ? Keycode::UP : Keycode::JS_POV_YAXIS_NEG;
					e.keyEmu.highKey = on ? Keycode::DOWN : Keycode::JS_POV_YAXIS_POS;
				}
			}
		}
	}
}

uint AndroidInputDevice::joystickAxisAsDpadBits()
{
	return joystickAxisAsDpadBits_;
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

static void processDevice(JNIEnv* jEnv, int devID, bool setSpecialDevices, bool notify)
{
	auto dev = AInputDeviceJ::getDevice(jEnv, devID);
	jstring jName = dev.getName(jEnv);
	if(!jName)
	{
		logWarn("no name from device id %d", devID);
		jEnv->DeleteLocalRef(dev);
		return;
	}
	const char *name = jEnv->GetStringUTFChars(jName, nullptr);
	jint src = dev.getSources(jEnv);
	bool hasKeys = src & AInputDeviceJ::SOURCE_CLASS_BUTTON;
	logMsg("%d: %s, source %X", devID, name, src);
	if(hasKeys && !devList.isFull() && !sysInputDev.isFull() && isUsefulDevice(name))
	{
		auto sysInput = new AndroidInputDevice(jEnv, dev, nextEnumID(name), devID, src, name);
		sysInputDev.push_back(sysInput);
		Input::addDevice(*sysInput);
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
			//logMsg("notifying app of added device");
			onInputDevChange(*sysInput, { Device::Change::ADDED });
		}
	}
	jEnv->ReleaseStringUTFChars(jName, name);
	jEnv->DeleteLocalRef(dev);
}

void devicesChanged(JNIEnv* jEnv)
{
	logMsg("rescanning all devices for changes");
	using namespace Base;
	auto jID = AInputDeviceJ::getDeviceIds(jEnv);
	auto id = jEnv->GetIntArrayElements(jID, 0);
	auto ids = jEnv->GetArrayLength(jID);

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
			Input::removeDevice(*dev);
			it.erase();
			delete dev;
			//logMsg("notifying app of removed device");
			onInputDevChange(removedDev, { Device::Change::REMOVED });
		}
	}

	// add new devices
	iterateTimes(ids, i)
	{
		if(deviceIDPresent(id[i]))
			continue;
		processDevice(jEnv, id[i], false, true);
	}

	jEnv->ReleaseIntArrayElements(jID, id, 0);
}

void devicesChanged()
{
	devicesChanged(Base::eEnv());
}

static void setupDevices(JNIEnv* jEnv)
{
	using namespace Base;
	auto jID = AInputDeviceJ::getDeviceIds(jEnv);
	auto id = jEnv->GetIntArrayElements(jID, 0);
	logMsg("scanning input devices");
	iterateTimes(jEnv->GetArrayLength(jID), i)
	{
		auto devID = id[i];
		processDevice(jEnv, devID, true, false);
	}
	jEnv->ReleaseIntArrayElements(jID, id, 0);

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
		Input::addDevice(*sysInput);
		virtualDev = sysInput;
	}
}

void setBuiltInKeyboardState(bool shown)
{
	if(builtinKeyboardDev)
	{
		Device::Change change { shown ? Device::Change::SHOWN : Device::Change::HIDDEN };
		onInputDevChange(*builtinKeyboardDev, change);
	}
}

CallResult init()
{
	if(Base::androidSDK() >= 12)
	{
		auto jEnv = Base::eEnv();

		// device change notifications
		if(Base::androidSDK() >= 16)
		{
			logMsg("setting up input notifications");
			JavaInstMethod<jobject> jInputDeviceListenerHelper;
			jInputDeviceListenerHelper.setup(jEnv, Base::jBaseActivityCls, "inputDeviceListenerHelper", "()Lcom/imagine/InputDeviceListenerHelper;");
			auto inputDeviceListenerHelper = jInputDeviceListenerHelper(jEnv, Base::jBaseActivity);
			assert(inputDeviceListenerHelper);
			auto inputDeviceListenerHelperCls = jEnv->GetObjectClass(inputDeviceListenerHelper);
			JNINativeMethod method[] =
			{
				{
					"deviceChange", "(II)V",
					(void*)(void JNICALL(*)(JNIEnv* env, jobject thiz, jint devID, jint change))
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
										Input::removeDevice(*dev);
										it.erase();
										delete dev;
										onInputDevChange(removedDev, { Device::Change::REMOVED });
										break;
									}
								}
						}
					})
				}
			};
			jEnv->RegisterNatives(inputDeviceListenerHelperCls, method, sizeofArray(method));
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
									Input::devicesChanged(Base::eEnv());
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

		AInputDeviceJ::jniInit(jEnv);
		//AInputDeviceMotionRangeJ::jniInit();
		setupDevices(jEnv);
	}
	else
	{
		// no multi-input device support
		if(Config::MACHINE_IS_GENERIC_ARMV7)
		{
			if(isXperiaPlayDeviceStr(Base::androidBuildDevice()))
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
		Input::addDevice(genericKeyDev);
		builtinKeyboardDev = &genericKeyDev;
	}
	return OK;
}

}
