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

	constexpr BaseEvent(Map map, Key key, uint32_t metaState, Action state, Source src, SteadyClockTimePoint time, const Device *device)
		: time_{time}, device_{device}, metaState{metaState}, key_{key}, state_{state}, map_{map}, src{src} {}

	Map map() const;
	Action state() const;
	Key key() const;
	bool pushed() const;
	bool released() const;
	bool pushed(Key key) const;
	bool released(Key key) const;
	uint32_t metaKeyBits() const;
	bool isShiftPushed() const;
	SteadyClockTimePoint time() const;
	const Device *device() const;
	void setMap(Map map);
	std::string_view mapName() const;
	static std::string_view mapName(Map map);
	static size_t mapNumKeys(Map map);
	static std::string_view toString(Action action);

protected:
	SteadyClockTimePoint time_{};
	const Device *device_{};
	uint32_t metaState{};
	Key key_{};
	Action state_{};
	Map map_{};
	Source src{};
};

class KeyEvent : public BaseEvent
{
public:
	constexpr KeyEvent() = default;

	constexpr KeyEvent(Map map, Key key, Action state, uint32_t metaState, int repeatCount, Source src, SteadyClockTimePoint time, const Device *device)
		: BaseEvent{map, key, metaState, state, src, time, device}, repeatCount{repeatCount} {}

	#ifdef CONFIG_PACKAGE_X11
	void setX11RawKey(Key key);
	#endif
	using BaseEvent::pushed;
	using BaseEvent::released;
	bool pushed(DefaultKey) const;
	bool released(DefaultKey) const;
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
	#ifdef CONFIG_PACKAGE_X11
	Key rawKey{};
	#endif
	int repeatCount{};
};

class MotionEvent : public BaseEvent
{
public:
	constexpr MotionEvent() = default;

	constexpr MotionEvent(Map map, Key key, uint32_t metaState, Action state, float x, float y, PointerId pointerId, Source src, SteadyClockTimePoint time, const Device *device)
		: BaseEvent{map, key, metaState, state, src, time, device}, pointerId_{pointerId}, x{x}, y{y} {}

	WPt pos() const { return {int(x), int(y)}; }
	F2Pt posF() const { return {x, y}; }
	PointerId pointerId() const;
	bool pointerDown(Key btnMask) const;
	int scrolledVertical() const;
	bool canceled() const;
	bool isOff() const;
	bool moved() const;
	bool isPointer() const;
	bool isJoystick() const;
	bool isRelative() const;
	bool isTouch() const;
protected:
	PointerId pointerId_{};
	float x{}, y{};
};

using EventVariant = std::variant<MotionEvent, KeyEvent>;

class Event : public EventVariant, public AddVisit
{
public:
	using EventVariant::EventVariant;
	using AddVisit::visit;

	constexpr auto motionEvent(this auto&& self) { return std::get_if<MotionEvent>(&self); }
	constexpr auto keyEvent(this auto&& self) { return std::get_if<KeyEvent>(&self); }
	constexpr auto state() const { return visit([](auto &e){ return e.state(); }); }
	constexpr auto metaKeyBits() const { return visit([](auto &e){ return e.metaKeyBits(); }); }

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

DirectionKeys directionKeys();

}
