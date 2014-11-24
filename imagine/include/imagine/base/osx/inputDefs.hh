#pragma once

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

namespace Input
{

using Time = double; // NSTimeInterval

static Time msToTime(int ms)
{
	return ms / 1000.;
}

// TODO: remove dummy defs
namespace OSX
{
	static const uint ESCAPE = 27,
	ENTER = 13,
	LALT = 2003, // TODO
	RALT = 2004, // TODO
	LSHIFT = 2005, // TODO
	RCTRL = 2006, // TODO
	LEFT = NSLeftArrowFunctionKey,
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
	PRINT_SCREEN = NSPrintScreenFunctionKey
	;

	static const uint COUNT = 0xffff;
};

typedef uint16 Key;

namespace Keycode = OSX;

namespace Pointer
{
	static const uint LBUTTON = 1;
}

}
