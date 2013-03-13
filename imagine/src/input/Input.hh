#pragma once

#include <engine-globals.h>
#include <util/number.h>
#include <util/rectangle2.h>
#include <util/Delegate.hh>
#include <util/collection/DLList.hh>

#ifdef CONFIG_BASE_X11
	#include <base/x11/inputDefs.hh>
#elif defined(CONFIG_BASE_ANDROID)
	#include <input/android/inputDefs.hh>
#elif defined(CONFIG_BASE_SDL)
	#include <base/sdl/inputDefs.hh>
#elif defined(CONFIG_BASE_IOS)
	#include <base/iphone/inputDefs.hh>
#elif defined(CONFIG_BASE_MACOSX)
	#include <base/osx/inputDefs.hh>
#elif defined(CONFIG_BASE_PS3)
	#include <input/ps3/inputDefs.hh>
#endif

namespace Input
{

// mouse/pointer/touch support
#ifndef CONFIG_INPUT_PS3
	static const bool SUPPORTS_POINTER = 1;
	#define INPUT_SUPPORTS_POINTER
#else
	static const bool SUPPORTS_POINTER = 0;
#endif

#ifdef CONFIG_BASE_X11
	static const bool SUPPORTS_MOUSE = 1;
	#define INPUT_SUPPORTS_MOUSE
#else
	static const bool SUPPORTS_MOUSE = 0;
#endif

static const uchar maxCursors =
#if defined(CONFIG_BASE_X11)
	4 // unknown max
#elif defined(CONFIG_BASE_IOS)
	#if !defined(__ARM_ARCH_6K__)
		7
	#else
		4
	#endif
#elif defined(CONFIG_BASE_ANDROID)
	#if CONFIG_ENV_ANDROID_MINSDK == 4
		1 // no multi-touch
	#else
		4 // unknown max
	#endif
#elif defined(CONFIG_ENV_WEBOS)
	4 // max 5
#else
	1
#endif
;

// OS text input support
typedef Delegate<void (const char *str)> InputTextDelegate;
#if defined CONFIG_BASE_IOS || defined CONFIG_BASE_ANDROID
	#define CONFIG_INPUT_SYSTEM_CAN_COLLECT_TEXT
	static const bool SYSTEM_CAN_COLLECT_TEXT = 1;
	uint startSysTextInput(InputTextDelegate callback, const char *initialText, const char *promptText);
	void cancelSysTextInput();
	void finishSysTextInput();
	void placeSysTextInput(const Rect2<int> &rect);
	const Rect2<int> &sysTextInputRect();
#else
	static const bool SYSTEM_CAN_COLLECT_TEXT = 0;
	static uint startSysTextInput(InputTextDelegate callback, const char *initialText, const char *promptText) { return 0; }
#endif

// relative pointer/trackball support
#ifdef CONFIG_BASE_ANDROID
	static const bool supportsRelativePointer = 1;
	#define INPUT_SUPPORTS_RELATIVE_POINTER
#else
	static const bool supportsRelativePointer = 0;
#endif

// keyboard/key-based input support
#if !defined(CONFIG_BASE_IOS) && !defined(CONFIG_BASE_PS3)
	static const bool supportsKeyboard = 1;
	#define INPUT_SUPPORTS_KEYBOARD
#else
	static const bool supportsKeyboard = 0;
#endif

// dynamic input device list from system
#if defined CONFIG_BASE_X11 || (defined CONFIG_BASE_ANDROID && CONFIG_ENV_ANDROID_MINSDK >= 9)
	static const bool hasSystemDeviceHotswap = 1;
	#define INPUT_HAS_SYSTEM_DEVICE_HOTSWAP
#else
	static const bool hasSystemDeviceHotswap = 0;
#endif

static const uint MAX_BLUETOOTH_DEVS_PER_TYPE = 5;

CallResult init() ATTRS(cold);

void setKeyRepeat(bool on);

// Control if volume keys are used by the app or passed on to the OS
#ifdef CONFIG_BASE_ANDROID
	void setHandleVolumeKeys(bool on);
#else
	static void setHandleVolumeKeys(bool on) { }
#endif

#if defined(CONFIG_INPUT_ANDROID) || CONFIG_ENV_WEBOS_OS >= 3
	void showSoftInput();
	void hideSoftInput();
	#if CONFIG_ENV_WEBOS_OS >= 3
		bool softInputIsActive();
	#endif
#else
	static void showSoftInput() { }
	static void hideSoftInput() { }
	static bool softInputIsActive() { return 0; }
#endif

#if defined(CONFIG_INPUT_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9
	void setEventsUseOSInputMethod(bool on);
	bool eventsUseOSInputMethod();
#else
	static void setEventsUseOSInputMethod(bool on) { }
	static bool eventsUseOSInputMethod() { return 0; }
#endif

#ifdef INPUT_SUPPORTS_POINTER
	extern uint numCursors;
	void hideCursor();
	void showCursor();
#else
	static uint numCursors = 0;
	static void hideCursor() { }
	static void showCursor() { }
#endif

static const uint numAsciiKeys = (('~' - '!') + 1);

#ifdef CONFIG_BASE_ANDROID

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

static uint decodeAscii(Key k, bool isShiftPushed)
{
	switch(k)
	{
		case 7 ... 16: // 0 - 9
			return k + 41;
		case 29 ... 54: // a - z
		{
			uint ascii = k + 68;
			if(isShiftPushed)
				ascii -= 32;
			return ascii;
		}
		case 17: return '*';
		case 18: return '#';
		case 55: return ',';
		case 56: return '.';
		case 62: return ' ';
		case 66: return '\n';
		case 68: return '`';
		case 69: return '-';
		case 70: return '=';
		case 71: return '[';
		case 72: return ']';
		case 73: return '\\';
		case 74: return ';';
		case 75: return '\'';
		case 76: return '/';
		case 77: return '@';
		case 81: return '+';
	}
	return 0;
}

static bool isAsciiKey(Key k)
{
	return decodeAscii(k, 0) != 0;
}

#elif defined(CONFIG_BASE_IOS)

// TODO
static constexpr uint asciiKey(uint c) { return c; }

static uint decodeAscii(Key k, bool isShiftPushed)
{
	bug_exit("TODO");
	return 0;
}

#else

static constexpr uint asciiKey(uint c) { return c; }

static uint decodeAscii(Key k, bool isShiftPushed)
{
	if(isShiftPushed && (k >= 'a' || k <= 'z'))
		k -= 32;
	switch(k)
	{
		#ifdef INPUT_SUPPORTS_KEYBOARD
		case Keycode::ENTER: return '\n';
		case Keycode::BACK_SPACE: return '\b';
		#endif
		case ' ' ... '~': return k;
	}
	return 0;
}

static bool isAsciiKey(Key k)
{
	return decodeAscii(k, 0) != 0;
}

#endif

namespace Wiimote
{
	static const uint PLUS = 1,
	MINUS = 2,
	HOME = 3,
	LEFT = 4, RIGHT = 5, UP = 6, DOWN = 7,
	_1 = 8,
	_2 = 9,
	A = 10,
	B = 11,
	// Nunchuk
	NUN_C = 12, NUN_Z = 13,
	NUN_STICK_LEFT = 14, NUN_STICK_RIGHT = 15, NUN_STICK_UP = 16, NUN_STICK_DOWN = 17
	;

