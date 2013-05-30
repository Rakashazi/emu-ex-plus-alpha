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

#define thisModuleName "input:android"
#include <base/android/sdk.hh>
#include <input/common/common.h>
#include <input/AxisKeyEmu.hh>
#include <base/android/private.hh>
#include <config/machine.hh>
#include <util/jni.hh>
#include <util/collection/ArrayList.hh>
#include <android/input.h>
#include <android/configuration.h>
#include <dlfcn.h>

#include "common.hh"

namespace Input
{

typedef float (*AMotionEvent_getAxisValueProto)(const AInputEvent* motion_event, int32_t axis, size_t pointer_index);
static AMotionEvent_getAxisValueProto AMotionEvent_getAxisValue = nullptr;
static bool handleVolumeKeys = 0;
static bool allowKeyRepeats = 1;
static const int AINPUT_SOURCE_JOYSTICK = 0x01000010;
static const uint maxJoystickAxisPairs = 4; // 2 sticks + POV hat + L/R Triggers
static const uint maxSysInputDevs = MAX_DEVS;
static const uint MAX_STICK_AXES = 6; // 6 possible axes defined in key codes
static const uint MAX_AXES = 6;
static_assert(MAX_STICK_AXES <= MAX_AXES, "MAX_AXES must be large enough to hold MAX_STICK_AXES");
static uint sysInputDevs = 0;
struct SysInputDevice
{
	constexpr SysInputDevice() { }
	int osId = 0;
	Device *dev = nullptr;
	char name[48] {0};
	struct Axis
	{
		constexpr Axis() {}
		constexpr Axis(uint8 id, AxisKeyEmu<float> keyEmu): id(id), keyEmu(keyEmu) {}
		uint8 id = 0;
		AxisKeyEmu<float> keyEmu;
	};
	StaticArrayList<Axis, MAX_AXES> axis;

	bool operator ==(SysInputDevice const& rhs) const
	{
		return osId == rhs.osId && string_equal(name, rhs.name);
	}
};
static SysInputDevice sysInputDev[maxSysInputDevs];
static Device *virtualDev = nullptr;
static const int AXIS_X = 0, AXIS_Y = 1, AXIS_Z = 11,
	AXIS_RX = 12, AXIS_RY = 13, AXIS_RZ = 14,
	AXIS_HAT_X = 15, AXIS_HAT_Y = 16,
	AXIS_LTRIGGER = 17, AXIS_RTRIGGER = 18,
	AXIS_RUDDER = 20, AXIS_WHEEL = 21,
	AXIS_GAS = 22, AXIS_BRAKE = 23;

static Key axisToKeycode(int axis)
{
	switch(axis)
	{
		case AXIS_X: return Keycode::JS1_XAXIS_POS;
		case AXIS_Y: return Keycode::JS1_YAXIS_POS;
		case AXIS_Z: return Keycode::JS2_XAXIS_POS;
		case AXIS_RZ: return Keycode::JS2_YAXIS_POS;
		case AXIS_HAT_X: return Keycode::JS3_XAXIS_POS;
		case AXIS_HAT_Y: return Keycode::JS3_YAXIS_POS;
		case AXIS_LTRIGGER: return Keycode::JS_LTRIGGER_AXIS;
		case AXIS_RTRIGGER: return Keycode::JS_RTRIGGER_AXIS;
		case AXIS_GAS : return Keycode::JS_GAS_AXIS;
		case AXIS_BRAKE : return Keycode::JS_BRAKE_AXIS;
	}
	return Keycode::JS3_YAXIS_POS;
}

// JNI classes/methods

class JObject
{
protected:
	jobject o = nullptr;
	constexpr JObject() { };
	constexpr JObject(jobject o): o(o) { };

public:
	operator jobject() const
	{
		return o;
	}
};

class AInputDeviceMotionRangeJ : public JObject
{
public:
	constexpr AInputDeviceMotionRangeJ(jobject motionRange): JObject(motionRange) {};

