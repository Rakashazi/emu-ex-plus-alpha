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

#include <imagine/engine-globals.h>
#include <imagine/util/number.h>
#include <imagine/util/rectangle2.h>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/container/ArrayList.hh>
#include <imagine/input/config.hh>
#include <imagine/input/Device.hh>

namespace Base
{
class Window;
}

namespace Input
{

// OS text input support
typedef DelegateFunc<void (const char *str)> InputTextDelegate;
#if defined CONFIG_BASE_IOS || defined CONFIG_BASE_ANDROID
#define CONFIG_INPUT_SYSTEM_CAN_COLLECT_TEXT
static const bool SYSTEM_CAN_COLLECT_TEXT = 1;
uint startSysTextInput(InputTextDelegate callback, const char *initialText, const char *promptText, uint fontSizePixels);
void cancelSysTextInput();
void finishSysTextInput();
void placeSysTextInput(const IG::WindowRect &rect);
const IG::WindowRect &sysTextInputRect();
#else
static const bool SYSTEM_CAN_COLLECT_TEXT = 0;
static uint startSysTextInput(InputTextDelegate callback, const char *initialText, const char *promptText, uint fontSizePixels) { return 0; }
#endif

[[gnu::cold]] CallResult init();

void setKeyRepeat(bool on);

// Control if volume keys are used by the app or passed on to the OS
#ifdef CONFIG_BASE_ANDROID
void setHandleVolumeKeys(bool on);
#else
static void setHandleVolumeKeys(bool on) {}
#endif

#if defined(CONFIG_INPUT_ANDROID) || CONFIG_ENV_WEBOS_OS >= 3
void showSoftInput();
void hideSoftInput();
	#if CONFIG_ENV_WEBOS_OS >= 3
	bool softInputIsActive();
	#endif
#else
static void showSoftInput() {}
static void hideSoftInput() {}
static bool softInputIsActive() { return 0; }
#endif

#if defined(CONFIG_INPUT_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9
void setEventsUseOSInputMethod(bool on);
bool eventsUseOSInputMethod();
#else
static void setEventsUseOSInputMethod(bool on) {}
static bool eventsUseOSInputMethod() { return 0; }
#endif

#ifdef INPUT_SUPPORTS_POINTER
extern uint numCursors;
void hideCursor();
void showCursor();
#else
static uint numCursors = 0;
static void hideCursor() {}
static void showCursor() {}
#endif

static bool isVolumeKey(Key event)
{
	#if defined CONFIG_BASE_SDL || defined CONFIG_BASE_MACOSX || defined CONFIG_BASE_WIN32 || !defined INPUT_SUPPORTS_KEYBOARD
	return 0;
	#else
	return event == Keycode::VOL_UP || event == Keycode::VOL_DOWN;
	#endif
}

static constexpr uint MAX_DEVS = Config::envIsAndroid ? 24 : 16;
extern StaticArrayList<Device*, MAX_DEVS> devList;

void addDevice(Device &d);
void removeDevice(Device &d);
void indexDevices();

enum { UNUSED, RELEASED, PUSHED, MOVED, MOVED_RELATIVE, EXIT_VIEW, ENTER_VIEW };
enum { POINTER_NORMAL, POINTER_INVERT };

void xPointerTransform(uint mode);
void yPointerTransform(uint mode);
void pointerAxis(uint mode);
void configureInputForOrientation(const Base::Window &win);

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
static constexpr bool SWAPPED_GAMEPAD_CONFIRM_DEFAULT = Config::MACHINE_IS_PANDORA ? true : false;

class Event
{
public:
	static constexpr uint MAP_NULL = 0,
		MAP_SYSTEM = 1,
		MAP_X = 1,
		MAP_ANDROID = 1,
		MAP_WIN32 = 1,
		MAP_MACOSX = 1,

		MAP_POINTER = 2,
		MAP_REL_POINTER = 3,
		MAP_WIIMOTE = 10, MAP_WII_CC = 11,
		MAP_ICONTROLPAD = 20,
		MAP_ZEEMOTE = 21,
		MAP_ICADE = 22,
		MAP_PS3PAD = 23,
		MAP_EVDEV = 30
		;

