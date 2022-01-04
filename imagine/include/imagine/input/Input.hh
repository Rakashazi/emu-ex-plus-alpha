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
#include <imagine/input/bluetoothInputDefs.hh>
#include <imagine/input/inputDefs.hh>
#include <string>

namespace IG
{
class Window;
class ApplicationContext;
class Application;
}

namespace IG::Input
{

class Device;

enum class Action : uint8_t
{
	UNUSED,
	RELEASED,
	PUSHED,
	MOVED,
	MOVED_RELATIVE,
	EXIT_VIEW,
	ENTER_VIEW,
	SCROLL_UP,
	SCROLL_DOWN,
	CANCELED
};

static constexpr bool SWAPPED_CONFIRM_KEYS_DEFAULT = Config::MACHINE_IS_PANDORA ? true : false;

class Event
{
public:
	constexpr Event() = default;

	constexpr Event(Map map, Key button, uint32_t metaState, Action state, int x, int y, PointerId pointerId, Source src, Time time, const Device *device)
		: device_{device}, pointerId_{pointerId}, time_{time}, x{x}, y{y}, metaState{metaState}, button{button}, state_{state}, map_{map}, src{src} {}

	constexpr Event(Map map, Key button, Key sysKey, Action state, uint32_t metaState, int repeatCount, Source src, Time time, const Device *device)
		: device_{device}, time_{time}, metaState{metaState}, repeatCount{repeatCount}, button{button}, sysKey_{sysKey}, state_{state}, map_{map}, src{src} {}

	static std::string_view mapName(Map map);
	std::string_view mapName() const;
	static uint32_t mapNumKeys(Map map);
	Map map() const;
	void setMap(Map map);
	PointerId pointerId() const;
	Action state() const;
	void setKeyFlags(uint8_t flags);
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
	uint32_t metaKeyBits() const;
	bool isShiftPushed() const;
	int repeated() const;
	void setRepeatCount(int count);
	IG::WP pos() const;
	bool pointerDown(Key btnMask) const;
	int scrolledVertical() const;
	bool isSystemFunction() const;
	static std::string_view actionToStr(Action action);
	std::string keyString(ApplicationContext) const;
	Time time() const;
	const Device *device() const;
	bool hasSwappedConfirmKeys() const;

protected:
	const Device *device_{};
	PointerId pointerId_{};
	Time time_{};
	int x{}, y{};
	uint32_t metaState{};
	int repeatCount{};
	Key button{}, sysKey_{};
	#ifdef CONFIG_BASE_X11
	Key rawKey{};
	#endif
	Action state_{};
	Map map_{};
	Source src{};
	uint8_t keyFlags{};
};

std::string_view sourceStr(Source);
std::string_view actionStr(Action);
Map validateMap(uint8_t mapValue);

struct DirectionKeys
{
	Key up{}, right{}, down{}, left{};
};

DirectionKeys directionKeys(Map map = Map::SYSTEM);

}
