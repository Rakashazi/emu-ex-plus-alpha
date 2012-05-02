#define thisModuleName "input:android"
#include <base/android/sdk.hh>
#include <input/interface.h>
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
static fbool handleVolumeKeys = 0;
static fbool allowKeyRepeats = 1;

static const uint maxJoysticks = 5;
static uint joysticks = 0;
struct JoystickID
{
	constexpr JoystickID(): osId(0), devId(0) { }
	uint osId, devId;
};
static JoystickID joystickID[maxJoysticks];

// JNI classes/methods
struct JNIInputDevice
{
	static jclass cls;
	static JavaClassMethod getDeviceIds, getDevice;
	static JavaInstMethod getName, getSources;
	static const jint SOURCE_GAMEPAD = 0x00000401, SOURCE_JOYSTICK = 0x01000010;

	static void jniInit()
	{
		using namespace Base;
		cls = (jclass)jEnv->NewGlobalRef(jEnv->FindClass("android/view/InputDevice"));
		getDeviceIds.setup(jEnv, cls, "getDeviceIds", "()[I");
		getDevice.setup(jEnv, cls, "getDevice", "(I)Landroid/view/InputDevice;");
		getName.setup(jEnv, cls, "getName", "()Ljava/lang/String;");
		getSources.setup(jEnv, cls, "getSources", "()I");
	}
};

jclass JNIInputDevice::cls = 0;
JavaClassMethod JNIInputDevice::getDeviceIds, JNIInputDevice::getDevice;
JavaInstMethod JNIInputDevice::getName, JNIInputDevice::getSources;

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

fbool sendInputToIME = 1;
void setEventsUseOSInputMethod(fbool on)
{
	logMsg("set IME use %s", on ? "On" : "Off");
	sendInputToIME = on;
}

fbool eventsUseOSInputMethod()
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
		case AINPUT_SOURCE_ANY: return "Any";
		default:  return "Unhandled value";
	}
}

int32_t onInputEvent(struct android_app* app, AInputEvent* event)
{
	var_copy(type, AInputEvent_getType(event));
	var_copy(source, AInputEvent_getSource(event));
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
					int action = eventAction & AMOTION_EVENT_ACTION_MASK;
					int pid = eventAction >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
					int pointers = AMotionEvent_getPointerCount(event);
					for (int i = 0; i < pointers; i++)
					{
						int sendAction = action;
						int pointerId = AMotionEvent_getPointerId(event, i);
						if(action == AMOTION_EVENT_ACTION_POINTER_DOWN || action == AMOTION_EVENT_ACTION_POINTER_UP)
						{
							// send ACTION_POINTER_* for only the pointer it belongs to
							if(pid != pointerId)
								sendAction = AMOTION_EVENT_ACTION_MOVE;
						}
						handleTouchEvent(sendAction, AMotionEvent_getX(event, i), AMotionEvent_getY(event, i), pointerId);
					}
					return 1;
				}
				case 0x01000010: // Joystick
				{
					uint eventDevID = 0;
					uint id = AInputEvent_getDeviceId(event);
					iterateTimes(joysticks, i)
					{
						if(joystickID[i].osId == id)
						{
							eventDevID = joystickID[i].devId;
							break;
						}
					}
					//logMsg("input from JS %d -> %d", id, eventDevID);

					static const int maxAxes = 3; // 2 sticks + POV hat
					/*int count = AMotionEvent_getPointerCount(event);
					logMsg("got %d js events", count);*/
					/*if(AMotionEvent_getAxisValue)
					{
					logMsg("axis [%f %f %f] [%f %f %f] [%f %f]", (double)AMotionEvent_getAxisValue(event, 0, 0), (double)AMotionEvent_getAxisValue(event, 1, 0), (double)AMotionEvent_getAxisValue(event, 2, 0),
							(double)AMotionEvent_getAxisValue(event, 12, 0), (double)AMotionEvent_getAxisValue(event, 13, 0), (double)AMotionEvent_getAxisValue(event, 14, 0),
							(double)AMotionEvent_getAxisValue(event, 15, 0), (double)AMotionEvent_getAxisValue(event, 16, 0));
					}*/

					iterateTimes(AMotionEvent_getAxisValue ? maxAxes : 1, i)
					{
						/*int id = AMotionEvent_getPointerId(event, i);*/
						static const int32_t AXIS_X = 0, AXIS_Y = 1, AXIS_Z = 11, AXIS_RZ = 14, AXIS_HAT_X = 15, AXIS_HAT_Y = 16;
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
								bdefault: bug_branch("%d", i);
							}
						}
						else
						{
							pos[0] = AMotionEvent_getX(event, 0);
							pos[1] = AMotionEvent_getY(event, 0);
						}
						logMsg("from Joystick, %f, %f", (double)pos[0], (double)pos[1]);
						static bool stickBtn[maxJoysticks][maxAxes][4] = { { { 0 } } };
						forEachInArray(stickBtn[eventDevID][i], e)
						{
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
								static const uint btnEvent[4] =
								{
									Input::Key::LEFT, Input::Key::RIGHT, Input::Key::DOWN, Input::Key::UP,
								};
								Input::onInputEvent(InputEvent(eventDevID, InputEvent::DEV_KEYBOARD, btnEvent[e_i], newState ? INPUT_PUSHED : INPUT_RELEASED));
								//callSafe(Input::onInputEventHandler, Input::onInputEventHandlerCtx, InputEvent(eventDevID, InputEvent::DEV_KEYBOARD, btnEvent[e_i], newState ? INPUT_PUSHED : INPUT_RELEASED));
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
			uint keyCode = AKeyEvent_getKeyCode(event);
			if(!handleVolumeKeys &&
				(keyCode == Key::VOL_UP || keyCode == Key::VOL_DOWN))
				return 0;
			uint devId = 0;
			if(bit_isMaskSet(source, JNIInputDevice::SOURCE_GAMEPAD))
			{
				uint id = AInputEvent_getDeviceId(event);
				iterateTimes(joysticks, i)
				{
					if(joystickID[i].osId == id)
					{
						devId = joystickID[i].devId;
						break;
					}
				}
				//logMsg("input from JS %d -> %d", id, devId);
			}
			//logMsg("key event, code: %d source: %d repeat: %d action: %d", keyCode, source, AKeyEvent_getRepeatCount(event), AKeyEvent_getAction(event));
			if(allowKeyRepeats || AKeyEvent_getRepeatCount(event) == 0)
			{
				handleKeyEvent(keyCode, AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_UP ? 0 : 1, devId);
			}
			return 1;
		}
	}
	logWarn("unhandled input event type %d", type);
	return 0;
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