	static const uint COUNT = 14;
}

namespace WiiCC
{
	static const uint PLUS = 1,
	MINUS = 2,
	HOME = 3,
	LEFT = 4, RIGHT = 5, UP = 6, DOWN = 7,
	A = 8, B = 9,
	X = 10, Y = 11,
	L = 12, R = 13,
	ZL = 14, ZR = 15,
	LSTICK_LEFT = 16, LSTICK_RIGHT = 17, LSTICK_UP = 18, LSTICK_DOWN = 19,
	RSTICK_LEFT = 20, RSTICK_RIGHT = 21, RSTICK_UP = 22, RSTICK_DOWN = 23
	;

	static const uint COUNT = 24;
}

namespace iControlPad
{
	static const uint A = 1,
	B = 2,
	X = 3,
	Y = 4,
	L = 5,
	R = 6,
	START = 7,
	SELECT = 8,
	LNUB_LEFT = 9, LNUB_RIGHT = 10, LNUB_UP = 11, LNUB_DOWN = 12,
	RNUB_LEFT = 13, RNUB_RIGHT = 14, RNUB_UP = 15, RNUB_DOWN = 16,
	LEFT = 17, RIGHT = 18, UP = 19, DOWN = 20
	;

	static const uint COUNT = 21;
}

namespace Zeemote
{
	static const uint A = 1,
	B = 2,
	C = 3,
	POWER = 4,
	// Directions (from analog stick)
	LEFT = 5, RIGHT = 6, UP = 7, DOWN = 8
	;

