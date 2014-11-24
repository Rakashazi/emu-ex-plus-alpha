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

[[gnu::cold]] CallResult init();

// OS text input support
typedef DelegateFunc<void (const char *str)> InputTextDelegate;
uint startSysTextInput(InputTextDelegate callback, const char *initialText, const char *promptText, uint fontSizePixels);
void cancelSysTextInput();
void finishSysTextInput();
void placeSysTextInput(IG::WindowRect rect);
IG::WindowRect sysTextInputRect();

void setKeyRepeat(bool on);

// Control if volume keys are used by the app or passed on to the OS
void setHandleVolumeKeys(bool on);

void showSoftInput();
void hideSoftInput();
bool softInputIsActive();

void hideCursor();
void showCursor();

bool isVolumeKey(Key event);

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
	Key keyEvent;
	Key sysKey;

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
		MAP_EVDEV = 1,
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
		MAP_APPLE_GAME_CONTROLLER = 31
		;

	using KeyString = std::array<char, 4>;

	static const char *mapName(uint map);

	const char *mapName()
	{
		return mapName(map);
	}

	static uint mapNumKeys(uint map);

	constexpr Event() {}

	constexpr Event(uint devId, uint map, Key button, uint state, int x, int y, bool pointerIsTouch, Time time, const Device *device)
		: devId{devId}, map{map}, button{button}, state{state}, x{x}, y{y}, time{time}, device{device}, pointerIsTouch{pointerIsTouch} {}

	constexpr Event(uint devId, uint map, Key button, Key sysKey, uint state, uint metaState, Time time, const Device *device)
		: devId{devId}, map{map}, button{button}, sysKey_{sysKey}, state{state}, metaState{metaState}, time{time}, device{device} {}

	uint devId = 0, map = MAP_NULL;
	Key button = 0, sysKey_ = 0;
	#ifdef CONFIG_BASE_X11
	Key rawKey = 0;
	#endif
	uint state = 0;
	int x = 0, y = 0;
	uint metaState = 0;
	Time time{};
	const Device *device{};
	bool pointerIsTouch = false;

	bool stateIsPointer() const
	{
		return state == MOVED || state == EXIT_VIEW || state == ENTER_VIEW;
	}

	bool isPointer() const
	{
		return Config::Input::POINTING_DEVICES && (map == MAP_POINTER || stateIsPointer());
	}

	bool isRelativePointer() const
	{
		return Config::Input::RELATIVE_MOTION_DEVICES && state == MOVED_RELATIVE;
	}

	bool isTouch() const
	{
		return Config::Input::TOUCH_DEVICES && pointerIsTouch;
	}

	bool isKey() const
	{
		return !isPointer() && !isRelativePointer();
	}

	bool isDefaultConfirmButton(uint swapped = Input::swappedGamepadConfirm) const;
	bool isDefaultCancelButton(uint swapped = Input::swappedGamepadConfirm) const;
	bool isDefaultLeftButton() const;
	bool isDefaultRightButton() const;
	bool isDefaultUpButton() const;
	bool isDefaultDownButton() const;
	bool isDefaultPageUpButton() const;
	bool isDefaultPageDownButton() const;

	Key key() const
	{
		return sysKey_;
	}

	bool pushed() const
	{
		return state == PUSHED;
	}

	bool pushed(Key button) const
	{
		return pushed() && this->button == button;
	}

	bool pushedKey(Key sysKey) const
	{
		return pushed() && sysKey_ == sysKey;
	}

	bool released() const
	{
		return state == RELEASED;
	}

	bool moved() const
	{
		return state == MOVED;
	}

	bool isShiftPushed() const
	{
		return metaState != 0;
	}

	static const char *actionToStr(int action);

	KeyString keyString() const;
};

// Input device status

bool keyInputIsPresent();

void dispatchInputEvent(const Event &event);
void startKeyRepeatTimer(const Event &event);
void cancelKeyRepeatTimer();
void deinitKeyRepeatTimer();

IG::Point2D<int> transformInputPos(const Base::Window &win, IG::Point2D<int> srcPos);

using DeviceChangeDelegate = DelegateFunc<void (const Device &dev, Device::Change change)>;

// Called when a known input device addition/removal/change occurs
void setOnDeviceChange(DeviceChangeDelegate del);

}
