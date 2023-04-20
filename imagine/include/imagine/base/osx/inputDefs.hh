#pragma once

#include <Carbon/Carbon.h>
#ifdef __OBJC__
#import <AppKit/NSEvent.h>
#else
// Can't use Apple Header when not using Obj-C, duplicate just the needed key definitions instead

/* Unicodes we reserve for function keys on the keyboard,  OpenStep reserves the range 0xF700-0xF8FF for this purpose.  The availability of various keys will be system dependent. */
enum {
    NSUpArrowFunctionKey        = 0xF700,
    NSDownArrowFunctionKey      = 0xF701,
    NSLeftArrowFunctionKey      = 0xF702,
    NSRightArrowFunctionKey     = 0xF703,
    NSF1FunctionKey             = 0xF704,
    NSF2FunctionKey             = 0xF705,
    NSF3FunctionKey             = 0xF706,
    NSF4FunctionKey             = 0xF707,
    NSF5FunctionKey             = 0xF708,
    NSF6FunctionKey             = 0xF709,
    NSF7FunctionKey             = 0xF70A,
    NSF8FunctionKey             = 0xF70B,
    NSF9FunctionKey             = 0xF70C,
    NSF10FunctionKey            = 0xF70D,
    NSF11FunctionKey            = 0xF70E,
    NSF12FunctionKey            = 0xF70F,
    NSF13FunctionKey            = 0xF710,
    NSF14FunctionKey            = 0xF711,
    NSF15FunctionKey            = 0xF712,
    NSF16FunctionKey            = 0xF713,
    NSF17FunctionKey            = 0xF714,
    NSF18FunctionKey            = 0xF715,
    NSF19FunctionKey            = 0xF716,
    NSF20FunctionKey            = 0xF717,
    NSF21FunctionKey            = 0xF718,
    NSF22FunctionKey            = 0xF719,
    NSF23FunctionKey            = 0xF71A,
    NSF24FunctionKey            = 0xF71B,
    NSF25FunctionKey            = 0xF71C,
    NSF26FunctionKey            = 0xF71D,
    NSF27FunctionKey            = 0xF71E,
    NSF28FunctionKey            = 0xF71F,
    NSF29FunctionKey            = 0xF720,
    NSF30FunctionKey            = 0xF721,
    NSF31FunctionKey            = 0xF722,
    NSF32FunctionKey            = 0xF723,
    NSF33FunctionKey            = 0xF724,
    NSF34FunctionKey            = 0xF725,
    NSF35FunctionKey            = 0xF726,
    NSInsertFunctionKey         = 0xF727,
    NSDeleteFunctionKey         = 0xF728,
    NSHomeFunctionKey           = 0xF729,
    NSBeginFunctionKey          = 0xF72A,
    NSEndFunctionKey            = 0xF72B,
    NSPageUpFunctionKey         = 0xF72C,
    NSPageDownFunctionKey       = 0xF72D,
    NSPrintScreenFunctionKey    = 0xF72E,
    NSScrollLockFunctionKey     = 0xF72F,
    NSPauseFunctionKey          = 0xF730,
    NSSysReqFunctionKey         = 0xF731,
    NSBreakFunctionKey          = 0xF732,
    NSResetFunctionKey          = 0xF733,
    NSStopFunctionKey           = 0xF734,
    NSMenuFunctionKey           = 0xF735,
    NSUserFunctionKey           = 0xF736,
    NSSystemFunctionKey         = 0xF737,
    NSPrintFunctionKey          = 0xF738,
    NSClearLineFunctionKey      = 0xF739,
    NSClearDisplayFunctionKey   = 0xF73A,
    NSInsertLineFunctionKey     = 0xF73B,
    NSDeleteLineFunctionKey     = 0xF73C,
    NSInsertCharFunctionKey     = 0xF73D,
    NSDeleteCharFunctionKey     = 0xF73E,
    NSPrevFunctionKey           = 0xF73F,
    NSNextFunctionKey           = 0xF740,
    NSSelectFunctionKey         = 0xF741,
    NSExecuteFunctionKey        = 0xF742,
    NSUndoFunctionKey           = 0xF743,
    NSRedoFunctionKey           = 0xF744,
    NSFindFunctionKey           = 0xF745,
    NSHelpFunctionKey           = 0xF746,
    NSModeSwitchFunctionKey     = 0xF747
};

