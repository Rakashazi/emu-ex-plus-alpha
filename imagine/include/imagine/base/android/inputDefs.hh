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

#include <cstdint>

namespace IG::Input
{

using PointerIdImpl = int32_t;

using Key = uint16_t;

	namespace Keycode
	{
	static constexpr Key
	// SYS_HOME = 3, // Never sent to apps
	BACK = 4,
	CALL = 5,
	END_CALL = 6,
	_0 = 7,
	_1 = 8,
	_2 = 9,
	_3 = 10,
	_4 = 11,
	_5 = 12,
	_6 = 13,
	_7 = 14,
	_8 = 15,
	_9 = 16,
	STAR = 17,
	POUND = 18,
	UP = 19,
	DOWN = 20,
	LEFT = 21,
	RIGHT = 22,
	CENTER = 23,
	VOL_UP = 24,
	VOL_DOWN = 25,
	// POWER = 26, handled by OS
	CAMERA = 27,
	CLEAR = 28,
	A = 29,
	B = 30,
	C = 31,
	D = 32,
	E = 33,
	F = 34,
	G = 35,
	H = 36,
	I = 37,
	J = 38,
	K = 39,
	L = 40,
	M = 41,
	N = 42,
	O = 43,
	P = 44,
	Q = 45,
	R = 46,
	S = 47,
	T = 48,
	U = 49,
	V = 50,
	W = 51,
	X = 52,
	Y = 53,
	Z = 54,
	COMMA = 55,
	PERIOD = 56,
	LALT = 57,
	RALT = 58,
	LSHIFT = 59,
	RSHIFT = 60,
	TAB = 61,
	SPACE = 62,
	SYMBOL = 63,
	EXPLORER = 64,
	MAIL = 65,
	ENTER = 66,
	BACK_SPACE = 67,
	GRAVE = 68,
	MINUS = 69,
	EQUALS = 70,
	LEFT_BRACKET = 71,
	RIGHT_BRACKET = 72,
	BACKSLASH = 73,
	SEMICOLON = 74,
	APOSTROPHE = 75,
	SLASH = 76,
	AT = 77,
	NUM = 78,
	HEADSET_HOOK = 79,
	FOCUS = 80,
	PLUS = 81,
	MENU = 82,
	// NOTIFICATION = 83,
	SEARCH = 84,
	MEDIA_PLAY_PAUSE = 85,
	MEDIA_STOP = 86,
	MEDIA_NEXT = 87,
	MEDIA_PREVIOUS = 88,
	MEDIA_REWIND = 89,
	MEDIA_FAST_FORWARD = 90,
	MUTE = 91,
	PGUP = 92,
	PGDOWN = 93,
	PICTSYMBOLS = 94,
	SWITCH_CHARSET = 95,
	GAME_A = 96,
	GAME_B = 97,
	GAME_C = 98,
	GAME_X = 99,
	GAME_Y = 100,
	GAME_Z = 101,
	GAME_L1 = 102,
	GAME_R1 = 103,
	GAME_L2 = 104,
	GAME_R2 = 105,
	GAME_LEFT_THUMB = 106,
	GAME_RIGHT_THUMB = 107,
	GAME_START = 108,
	GAME_SELECT = 109,
	GAME_MODE = 110,
	ESCAPE = 111,
	DELETE = 112,
	LCTRL = 113,
	RCTRL = 114,
	CAPS = 115,
	SCROLL_LOCK = 116,
	LSUPER = 117,
	RSUPER = 118,
	FUNCTION = 119,
	PRINT_SCREEN = 120,
	PAUSE = 121,
	HOME = 122,
	END = 123,
	INSERT = 124,

	F1 = 131, F2 = 132, F3 = 133, F4 = 134, F5 = 135, F6 = 136,
	F7 = 137, F8 = 138, F9 = 139, F10 = 140, F11 = 141, F12 = 142,

	NUM_LOCK = 143,
	NUMPAD_0 = 144,
	NUMPAD_1 = 145,
	NUMPAD_2 = 146,
	NUMPAD_3 = 147,
	NUMPAD_4 = 148,
	NUMPAD_5 = 149,
	NUMPAD_6 = 150,
	NUMPAD_7 = 151,
	NUMPAD_8 = 152,
	NUMPAD_9 = 153,
	NUMPAD_DIV = 154,
	NUMPAD_MULT = 155,
	NUMPAD_SUB = 156,
	NUMPAD_ADD = 157,
	NUMPAD_DOT = 158,
	NUMPAD_COMMA = 159,
	NUMPAD_ENTER = 160,
	NUMPAD_EQUALS = 161,
	LEFT_PAREN = 162,
	RIGHT_PAREN = 163,

