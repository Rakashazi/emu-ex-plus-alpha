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

#include <imagine/input/Device.hh>
#include <imagine/input/AxisKeyEmu.hh>
#include <imagine/util/container/ArrayList.hh>
#include <imagine/util/basicString.h>
#include <imagine/util/jni.hh>

namespace Input
{

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
		cls = (jclass)jEnv()->NewGlobalRef(jEnv()->FindClass("android/view/InputDevice$MotionRange"));
		getMax_.setup(jEnv(), cls, "getMax", "()F");
		getMin_.setup(jEnv(), cls, "getMin", "()F");
	}*/
};

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

//	jstring getDescriptor(JNIEnv *j)
//	{
//		return (jstring)getDescriptor_(j, o);
//	}

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
	static JavaInstMethod<jobject> getName_, getKeyCharacterMap_, getMotionRange_, getDescriptor_;
	static JavaInstMethod<jint> getSources_, getKeyboardType_;
	static constexpr jint SOURCE_CLASS_BUTTON = 0x00000001, SOURCE_CLASS_POINTER = 0x00000002, SOURCE_CLASS_TRACKBALL = 0x00000004,
			SOURCE_CLASS_POSITION = 0x00000008, SOURCE_CLASS_JOYSTICK = 0x00000010;
	static constexpr jint SOURCE_KEYBOARD = 0x00000101, SOURCE_DPAD = 0x00000201, SOURCE_GAMEPAD = 0x00000401,
			SOURCE_TOUCHSCREEN = 0x00001002, SOURCE_MOUSE = 0x00002002, SOURCE_STYLUS = 0x00004002,
			SOURCE_TRACKBALL = 0x00010004, SOURCE_TOUCHPAD = 0x00100008, SOURCE_JOYSTICK = 0x01000010;

	static constexpr jint KEYBOARD_TYPE_NONE = 0,  KEYBOARD_TYPE_NON_ALPHABETIC = 1, KEYBOARD_TYPE_ALPHABETIC = 2;

	static void jniInit(JNIEnv* env)
	{
		using namespace Base;
		cls = (jclass)env->NewGlobalRef(env->FindClass("android/view/InputDevice"));
		getDeviceIds_.setup(env, cls, "getDeviceIds", "()[I");
		getDevice_.setup(env, cls, "getDevice", "(I)Landroid/view/InputDevice;");
		getName_.setup(env, cls, "getName", "()Ljava/lang/String;");
		//getDescriptor_.setup(env, cls, "getDescriptor", "()Ljava/lang/String;");
		getSources_.setup(env, cls, "getSources", "()I");
		getKeyboardType_.setup(env, cls, "getKeyboardType", "()I");
		getMotionRange_.setup(env, cls, "getMotionRange", "(I)Landroid/view/InputDevice$MotionRange;");
	}
};

struct AndroidInputDevice : public Input::Device
{
	int osId = 0;
	char nameStr[48]{};
	uint joystickAxisAsDpadBits_ = 0, joystickAxisAsDpadBitsDefault_ = 0;
	uint axisBits = 0;
	bool iCadeMode_ = false;
	//static constexpr uint MAX_STICK_AXES = 6; // 6 possible axes defined in key codes
	static constexpr uint MAX_AXES = 10;
	//static_assert(MAX_STICK_AXES <= MAX_AXES, "MAX_AXES must be large enough to hold MAX_STICK_AXES");

	struct Axis
	{
		constexpr Axis() {}
		constexpr Axis(uint8 id, AxisKeyEmu<float> keyEmu): id{id}, keyEmu{keyEmu} {}
		uint8 id = 0;
		AxisKeyEmu<float> keyEmu;
	};
	StaticArrayList<Axis, MAX_AXES> axis;

	constexpr AndroidInputDevice():
		Device{0, Event::MAP_SYSTEM, Device::TYPE_BIT_KEY_MISC, nameStr}
	{}

	constexpr AndroidInputDevice(int osId, uint typeBits):
		Device{0, Event::MAP_SYSTEM, typeBits, nameStr},
		osId{osId}
	{}

	AndroidInputDevice(int osId, uint typeBits, const char *name):
		Device{0, Event::MAP_SYSTEM, typeBits, nameStr},
		osId{osId}
	{
		string_copy(nameStr, name);
	}

	AndroidInputDevice(JNIEnv* env, AInputDeviceJ aDev, uint enumId, int osId, int src, const char *name);

	bool operator ==(AndroidInputDevice const& rhs) const
	{
		return osId == rhs.osId && string_equal(nameStr, rhs.nameStr);
	}

	void setTypeBits(int bits) { type_ = bits; }

	void setJoystickAxisAsDpadBits(uint axisMask) override;
	uint joystickAxisAsDpadBits() override;
	uint joystickAxisAsDpadBitsDefault() override { return joystickAxisAsDpadBitsDefault_; };
	uint joystickAxisBits() override { return axisBits; };

	void setICadeMode(bool on) override
	{
		logMsg("set iCade mode %s for %s", on ? "on" : "off", name());
		iCadeMode_ = on;
	}

	bool iCadeMode() const override
	{
		return iCadeMode_;
	}
};

static constexpr int AXIS_X = 0, AXIS_Y = 1, AXIS_Z = 11,
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
		case AXIS_RX: return Keycode::JS3_XAXIS_POS;
		case AXIS_RY: return Keycode::JS3_YAXIS_POS;
		case AXIS_HAT_X: return Keycode::JS_POV_XAXIS_POS;
		case AXIS_HAT_Y: return Keycode::JS_POV_YAXIS_POS;
		case AXIS_RUDDER: return Keycode::JS_RUDDER_AXIS_POS;
		case AXIS_WHEEL: return Keycode::JS_WHEEL_AXIS_POS;
		case AXIS_LTRIGGER: return Keycode::JS_LTRIGGER_AXIS;
		case AXIS_RTRIGGER: return Keycode::JS_RTRIGGER_AXIS;
		// map brake/gas to L/R triggers for now
		case AXIS_BRAKE : return Keycode::JS_LTRIGGER_AXIS;//return Keycode::JS_BRAKE_AXIS;
		case AXIS_GAS : return Keycode::JS_RTRIGGER_AXIS;//return Keycode::JS_GAS_AXIS;
	}
	return Keycode::JS3_YAXIS_POS;
}

}
