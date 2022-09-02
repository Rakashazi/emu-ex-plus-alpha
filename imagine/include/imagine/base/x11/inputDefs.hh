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

#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <imagine/input/evdev/inputDefs.hh>

namespace IG::Input
{

using PointerIdImpl = int;

// Note: only first 15-bits of XF86XK_* values are used
// so they fit in 2 bytes and don't conflict with other keys

	namespace Keycode
	{
	static constexpr Key
	ESCAPE = XK_Escape,
	BACK = XF86XK_Back & 0xEFFF,
	A = XK_a,
	B = XK_b,
	C = XK_c,
	D = XK_d,
	E = XK_e,
	F = XK_f,
	G = XK_g,
	H = XK_h,
	I = XK_i,
	J = XK_j,
	K = XK_k,
	L = XK_l,
	M = XK_m,
	N = XK_n,
	O = XK_o,
	P = XK_p,
	Q = XK_q,
	R = XK_r,
	S = XK_s,
	T = XK_t,
	U = XK_u,
	V = XK_v,
	W = XK_w,
	X = XK_x,
	Y = XK_y,
	Z = XK_z,
	_0 = XK_0,
	_1 = XK_1,
	_2 = XK_2,
	_3 = XK_3,
	_4 = XK_4,
	_5 = XK_5,
	_6 = XK_6,
	_7 = XK_7,
	_8 = XK_8,
	_9 = XK_9,
	F1 = XK_F1,
	F2 = XK_F2,
	F3 = XK_F3,
	F4 = XK_F4,
	F5 = XK_F5,
	F6 = XK_F6,
	F7 = XK_F7,
	F8 = XK_F8,
	F9 = XK_F9,
	F10 = XK_F10,
	F11 = XK_F11,
	F12 = XK_F12,
	SPACE = XK_space,
	ENTER = XK_Return,
	LALT = XK_Alt_L,
	RALT = XK_Alt_R,
	LSHIFT = XK_Shift_L,
	RSHIFT = XK_Shift_R,
	LCTRL = XK_Control_L,
	RCTRL = XK_Control_R,
	PGUP = XK_Page_Up,
	PGDOWN = XK_Page_Down,
	LEFT = XK_Left,
	RIGHT = XK_Right,
	UP = XK_Up,
	DOWN = XK_Down,
	BACK_SPACE = XK_BackSpace,
	MENU = XK_Menu,
	HOME = XK_Home,
	DELETE = XK_Delete,
	CLEAR = XK_Clear,
	TAB = XK_Tab,
	SEARCH = XK_Find,
	VOL_UP = XF86XK_AudioRaiseVolume & 0xEFFF,
	VOL_DOWN = XF86XK_AudioLowerVolume & 0xEFFF,
	SCROLL_LOCK = XK_Scroll_Lock,
	END = XK_End,
	INSERT = XK_Insert,
	CAPS = XK_Caps_Lock,
	LMETA = XK_Meta_L,
	RMETA = XK_Meta_R,
	LSUPER = XK_Super_L,
	RSUPER = XK_Super_R,
	PAUSE = XK_Pause,
	UNDO = XK_Undo,
	PRINT_SCREEN = XK_Print,
	POUND = XK_numbersign,
	STAR = XK_asterisk,
	LEFT_PAREN = XK_parenleft,
	RIGHT_PAREN = XK_parenright,
	PLUS = XK_plus,
	MINUS = XK_minus,
	APOSTROPHE = XK_apostrophe,
	COMMA = XK_comma,
	PERIOD = XK_period,
	SLASH = XK_slash,
	SEMICOLON = XK_semicolon,
	EQUALS = XK_equal,
	AT = XK_at,
	LEFT_BRACKET = XK_bracketleft,
	RIGHT_BRACKET = XK_bracketright,
	GRAVE = XK_grave,
	BACKSLASH = XK_backslash,
	MAIL = XF86XK_Mail & 0xEFFF,
	EXPLORER = XF86XK_WWW & 0xEFFF,