	/*static jclass cls;
	static JavaInstMethod<jfloat> getMax_, getMin_;

	jfloat getMin(JNIEnv *j)
	{
		return getMin_(j, o);
	}

	jfloat getMax(JNIEnv *j)
	{
		return getMax_(j, o);
	}

	static void jniInit()
	{
		using namespace Base;
		cls = (jclass)eEnv()->NewGlobalRef(eEnv()->FindClass("android/view/InputDevice$MotionRange"));
		getMax_.setup(eEnv(), cls, "getMax", "()F");
		getMin_.setup(eEnv(), cls, "getMin", "()F");
	}*/
};

//jclass AInputDeviceMotionRangeJ::cls = nullptr;
//JavaInstMethod<jfloat> AInputDeviceMotionRangeJ::getMin_, AInputDeviceMotionRangeJ::getMax_;

class AInputDeviceJ : public JObject
{
public:
	constexpr AInputDeviceJ(jobject inputDevice): JObject(inputDevice) {};

	static AInputDeviceJ getDevice(JNIEnv *j, jint id)
	{
		return AInputDeviceJ {getDevice_(j, id)};
	}

	static jintArray getDeviceIds(JNIEnv *j)
	{
		return (jintArray)getDeviceIds_(j);
	}

	jstring getName(JNIEnv *j)
	{
		return (jstring)getName_(j, o);
	}

	jint getSources(JNIEnv *j)
	{
		return getSources_(j, o);
	}

	jint getKeyboardType(JNIEnv *j)
	{
		return getKeyboardType_(j, o);
	}

	AInputDeviceMotionRangeJ getMotionRange(JNIEnv *j, jint axis)
	{
		return AInputDeviceMotionRangeJ {getMotionRange_(j, o, axis)};
	}

	static jclass cls;
	static JavaClassMethod<jobject> getDeviceIds_, getDevice_;
	static JavaInstMethod<jobject> getName_, getKeyCharacterMap_, getMotionRange_;
	static JavaInstMethod<jint> getSources_, getKeyboardType_;
	static constexpr jint SOURCE_CLASS_BUTTON = 0x00000001, SOURCE_CLASS_POINTER = 0x00000002, SOURCE_CLASS_TRACKBALL = 0x00000004,
			SOURCE_CLASS_POSITION = 0x00000008, SOURCE_CLASS_JOYSTICK = 0x00000010;
	static constexpr jint SOURCE_KEYBOARD = 0x00000101, SOURCE_DPAD = 0x00000201, SOURCE_GAMEPAD = 0x00000401,
			SOURCE_TOUCHSCREEN = 0x00001002, SOURCE_MOUSE = 0x00002002, SOURCE_STYLUS = 0x00004002,
			SOURCE_TRACKBALL = 0x00010004, SOURCE_TOUCHPAD = 0x00100008, SOURCE_JOYSTICK = 0x01000010;

	static constexpr jint KEYBOARD_TYPE_NONE = 0,  KEYBOARD_TYPE_NON_ALPHABETIC = 1, KEYBOARD_TYPE_ALPHABETIC = 2;

	static void jniInit(JNIEnv* jEnv)
	{
		using namespace Base;
		cls = (jclass)jEnv->NewGlobalRef(jEnv->FindClass("android/view/InputDevice"));
		getDeviceIds_.setup(jEnv, cls, "getDeviceIds", "()[I");
		getDevice_.setup(jEnv, cls, "getDevice", "(I)Landroid/view/InputDevice;");
		getName_.setup(jEnv, cls, "getName", "()Ljava/lang/String;");
		getSources_.setup(jEnv, cls, "getSources", "()I");
		getKeyboardType_.setup(jEnv, cls, "getKeyboardType", "()I");
		getMotionRange_.setup(jEnv, cls, "getMotionRange", "(I)Landroid/view/InputDevice$MotionRange;");
	}
};

jclass AInputDeviceJ::cls = nullptr;
JavaClassMethod<jobject> AInputDeviceJ::getDeviceIds_, AInputDeviceJ::getDevice_;
JavaInstMethod<jobject> AInputDeviceJ::getName_, AInputDeviceJ::getKeyCharacterMap_, AInputDeviceJ::getMotionRange_;
JavaInstMethod<jint> AInputDeviceJ::getSources_, AInputDeviceJ::getKeyboardType_;

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

void setKeyRepeat(bool on)
{
	// always accept repeats on Android 3.1+ because 2+ devices pushing
	// the same button is considered a repeat by the OS
	if(Base::androidSDK() < 12)
	{
		logMsg("set key repeat %s", on ? "On" : "Off");
		allowKeyRepeats = on;
	}
}

void setHandleVolumeKeys(bool on)
{
	logMsg("set volume key use %s", on ? "On" : "Off");
	handleVolumeKeys = on;
}

bool sendInputToIME = 1;
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

static void handleKeycodesForSpecialDevices(const Input::Device &dev, int32_t &keyCode, int32_t &metaState, AInputEvent *event)
{
	switch(dev.subtype)
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
		assert(devList.first()->map() == Event::MAP_KEYBOARD);
		return devList.first(); // head of list is always catch-all-android-input device
	}
	iterateTimes(sysInputDevs, i)
	{
		if(sysInputDev[i].osId == osId)
		{
			return sysInputDev[i].dev;
		}
	}
	return nullptr;
}

