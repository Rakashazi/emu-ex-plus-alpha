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
#include <imagine/base/Base.hh>
#include <imagine/base/Timer.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/fd-utils.h>
#include <imagine/util/algorithm.h>
#include "internal.hh"
#include "android.hh"
#include "../../input/private.hh"
#include "AndroidInputDevice.hh"
#include <sys/inotify.h>
#include <dlfcn.h>
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

static int aHardKeyboardState = 0;
static int aKeyboardType = ACONFIGURATION_KEYBOARD_NOKEYS;
static int aHasHardKeyboard = 0;
static bool trackballNav = false;
bool sendInputToIME = false;
static constexpr uint32_t maxJoystickAxisPairs = 4; // 2 sticks + POV hat + L/R Triggers
std::vector<std::unique_ptr<AndroidInputDevice>> sysInputDev{};
static const AndroidInputDevice *builtinKeyboardDev{};
const AndroidInputDevice *virtualDev{};
static jclass inputDeviceHelperCls{};
static JavaClassMethod<void()> jEnumDevices{};

// InputDeviceListener-based device changes
static jobject inputDeviceListenerHelper{};
static JavaInstMethod<void()> jRegister{};
static JavaInstMethod<void()> jUnregister{};

// Note: values must remain in sync with Java code
static constexpr int DEVICE_ADDED = 0;
static constexpr int DEVICE_CHANGED = 1;
static constexpr int DEVICE_REMOVED = 2;

// inotify-based device changes
static std::optional<Base::Timer> inputRescanCallback{};
int inputDevNotifyFd = -1;
int watch = -1;

static AndroidInputDevice makeGenericKeyDevice()
{
	return {-1, Device::TYPE_BIT_VIRTUAL | Device::TYPE_BIT_KEYBOARD | Device::TYPE_BIT_KEY_MISC,
		"Key Input (All Devices)"};
}

