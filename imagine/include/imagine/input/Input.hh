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
#include <imagine/input/inputDefs.hh>
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
	using KeyString = std::array<char, 4>;

	constexpr Event() {}

	constexpr Event(uint32_t devId, Map map, Key button, uint32_t metaState, uint32_t state, int x, int y, int pointerID, Source src, Time time, const Device *device)
		: device_{device}, time_{time}, devId{devId}, state_{state}, x{x}, y{y}, pointerID_{pointerID}, metaState{metaState}, button{button}, map_{map}, src{src} {}

	constexpr Event(uint32_t devId, Map map, Key button, Key sysKey, uint32_t state, uint32_t metaState, int repeatCount, Source src, Time time, const Device *device)
		: device_{device}, time_{time}, devId{devId}, state_{state}, metaState{metaState}, repeatCount{repeatCount}, button{button}, sysKey_{sysKey}, map_{map}, src{src} {}

	uint32_t deviceID() const;
	static const char *mapName(Map map);
	const char *mapName();
	static uint32_t mapNumKeys(Map map);
	Map map() const;
	void setMap(Map map);
	int pointerID() const;
	uint32_t state() const;
	bool stateIsPointer() const;
	bool isPointer() const;
	bool isRelativePointer() const;
	bool isTouch() const;
	bool isKey() const;
	bool isGamepad() const;
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
	const Device *device_{};
	Time time_{};
	uint32_t devId = 0;
	uint32_t state_ = 0;
	int x = 0, y = 0;
	int pointerID_ = 0;
	uint32_t metaState = 0;
	int repeatCount = 0;
	Key button = 0, sysKey_ = 0;
	#ifdef CONFIG_BASE_X11
	Key rawKey = 0;
	#endif
	Map map_{Map::UNKNOWN};
	Source src{Source::UNKNOWN};
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

const char *sourceStr(Source);
Map validateMap(uint8_t mapValue);

}