	static const uint COUNT = 9;
}

namespace Ps3
{
	static const uint CROSS = 1,
	CIRCLE = 2,
	SQUARE = 3,
	TRIANGLE = 4,
	L1 = 5,
	L2 = 6,
	L3 = 7,
	R1 = 8,
	R2 = 9,
	R3 = 10,
	SELECT = 11,
	START = 12,
	UP = 13, RIGHT = 14, DOWN = 15, LEFT = 16
	;

	static const uint COUNT = 17;
}

namespace ICade
{
#ifdef CONFIG_BASE_IOS
	// dedicated mapping
	static const uint UP = 1,
	RIGHT = 2,
	DOWN = 3,
	LEFT = 4,
	A = 5,
	B = 6,
	C = 7,
	D = 8,
	E = 9,
	F = 10,
	G = 11,
	H = 12
	;

	static const uint COUNT = 13;
#else
	// mapping overlaps system/keyboard so the same "device" can send iCade
	// events as well as other key events that don't conflict with its mapping.
	// Here we just use all the On-States since they won't be sent as regular
	// key events.
	static const uint UP = asciiKey('w'),
		RIGHT = asciiKey('d'),
		DOWN = asciiKey('x'),
		LEFT = asciiKey('a'),
		A = asciiKey('y'),
		B = asciiKey('h'),
		C = asciiKey('u'),
		D = asciiKey('j'),
		E = asciiKey('i'),
		F = asciiKey('k'),
		G = asciiKey('o'),
		H = asciiKey('l')
		;

	static const uint COUNT = Keycode::COUNT;
#endif
}

static bool isVolumeKey(Key event)
{
	#if defined CONFIG_BASE_SDL || defined CONFIG_BASE_MACOSX || !defined INPUT_SUPPORTS_KEYBOARD
		return 0;
	#else
		return event == Keycode::VOL_UP || event == Keycode::VOL_DOWN;
	#endif
}

const char *buttonName(uint map, Key b) ATTRS(const);

struct Device
{
private:
	uint map_ = 0;
	#ifdef CONFIG_INPUT_ICADE
		bool iCadeMode_ = 0;
	#endif
	uint type_ = 0;
public:
	uint devId = 0, subtype = 0, idx = 0;
	const char *name_ = nullptr;
	bool mapJoystickAxis1ToDpad = 0;

	constexpr Device() { }
	constexpr Device(uint devId, uint map, uint type, const char *name_): map_(map), type_(type), devId(devId), name_(name_) { }

	static constexpr uint SUBTYPE_NONE = 0,
			SUBTYPE_XPERIA_PLAY = 1,
			SUBTYPE_PS3_CONTROLLER = 2
			;

	static constexpr uint
			TYPE_BIT_KEY_MISC = BIT(0),
			TYPE_BIT_KEYBOARD = BIT(1),
			TYPE_BIT_GAMEPAD = BIT(2),
			TYPE_BIT_JOYSTICK = BIT(3),
			TYPE_BIT_VIRTUAL = BIT(4)
			;

	bool hasKeyboard() const
	{
		return typeBits() & TYPE_BIT_KEYBOARD;
	}

