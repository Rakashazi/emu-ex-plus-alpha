#pragma once

#include <X11/keysym.h>
#include <X11/XF86keysym.h>

namespace Input
{

namespace Keycode
{
	static const uint ESCAPE = XK_Escape,
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
	TAB = XK_Tab,
	SEARCH = XK_Find,
	// convert volume keys into 16-bit values and shift to avoid some conflicts with other keysyms
	VOL_UP = (XF86XK_AudioRaiseVolume >> 4) & 0xFFFF,
	VOL_DOWN = (XF86XK_AudioLowerVolume >> 4) & 0xFFFF,
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
	NUMPAD_LEFT = XK_KP_Left
	;

	static const uint COUNT = 0xffff + 1;

	namespace Pandora
	{
		static const uint L = RSHIFT,
			R = RCTRL,
			A = HOME,
			B = END,
			Y = PGUP,
			X = PGDOWN,
			SELECT = LCTRL,
			START = LALT,
			LOGO = UNDO,
			UP = Keycode::UP, RIGHT = Keycode::RIGHT, DOWN = Keycode::DOWN, LEFT = Keycode::LEFT
			;
	}
};

typedef uint16 Key;

namespace Pointer
{
	static const uint LBUTTON = 1,
	MBUTTON = 2,
	RBUTTON = 3,
	WHEEL_UP = 4,
	WHEEL_DOWN = 5,
	DOWN_BUTTON = 8,
	UP_BUTTON = 9
	;
};

};
