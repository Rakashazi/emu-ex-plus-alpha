#pragma once

#include <imagine/util/windows/windows.h>

namespace IG::Input
{

// TODO: remove dummy defs
namespace Keycode
{
	static const uint32_t ESCAPE = VK_ESCAPE,
	ENTER = VK_RETURN,
	LALT = 2003, // TODO
	RALT = 2004, // TODO
	LSHIFT = VK_LSHIFT,
	RCTRL = VK_RCONTROL,
	LEFT = VK_LEFT,
	RIGHT = VK_RIGHT,
	UP = VK_UP,
	DOWN = VK_DOWN,
	BACK_SPACE = VK_BACK,
	MENU = VK_MENU,
	PGUP = 2011, // TODO
	PGDOWN = 2010, // TODO
	RSHIFT = VK_RSHIFT,
	LCTRL = VK_LCONTROL,
	CAPS = 2007, // TODO
	LMETA = VK_LWIN,
	RMETA = VK_RWIN,
	TAB = VK_TAB,
	HOME = VK_HOME,
	DELETE = VK_DELETE,
	SEARCH = 2005, // TODO
	END = VK_END,
	INSERT = VK_INSERT,
	SCROLL_LOCK = VK_SCROLL,
	PAUSE = VK_PAUSE,
	PRINT_SCREEN = VK_PRINT,
	NUM_LOCK = VK_NUMLOCK,
	NUMPAD_0 = VK_NUMPAD0,
	NUMPAD_1 = VK_NUMPAD1,
	NUMPAD_2 = VK_NUMPAD2,
	NUMPAD_3 = VK_NUMPAD3,
	NUMPAD_4 = VK_NUMPAD4,
	NUMPAD_5 = VK_NUMPAD5,
	NUMPAD_6 = VK_NUMPAD6,
	NUMPAD_7 = VK_NUMPAD7,
	NUMPAD_8 = VK_NUMPAD8,
	NUMPAD_9 = VK_NUMPAD9,
	NUMPAD_ADD = VK_ADD,
	NUMPAD_DIV = VK_DIVIDE,
	NUMPAD_MULT = VK_MULTIPLY,
	NUMPAD_SUB = VK_SUBTRACT,
	NUMPAD_DOT = VK_DECIMAL,
	NUMPAD_COMMA = VK_SEPARATOR,
	F1 = VK_F1,
	F2 = VK_F2,
	F3 = VK_F3,
	F4 = VK_F4,
	F5 = VK_F5,
	F6 = VK_F6,
	F7 = VK_F7,
	F8 = VK_F8,
	F9 = VK_F9,
	F10 = VK_F10,
	F11 = VK_F11,
	F12 = VK_F12
	;

	static const uint32_t COUNT = 0xffff;

	/*static constexpr uint32_t asciiKey(uint32_t c)
	{
		return (c >= 'a' && c <= 'z') ? c-0x20 :
			((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) ? c :
			(c == ' ') ? c :
			(c == ',') ? VK_OEM_COMMA :
			(c == '.') ? VK_OEM_PERIOD :
			(c == '`') ? 0x100 : // TODO: remove
			(c == '-') ? VK_OEM_MINUS :
			(c == '=') ? VK_OEM_PLUS :
			(c == '[') ? VK_OEM_4 :
			(c == ']') ? VK_OEM_6 :
			(c == '\\') ? VK_OEM_5 :
			(c == ';') ? VK_OEM_1 :
			(c == '\'') ? VK_OEM_7 :
			(c == '/') ? 0x101 : // TODO: remove
			(c == '+') ? 0x102 : // TODO: remove
			(c == '#') ? 0x103 : // TODO: remove
			(c == '*') ? 0x104 : // TODO: remove
			(c == '@') ? 0x105 : // TODO: remove
			0;
	}*/
};

typedef uint16_t Key;

namespace Pointer
{
	static const uint32_t LBUTTON = 1,
	MBUTTON = 2,
	RBUTTON = 3,
	WHEEL_UP = 4,
	WHEEL_DOWN = 5,
	DOWN_BUTTON = 8,
	UP_BUTTON = 9
	;
};

}
