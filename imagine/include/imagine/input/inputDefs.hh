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

#include <imagine/input/config.hh>
#include <imagine/util/DelegateFunc.hh>
#include <array>
#include <type_traits>

namespace IG::Input
{

enum class Source : uint8_t
{
	UNKNOWN,
	KEYBOARD,
	GAMEPAD,
	MOUSE,
	TOUCHSCREEN,
	NAVIGATION,
	JOYSTICK,
};

enum class Map : uint8_t
{
	UNKNOWN = 0,
	SYSTEM = 1,
	POINTER = 2,
	REL_POINTER = 3,

	WIIMOTE = 10,
	WII_CC = 11,

	ICONTROLPAD = 20,
	ZEEMOTE = 21,
	ICADE = 22,
	PS3PAD = 23,

	APPLE_GAME_CONTROLLER = 31
};

enum class DeviceSubtype : uint8_t
{
	NONE = 0,
	XPERIA_PLAY = 1,
	PS3_CONTROLLER = 2,
	MOTO_DROID_KEYBOARD = 3,
	OUYA_CONTROLLER = 4,
	PANDORA_HANDHELD = 5,
	XBOX_360_CONTROLLER = 6,
	NVIDIA_SHIELD = 7,
	GENERIC_GAMEPAD = 8,
	APPLE_EXTENDED_GAMEPAD = 9,
	_8BITDO_SF30_PRO = 10,
	_8BITDO_SN30_PRO_PLUS = 11,
	_8BITDO_M30_GAMEPAD = 12
};

using PointerId = PointerIdImpl;

static constexpr PointerId NULL_POINTER_ID
{
	[]()
	{
		if constexpr(std::is_pointer_v<PointerId>)
			return nullptr;
		else
			return -1;
	}()
};

using DeviceTypeBits = uint8_t;

class Device;

}
