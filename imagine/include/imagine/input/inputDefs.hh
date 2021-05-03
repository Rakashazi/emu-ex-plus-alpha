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
#include <imagine/util/DelegateFunc.hh>
#include <array>

namespace Input
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

using EventKeyString = std::array<char, 4>;

using TextFieldDelegate = DelegateFunc<void (const char *str)>;

}
