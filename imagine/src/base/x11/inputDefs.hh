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
	PAUSE = XK_Pause
	;

	static const uint COUNT = 0xffff + 1;
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
