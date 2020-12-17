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

#include <imagine/config/defs.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/input/config.hh>
#include <imagine/input/bluetoothInputDefs.hh>
#include <array>
#include <vector>

namespace Base
{
class Window;
}

namespace Input
{

class Device;

enum { UNUSED, RELEASED, PUSHED, MOVED, MOVED_RELATIVE, EXIT_VIEW, ENTER_VIEW, CANCELED };
enum { POINTER_NORMAL, POINTER_INVERT };

class Event
{
public:
	static constexpr uint32_t MAP_NULL = 0,
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

	constexpr Event() {}

	constexpr Event(uint32_t devId, uint32_t map, Key button, uint32_t metaState, uint32_t state, int x, int y, int pointerID, bool pointerIsTouch, Time time, const Device *device)
		: devId{devId}, map_{map}, button{button}, state_{state}, x{x}, y{y}, pointerID_{pointerID}, metaState{metaState}, time_{time}, device_{device}, pointerIsTouch{pointerIsTouch} {}

	constexpr Event(uint32_t devId, uint32_t map, Key button, Key sysKey, uint32_t state, uint32_t metaState, int repeatCount, Time time, const Device *device)
		: devId{devId}, map_{map}, button{button}, sysKey_{sysKey}, state_{state}, metaState{metaState}, repeatCount{repeatCount}, time_{time}, device_{device} {}

	uint32_t deviceID() const;
	static const char *mapName(uint32_t map);
	const char *mapName();
	static uint32_t mapNumKeys(uint32_t map);
	uint32_t map() const;
	void setMap(uint32_t map);
	int pointerID() const;
	uint32_t state() const;
	bool stateIsPointer() const;
	bool isPointer() const;
	bool isRelativePointer() const;
	bool isTouch() const;
	bool isKey() const;
	bool isDefaultConfirmButton(uint32_t swapped) const;
	bool isDefaultCancelButton(uint32_t swapped) const;
	bool isDefaultConfirmButton() const;
	bool isDefaultCancelButton() const;
	bool isDefaultLeftButton() const;
	bool isDefaultRightButton() const;
	bool isDefaultUpButton() const;
	bool isDefaultDownButton() const;
	bool isDefaultDirectionButton() const;
	bool isDefaultPageUpButton() const;
	bool isDefaultPageDownButton() const;
	Key key() const;
	Key mapKey() const;
	#ifdef CONFIG_BASE_X11
	void setX11RawKey(Key key);
	#endif
	bool pushed() const;
	bool pushed(Key key) const;
	bool pushedKey(Key sysKey) const;
	bool released() const;
	bool released(Key key) const;
	bool releasedKey(Key sysKey) const;
	bool canceled() const;
	bool isOff() const;
	bool moved() const;
	bool isShiftPushed() const;
	int repeated() const;
	void setRepeatCount(int count);
	IG::WP pos() const;
	bool isPointerPushed(Key k) const;
	bool isSystemFunction() const;
	static const char *actionToStr(int action);
	KeyString keyString() const;
	Time time() const;
	const Device *device() const;

protected:
	uint32_t devId = 0, map_ = MAP_NULL;
	Key button = 0, sysKey_ = 0;
	#ifdef CONFIG_BASE_X11
	Key rawKey = 0;
	#endif
	uint32_t state_ = 0;
	int x = 0, y = 0;
	int pointerID_ = 0;
	uint32_t metaState = 0;
	int repeatCount = 0;
	Time time_{};
	const Device *device_{};
	bool pointerIsTouch = false;
};

static constexpr bool SWAPPED_GAMEPAD_CONFIRM_DEFAULT = Config::MACHINE_IS_PANDORA ? true : false;

const std::vector<Device*> &deviceList();

// OS text input support
typedef DelegateFunc<void (const char *str)> InputTextDelegate;
uint32_t startSysTextInput(InputTextDelegate callback, const char *initialText, const char *promptText, uint32_t fontSizePixels);
void cancelSysTextInput();
void finishSysTextInput();
void placeSysTextInput(IG::WindowRect rect);
IG::WindowRect sysTextInputRect();

void setHintKeyRepeat(bool on);

void showSoftInput();
void hideSoftInput();
bool softInputIsActive();

void hideCursor();
void showCursor();

void addDevice(Device &d);
void removeDevice(Device &d);
void indexDevices();

void xPointerTransform(uint32_t mode);
void yPointerTransform(uint32_t mode);
void pointerAxis(uint32_t mode);
void configureInputForOrientation(const Base::Window &win);

Event defaultEvent();

// Input device status

bool keyInputIsPresent();

bool dispatchInputEvent(Event event);
void flushEvents();
void flushSystemEvents();
void flushInternalEvents();
void startKeyRepeatTimer(Event event);
void cancelKeyRepeatTimer();
void deinitKeyRepeatTimer();

IG::Point2D<int> transformInputPos(const Base::Window &win, IG::Point2D<int> srcPos);

void setSwappedGamepadConfirm(bool swapped);
bool swappedGamepadConfirm();

}