	// Numpad
	NUM_LOCK = XK_Num_Lock,
	NUMPAD_0 = XK_KP_0,
	NUMPAD_1 = XK_KP_1,
	NUMPAD_2 = XK_KP_2,
	NUMPAD_3 = XK_KP_3,
	NUMPAD_4 = XK_KP_4,
	NUMPAD_5 = XK_KP_5,
	NUMPAD_6 = XK_KP_6,
	NUMPAD_7 = XK_KP_7,
	NUMPAD_8 = XK_KP_8,
	NUMPAD_9 = XK_KP_9,
	NUMPAD_DIV = XK_KP_Divide,
	NUMPAD_MULT = XK_KP_Multiply,
	NUMPAD_SUB = XK_KP_Subtract,
	NUMPAD_ADD = XK_KP_Add,
	NUMPAD_DOT = XK_KP_Decimal,
	NUMPAD_COMMA = XK_KP_Separator,
	NUMPAD_ENTER = XK_KP_Enter,
	NUMPAD_EQUALS = XK_KP_Equal,
	NUMPAD_INSERT = XK_KP_Insert,
	NUMPAD_DELETE = XK_KP_Delete,
	NUMPAD_BEGIN = XK_KP_Begin,
	NUMPAD_HOME = XK_KP_Home,
	NUMPAD_END = XK_KP_End,
	NUMPAD_PGUP = XK_KP_Page_Up,
	NUMPAD_PGDOWN = XK_KP_Page_Down,
	NUMPAD_UP = XK_KP_Up,
	NUMPAD_RIGHT = XK_KP_Right,
	NUMPAD_DOWN = XK_KP_Down,
	NUMPAD_LEFT = XK_KP_Left,

	// private key-codes for analog -> digital joystick axis emulation
	GAME_A = 0xfe01,
	GAME_B = 0xfe02,
	GAME_C = 0xfe03,
	GAME_X = 0xfe04,
	GAME_Y = 0xfe05,
	GAME_Z = 0xfe06,
	GAME_L1 = 0xfe07,
	GAME_R1 = 0xfe08,
	GAME_L2 = 0xfe09,
	GAME_R2 = 0xfe0a,
	GAME_LEFT_THUMB = 0xfe0b,
	GAME_RIGHT_THUMB = 0xfe0c,
	GAME_START = 0xfe0d,
	GAME_SELECT = 0xfe0e,
	GAME_MODE = 0xfe0f,
	GAME_1 = 0xfe10, GAME_2 = 0xfe11, GAME_3 = 0xfe12, GAME_4 = 0xfe13, GAME_5 = 0xfe14, GAME_6 = 0xfe15,
	GAME_7 = 0xfe16, GAME_8 = 0xfe17, GAME_9 = 0xfe18, GAME_10 = 0xfe19, GAME_11 = 0xfe1a, GAME_12 = 0xfe1b,
	GAME_13 = 0xfe1c, GAME_14 = 0xfe1d, GAME_15 = 0xfe1e, GAME_16 = 0xfe1f,

	JS_RUDDER_AXIS_POS = 0xfe20, JS_RUDDER_AXIS_NEG = 0xfe21,
	JS_WHEEL_AXIS_POS = 0xfe22, JS_WHEEL_AXIS_NEG = 0xfe23,
	JS_POV_XAXIS_POS = 0xfe24, JS_POV_XAXIS_NEG = 0xfe25,
	JS_POV_YAXIS_POS = 0xfe26, JS_POV_YAXIS_NEG = 0xfe27,
	JS1_XAXIS_POS = 0xfe28, JS1_XAXIS_NEG = 0xfe29,
	JS1_YAXIS_POS = 0xfe2a, JS1_YAXIS_NEG = 0xfe2b,

	JS2_XAXIS_POS = 0xfe2c, JS2_XAXIS_NEG = 0xfe2d,
	JS2_YAXIS_POS = 0xfe2e, JS2_YAXIS_NEG = 0xfe2f,

	JS3_XAXIS_POS = 0xfe30, JS3_XAXIS_NEG = 0xfe31,
	JS3_YAXIS_POS = 0xfe32, JS3_YAXIS_NEG = 0xfe33,

	JS_LTRIGGER_AXIS = 0xfe34, JS_RTRIGGER_AXIS = 0xfe35,
	JS_GAS_AXIS = 0xfe36, JS_BRAKE_AXIS = 0xfe37,

	BACK_KEY = ESCAPE;

	static constexpr uint32_t COUNT = 0xffff + 1;

		namespace Pandora
		{
		static constexpr Key
		L = RSHIFT,
		R = RCTRL,
		A = HOME,
		B = END,
		Y = PGUP,
		X = PGDOWN,
		SELECT = LCTRL,
		START = LALT,
		LOGO = UNDO,
		UP = Keycode::UP, RIGHT = Keycode::RIGHT, DOWN = Keycode::DOWN, LEFT = Keycode::LEFT;
		}
	}

	namespace Pointer
	{
	static constexpr Key
	LBUTTON = 1,
	MBUTTON = 2,
	RBUTTON = 4,
	DOWN_BUTTON = 128,
	UP_BUTTON = 256;
	}

	namespace Meta
	{
	static constexpr uint32_t
	ALT = 0x8,
	ALT_L = 0x8,
	ALT_R = 0x8,
	SHIFT = 0x1,
	SHIFT_L = 0x1,
	SHIFT_R = 0x1,
	CTRL = 0x4,
	CTRL_L = 0x4,
	CTRL_R = 0x4,
	META = 0x40,
	META_L = 0x40,
	META_R = 0x40,
	CAPS_LOCK = 0x2;
	}
}
