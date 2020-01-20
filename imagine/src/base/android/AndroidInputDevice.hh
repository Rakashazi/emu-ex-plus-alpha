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
#include <imagine/util/string.h>
#include <imagine/util/jni.hh>

namespace Input
{

	namespace AInputDevice
	{

	static constexpr jint SOURCE_CLASS_BUTTON = 0x00000001, SOURCE_CLASS_POINTER = 0x00000002, SOURCE_CLASS_TRACKBALL = 0x00000004,
			SOURCE_CLASS_POSITION = 0x00000008, SOURCE_CLASS_JOYSTICK = 0x00000010;
	static constexpr jint SOURCE_KEYBOARD = 0x00000101, SOURCE_DPAD = 0x00000201, SOURCE_GAMEPAD = 0x00000401,
			SOURCE_TOUCHSCREEN = 0x00001002, SOURCE_MOUSE = 0x00002002, SOURCE_STYLUS = 0x00004002,
			SOURCE_TRACKBALL = 0x00010004, SOURCE_TOUCHPAD = 0x00100008, SOURCE_JOYSTICK = 0x01000010;

	static constexpr jint KEYBOARD_TYPE_NONE = 0,  KEYBOARD_TYPE_NON_ALPHABETIC = 1, KEYBOARD_TYPE_ALPHABETIC = 2;

	}

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
		constexpr Axis(uint8 id, AxisKeyEmu<float> keyEmu): id{id}, keyEmu{keyEmu} {}
		uint8 id = 0;
		AxisKeyEmu<float> keyEmu;
	};
	StaticArrayList<Axis, MAX_AXES> axis;

	AndroidInputDevice(int osId, uint32_t typeBits, const char *name):
		Device{0, Event::MAP_SYSTEM, typeBits, name},
		osId{osId}
	{}

	AndroidInputDevice(JNIEnv* env, jobject aDev, uint32_t enumId, int osId, int src,
		const char *name, int kbType, int axisBits, bool isPowerButton);

	bool operator ==(AndroidInputDevice const& rhs) const
	{
		return osId == rhs.osId && string_equal(name(), rhs.name());
	}

	void setTypeBits(int bits) { type_ = bits; }

	void setJoystickAxisAsDpadBits(uint32_t axisMask) final;
	uint32_t joystickAxisAsDpadBits() final;
	uint32_t joystickAxisAsDpadBitsDefault() final { return joystickAxisAsDpadBitsDefault_; };
	uint32_t joystickAxisBits() final { return axisBits; };

	void setICadeMode(bool on) final
	{
		logMsg("set iCade mode %s for %s", on ? "on" : "off", name());
		iCadeMode_ = on;
	}

	bool iCadeMode() const final
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
