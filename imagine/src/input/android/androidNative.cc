#define thisModuleName "input:android"
#include <base/android/sdk.hh>
#include <input/common/common.h>
#include <base/android/nativeGlue.hh>
#include <base/android/private.hh>
#include <util/jni.hh>
#include <android/input.h>
#include <dlfcn.h>

#include "common.hh"

namespace Input
{

static float (*AMotionEvent_getAxisValue)(const AInputEvent* motion_event, int32_t axis, size_t pointer_index) = 0;
static bool handleVolumeKeys = 0;
static bool allowKeyRepeats = 1;
static const int AINPUT_SOURCE_JOYSTICK = 0x01000010;
static const uint maxJoystickAxisPairs = 4; // 2 sticks + POV hat + L/R Triggers
static const uint maxSysInputDevs = MAX_DEVS;
static uint sysInputDevs = 0;
struct SysInputDevice
{
	constexpr SysInputDevice() { }
	int osId = 0;
	Device *dev = nullptr;
	char name[48] {0};
	bool axisBtnState[maxJoystickAxisPairs][4] { { 0 } };

	bool operator ==(SysInputDevice const& rhs) const
	{
		return osId == rhs.osId && string_equal(name, rhs.name);
	}
};
static SysInputDevice sysInputDev[maxSysInputDevs];
static Device *builtinKeyboardDev = nullptr;
static Device *virtualDev = nullptr;

// JNI classes/methods
struct JNIInputDevice
{
	static jclass cls;
	static JavaClassMethod<jobject> getDeviceIds, getDevice;
	static JavaInstMethod<jobject> getName;
	static JavaInstMethod<jint> getSources;
	static constexpr jint SOURCE_DPAD = 0x00000201, SOURCE_GAMEPAD = 0x00000401, SOURCE_JOYSTICK = 0x01000010,
			SOURCE_CLASS_BUTTON = 0x00000001;