	GAME_1 = 188, GAME_2 = 189, GAME_3 = 190, GAME_4 = 191, GAME_5 = 192, GAME_6 = 193,
	GAME_7 = 194, GAME_8 = 195, GAME_9 = 196, GAME_10 = 197, GAME_11 = 198, GAME_12 = 199,
	GAME_13 = 200, GAME_14 = 201, GAME_15 = 202, GAME_16 = 203,

	// private key-codes for analog -> digital joystick axis emulation
	axisKeyBase = 350,

	JS_RUDDER_AXIS_POS = axisKeyBase, JS_RUDDER_AXIS_NEG = axisKeyBase+1,
	JS_WHEEL_AXIS_POS = axisKeyBase+2, JS_WHEEL_AXIS_NEG = axisKeyBase+3,
	JS_POV_XAXIS_POS = axisKeyBase+4, JS_POV_XAXIS_NEG = axisKeyBase+5,
	JS_POV_YAXIS_POS = axisKeyBase+6, JS_POV_YAXIS_NEG = axisKeyBase+7,
	JS1_XAXIS_POS = axisKeyBase+8, JS1_XAXIS_NEG = axisKeyBase+9,
	JS1_YAXIS_POS = axisKeyBase+10, JS1_YAXIS_NEG = axisKeyBase+11,

	JS2_XAXIS_POS = axisKeyBase+12, JS2_XAXIS_NEG = axisKeyBase+13,
	JS2_YAXIS_POS = axisKeyBase+14, JS2_YAXIS_NEG = axisKeyBase+15,

	JS3_XAXIS_POS = axisKeyBase+16, JS3_XAXIS_NEG = axisKeyBase+17,
	JS3_YAXIS_POS = axisKeyBase+18, JS3_YAXIS_NEG = axisKeyBase+19,

	JS_LTRIGGER_AXIS = axisKeyBase+20, JS_RTRIGGER_AXIS = axisKeyBase+21,
	JS_GAS_AXIS = axisKeyBase+22, JS_BRAKE_AXIS = axisKeyBase+23,

	BACK_KEY = BACK;

	static constexpr Key LAST_KEY = JS_BRAKE_AXIS;
	static constexpr uint32_t COUNT = LAST_KEY + 1;

		namespace XperiaPlay
		{
		static constexpr Key
		CROSS = CENTER,
		CIRCLE = GAME_B, // re-mapped from "Back" in input event handler
		SQUARE = GAME_X,
		TRIANGLE = GAME_Y,
		L1 = GAME_L1,
		R1 = GAME_R1,
		SELECT = GAME_SELECT,
		START = GAME_START,
		UP = Keycode::UP, RIGHT = Keycode::RIGHT, DOWN = Keycode::DOWN, LEFT = Keycode::LEFT;
		}
}

	namespace Pointer
	{
	static constexpr Key
	LBUTTON = 1,
	MBUTTON = 4,
	RBUTTON = 2,
	DOWN_BUTTON = 8,
	UP_BUTTON = 16,
	ALL_BUTTONS = LBUTTON | MBUTTON | RBUTTON | DOWN_BUTTON | UP_BUTTON;
	};

	namespace Meta
	{
	static constexpr uint32_t
	ALT = 0x2,
	ALT_L = 0x10,
	ALT_R = 0x20,
	SHIFT = 0x1,
	SHIFT_L = 0x40,
	SHIFT_R = 0x80,
	CTRL = 0x1000,
	CTRL_L = 0x2000,
	CTRL_R = 0x4000,
	META = 0x10000,
	META_L = 0x20000,
	META_R = 0x40000,
	CAPS_LOCK = 0x100000;
	}

enum class AxisId : uint8_t
{
	X = 0,
	Y = 1,
	Z = 11,
	RX = 12,
	RY = 13,
	RZ = 14,
	HAT0X = 15,
	HAT0Y = 16,
	HAT1X = 32,
	HAT1Y = 33,
	HAT2X = 34,
	HAT2Y = 35,
	HAT3X = 36,
	HAT3Y = 37,
	LTRIGGER = 17,
	RTRIGGER = 18,
	RUDDER = 20,
	WHEEL = 21,
	GAS = 22,
	BRAKE = 23,
};

}