	bool hasGamepad() const
	{
		return typeBits() & TYPE_BIT_GAMEPAD;
	}

	bool hasJoystick() const
	{
		return typeBits() & TYPE_BIT_JOYSTICK;
	}

	bool isVirtual() const
	{
		return typeBits() & TYPE_BIT_VIRTUAL;
	}

	const char *name() const { return name_; }
	uint map() const;
	void setMap(uint map) { map_ = map; };

	uint typeBits() const
	{
		return
		#ifdef CONFIG_INPUT_ICADE
			iCadeMode_ ? TYPE_BIT_GAMEPAD :
		#endif
		type_;
	}
	void setTypeBits(uint type) { type_ = type; }

	bool operator ==(Device const& rhs) const
	{
		return devId == rhs.devId && map_ == rhs.map_ && string_equal(name(), rhs.name());
	}

	#ifdef CONFIG_INPUT_ICADE
		void setICadeMode(bool on);
		bool iCadeMode() const { return iCadeMode_; }
	#endif

	// TODO
	//bool isDisconnectable() { return 0; }
	//void disconnect() { }

	#if defined CONFIG_ENV_WEBOS
		bool anyTypeBitsPresent(uint typeBits) { bug_exit("TODO"); return 0; }
	#else
		static bool anyTypeBitsPresent(uint typeBits);
	#endif
};

static constexpr uint MAX_DEVS = Config::envIsAndroid ? 24 : 16;
extern StaticDLList<Device, MAX_DEVS> devList;

void addDevice(Device d);
void removeDevice(Device d);
void indexDevices();

enum { UNUSED, RELEASED, PUSHED, MOVED, MOVED_RELATIVE, EXIT_VIEW, ENTER_VIEW };
enum { POINTER_NORMAL, POINTER_INVERT };

void xPointerTransform(uint mode);
void yPointerTransform(uint mode);
void pointerAxis(uint mode);

struct PackedInputAccess
{
	uint byteOffset;
	uint mask;
	uint keyEvent;

	int updateState(const uchar *prev, const uchar *curr) const
	{
		bool oldState = prev[byteOffset] & mask,
			newState = curr[byteOffset] & mask;
		if(oldState != newState)
		{
			return newState;
		}
		return -1; // no state change
	}

};

extern bool swappedGamepadConfirm;

class Event
{
public:
	static constexpr uint MAP_NULL = 0,
		MAP_KEYBOARD = 1,
		MAP_POINTER = 2,
		MAP_REL_POINTER = 3,
		MAP_WIIMOTE = 10, MAP_WII_CC = 11,
		MAP_ICONTROLPAD = 20,
		MAP_ZEEMOTE = 21,
		MAP_ICADE = 22,
		MAP_PS3PAD = 23
		;

	static const char *mapName(uint map)
	{
		switch(map)
		{
			case MAP_NULL: return "Null";
			case MAP_KEYBOARD: return "Key Input";
			case MAP_POINTER: return "Pointer";
			case MAP_REL_POINTER: return "Relative Pointer";
			#ifdef CONFIG_BLUETOOTH
			case MAP_WIIMOTE: return "Wiimote";
			case MAP_WII_CC: return "Classic / Wii U Pro Controller";
			case MAP_ICONTROLPAD: return "iControlPad";
			case MAP_ZEEMOTE: return "Zeemote JS1";
			#endif
			#ifdef CONFIG_BASE_PS3
			case MAP_PS3PAD: return "PS3 Gamepad";
			#endif
			#ifdef CONFIG_INPUT_ICADE
			case MAP_ICADE: return "iCade";
			#endif
			default: return "Unknown";
		}
	}

	const char *mapName()
	{
		return mapName(map);
	}