int32_t onInputEvent(AInputEvent* event)
{
	auto type = AInputEvent_getType(event);
	auto source = AInputEvent_getSource(event);
	switch(type)
	{
		case AINPUT_EVENT_TYPE_MOTION:
		{
			int eventAction = AMotionEvent_getAction(event);
			//logMsg("get motion event action %d", eventAction);
			bool isTouch = 0;
			switch(source)
			{
				case AINPUT_SOURCE_TRACKBALL:
				{
					//logMsg("from trackball");
					handleTrackballEvent(eventAction, AMotionEvent_getX(event, 0), AMotionEvent_getY(event, 0));
					return 1;
				}
				case AINPUT_SOURCE_TOUCHPAD: // TODO
				{
					//logMsg("from touchpad");
					return 0;
				}
				case AINPUT_SOURCE_TOUCHSCREEN:
				{
					isTouch = 1;
				}
				case AINPUT_SOURCE_MOUSE:
				{
					//logMsg("from touchscreen or mouse");
					uint action = eventAction & AMOTION_EVENT_ACTION_MASK;
					if(action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL)
					{
						// touch gesture ended
						handleTouchEvent(AMOTION_EVENT_ACTION_UP,
								AMotionEvent_getX(event, 0) - Base::window().rect.x,
								AMotionEvent_getY(event, 0) - Base::window().rect.y,
								AMotionEvent_getPointerId(event, 0), isTouch);
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
							AMotionEvent_getX(event, i) - Base::window().rect.x,
							AMotionEvent_getY(event, i) - Base::window().rect.y,
							AMotionEvent_getPointerId(event, i), isTouch);
					}
					return 1;
				}
				case AINPUT_SOURCE_JOYSTICK: // Joystick
				{
					auto dev = deviceForInputId(AInputEvent_getDeviceId(event));
					if(unlikely(!dev))
					{
						logWarn("discarding joystick input from unknown device ID: %d", AInputEvent_getDeviceId(event));
						return 0;
					}
					auto eventDevID = dev->devId;
					//logMsg("Joystick input from %s,%d", dev->name(), dev->devId);
					auto &sysDev = sysInputDev[dev->idx];

					if(likely(AMotionEvent_getAxisValue))
					{
						forEachInContainer(sysDev.axis, e)
						{
							auto &axis = *e;
							auto pos = AMotionEvent_getAxisValue(event, axis.id, 0);
							axis.keyEmu.dispatch(pos, eventDevID, Event::MAP_KEYBOARD, *dev);
						}
					}
					else
					{
						// no getAxisValue, can only use 2 axis values (X and Y)
						iterateTimes(std::min(sysDev.axis.size(), (uint)2), i)
						{
							auto pos = i ? AMotionEvent_getY(event, 0) : AMotionEvent_getX(event, 0);
							sysDev.axis[i].keyEmu.dispatch(pos, eventDevID, Event::MAP_KEYBOARD, *dev);
						}
					}
					return 1;
				}
				default:
				{
					logWarn("from other source: %s, %dx%d", aInputSourceToStr(source), (int)AMotionEvent_getX(event, 0), (int)AMotionEvent_getY(event, 0));
					return 0;
				}
			}
		}
		bcase AINPUT_EVENT_TYPE_KEY:
		{
			auto keyCode = AKeyEvent_getKeyCode(event);
			//logMsg("key event, code: %d id: %d source: 0x%X repeat: %d action: %d", keyCode, AInputEvent_getDeviceId(event), source, AKeyEvent_getRepeatCount(event), AKeyEvent_getAction(event));
			if(!handleVolumeKeys &&
				(keyCode == (int)Keycode::VOL_UP || keyCode == (int)Keycode::VOL_DOWN))
			{
				return 0;
			}
			//auto isGamepad = bit_isMaskSet(source, AInputDeviceJ::SOURCE_GAMEPAD);

			if(allowKeyRepeats || AKeyEvent_getRepeatCount(event) == 0)
			{
				auto dev = deviceForInputId(AInputEvent_getDeviceId(event));
				if(unlikely(!dev))
				{
					assert(virtualDev);
					//logWarn("re-mapping unknown device ID %d to Virtual", AInputEvent_getDeviceId(event));
					dev = virtualDev;
				}
				auto metaState = AKeyEvent_getMetaState(event);
				handleKeycodesForSpecialDevices(*dev, keyCode, metaState, event);
				if(unlikely(!keyCode)) // ignore "unknown" key codes
				{
					return 0;
				}
				handleKeyEvent(keyCode, AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_UP ? 0 : 1, dev->devId, metaState & AMETA_SHIFT_ON, *dev);
			}
			return 1;
		}
	}
	logWarn("unhandled input event type %d", type);
	return 0;
}

