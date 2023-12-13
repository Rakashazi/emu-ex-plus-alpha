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

#include <linux/input.h>

namespace IG::Input
{

using Key = uint16_t;

namespace Evdev
{
	static const uint32_t

	F1 = KEY_F1,
	F2 = KEY_F2,
	F3 = KEY_F3,
	F4 = KEY_F4,
	F5 = KEY_F5,
	F6 = KEY_F6,
	F7 = KEY_F7,
	F8 = KEY_F8,
	F9 = KEY_F9,
	F10 = KEY_F10,
	F11 = KEY_F11,
	F12 = KEY_F12,
	ENTER = KEY_ENTER,
	LALT = KEY_LEFTALT,
	RALT = KEY_RIGHTALT,
	LSHIFT = KEY_LEFTSHIFT,
	RSHIFT = KEY_RIGHTSHIFT,
	LCTRL = KEY_LEFTCTRL,
	RCTRL = KEY_RIGHTCTRL,
	PGUP = KEY_PAGEUP,
	PGDOWN = KEY_PAGEDOWN,
	UP = KEY_UP,
	DOWN = KEY_DOWN,
	LEFT = KEY_LEFT,
	RIGHT = KEY_RIGHT,
	BACK_SPACE = KEY_BACKSPACE,
	MENU = KEY_MENU,
	HOME = KEY_HOME,
	DELETE = KEY_DELETE,
	TAB = KEY_TAB,
	SEARCH = KEY_FIND,
	VOL_UP = KEY_VOLUMEUP,
	VOL_DOWN = KEY_VOLUMEDOWN,
	SCROLL_LOCK = KEY_SCROLLLOCK,
	END = KEY_END,
	INSERT = KEY_INSERT,
	CAPS = KEY_CAPSLOCK,
	LSUPER = KEY_LEFTMETA,
	RSUPER = KEY_RIGHTMETA,
	PAUSE = KEY_PAUSE,
	UNDO = KEY_UNDO,
	PRINT_SCREEN = KEY_SYSRQ,

	GAME_A = BTN_A,
	GAME_B = BTN_B,
	GAME_C = BTN_C,
	GAME_X = BTN_X,
	GAME_Y = BTN_Y,
	GAME_Z = BTN_Z,
	GAME_L1 = BTN_TL,
	GAME_R1 = BTN_TR,
	GAME_L2 = BTN_TL2,
	GAME_R2 = BTN_TR2,
	GAME_LEFT_THUMB = BTN_THUMBL,
	GAME_RIGHT_THUMB = BTN_THUMBR,
	GAME_START = BTN_START,
	GAME_SELECT = BTN_SELECT,
	GAME_MODE = BTN_MODE,

	GAME_1 = BTN_TRIGGER, GAME_2 = BTN_THUMB, GAME_3 = BTN_THUMB2, GAME_4 = BTN_TOP, GAME_5 = BTN_TOP2, GAME_6 = BTN_PINKIE,
	GAME_7 = BTN_BASE, GAME_8 = BTN_BASE2, GAME_9 = BTN_BASE3, GAME_10 = BTN_BASE4, GAME_11 = BTN_BASE5, GAME_12 = BTN_BASE6,
	GAME_13 = BTN_BASE6+1, GAME_14 = BTN_BASE6+2, GAME_15 = BTN_BASE6+3, GAME_16 = BTN_DEAD,

	// private key-codes for analog -> digital joystick axis emulation
	JS1_XAXIS_POS = KEY_MAX+1, JS1_XAXIS_NEG = KEY_MAX+2,
	JS1_YAXIS_POS = KEY_MAX+3, JS1_YAXIS_NEG = KEY_MAX+4,

	JS2_XAXIS_POS = KEY_MAX+5, JS2_XAXIS_NEG = KEY_MAX+6,
	JS2_YAXIS_POS = KEY_MAX+7, JS2_YAXIS_NEG = KEY_MAX+8,

	JS3_XAXIS_POS = KEY_MAX+9, JS3_XAXIS_NEG = KEY_MAX+10,
	JS3_YAXIS_POS = KEY_MAX+11, JS3_YAXIS_NEG = KEY_MAX+12,

	JS_LTRIGGER_AXIS = KEY_MAX+13, JS_RTRIGGER_AXIS = KEY_MAX+14,

	JS_POV_XAXIS_POS = KEY_MAX+15, JS_POV_XAXIS_NEG = KEY_MAX+16,
	JS_POV_YAXIS_POS = KEY_MAX+17, JS_POV_YAXIS_NEG = KEY_MAX+18;

	static const uint32_t EX_KEYS = 18;
	static const uint32_t COUNT = KEY_CNT + EX_KEYS;
}

enum class AxisId : uint8_t
{
	X = ABS_X,
	Y = ABS_Y,
	LTRIGGER = ABS_Z,
	Z = ABS_RX,
	RZ = ABS_RY,
	RTRIGGER = ABS_RZ,
	HAT0X = ABS_HAT0X,
	HAT0Y = ABS_HAT0Y,
	HAT1X = ABS_HAT1X,
	HAT1Y = ABS_HAT1Y,
	HAT2X = ABS_HAT2X,
	HAT2Y = ABS_HAT2Y,
	HAT3X = ABS_HAT3X,
	HAT3Y = ABS_HAT3Y,
	RX = ABS_THROTTLE,
	RY = ABS_PRESSURE,
	RUDDER = ABS_RUDDER,
	WHEEL = ABS_WHEEL,
	GAS = ABS_GAS,
	BRAKE = ABS_BRAKE,
};


}