	static uint mapNumKeys(uint map)
	{
		switch(map)
		{
			case MAP_NULL: return 0;
			#ifdef INPUT_SUPPORTS_KEYBOARD
			case MAP_KEYBOARD: return Input::Keycode::COUNT;
			#endif
			#ifdef CONFIG_BLUETOOTH
			case MAP_WIIMOTE: return Input::Wiimote::COUNT;
			case MAP_WII_CC: return Input::WiiCC::COUNT;
			case MAP_ICONTROLPAD: return Input::iControlPad::COUNT;
			case MAP_ZEEMOTE: return Input::Zeemote::COUNT;
			#endif
			#ifdef CONFIG_BASE_PS3
			case MAP_PS3PAD: return Input::Ps3::COUNT;
			#endif
			#ifdef CONFIG_INPUT_ICADE
			case MAP_ICADE: return Input::ICade::COUNT;
			#endif
			default: bug_branch("%d", map); return 0;
		}
	}

	constexpr Event() { }

	constexpr Event(uint devId, uint map, Key button, uint state, int x, int y, const Device *device)
		: devId(devId), map(map), button(button), state(state), x(x), y(y), device(device) { }

	constexpr Event(uint devId, uint map, Key button, uint state, uint metaState, const Device *device)
		: devId(devId), map(map), button(button), state(state), metaState(metaState), device(device) { }

	uint devId = 0, map = MAP_NULL;
	Key button = 0;
	uint state = 0;
	int x = 0, y = 0;
	uint metaState = 0;
	const Device *device = nullptr;

	bool stateIsPointer() const
	{
		return state == MOVED || state == EXIT_VIEW || state == ENTER_VIEW;
	}

	bool isPointer() const
	{
		return Input::SUPPORTS_POINTER && (map == MAP_POINTER/*input_eventIsFromPointer(button)*/ || stateIsPointer());
	}

	bool isRelativePointer() const
	{
		return Input::supportsRelativePointer && state == MOVED_RELATIVE;
	}

	bool isGamepad() const
	{
		switch(map)
		{
			#ifdef CONFIG_BLUETOOTH
			case MAP_WIIMOTE:
			case MAP_WII_CC:
			case MAP_ICONTROLPAD:
			case MAP_ZEEMOTE: return 1;
			#endif
			#ifdef CONFIG_INPUT_ICADE
			case MAP_ICADE: return 1;
			#endif
			default : return 0;
		}
	}

	bool isKey() const
	{
		return Input::supportsKeyboard && !isPointer() && !isRelativePointer();
	}

	bool isKeyboard() const
	{
		return Input::supportsKeyboard && map == MAP_KEYBOARD;
	}

	bool isDefaultConfirmButton(uint swapped = Input::swappedGamepadConfirm) const;
	bool isDefaultCancelButton(uint swapped = Input::swappedGamepadConfirm) const;
	bool isDefaultLeftButton() const;
	bool isDefaultRightButton() const;
	bool isDefaultUpButton() const;
	bool isDefaultDownButton() const;
	bool isDefaultPageUpButton() const;
	bool isDefaultPageDownButton() const;

	bool pushed() const
	{
		return state == PUSHED;
	}

	bool pushed(Key button) const
	{
		return pushed() && this->button == button;
	}

	bool released() const
	{
		return state == RELEASED;
	}

	bool moved() const
	{
		return state == MOVED;
	}

	uint decodeAscii() const
	{
		return Input::decodeAscii(button, 0);
	}

	bool isShiftPushed() const
	{
		return metaState != 0;
	}
};

// Input device status

struct DeviceChange
{
	uint devId, map;
	uint state;
	enum { ADDED, REMOVED, SHOWN, HIDDEN };

	bool added() const { return state == ADDED; }
	bool removed() const { return state == REMOVED; }
	bool shown() const { return state == SHOWN; }
	bool hidden() const { return state == HIDDEN; }
};

bool keyInputIsPresent();

const char *eventActionToStr(int action);

#ifdef CONFIG_BASE_X11
	void setTranslateKeyboardEventsByModifiers(bool on);
#else
	static void setTranslateKeyboardEventsByModifiers(bool on) { }
#endif

// App Callbacks

// Called when a known input device addition/removal/change occurs
void onInputDevChange(const DeviceChange &change);

// Called to process an event from an input device
void onInputEvent(const Input::Event &event);

}