CallResult init()
{
	commonInit();

	if(Base::androidSDK() >= 12)
	{
		JNIInputDevice::jniInit();
		using namespace Base;
		jintArray jID = (jintArray)jEnv->CallStaticObjectMethod(JNIInputDevice::getDeviceIds.c, JNIInputDevice::getDeviceIds.m);
		jint *id = jEnv->GetIntArrayElements(jID, 0);
		logMsg("checking input devices");
		uint devId = 0;
		iterateTimes(jEnv->GetArrayLength(jID), i)
		{
			jobject dev = jEnv->CallStaticObjectMethod(JNIInputDevice::getDevice.c, JNIInputDevice::getDevice.m, id[i]);
			jint src = jEnv->CallIntMethod(dev, JNIInputDevice::getSources.m);
			jstring jName = (jstring)jEnv->CallObjectMethod(dev, JNIInputDevice::getName.m);
			if(!jName)
			{
				logWarn("no name from device %d, id %d", i, id[i]);
				continue;
			}
			const char *name = jEnv->GetStringUTFChars(jName, 0);
			bool isJoystick = bit_isMaskSet(src, JNIInputDevice::SOURCE_GAMEPAD) || bit_isMaskSet(src, JNIInputDevice::SOURCE_JOYSTICK);
			logMsg("#%d: %s, id %d, source %X, is Gamepad/JS %d", i, name, id[i], src, isJoystick);
			if(isJoystick && joysticks < maxJoysticks)
			{
				joystickID[joysticks].osId = id[i];
				if(strstr(name, "-zeus")) // map Xperia Play slide-out gamepad to idx 0
				{
					joystickID[joysticks].devId = 0;
					if(devId == 0) devId++; // only increment devId if first device
				}
				else
				{
					joystickID[joysticks].devId = devId;
					devId++;
				}
				logMsg("Joystick ID %d -> %d", id[i], joystickID[joysticks].devId);
				joysticks++;
			}
			jEnv->ReleaseStringUTFChars(jName, name);
		}
		jEnv->ReleaseIntArrayElements(jID, id, 0);
	}
	return OK;
}

}

#undef thisModuleName
