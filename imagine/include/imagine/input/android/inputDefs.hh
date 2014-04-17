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

namespace Input
{

using Time = int64_t; // java.lang.System.nanoTime() time base

static Time msToTime(int ms)
{
	return ms * 1000000.;
}

using Key = uint8;

namespace Android
{
	static const uint
	//SYS_HOME = 3, // Never sent to apps
	ESCAPE = 4,
	CALL = 5,
	END_CALL = 6,

	#ifdef CONFIG_BASE_ANDROID_SOFT_ORIENTATION
	// keyboard is rotated due to running internally in portrait mode
	LEFT = 19,
	RIGHT = 20,
	DOWN = 21,
	UP = 22,
	#else
	UP = 19,
	DOWN = 20,
	LEFT = 21,
	RIGHT = 22,
	#endif

	CENTER = 23,
	VOL_UP = 24,
	VOL_DOWN = 25,
	//26: power, handled by OS
	CAMERA = 27,
	CLEAR = 28,

	LALT = 57,
	RALT = 58,
	LSHIFT = 59,
	RSHIFT = 60,
	TAB = 61,

	SYMBOL = 63,
	EXPLORER = 64,
	MAIL = 65,
	ENTER = 66,
	BACK_SPACE = 67,

	NUM = 78,
	HEADSET_HOOK = 79,
	FOCUS = 80,

	MENU = 82,
	//case 83: notification
	SEARCH = 84,
	//case 85: media play
	//case 86: media stop
	//case 87: media next
	//case 88: media prev
	//case 89: media rew
	//case 90: media ff
	//case 91: mute
	PGUP = 92,
	PGDOWN = 93,

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
	//111: escape
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
	// 162, left paren handled by ascii key macro
	// 163, right paren handled by ascii key macro

	GAME_1 = 188, GAME_2 = 189, GAME_3 = 190, GAME_4 = 191, GAME_5 = 192, GAME_6 = 193,
	GAME_7 = 194, GAME_8 = 195, GAME_9 = 196, GAME_10 = 197, GAME_11 = 198, GAME_12 = 199,
	GAME_13 = 200, GAME_14 = 201, GAME_15 = 202, GAME_16 = 203,

	// Our own key-codes for analog -> digital joystick axis emulation
	JS_RUDDER_AXIS_POS = 232, JS_RUDDER_AXIS_NEG = 233,
	JS_WHEEL_AXIS_POS = 234, JS_WHEEL_AXIS_NEG = 235,
	JS_POV_XAXIS_POS = 236, JS_POV_XAXIS_NEG = 237,
	JS_POV_YAXIS_POS = 238, JS_POV_YAXIS_NEG = 239,
	JS1_XAXIS_POS = 240, JS1_XAXIS_NEG = 241,
	JS1_YAXIS_POS = 242, JS1_YAXIS_NEG = 243,

	JS2_XAXIS_POS = 244, JS2_XAXIS_NEG = 245,
	JS2_YAXIS_POS = 246, JS2_YAXIS_NEG = 247,

	JS3_XAXIS_POS = 248, JS3_XAXIS_NEG = 249,
	JS3_YAXIS_POS = 250, JS3_YAXIS_NEG = 251,

	JS_LTRIGGER_AXIS = 252, JS_RTRIGGER_AXIS = 253,
	JS_GAS_AXIS = 254, JS_BRAKE_AXIS = 255;

	static const uint COUNT = 0xff + 1;

	namespace XperiaPlay
	{
		static const uint CROSS = CENTER,
		CIRCLE = GAME_B, // re-mapped from "Back" in input event handler
		SQUARE = GAME_X,
		TRIANGLE = GAME_Y,
		L1 = GAME_L1,
		R1 = GAME_R1,
		SELECT = GAME_SELECT,
		START = GAME_START,
		UP = Android::UP, RIGHT = Android::RIGHT, DOWN = Android::DOWN, LEFT = Android::LEFT
		;
	}

	namespace Ouya
	{
		static const uint O = GAME_A,
		U = GAME_X,
		Y = GAME_Y,
		A = GAME_B,
		L1 = GAME_L1,
		L2 = GAME_L2,
		L3 = GAME_LEFT_THUMB,
		R1 = GAME_R1,
		R2 = GAME_R2,
		R3 = GAME_RIGHT_THUMB,
		UP = Android::UP, RIGHT = Android::RIGHT, DOWN = Android::DOWN, LEFT = Android::LEFT,
		SYSTEM = MENU
		;
	}

	namespace PS3
	{
		static const uint CROSS = GAME_X,
		CIRCLE = GAME_Y,
		SQUARE = GAME_A,
		TRIANGLE = GAME_B,
		L1 = GAME_L1,
		L2 = GAME_L2,
		L3 = GAME_LEFT_THUMB,
		R1 = GAME_R1,
		R2 = GAME_R2,
		R3 = GAME_RIGHT_THUMB,
		SELECT = GAME_SELECT,
		START = GAME_START,
		UP = Android::UP, RIGHT = Android::RIGHT, DOWN = Android::DOWN, LEFT = Android::LEFT,
		PS = GAME_1
		;
	}

	// Android key-codes don't directly map to ASCII
	static constexpr uint asciiKey(uint c)
	{
		return (c >= 'a' && c <= 'z') ? c-68 :
			(c >= '0' && c <= '9') ? c-41 :
			(c == ' ') ? 62 :
			(c == '*') ? 17 :
			(c == '#') ? 18 :
			(c == ',') ? 55 :
			(c == '.') ? 56 :
			(c == '`') ? 68 :
			(c == '-') ? 69 :
			(c == '=') ? 70 :
			(c == '[') ? 71 :
			(c == ']') ? 72 :
			(c == '\\') ? 73 :
			(c == ';') ? 74 :
			(c == '\'') ? 75 :
			(c == '/') ? 76 :
			(c == '@') ? 77 :
			(c == '+') ? 81 :
			0;
	}
}

namespace Keycode = Android;
#define CONFIG_INPUT_KEYCODE_NAMESPACE Android

namespace Pointer
{
	static const uint LBUTTON = 1;
	static const uint RBUTTON = 2; // TODO: add real mouse support
}

}