#endif

namespace IG::Input
{

using PointerIdImpl = void*;

using Key = uint16_t;

// TODO: remove dummy defs
namespace Keycode
{
	static const Key
	A = 3000, // TODO kVK_ANSI_A,
	B = kVK_ANSI_B,
	C = kVK_ANSI_C,
	D = kVK_ANSI_D,
	E = kVK_ANSI_E,
	F = kVK_ANSI_F,
	G = kVK_ANSI_G,
	H = kVK_ANSI_H,
	I = kVK_ANSI_I,
	J = kVK_ANSI_J,
	K = kVK_ANSI_K,
	L = kVK_ANSI_L,
	M = kVK_ANSI_M,
	N = kVK_ANSI_N,
	O = kVK_ANSI_O,
	P = kVK_ANSI_P,
	Q = kVK_ANSI_Q,
	R = kVK_ANSI_R,
	S = kVK_ANSI_S,
	T = kVK_ANSI_T,
	U = kVK_ANSI_U,
	V = kVK_ANSI_V,
	W = kVK_ANSI_W,
	X = kVK_ANSI_X,
	Y = kVK_ANSI_Y,
	Z = kVK_ANSI_Z,
	_1 = kVK_ANSI_1,
	_2 = kVK_ANSI_2,
	_3 = kVK_ANSI_3,
	_4 = kVK_ANSI_4,
	_5 = kVK_ANSI_5,
	_6 = kVK_ANSI_6,
	_7 = kVK_ANSI_7,
	_8 = kVK_ANSI_8,
	_9 = kVK_ANSI_9,
	_0 = kVK_ANSI_0,
	ENTER = kVK_Return,
	ESCAPE = kVK_Escape,
	BACK = ESCAPE,
	BACK_SPACE = kVK_Delete,
	TAB = kVK_Tab,
	SPACE = kVK_Space,
	MINUS = kVK_ANSI_Minus,
	EQUALS = kVK_ANSI_Equal,
	LEFT_BRACKET = kVK_ANSI_LeftBracket,
	RIGHT_BRACKET = kVK_ANSI_RightBracket,
	BACKSLASH = kVK_ANSI_Backslash,

	SEMICOLON = kVK_ANSI_Semicolon,
	APOSTROPHE = kVK_ANSI_Quote,
	GRAVE = kVK_ANSI_Grave,
	COMMA = kVK_ANSI_Comma,
	PERIOD = kVK_ANSI_Period,
	SLASH = kVK_ANSI_Slash,
	CAPS = kVK_CapsLock,
	F1 = kVK_F1,
	F2 = kVK_F2,
	F3 = kVK_F3,
	F4 = kVK_F4,
	F5 = kVK_F5,
	F6 = kVK_F6,
	F7 = kVK_F7,
	F8 = kVK_F8,
	F9 = kVK_F9,
	F10 = kVK_F10,
	F11 = kVK_F11,
	F12 = kVK_F12,
	PRINT_SCREEN = NSPrintScreenFunctionKey, // TODO
	SCROLL_LOCK = NSScrollLockFunctionKey, // TODO
	PAUSE = NSPauseFunctionKey, // TODO
	INSERT = NSInsertFunctionKey, // TODO
	HOME = kVK_Home,
	PGUP = kVK_PageUp,
	DELETE = kVK_ForwardDelete,
	END = kVK_End,
	PGDOWN = kVK_PageDown,
	RIGHT = kVK_RightArrow,
	LEFT = kVK_LeftArrow,
	DOWN = kVK_DownArrow,
	UP = kVK_UpArrow,
	NUM_LOCK = 2010, // TODO
	NUMPAD_DIV = kVK_ANSI_KeypadDivide,
	NUMPAD_MULT = kVK_ANSI_KeypadMultiply,
	NUMPAD_SUB = kVK_ANSI_KeypadMinus,
	NUMPAD_ADD = kVK_ANSI_KeypadPlus,
	NUMPAD_ENTER = kVK_ANSI_KeypadEnter,
	NUMPAD_1 = kVK_ANSI_Keypad1,
	NUMPAD_2 = kVK_ANSI_Keypad2,
	NUMPAD_3 = kVK_ANSI_Keypad3,
	NUMPAD_4 = kVK_ANSI_Keypad4,
	NUMPAD_5 = kVK_ANSI_Keypad5,
	NUMPAD_6 = kVK_ANSI_Keypad6,
	NUMPAD_7 = kVK_ANSI_Keypad7,
	NUMPAD_8 = kVK_ANSI_Keypad8,
	NUMPAD_9 = kVK_ANSI_Keypad9,
	NUMPAD_0 = kVK_ANSI_Keypad0,
	NUMPAD_DOT = kVK_ANSI_KeypadDecimal,
	NUMPAD_COMMA = 2026, // TODO

