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

#if !defined __ARM_ARCH_6K__
#define CONFIG_INPUT_APPLE_GAME_CONTROLLER 1
#endif

namespace IG::Input
{

using PointerIdImpl = void*;

using Key = uint16_t;

	namespace Keycode
	{

	static constexpr Key
	A = 4,
	B = 5,
	C = 6,
	D = 7,
	E = 8,
	F = 9,
	G = 10,
	H = 11,
	I = 12,
	J = 13,
	K = 14,
	L = 15,
	M = 16,
	N = 17,
	O = 18,
	P = 19,
	Q = 20,
	R = 21,
	S = 22,
	T = 23,
	U = 24,
	V = 25,
	W = 26,
	X = 27,
	Y = 28,
	Z = 29,
	_1 = 30,
	_2 = 31,
	_3 = 32,
	_4 = 33,
	_5 = 34,
	_6 = 35,
	_7 = 36,
	_8 = 37,
	_9 = 38,
	_0 = 39,
	ENTER = 40,
	ESCAPE = 41,
	BACK_SPACE = 42,
	TAB = 43,
	SPACE = 44,
	MINUS = 45,
	EQUALS = 46,
	LEFT_BRACKET = 47,
	RIGHT_BRACKET = 48,
	BACKSLASH = 49,

	SEMICOLON = 51,
	APOSTROPHE = 52,
	GRAVE = 53,
	COMMA = 54,
	PERIOD = 55,
	SLASH = 56,
	CAPS = 57,
	F1 = 58, F2 = 59, F3 = 60, F4 = 61, F5 = 62, F6 = 63,
	F7 = 64, F8 = 65, F9 = 66, F10 = 67, F11 = 68, F12 = 69,
	PRINT_SCREEN = 70,
	SCROLL_LOCK = 71,
	PAUSE = 72,
	INSERT = 73,
	HOME = 74,
	PGUP = 75,
	DELETE = 76,
	END = 77,
	PGDOWN = 78,
	RIGHT = 79,
	LEFT = 80,
	DOWN = 81,
	UP = 82,
	NUM_LOCK = 83,
	NUMPAD_DIV = 84,
	NUMPAD_MULT = 85,
	NUMPAD_SUB = 86,
	NUMPAD_ADD = 87,
	NUMPAD_ENTER = 88,
	NUMPAD_1 = 89,
	NUMPAD_2 = 90,
	NUMPAD_3 = 91,
	NUMPAD_4 = 92,
	NUMPAD_5 = 93,
	NUMPAD_6 = 94,
	NUMPAD_7 = 95,
	NUMPAD_8 = 96,
	NUMPAD_9 = 97,
	NUMPAD_0 = 98,
	NUMPAD_DOT = 99,

	NUMPAD_EQUALS = 103,

	MENU = 118,

	LCTRL = 224,
	LSHIFT = 225,
	LALT = 226,
	LSUPER = 227,
	RCTRL = 228,
	RSHIFT = 229,
	RALT = 230,
	RSUPER = 231,

	// private key-codes for misc keys, gamepads, & analog -> digital joystick axis emulation
	miscKeyBase = 256,
	BACK = miscKeyBase,
	NUMPAD_COMMA = miscKeyBase+1,

	gpKeyBase = NUMPAD_COMMA + 1,
	GAME_A = gpKeyBase,
	GAME_B = gpKeyBase+1,
	GAME_C = gpKeyBase+2,
	GAME_X = gpKeyBase+3,
	GAME_Y = gpKeyBase+4,
	GAME_Z = gpKeyBase+5,
	GAME_L1 = gpKeyBase+6,
	GAME_R1 = gpKeyBase+7,
	GAME_L2 = gpKeyBase+8,
	GAME_R2 = gpKeyBase+9,
	GAME_LEFT_THUMB = gpKeyBase+10,
	GAME_RIGHT_THUMB = gpKeyBase+11,
	GAME_START = gpKeyBase+12,
	GAME_SELECT = gpKeyBase+13,
	GAME_MODE = gpKeyBase+14,
	GAME_1 = gpKeyBase+15, GAME_2 = gpKeyBase+16, GAME_3 = gpKeyBase+17, GAME_4 = gpKeyBase+18,
	GAME_5 = gpKeyBase+19, GAME_6 = gpKeyBase+20, GAME_7 = gpKeyBase+21, GAME_8 = gpKeyBase+22,
	GAME_9 = gpKeyBase+23, GAME_10 = gpKeyBase+24, GAME_11 = gpKeyBase+25, GAME_12 = gpKeyBase+26,
	GAME_13 = gpKeyBase+27, GAME_14 = gpKeyBase+28, GAME_15 = gpKeyBase+29, GAME_16 = gpKeyBase+30,

	axisKeyBase = GAME_16 + 1,
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

	BACK_KEY = ESCAPE;

	static constexpr Key LAST_KEY = JS_BRAKE_AXIS;
	static constexpr uint32_t COUNT = LAST_KEY + 1;

	}

	namespace Pointer
	{
	// fake keycodes, no actual mouse support
	static constexpr Key
	LBUTTON = 1,
	MBUTTON = 2,
	RBUTTON = 4,
	DOWN_BUTTON = 8,
	UP_BUTTON = 16;
	};

	namespace AppleGC
	{
	static constexpr Key
	A = Keycode::GAME_A,
	B = Keycode::GAME_B,
	X = Keycode::GAME_X,
	Y = Keycode::GAME_Y,
	L1 = Keycode::GAME_L1,
	L2 = Keycode::GAME_L2,
	R1 = Keycode::GAME_R1,
	R2 = Keycode::GAME_R2,
	PAUSE = Keycode::GAME_MODE,
	LEFT = Keycode::LEFT, RIGHT = Keycode::RIGHT,
	UP = Keycode::UP, DOWN = Keycode::DOWN,
	LSTICK_RIGHT = Keycode::JS1_XAXIS_POS, LSTICK_LEFT = Keycode::JS1_XAXIS_NEG,
	LSTICK_DOWN =  Keycode::JS1_YAXIS_POS, LSTICK_UP =   Keycode::JS1_YAXIS_NEG,
	RSTICK_RIGHT = Keycode::JS2_XAXIS_POS, RSTICK_LEFT = Keycode::JS2_XAXIS_NEG,
	RSTICK_DOWN =  Keycode::JS2_YAXIS_POS, RSTICK_UP =   Keycode::JS2_YAXIS_NEG;
	}

	namespace Meta
	{
		// TODO
		static constexpr uint32_t
		ALT = 0x1,
		ALT_L = 0x1,
		ALT_R = 0x1,
		SHIFT = 0x1,
		SHIFT_L = 0x1,
		SHIFT_R = 0x1,
		CTRL = 0x1,
		CTRL_L = 0x1,
		CTRL_R = 0x1,
		META = 0x1,
		META_L = 0x1,
		META_R = 0x1,
		CAPS_LOCK = 0x1;
	}

enum class AxisId : uint8_t
{
	X,
	Y,
	Z,
	RX,
	RY,
	RZ,
	HAT0X,
	HAT0Y,
	HAT1X,
	HAT1Y,
	HAT2X,
	HAT2Y,
	HAT3X,
	HAT3Y,
	LTRIGGER,
	RTRIGGER,
	RUDDER,
	WHEEL,
	GAS,
	BRAKE,
};

}
