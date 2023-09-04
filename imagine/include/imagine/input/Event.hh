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
#include <imagine/input/inputDefs.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/variant.hh>
#include <imagine/util/Point2D.hh>
#include <string>
#include <string_view>

namespace IG
{
class Window;
class ApplicationContext;
class Application;
}

namespace IG::Input
{

class Device;

enum class DefaultKey : uint8_t
{
	CONFIRM,
	CANCEL,
	LEFT,
	RIGHT,
	UP,
	DOWN,
	DIRECTION,
	PAGE_UP,
	PAGE_DOWN,
};

static constexpr bool SWAPPED_CONFIRM_KEYS_DEFAULT = Config::MACHINE_IS_PANDORA ? true : false;

class BaseEvent
{
public:
	constexpr BaseEvent() = default;

	constexpr BaseEvent(Map map, Key button, uint32_t metaState, Action state, Source src, SteadyClockTimePoint time, const Device *device)
		: time_{time}, device_{device}, metaState{metaState}, button{button}, state_{state}, map_{map}, src{src} {}

	Map map() const;
	Action state() const;
	Key mapKey() const;
	bool pushed(Key key = {}) const;
	bool released(Key key = {}) const;
	uint32_t metaKeyBits() const;
	bool isShiftPushed() const;
	SteadyClockTimePoint time() const;
	const Device *device() const;
	void setMap(Map map);
	std::string_view mapName() const;
	static std::string_view mapName(Map map);
	static uint32_t mapNumKeys(Map map);
	static std::string_view actionToStr(Action action);

protected:
	SteadyClockTimePoint time_{};
	const Device *device_{};
	uint32_t metaState{};
	Key button{};
	Action state_{};
	Map map_{};
	Source src{};
};

class KeyEvent : public BaseEvent
{
public:
	constexpr KeyEvent() = default;

	constexpr KeyEvent(Map map, Key button, Key sysKey, Action state, uint32_t metaState, int repeatCount, Source src, SteadyClockTimePoint time, const Device *device)
		: BaseEvent{map, button, metaState, state, src, time, device}, sysKey_{sysKey}, repeatCount{repeatCount} {}

	Key key() const;
	#ifdef CONFIG_BASE_X11
	void setX11RawKey(Key key);
	#endif
	using BaseEvent::pushed;
	using BaseEvent::released;
	bool pushed(DefaultKey) const;
	bool released(DefaultKey) const;
	bool pushedKey(Key sysKey) const;
	bool releasedKey(Key sysKey) const;
	bool isSystemFunction() const;
	bool hasSwappedConfirmKeys() const;
	std::string keyString(ApplicationContext) const;
	int repeated() const;
	void setRepeatCount(int count);
	void setKeyFlags(uint8_t flags);
	bool isGamepad() const;
	bool isKeyboard() const;
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
	bool isDefaultKey(DefaultKey) const;

protected:
	uint8_t keyFlags{};
	Key sysKey_{};
	#ifdef CONFIG_BASE_X11
	Key rawKey{};
	#endif
	int repeatCount{};
};

class MotionEvent : public BaseEvent
{
public:
	constexpr MotionEvent() = default;

	constexpr MotionEvent(Map map, Key button, uint32_t metaState, Action state, int x, int y, PointerId pointerId, Source src, SteadyClockTimePoint time, const Device *device)
		: BaseEvent{map, button, metaState, state, src, time, device}, pointerId_{pointerId}, x{x}, y{y} {}

	WPt pos() const;
	PointerId pointerId() const;
	bool pointerDown(Key btnMask) const;
	int scrolledVertical() const;
	bool canceled() const;
	bool isOff() const;
	bool moved() const;
	bool isAbsolute() const;
	bool isRelative() const;
	bool isTouch() const;
protected:
	PointerId pointerId_{};
	int x{}, y{};
};

using EventVariant = std::variant<MotionEvent, KeyEvent>;

class Event : public EventVariant
{
public:
	using EventVariant::EventVariant;

	constexpr auto motionEvent() { return std::get_if<MotionEvent>(this); }
	constexpr auto motionEvent() const { return std::get_if<MotionEvent>(this); }
	constexpr auto keyEvent() { return std::get_if<KeyEvent>(this); }
	constexpr auto keyEvent() const { return std::get_if<KeyEvent>(this); }
	constexpr auto state() const { return visit([](auto &e){ return e.state(); }, *this); }
	constexpr auto metaKeyBits() const { return visit([](auto &e){ return e.metaKeyBits(); }, *this); }

	SteadyClockTimePoint time() const;
	const Device *device() const;
};

const char *sourceStr(Source);
const char *actionStr(Action);
Map validateMap(uint8_t mapValue);

struct DirectionKeys
{
	Key up{}, right{}, down{}, left{};
};

DirectionKeys directionKeys(Map map = Map::SYSTEM);

}
