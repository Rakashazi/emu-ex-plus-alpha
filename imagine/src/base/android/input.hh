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

bool hasGetAxisValue();

}