	NUMPAD_EQUALS = kVK_ANSI_KeypadEquals,

	MENU = NSMenuFunctionKey, // TODO

	LCTRL = 2006, // TODO
	LSHIFT = 2005, // TODO
	LALT = 2003, // TODO
	LSUPER = 2007, // TODO
	RCTRL = kVK_RightControl,
	RSHIFT = kVK_RightShift,
	RALT = kVK_RightOption,
	RSUPER = kVK_Command,

	/*LEFT = NSLeftArrowFunctionKey,
	RIGHT = NSRightArrowFunctionKey,
	UP = NSUpArrowFunctionKey,
	DOWN = NSDownArrowFunctionKey,
	BACK_SPACE = 127,
	MENU = NSMenuFunctionKey,
	PGUP = NSPageUpFunctionKey,
	PGDOWN = NSPageDownFunctionKey,
	RSHIFT = 2001, // TODO
	LCTRL = 2002, // TODO
	CAPS = 2007, // TODO
	LMETA = 2008, // TODO
	RMETA = 2009, // TODO
	NUM_LOCK = 2010, // TODO
	NUMPAD_0 = 2011, // TODO
	NUMPAD_1 = 2012, // TODO
	NUMPAD_2 = 2013, // TODO
	NUMPAD_3 = 2014, // TODO
	NUMPAD_4 = 2015, // TODO
	NUMPAD_5 = 2016, // TODO
	NUMPAD_6 = 2017, // TODO
	NUMPAD_7 = 2018, // TODO
	NUMPAD_8 = 2019, // TODO
	NUMPAD_9 = 2020, // TODO
	NUMPAD_DIV = 2021, // TODO
	NUMPAD_MULT = 2022, // TODO
	NUMPAD_SUB = 2023, // TODO
	NUMPAD_ADD = 2024, // TODO
	NUMPAD_DOT = 2025, // TODO
	NUMPAD_COMMA = 2026, // TODO
	NUMPAD_ENTER = 2027, // TODO
	NUMPAD_EQUALS = 2028, // TODO
	LSUPER = 2039, // TODO
	RSUPER = 2040, // TODO
	TAB = 9,
	HOME = NSHomeFunctionKey,
	DELETE = NSDeleteFunctionKey,
	SEARCH = NSFindFunctionKey,
	END = NSEndFunctionKey,
	INSERT = NSInsertFunctionKey,
	SCROLL_LOCK = NSScrollLockFunctionKey,
	PAUSE = NSPauseFunctionKey,
	F1 = NSF1FunctionKey,
	F2 = NSF2FunctionKey,
	F3 = NSF3FunctionKey,
	F4 = NSF4FunctionKey,
	F5 = NSF5FunctionKey,
	F6 = NSF6FunctionKey,
	F7 = NSF7FunctionKey,
	F8 = NSF8FunctionKey,
	F9 = NSF9FunctionKey,
	F10 = NSF10FunctionKey,
	F11 = NSF11FunctionKey,
	F12 = NSF12FunctionKey,
	PRINT_SCREEN = NSPrintScreenFunctionKey,*/

	// private key-codes for misc keys, gamepads, & analog -> digital joystick axis emulation
	gpKeyBase = 30000,
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

	static const uint32_t COUNT = 0xffff;
};

namespace Pointer
{
	// TODO
	static constexpr Key
	LBUTTON = 1,
	MBUTTON = 2,
	RBUTTON = 4,
	DOWN_BUTTON = 8,
	UP_BUTTON = 16;
}

}