static void JNICALL textInputEnded(JNIEnv* env, jobject thiz, jstring jStr)
{
	// On Android 4.x, the dialog may steal the app's OpenGL context, so set it back
	Base::restoreOpenGLContext();
	auto delegate = vKeyboardTextDelegate;
	vKeyboardTextDelegate = {};
	if(delegate)
	{
		if(jStr)
		{
			const char *str = env->GetStringUTFChars(jStr, 0);
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
	Base::postDrawWindowIfNeeded();
}

bool dlLoadAndroidFuncs(void *libandroid)
{
	if(Base::androidSDK() < 12)
	{
		return 0;
	}
	// Google seems to have forgotten to put AMotionEvent_getAxisValue() in the NDK libandroid.so even though it's
	// present in at least Android 4.0, so we'll load it dynamically to be safe
	if((AMotionEvent_getAxisValue = (AMotionEvent_getAxisValueProto)dlsym(libandroid, "AMotionEvent_getAxisValue")) == nullptr)
	{
		logWarn("AMotionEvent_getAxisValue not found");
		return 0;
	}
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

void Device::setJoystickAxis1AsDpad(bool on)
{
	if(Base::androidSDK() >= 12 && joystickAxis1AsDpad_ != on)
	{
		Key jsKey[4] = { Keycode::JS1_XAXIS_NEG, Keycode::JS1_XAXIS_POS, Keycode::JS1_YAXIS_POS, Keycode::JS1_YAXIS_NEG };
		Key dpadKey[4] = { Input::Keycode::LEFT, Input::Keycode::RIGHT, Input::Keycode::DOWN, Input::Keycode::UP };
		joystickAxis1AsDpad_ = on;
		Key (&matchKey)[4] = on ? jsKey : dpadKey;
		Key (&setKey)[4] = on ? dpadKey : jsKey;
		iterateTimes(sysInputDevs, i)
		{
			if(sysInputDev[i].dev == this)
			{
				forEachInContainer(sysInputDev[i].axis, e)
				{
					forEachDInArray(matchKey, match)
					{
						if(e->keyEmu.lowKey == match)
						{
							e->keyEmu.lowKey = setKey[match_i];
							logMsg("remapped device %d axis %d low key from %s to %s", i, e->id, keyName(match), keyName(e->keyEmu.lowKey));
						}
						if(e->keyEmu.highKey == match)
						{
							e->keyEmu.highKey = setKey[match_i];
							logMsg("remapped device %d axis %d high key from %s to %s", i, e->id, keyName(match), keyName(e->keyEmu.highKey));
						}
					}
				}
				break;
			}
		}
	}
}

bool Device::joystickAxis1AsDpad()
{
	return joystickAxis1AsDpad_;
}

static void rescanDevices(JNIEnv* jEnv, bool firstRun)
{
	forEachInDLList(&Input::devList, e)
	{
		if(e.map() == Event::MAP_KEYBOARD || e.map() == Event::MAP_ICADE)
			e_it.removeElem();
	}
	indexDevices();
	virtualDev = nullptr;
	builtinKeyboardDev = nullptr;
	sysInputDevs = 0;
	using namespace Base;
	auto jID = AInputDeviceJ::getDeviceIds(jEnv);
	auto id = jEnv->GetIntArrayElements(jID, 0);
	bool foundVirtual = 0;
	logMsg("checking input devices");
	iterateTimes(jEnv->GetArrayLength(jID), i)
	{
		auto dev = AInputDeviceJ::getDevice(jEnv, id[i]);
		jint src = dev.getSources(jEnv);
		jstring jName = dev.getName(jEnv);
		if(!jName)
		{
			logWarn("no name from device %d, id %d", i, id[i]);
			continue;
		}
		const char *name = jEnv->GetStringUTFChars(jName, 0);
		bool hasKeys = src & AInputDeviceJ::SOURCE_CLASS_BUTTON;
		logMsg("#%d: %s, id %d, source %X", i, name, id[i], src);
		if(hasKeys && !devList.isFull() && sysInputDevs != MAX_DEVS && isUsefulDevice(name))
		{
			auto &sysInput = sysInputDev[sysInputDevs];
			sysInput = {};
			uint devId = 0;
			// find the next available ID number for devices with this name, starting from 0
			forEachInDLList(&devList, e)
			{
				if(e.map() != Event::MAP_KEYBOARD)
					continue;
				if(string_equal(e.name(), name) && e.devId == devId)
					devId++;
			}
			sysInput.osId = id[i];
			string_copy(sysInput.name, name);
			Input::addDevice((Device){devId, Event::MAP_KEYBOARD, Device::TYPE_BIT_KEY_MISC, sysInput.name});
			auto newDev = devList.last();
			sysInput.dev = newDev;
			if(id[i] == 0) // built-in keyboard is always id 0 according to Android docs
			{
				builtinKeyboardDev = newDev;
			}
			else if(id[i] == -1)
			{
				foundVirtual = 1;
				virtualDev = newDev;
				newDev->setTypeBits(newDev->typeBits() | Device::TYPE_BIT_VIRTUAL);
			}

			if(bit_isMaskSet(src, AInputDeviceJ::SOURCE_GAMEPAD))
			{
				bool isGamepad = 1;
				if(Config::MACHINE_IS_GENERIC_ARMV7 && strstr(name, "-zeus"))
				{
					logMsg("detected Xperia Play gamepad");
					newDev->subtype = Device::SUBTYPE_XPERIA_PLAY;
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
					newDev->subtype = Device::SUBTYPE_PS3_CONTROLLER;
				}
				else if(string_equal(name, "OUYA Game Controller"))
				{
					logMsg("detected OUYA gamepad");
					newDev->subtype = Device::SUBTYPE_OUYA_CONTROLLER;
				}
				else if(string_equal(name, "Xbox 360 Wireless Receiver"))
				{
					logMsg("detected wireless 360 gamepad");
					newDev->subtype = Device::SUBTYPE_XBOX_360_CONTROLLER;
				}
				else
				{
					logMsg("detected a gamepad");
				}
				if(isGamepad)
					newDev->setTypeBits(Device::TYPE_BIT_GAMEPAD);
			}
			if(bit_isMaskSet(src, AInputDeviceJ::SOURCE_KEYBOARD))
			{
				auto kbType = dev.getKeyboardType(jEnv);
				// Classify full alphabetic keyboards, and also devices with other keyboard
				// types, as long as they are exclusively SOURCE_KEYBOARD
				// (needed for the iCade 8-bitty since it reports a non-alphabetic keyboard type)
				if(kbType == AInputDeviceJ::KEYBOARD_TYPE_ALPHABETIC
					|| src == AInputDeviceJ::SOURCE_KEYBOARD)
				{
					newDev->setTypeBits(newDev->typeBits() | Device::TYPE_BIT_KEYBOARD);
					logMsg("has keyboard type: %s", inputDeviceKeyboardTypeToStr(kbType));
				}
			}
			if(bit_isMaskSet(src, AInputDeviceJ::SOURCE_JOYSTICK))
			{
				newDev->setTypeBits(newDev->typeBits() | Device::TYPE_BIT_JOYSTICK);
				logMsg("detected a joystick");

				// check joystick axes
				{
					uint keycodeIdx = 0;
					Key axisKeycode[] =
					{
						Keycode::JS1_XAXIS_NEG, Keycode::JS1_XAXIS_POS,
						Keycode::JS1_YAXIS_NEG, Keycode::JS1_YAXIS_POS,
						Keycode::JS2_XAXIS_NEG, Keycode::JS2_XAXIS_POS,
						Keycode::JS2_YAXIS_NEG, Keycode::JS2_YAXIS_POS,
						Keycode::JS3_XAXIS_NEG, Keycode::JS3_XAXIS_POS,
						Keycode::JS3_YAXIS_NEG, Keycode::JS3_YAXIS_POS,
					};
					const uint8 stickAxes[] { AXIS_X, AXIS_Y, AXIS_Z, AXIS_RX, AXIS_RY, AXIS_RZ,
							AXIS_HAT_X, AXIS_HAT_Y, AXIS_RUDDER, AXIS_WHEEL };
					for(auto axisId : stickAxes)
					{
						auto range = dev.getMotionRange(jEnv, axisId);
						if(!range)
							continue;
						jEnv->DeleteLocalRef(range);
						logMsg("joystick axis: %d", axisId);
						auto size = 2.0f;
						sysInput.axis.emplace_back(axisId, (AxisKeyEmu<float>){-1.f + size/4.f, 1.f - size/4.f, axisKeycode[keycodeIdx], axisKeycode[keycodeIdx+1]});
						keycodeIdx += 2; // move to the next +/- axis keycode pair
						if(sysInput.axis.size() == sizeofArray(axisKeycode)/2)
						{
							logMsg("reached maximum joystick axes");
							break;
						}
					}
				}
				// check trigger axes
				if(!sysInput.axis.isFull())
				{
					const uint8 triggerAxes[] { AXIS_LTRIGGER, AXIS_RTRIGGER, AXIS_GAS, AXIS_BRAKE };
					for(auto axisId : triggerAxes)
					{
						auto range = dev.getMotionRange(jEnv, axisId);
						if(!range)
							continue;
						jEnv->DeleteLocalRef(range);
						logMsg("trigger axis: %d", axisId);
						auto size = 1.0f;
						// use unreachable lowLimit value so only highLimit is used
						sysInput.axis.emplace_back(axisId, (AxisKeyEmu<float>){-1.f, 1.f - size/4.f, 0, axisToKeycode(axisId)});
						if(sysInput.axis.isFull())
						{
							logMsg("reached maximum total axes");
							break;
						}
					}
				}
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

			logMsg("added to list with device id %d", newDev->devId);
			sysInputDevs++;
		}
		jEnv->ReleaseStringUTFChars(jName, name);
		jEnv->DeleteLocalRef(dev);
	}
	jEnv->ReleaseIntArrayElements(jID, id, 0);

	if(!foundVirtual)
	{
		if(sysInputDevs == MAX_DEVS || devList.isFull())
		{
			// remove last device to make room
			devList.remove(*sysInputDev[sysInputDevs-1].dev);
			sysInputDevs--;
		}
		logMsg("no \"Virtual\" device id found, adding one");
		Input::addDevice((Device){0, Event::MAP_KEYBOARD, Device::TYPE_BIT_VIRTUAL | Device::TYPE_BIT_KEYBOARD | Device::TYPE_BIT_KEY_MISC, "Virtual"});
		auto newDev = devList.last();
		auto &sysInput = sysInputDev[sysInputDevs];
		sysInput.osId = -1;
		sysInput.dev = newDev;
		string_copy(sysInput.name, "Virtual");
		virtualDev = newDev;
		sysInputDevs++;
	}

	if(!firstRun)
	{
		// TODO: dummy event, apps will just re-scan whole device list for now
		onInputDevChange((DeviceChange){ 0, Event::MAP_KEYBOARD, DeviceChange::ADDED });
	}
}

void rescanDevices(bool firstRun)
{
	rescanDevices(Base::eEnv(), firstRun);
}


CallResult init()
{
	if(Base::androidSDK() >= 12)
	{
		auto jEnv = Base::eEnv();
		AInputDeviceJ::jniInit(jEnv);
		//AInputDeviceMotionRangeJ::jniInit();
		rescanDevices(jEnv, 1);
	}
	else
	{
		// no multi-input device support
		Device genericKeyDev { 0, Event::MAP_KEYBOARD,
			Device::TYPE_BIT_VIRTUAL | Device::TYPE_BIT_KEYBOARD | Device::TYPE_BIT_KEY_MISC, "Key Input (All Devices)" };
		if(Config::MACHINE_IS_GENERIC_ARMV7)
		{
			if(isXperiaPlayDeviceStr(Base::androidBuildDevice()))
			{
				logMsg("detected Xperia Play gamepad");
				genericKeyDev.subtype = Device::SUBTYPE_XPERIA_PLAY;
			}
			else if(string_equal(Base::androidBuildDevice(), "sholes"))
			{
				logMsg("detected Droid/Milestone keyboard");
				genericKeyDev.subtype = Device::SUBTYPE_MOTO_DROID_KEYBOARD;
			}
		}
		Input::addDevice(genericKeyDev);
		builtinKeyboardDev = devList.last();
	}
	return OK;
}

}
