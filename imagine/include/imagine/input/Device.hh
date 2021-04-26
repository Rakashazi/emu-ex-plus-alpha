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

#include <imagine/config/defs.hh>
#include <imagine/input/config.hh>
#include <imagine/input/inputDefs.hh>
#include <imagine/util/bitset.hh>
#include <imagine/util/DelegateFunc.hh>
#include <string>

namespace Base
{
class ApplicationContext;
}

namespace Input
{

enum class DeviceAction: uint8_t { ADDED, REMOVED, CHANGED, SHOWN, HIDDEN, CONNECT_ERROR };

class DeviceChange
{
public:
	constexpr DeviceChange(DeviceAction action): action(action) {}
	constexpr bool added() const { return action == DeviceAction::ADDED; }
	constexpr bool removed() const { return action == DeviceAction::REMOVED; }
	constexpr bool changed() const { return action == DeviceAction::CHANGED; }
	constexpr bool shown() const { return action == DeviceAction::SHOWN; }
	constexpr bool hidden() const { return action == DeviceAction::HIDDEN; }
	constexpr bool hadConnectError() const { return action == DeviceAction::CONNECT_ERROR; }

protected:
	DeviceAction action;
};

class Device
{
public:
	static constexpr uint32_t
		SUBTYPE_NONE = 0,
		SUBTYPE_XPERIA_PLAY = 1,
		SUBTYPE_PS3_CONTROLLER = 2,
		SUBTYPE_MOTO_DROID_KEYBOARD = 3,
		SUBTYPE_OUYA_CONTROLLER = 4,
		SUBTYPE_PANDORA_HANDHELD = 5,
		SUBTYPE_XBOX_360_CONTROLLER = 6,
		SUBTYPE_NVIDIA_SHIELD = 7,
		SUBTYPE_GENERIC_GAMEPAD = 8,
		SUBTYPE_APPLE_EXTENDED_GAMEPAD = 9,
		SUBTYPE_8BITDO_SF30_PRO = 10,
		SUBTYPE_8BITDO_SN30_PRO_PLUS = 11,
		SUBTYPE_8BITDO_M30_GAMEPAD = 12;

	static constexpr uint32_t
		TYPE_BIT_KEY_MISC = IG::bit(0),
		TYPE_BIT_KEYBOARD = IG::bit(1),
		TYPE_BIT_GAMEPAD = IG::bit(2),
		TYPE_BIT_JOYSTICK = IG::bit(3),
		TYPE_BIT_VIRTUAL = IG::bit(4),
		TYPE_BIT_MOUSE = IG::bit(5),
		TYPE_BIT_TOUCHSCREEN = IG::bit(6),
		TYPE_BIT_POWER_BUTTON = IG::bit(7);

	static constexpr uint32_t
		AXIS_BIT_X = IG::bit(0), AXIS_BIT_Y = IG::bit(1), AXIS_BIT_Z = IG::bit(2),
		AXIS_BIT_RX = IG::bit(3), AXIS_BIT_RY = IG::bit(4), AXIS_BIT_RZ = IG::bit(5),
		AXIS_BIT_HAT_X = IG::bit(6), AXIS_BIT_HAT_Y = IG::bit(7),
		AXIS_BIT_LTRIGGER = IG::bit(8), AXIS_BIT_RTRIGGER = IG::bit(9),
		AXIS_BIT_RUDDER = IG::bit(10), AXIS_BIT_WHEEL = IG::bit(11),
		AXIS_BIT_GAS = IG::bit(12), AXIS_BIT_BRAKE = IG::bit(13);

	static constexpr uint32_t
		AXIS_BITS_STICK_1 = AXIS_BIT_X | AXIS_BIT_Y,
		AXIS_BITS_STICK_2 = AXIS_BIT_Z | AXIS_BIT_RZ,
		AXIS_BITS_HAT = AXIS_BIT_HAT_X | AXIS_BIT_HAT_Y;

	Device() {}
	Device(uint32_t devId, Map map, uint32_t type, const char *name):
		name_{name}, map_{map}, type_{type}, devId{devId} {}
	virtual ~Device() {}

	bool hasKeyboard() const
	{
		return typeBits() & TYPE_BIT_KEYBOARD;
	}

	bool hasGamepad() const
	{
		return typeBits() & TYPE_BIT_GAMEPAD;
	}

	bool hasJoystick() const
	{
		return typeBits() & TYPE_BIT_JOYSTICK;
	}

	bool isVirtual() const
	{
		return typeBits() & TYPE_BIT_VIRTUAL;
	}

	bool hasKeys() const
	{
		return hasKeyboard() || hasGamepad() || typeBits() & TYPE_BIT_KEY_MISC;
	}

	bool isPowerButton() const
	{
		return typeBits() & TYPE_BIT_POWER_BUTTON;
	}

	uint32_t enumId() const { return devId; }
	const char *name() const { return name_.c_str(); }
	Map map() const;
	uint32_t typeBits() const;
	uint32_t subtype() const { return subtype_; }

	bool operator ==(Device const& rhs) const;

	virtual void setICadeMode(bool on);
	virtual bool iCadeMode() const { return false; }
	virtual void setJoystickAxisAsDpadBits(uint32_t axisMask) {}
	virtual uint32_t joystickAxisAsDpadBits() { return 0; }
	virtual uint32_t joystickAxisAsDpadBitsDefault() { return 0; }
	virtual uint32_t joystickAxisBits() { return 0; }

	virtual const char *keyName(Key k) const;

	// TODO
	//bool isDisconnectable() { return 0; }
	//void disconnect() { }

	static bool anyTypeBitsPresent(Base::ApplicationContext, uint32_t typeBits);

protected:
	std::string name_{""};
	Map map_{Map::UNKNOWN};
	uint32_t type_ = 0;
	uint32_t devId = 0;
public:
	uint32_t subtype_ = 0;
	uint32_t idx = 0;
};

}