static bool usesInputDeviceListener()
{
	return Base::androidSDK() >= 16;
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

bool hasGetAxisValue()
{
	#ifdef ANDROID_COMPAT_API
	return likely(AMotionEvent_getAxisValueFunc);
	#else
	return true;
	#endif
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

void setKeyRepeat(bool on)
{
	setAllowKeyRepeats(on);
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
		case AInputDevice::KEYBOARD_TYPE_NONE: return "None";
		case AInputDevice::KEYBOARD_TYPE_NON_ALPHABETIC: return "Non-Alphabetic";
		case AInputDevice::KEYBOARD_TYPE_ALPHABETIC: return "Alphabetic";
	}
	return "Unknown";
}

AndroidInputDevice::AndroidInputDevice(JNIEnv* env, jobject aDev, uint32_t enumId,
	int osId, int src, const char *name, int kbType, int jsAxisBits, bool isPowerButton):
	Device{enumId, Event::MAP_SYSTEM, Device::TYPE_BIT_KEY_MISC, name},
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

void AndroidInputDevice::setJoystickAxisAsDpadBits(uint32_t axisMask)
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

uint32_t AndroidInputDevice::joystickAxisAsDpadBits()
{
	return joystickAxisAsDpadBits_;
}

bool Device::anyTypeBitsPresent(uint32_t typeBits)
{
	if(typeBits & TYPE_BIT_KEYBOARD)
	{
		if(keyboardType() == ACONFIGURATION_KEYBOARD_QWERTY)
		{
			if(hardKeyboardState() == ACONFIGURATION_KEYSHIDDEN_YES || hardKeyboardState() == ACONFIGURATION_KEYSHIDDEN_SOFT)
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

	if(Config::MACHINE_IS_GENERIC_ARMV7 && hasXperiaPlayGamepad() &&
		(typeBits & TYPE_BIT_GAMEPAD) && hardKeyboardState() != ACONFIGURATION_KEYSHIDDEN_YES)
	{
		logDMsg("Xperia-play gamepad in use");
		return true;
	}

	for(auto &devPtr : devList)
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

static uint32_t nextEnumID(const char *name)
{
	uint32_t enumID = 0;
	// find the next available ID number for devices with this name, starting from 0
	for(auto &e : sysInputDev)
	{
		if(string_equal(e->name(), name) && e->enumId() == enumID)
			enumID++;
	}
	return enumID;
}

bool addInputDevice(AndroidInputDevice dev, bool updateExisting, bool notify)
{
	int id = dev.osId;
	auto existingIt = std::find_if(sysInputDev.cbegin(), sysInputDev.cend(),
		[=](const auto &e) { return e->osId == id; });
	if(existingIt == sysInputDev.end())
	{
		sysInputDev.emplace_back(std::make_unique<AndroidInputDevice>(dev));
		logMsg("added device id %d to list", id);
		auto devPtr = sysInputDev.back().get();
		addDevice(*devPtr);
		if(notify)
			onDeviceChange.callCopySafe(*devPtr, { Device::Change::ADDED });
		return true;
	}
	else
	{
		if(!updateExisting)
		{
			logMsg("device id %d already in list", id);
			return false;
		}
		else
		{
			logMsg("device id %d updated", id);
			auto devPtr = existingIt->get();
			*devPtr = dev;
			if(notify)
				onDeviceChange.callCopySafe(*devPtr, { Device::Change::CHANGED });
			return true;
		}
	}
}

bool removeInputDevice(int id, bool notify)
{
	auto existingIt = std::find_if(sysInputDev.cbegin(), sysInputDev.cend(),
		[=](const auto &e) { return e->osId == id; });
	if(existingIt == sysInputDev.end())
	{
		logMsg("device id %d not in list", id);
		return false;
	}
	logMsg("removed device id %d from list", id);
	auto devPtr = existingIt->get();
	auto removedDevCopy = *devPtr;
	removeDevice(*devPtr);
	sysInputDev.erase(existingIt);
	if(notify)
		onDeviceChange.callCopySafe(removedDevCopy, { Device::Change::REMOVED });
	return true;
}

static void enumDevices(JNIEnv* env, bool notify)
{
	using namespace Base;
	logMsg("doing input device scan");
	while(sysInputDev.size())
	{
		removeDevice(*sysInputDev.back());
		sysInputDev.pop_back();
	}
	jEnumDevices(env, inputDeviceHelperCls);
	if(!virtualDev)
	{
		logMsg("no \"Virtual\" device id found, adding one");
		AndroidInputDevice vDev{-1, Device::TYPE_BIT_VIRTUAL | Device::TYPE_BIT_KEYBOARD | Device::TYPE_BIT_KEY_MISC, "Virtual"};
		addInputDevice(vDev, false, false);
		auto sysInput = sysInputDev.back().get();
		virtualDev = sysInput;
	}
	if(notify)
	{
		onDevicesEnumerated.callCopySafe();
	}
}

void enumDevices()
{
	enumDevices(Base::jEnvForThread(), true);
}

void init(JNIEnv *env)
{
	if(Base::androidSDK() >= 12)
	{
		auto env = Base::jEnvForThread();
		processInput = processInputWithGetEvent;

		#ifdef ANDROID_COMPAT_API
		// load AMotionEvent_getAxisValue dynamically
		if(!(AMotionEvent_getAxisValueFunc = (typeof(AMotionEvent_getAxisValueFunc))dlsym(RTLD_DEFAULT, "AMotionEvent_getAxisValue")))
		{
			logWarn("AMotionEvent_getAxisValue not found even though using SDK %d", Base::androidSDK());
		}
		#endif

		auto inputDeviceCls = env->FindClass("android/view/InputDevice");
		JavaInstMethod<jobject()> jInputDeviceHelper{env, Base::jBaseActivityCls, "inputDeviceHelper", "()Lcom/imagine/InputDeviceHelper;"};
		auto inputDeviceHelper = jInputDeviceHelper(env, Base::jBaseActivity);
		assert(inputDeviceHelper);
		inputDeviceHelperCls = (jclass)env->NewGlobalRef(env->GetObjectClass(inputDeviceHelper));
		jEnumDevices.setup(env, inputDeviceHelperCls, "enumInputDevices", "()V");
		JNINativeMethod method[]
		{
			{
				"deviceEnumerated", "(ILandroid/view/InputDevice;Ljava/lang/String;IIIZ)V",
				(void*)(void (*)(JNIEnv*, jobject, jint, jobject, jstring, jint, jint, jint, jboolean))
				([](JNIEnv* env, jobject thiz, jint devID, jobject jDev, jstring jName, jint src, jint kbType, jint jsAxisBits, jboolean isPowerButton)
				{
					const char *name = env->GetStringUTFChars(jName, nullptr);
					AndroidInputDevice sysDev{env, jDev, nextEnumID(name), devID, src,
						name, kbType, jsAxisBits, (bool)isPowerButton};
					env->ReleaseStringUTFChars(jName, name);
					addInputDevice(sysDev, false, false);
					// check for special device IDs
					if(devID == -1)
					{
						virtualDev = sysInputDev.back().get();
					}
					else if(devID == 0)
					{
						// built-in keyboard is always id 0 according to Android docs
						builtinKeyboardDev = sysInputDev.back().get();
					}
				})
			},
		};
		env->RegisterNatives(inputDeviceHelperCls, method, std::size(method));

		// device change notifications
		if(usesInputDeviceListener())
		{
			logMsg("setting up input notifications");
			JavaInstMethod<jobject()> jInputDeviceListenerHelper{env, Base::jBaseActivityCls, "inputDeviceListenerHelper", "()Lcom/imagine/InputDeviceListenerHelper;"};
			inputDeviceListenerHelper = jInputDeviceListenerHelper(env, Base::jBaseActivity);
			assert(inputDeviceListenerHelper);
			auto inputDeviceListenerHelperCls = env->GetObjectClass(inputDeviceListenerHelper);
			inputDeviceListenerHelper = env->NewGlobalRef(inputDeviceListenerHelper);
			jRegister.setup(env, inputDeviceListenerHelperCls, "register", "()V");
			jUnregister.setup(env, inputDeviceListenerHelperCls, "unregister", "()V");
			JNINativeMethod method[]
			{
				{
					"deviceChanged", "(IILandroid/view/InputDevice;Ljava/lang/String;III)V",
					(void*)(void (*)(JNIEnv*, jobject, jint, jint, jobject, jstring, jint, jint, jint))
					([](JNIEnv* env, jobject thiz, jint change, jint devID, jobject jDev,
							jstring jName, jint src, jint kbType, jint jsAxisBits)
					{
						if(change == DEVICE_REMOVED) // remove
						{
							removeInputDevice(devID, true);
						}
						else // add or update
						{
							const char *name = env->GetStringUTFChars(jName, nullptr);
							AndroidInputDevice sysDev{env, jDev, nextEnumID(name), devID,
								src, name, kbType, jsAxisBits, false};
							env->ReleaseStringUTFChars(jName, name);
							addInputDevice(sysDev, change == DEVICE_CHANGED, true);
						}
					})
				}
			};
			env->RegisterNatives(inputDeviceListenerHelperCls, method, std::size(method));
			Base::addOnResume([env](bool)
				{
					enumDevices(env, true);
					logMsg("registering input device listener");
					jRegister(env, inputDeviceListenerHelper);
					return true;
				}, Base::INPUT_DEVICES_ON_RESUME_PRIORITY);
			Base::addOnExit([env](bool backgrounded)
				{
					logMsg("unregistering input device listener");
					jUnregister(env, inputDeviceListenerHelper);
					return true;
				}, Base::INPUT_DEVICES_ON_EXIT_PRIORITY);
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
						if(events == Base::POLLEV_IN)
						{
							char buffer[2048];
							auto size = read(fd, buffer, sizeof(buffer));
							if(Base::appIsRunning())
							{
								inputRescanCallback->run(IG::Milliseconds(250));
							}
						}
						return 1;
					}, nullptr);
				if(ret != 1)
				{
					logErr("couldn't add inotify fd to looper");
				}
				Base::addOnResume([env](bool)
					{
						inputRescanCallback.emplace("inputRescanCallback",
							[]()
							{
								enumDevices(Base::jEnvForThread(), true);
							});
						enumDevices(env, true);
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
					}, Base::INPUT_DEVICES_ON_RESUME_PRIORITY);
				Base::addOnExit([env](bool backgrounded)
					{
						if(watch != -1)
						{
							logMsg("unregistering inotify input device listener");
							inotify_rm_watch(inputDevNotifyFd, watch);
							watch = -1;
							inputRescanCallback.reset();
						}
						return true;
					}, Base::INPUT_DEVICES_ON_EXIT_PRIORITY);
			}
		}
	}
	else
	{
		// no multi-input device support
		auto genericKeyDev = makeGenericKeyDevice();
		if(Config::MACHINE_IS_GENERIC_ARMV7)
		{
			auto buildDevice = Base::androidBuildDevice();
			if(Base::isXperiaPlayDeviceStr(buildDevice.data()))
			{
				logMsg("detected Xperia Play gamepad");
				genericKeyDev.subtype_ = Device::SUBTYPE_XPERIA_PLAY;
			}
			else if(string_equal(buildDevice.data(), "sholes"))
			{
				logMsg("detected Droid/Milestone keyboard");
				genericKeyDev.subtype_ = Device::SUBTYPE_MOTO_DROID_KEYBOARD;
			}
		}
		sysInputDev.reserve(1);
		addInputDevice(genericKeyDev, false, false);
		builtinKeyboardDev = sysInputDev.back().get();
	}
}

}