	static void jniInit()
	{
		using namespace Base;
		cls = (jclass)eEnv()->NewGlobalRef(eEnv()->FindClass("android/view/InputDevice"));
		getDeviceIds.setup(eEnv(), cls, "getDeviceIds", "()[I");
		getDevice.setup(eEnv(), cls, "getDevice", "(I)Landroid/view/InputDevice;");
		getName.setup(eEnv(), cls, "getName", "()Ljava/lang/String;");
		getSources.setup(eEnv(), cls, "getSources", "()I");
	}
};

jclass JNIInputDevice::cls = 0;
JavaClassMethod<jobject> JNIInputDevice::getDeviceIds, JNIInputDevice::getDevice;
JavaInstMethod<jobject> JNIInputDevice::getName;
JavaInstMethod<jint> JNIInputDevice::getSources;

void setKeyRepeat(bool on)
{
	logMsg("set key repeat %s", on ? "On" : "Off");
	allowKeyRepeats = on;
}

void setHandleVolumeKeys(bool on)
{
	logMsg("set volume key use %s", on ? "On" : "Off");
	handleVolumeKeys = on;
}

void showSoftInput()
{
	using namespace Base;
	logMsg("showing soft input");
	postUIThread(eEnv(), jBaseActivity, 2, 0);
}

void hideSoftInput()
{
	using namespace Base;
	logMsg("hiding soft input");
	postUIThread(eEnv(), jBaseActivity, 3, 0);
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

static void handleKeycodesForSpecialDevices(const Input::Device &dev, int32_t &keyCode, int32_t &metaState)
{
	switch(dev.subtype)
	{
		#ifdef __ARM_ARCH_7A__
		bcase Device::SUBTYPE_XPERIA_PLAY:
		{
			if(unlikely(keyCode == (int)Keycode::ESCAPE && (metaState & AMETA_ALT_ON)))
			{
				keyCode = Keycode::GAME_B;
			}
		}
		#endif
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

int32_t onInputEvent(struct android_app* app, AInputEvent* event)
{
	auto type = AInputEvent_getType(event);
	auto source = AInputEvent_getSource(event);
	switch(type)
	{
		case AINPUT_EVENT_TYPE_MOTION:
		{
			int eventAction = AMotionEvent_getAction(event);
			//logMsg("get motion event action %d", eventAction);

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
				case AINPUT_SOURCE_MOUSE:
				{
					//logMsg("from touchscreen or mouse");
					uint action = eventAction & AMOTION_EVENT_ACTION_MASK;
					if(action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL)
					{
						// touch gesture ended
						handleTouchEvent(AMOTION_EVENT_ACTION_UP, AMotionEvent_getX(event, 0), AMotionEvent_getY(event, 0), AMotionEvent_getPointerId(event, 0));
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
							AMotionEvent_getPointerId(event, i));
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

					static const uint altBtnEvent[maxJoystickAxisPairs][4] =
					{
							{ Input::Keycode::LEFT, Input::Keycode::RIGHT, Input::Keycode::DOWN, Input::Keycode::UP },
							{ Keycode::JS2_XAXIS_NEG, Keycode::JS2_XAXIS_POS, Keycode::JS2_YAXIS_POS, Keycode::JS2_YAXIS_NEG },
							{ Keycode::JS3_XAXIS_NEG, Keycode::JS3_XAXIS_POS, Keycode::JS3_YAXIS_POS, Keycode::JS3_YAXIS_NEG },
							{ 0, Keycode::JS_LTRIGGER_AXIS, Keycode::JS_RTRIGGER_AXIS, 0 }
					};
					static const uint btnEvent[maxJoystickAxisPairs][4] =
					{
							{ Keycode::JS1_XAXIS_NEG, Keycode::JS1_XAXIS_POS, Keycode::JS1_YAXIS_POS, Keycode::JS1_YAXIS_NEG },
							{ Keycode::JS2_XAXIS_NEG, Keycode::JS2_XAXIS_POS, Keycode::JS2_YAXIS_POS, Keycode::JS2_YAXIS_NEG },
							{ Keycode::JS3_XAXIS_NEG, Keycode::JS3_XAXIS_POS, Keycode::JS3_YAXIS_POS, Keycode::JS3_YAXIS_NEG },
							{ 0, Keycode::JS_LTRIGGER_AXIS, Keycode::JS_RTRIGGER_AXIS, 0 }
					};

					auto &axisToBtnMap = dev->mapJoystickAxis1ToDpad ? altBtnEvent : btnEvent;
					auto &sysDev = sysInputDev[dev->idx];

//					if(AMotionEvent_getAxisValue)
//					{
//					logMsg("axis [%f %f] [%f %f] [%f %f] [%f %f]",
//							(double)AMotionEvent_getAxisValue(event, 0, 0), (double)AMotionEvent_getAxisValue(event, 1, 0),
//							(double)AMotionEvent_getAxisValue(event, 11, 0), (double)AMotionEvent_getAxisValue(event, 14, 0),
//							(double)AMotionEvent_getAxisValue(event, 15, 0), (double)AMotionEvent_getAxisValue(event, 16, 0),
//							(double)AMotionEvent_getAxisValue(event, 17, 0), (double)AMotionEvent_getAxisValue(event, 18, 0));
//					}

					iterateTimes(AMotionEvent_getAxisValue ? maxJoystickAxisPairs : 1, i)
					{
						static const int32_t AXIS_X = 0, AXIS_Y = 1, AXIS_Z = 11, AXIS_RZ = 14, AXIS_HAT_X = 15, AXIS_HAT_Y = 16,
								AXIS_LTRIGGER = 17, AXIS_RTRIGGER = 18;
						float pos[2];
						if(AMotionEvent_getAxisValue)
						{
							switch(i)
							{
								bcase 0:
									pos[0] = AMotionEvent_getAxisValue(event, AXIS_X, 0);
									pos[1] = AMotionEvent_getAxisValue(event, AXIS_Y, 0);
								bcase 1:
									pos[0] = AMotionEvent_getAxisValue(event, AXIS_Z, 0);
									pos[1] = AMotionEvent_getAxisValue(event, AXIS_RZ, 0);
								bcase 2:
									pos[0] = AMotionEvent_getAxisValue(event, AXIS_HAT_X, 0);
									pos[1] = AMotionEvent_getAxisValue(event, AXIS_HAT_Y, 0);
								bcase 3:
									pos[0] = AMotionEvent_getAxisValue(event, AXIS_LTRIGGER, 0);
									pos[1] = AMotionEvent_getAxisValue(event, AXIS_RTRIGGER, 0);
								bdefault: bug_branch("%d", i);
							}
						}
						else
						{
							pos[0] = AMotionEvent_getX(event, 0);
							pos[1] = AMotionEvent_getY(event, 0);
						}
						//logMsg("from Joystick, %f, %f", (double)pos[0], (double)pos[1]);
						forEachInArray(sysDev.axisBtnState[i], e)
						{
							if(i == 3 && (e_i == 0 || e_i == 3))
								continue; // skip negative test for trigger axis
							bool newState;
							switch(e_i)
							{
								case 0: newState = pos[0] < -0.5; break;
								case 1: newState = pos[0] > 0.5; break;
								case 2: newState = pos[1] > 0.5; break;
								case 3: newState = pos[1] < -0.5; break;
								default: bug_branch("%d", (int)e_i); break;
							}
							if(*e != newState)
							{
								onInputEvent(Event(eventDevID, Event::MAP_KEYBOARD, axisToBtnMap[i][e_i],
										newState ? PUSHED : RELEASED, 0, dev));
							}
							*e = newState;
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
			//logMsg("key event, code: %d id: %d source: %d repeat: %d action: %d", keyCode, AInputEvent_getDeviceId(event), source, AKeyEvent_getRepeatCount(event), AKeyEvent_getAction(event));
			if(!keyCode) // ignore "unknown" key codes
				return 0;
			if(!handleVolumeKeys &&
				(keyCode == (int)Keycode::VOL_UP || keyCode == (int)Keycode::VOL_DOWN))
			{
				return 0;
			}
			auto isGamepad = bit_isMaskSet(source, JNIInputDevice::SOURCE_GAMEPAD);

			// always accept repeats from gamepads because 2+ pads pushing the same button is
			// considered a repeat by the OS
			if(allowKeyRepeats || isGamepad || AKeyEvent_getRepeatCount(event) == 0)
			{
				auto dev = deviceForInputId(AInputEvent_getDeviceId(event));
				if(unlikely(!dev))
				{
					assert(virtualDev);
					//logWarn("re-mapping unknown device ID %d to Virtual", AInputEvent_getDeviceId(event));
					dev = virtualDev;
				}
				auto metaState = AKeyEvent_getMetaState(event);
				handleKeycodesForSpecialDevices(*dev, keyCode, metaState);
				handleKeyEvent(keyCode, AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_UP ? 0 : 1, dev->devId, metaState & AMETA_SHIFT_ON, *dev);
			}
			return 1;
		}
	}
	logWarn("unhandled input event type %d", type);
	return 0;
}

void textInputEndedMsg(const char* str, jstring jStr)
{
	vKeyboardTextDelegate.invoke(str);
	vKeyboardTextDelegate.clear();
	if(jStr)
	{
		Base::eEnv()->ReleaseStringUTFChars(jStr, str);
		Base::eEnv()->DeleteGlobalRef(jStr);
	}
}

static void JNICALL textInputEnded(JNIEnv* env, jobject thiz, jstring jStr)
{
	if(vKeyboardTextDelegate.hasCallback())
	{
		if(jStr)
		{
			const char *str = env->GetStringUTFChars(jStr, 0);
			logMsg("running text entry callback with text: %s", str);
			Base::sendTextEntryEnded(str, (jstring)env->NewGlobalRef(jStr));
		}
		else
		{
			logMsg("canceled text entry callback");
			Base::sendTextEntryEnded(nullptr, nullptr);
		}
	}
	else
		vKeyboardTextDelegate.clear();
}

bool dlLoadAndroidFuncs(void *libandroid)
{
	if(Base::androidSDK() < 12)
	{
		return 0;
	}
	// Google seems to have forgotten to put AMotionEvent_getAxisValue() in the NDK libandroid.so even though it's
	// present in at least Android 4.0, so we'll load it dynamically to be safe
	if((AMotionEvent_getAxisValue = (float (*)(const AInputEvent*, int32_t, size_t))dlsym(libandroid, "AMotionEvent_getAxisValue"))
			== 0)
	{
		logWarn("AMotionEvent_getAxisValue not found");
		return 0;
	}
	return 1;
}

void rescanDevices(bool firstRun)
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
	auto jID = (jintArray)JNIInputDevice::getDeviceIds(eEnv());
	auto id = eEnv()->GetIntArrayElements(jID, 0);
	bool foundVirtual = 0;
	logMsg("checking input devices");
	iterateTimes(eEnv()->GetArrayLength(jID), i)
	{
		jobject dev = JNIInputDevice::getDevice(eEnv(), id[i]);
		jint src = JNIInputDevice::getSources(eEnv(), dev);
		jstring jName = (jstring)JNIInputDevice::getName(eEnv(), dev);
		if(!jName)
		{
			logWarn("no name from device %d, id %d", i, id[i]);
			continue;
		}
		const char *name = eEnv()->GetStringUTFChars(jName, 0);
		bool hasKeys = src & JNIInputDevice::SOURCE_CLASS_BUTTON;
		logMsg("#%d: %s, id %d, source %X", i, name, id[i], src);
		if(hasKeys && !devList.isFull())
		{
			auto &sysInput = sysInputDev[sysInputDevs];
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
			mem_zero(sysInput.axisBtnState);
			if(id[i] == 0) // built-in keyboard is always id 0 according to Android docs
			{
				builtinKeyboardDev = newDev;
			}
			else if(id[i] == -1)
			{
				foundVirtual = 1;
				virtualDev = newDev;
			}

			// Special device handling
			if(bit_isMaskSet(src, JNIInputDevice::SOURCE_GAMEPAD) && !bit_isMaskSet(src, JNIInputDevice::SOURCE_DPAD))
			{
				#ifdef __ARM_ARCH_7A__
				if(strstr(name, "-zeus"))
				{
					logMsg("detected Xperia Play gamepad");
					newDev->subtype = Device::SUBTYPE_XPERIA_PLAY;
				}
				else
				#endif
				if(string_equal(name, "Sony PLAYSTATION(R)3 Controller"))
				{
					logMsg("detected PS3 gamepad");
					newDev->subtype = Device::SUBTYPE_PS3_CONTROLLER;
				}
				else
				{
					logMsg("detected a gamepad");
				}
				newDev->setTypeBits(Device::TYPE_BIT_GAMEPAD);
			}
			newDev->setTypeBits(newDev->typeBits() |
					(bit_isMaskSet(src, JNIInputDevice::SOURCE_JOYSTICK) ? Device::TYPE_BIT_JOYSTICK : 0));

			logMsg("added to list with device id %d", newDev->devId);
			sysInputDevs++;
		}
		eEnv()->ReleaseStringUTFChars(jName, name);
	}
	eEnv()->ReleaseIntArrayElements(jID, id, 0);

	if(!foundVirtual)
	{
		if(sysInputDevs == MAX_DEVS)
		{
			// remove last device to make room
			devList.remove(*sysInputDev[sysInputDevs-1].dev);
			sysInputDevs--;
		}
		logMsg("no \"Virtual\" device id found, adding one");
		Input::addDevice((Device){0, Event::MAP_KEYBOARD, Device::TYPE_BIT_KEY_MISC, "Virtual"});
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

CallResult init()
{
	if(Base::androidSDK() >= 12)
	{
		JNIInputDevice::jniInit();
		rescanDevices(1);
	}
	else
	{
		// no multi-input device support
		Device genericKeyDev { 0, Event::MAP_KEYBOARD, Device::TYPE_BIT_KEY_MISC, "Key Input (All Devices)" };
		#ifdef __ARM_ARCH_7A__
		if(Base::runningDeviceType() == Base::DEV_TYPE_XPERIA_PLAY)
		{
			genericKeyDev.subtype = Device::SUBTYPE_XPERIA_PLAY;
		}
		#endif
		Input::addDevice(genericKeyDev);
		builtinKeyboardDev = devList.last();
	}
	return OK;
}

}