	static const char *mapName(uint map)
	{
		switch(map)
		{
			case MAP_NULL: return "Null";
			case MAP_SYSTEM: return "Key Input";
			case MAP_POINTER: return "Pointer";
			case MAP_REL_POINTER: return "Relative Pointer";
			#ifdef CONFIG_BLUETOOTH
			case MAP_WIIMOTE: return "Wiimote";
			case MAP_WII_CC: return "Classic / Wii U Pro Controller";
			case MAP_ICONTROLPAD: return "iControlPad";
			case MAP_ZEEMOTE: return "Zeemote JS1";
			#endif
			#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
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
			case MAP_SYSTEM: return Input::Keycode::COUNT;
			#endif
			#ifdef CONFIG_BLUETOOTH
			case MAP_WIIMOTE: return Input::Wiimote::COUNT;
			case MAP_WII_CC: return Input::WiiCC::COUNT;
			case MAP_ICONTROLPAD: return Input::iControlPad::COUNT;
			case MAP_ZEEMOTE: return Input::Zeemote::COUNT;
			#endif
			#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
			case MAP_PS3PAD: return Input::PS3::COUNT;
			#endif
			#ifdef CONFIG_INPUT_ICADE
			case MAP_ICADE: return Input::ICade::COUNT;
			#endif
			#ifdef CONFIG_INPUT_EVDEV
			case MAP_EVDEV: return Input::Evdev::COUNT;
			#endif
			default: bug_branch("%d", map); return 0;
		}
	}

	constexpr Event() {}

	constexpr Event(uint devId, uint map, Key button, uint state, int x, int y, bool pointerIsTouch, Time time, const Device *device)
		: devId(devId), map(map), button(button), state(state), x(x), y(y), time(time), device(device), pointerIsTouch(pointerIsTouch) {}

	constexpr Event(uint devId, uint map, Key button, uint state, uint metaState, Time time, const Device *device)
		: devId(devId), map(map), button(button), state(state), metaState(metaState), time(time), device(device) {}

	uint devId = 0, map = MAP_NULL;
	Key button = 0;
	uint state = 0;
	int x = 0, y = 0;
	uint metaState = 0;
	Time time = (Time)0;
	const Device *device = nullptr;
	bool pointerIsTouch = false;

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

	bool isTouch() const
	{
		return Input::SUPPORTS_POINTER && pointerIsTouch;
	}

//	bool isGamepad() const
//	{
//		switch(map)
//		{
//			#ifdef CONFIG_BLUETOOTH
//			case MAP_WIIMOTE:
//			case MAP_WII_CC:
//			case MAP_ICONTROLPAD:
//			case MAP_ZEEMOTE: return 1;
//			#endif
//			#ifdef CONFIG_INPUT_ICADE
//			case MAP_ICADE: return 1;
//			#endif
//			default : return 0;
//		}
//	}

	bool isKey() const
	{
		return Input::supportsKeyboard && !isPointer() && !isRelativePointer();
	}

	bool isKeyboard() const
	{
		return Input::supportsKeyboard && map == MAP_SYSTEM;
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

	#ifdef INPUT_SUPPORTS_KEYBOARD
	uint decodeAscii() const
	{
		return Input::Keycode::decodeAscii(button, 0);
	}
	#endif

	bool isShiftPushed() const
	{
		return metaState != 0;
	}
};

// Input device status

bool keyInputIsPresent();

const char *eventActionToStr(int action);

#ifdef CONFIG_BASE_X11
void setTranslateKeyboardEventsByModifiers(bool on);
bool translateKeyboardEventsByModifiers();
#else
static void setTranslateKeyboardEventsByModifiers(bool on) {}
static bool translateKeyboardEventsByModifiers() { return false; }
#endif

void dispatchInputEvent(const Event &event);
void startKeyRepeatTimer(const Event &event);
void cancelKeyRepeatTimer();
void deinitKeyRepeatTimer();

// App Callbacks

// Called when a known input device addition/removal/change occurs
void onInputDevChange(const Device &dev, const Device::Change &change);

}
