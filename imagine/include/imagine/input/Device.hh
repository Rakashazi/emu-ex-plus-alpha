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
#include <imagine/input/config.hh>
#include <imagine/input/inputDefs.hh>
#include <imagine/input/AxisKeyEmu.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/utility.h>
#include <string>
#include <string_view>
#include <span>
#include <memory>

namespace IG
{

class ApplicationContext;

struct InputDeviceTypeFlags
{
	using BitSetClassInt = uint8_t;

	BitSetClassInt
	miscKeys:1{},
	keyboard:1{},
	gamepad:1{},
	joystick:1{},
	virtualInput:1{},
	mouse:1{},
	touchscreen:1{},
	powerButton:1{};

	constexpr bool operator==(InputDeviceTypeFlags const&) const = default;
};

struct InputAxisFlags
{
	using BitSetClassInt = uint32_t;

	BitSetClassInt
	x:1{},  y:1{},  z:1{},
	rx:1{}, ry:1{}, rz:1{},
	hatX:1{}, hatY:1{},
	lTrigger:1{}, rTrigger:1{},
	rudder:1{}, wheel:1{},
	gas:1{}, brake:1{};

	constexpr bool operator==(InputAxisFlags const&) const = default;
};

}

namespace IG::Input
{

class Axis;
using DeviceTypeFlags = InputDeviceTypeFlags;
using AxisFlags = InputAxisFlags;

enum class AxisSetId : uint8_t
{
	stick1,
	stick2,
	hat
};

constexpr std::pair<AxisId, AxisId> toAxisIds(AxisSetId id)
{
	using enum AxisSetId;
	switch(id)
	{
		case stick1: return {AxisId::X, AxisId::Y};
		case stick2: return {AxisId::Z, AxisId::RZ};
		case hat: return {AxisId::HAT0X, AxisId::HAT0Y};
	}
	std::unreachable();
}

struct KeyNameFlags
{
	bool basicModifiers{};
};

constexpr DeviceTypeFlags virtualDeviceFlags{.miscKeys = true, .keyboard = true, .virtualInput = true};

class Device
{
public:
	using Subtype = DeviceSubtype;

	Device() = default;
	Device(int id, Map map, DeviceTypeFlags, std::string name);
	virtual ~Device() = default;

	bool hasKeyboard() const { return typeFlags().keyboard; }
	bool hasGamepad() const { return typeFlags().gamepad; }
	bool hasJoystick() const { return typeFlags().joystick; }
	bool isVirtual() const { return typeFlags().virtualInput; }
	bool hasKeys() const { return hasKeyboard() || hasGamepad() || typeFlags().miscKeys; }
	bool isPowerButton() const { return typeFlags().powerButton; }

	constexpr bool isModifierKey(Key k) const
	{
		if(map() != Map::SYSTEM)
			return false;
		using namespace Keycode;
		switch(k)
		{
			case LALT:
			case RALT:
			case LSHIFT:
			case RSHIFT:
			case LCTRL:
			case RCTRL:
				return true;
		}
		return false;
	}

	constexpr Key swapModifierKey(Key k) const
	{
		if(map() != Map::SYSTEM)
			return false;
		using namespace Keycode;
		switch(k)
		{
			case LALT: return RALT;
			case RALT: return LALT;
			case LSHIFT: return RSHIFT;
			case RSHIFT: return LSHIFT;
			case LCTRL: return RCTRL;
			case RCTRL: return LCTRL;
		}
		return k;
	}

	int id() const { return id_; }
	uint8_t enumId() const { return enumId_; }
	void setEnumId(uint8_t id) { enumId_ = id; }
	std::string_view name() const { return name_; }
	Map map() const;
	DeviceTypeFlags typeFlags() const { return iCadeMode() ? DeviceTypeFlags{.gamepad = true} : typeFlags_; }
	Subtype subtype() const { return subtype_; }
	void setSubtype(Subtype s) { subtype_ = s; }
	bool operator==(Device const&) const = default;
	virtual void setICadeMode(bool on);
	[[nodiscard]]
	virtual bool iCadeMode() const;
	void setJoystickAxesAsDpad(AxisSetId, bool on);
	bool joystickAxesAsDpad(AxisSetId);
	Axis *motionAxis(AxisId);
	virtual std::span<Axis> motionAxes();
	virtual const char *keyName(Key k) const;
	std::string keyString(Key k, KeyNameFlags flags = {}) const;
	static bool anyTypeFlagsPresent(ApplicationContext, DeviceTypeFlags);

	// TODO
	//bool isDisconnectable() { return 0; }
	//void disconnect() { }

	template <class T>
	T &makeAppData(auto &&...args)
	{
		appDataPtr = std::make_shared<T>(IG_forward(args)...);
		return *appData<T>();
	}

	template<class T>
	T *appData() const
	{
		return static_cast<T*>(appDataPtr.get());
	}

protected:
	std::shared_ptr<void> appDataPtr;
	std::string name_;
	int id_{};
	DeviceTypeFlags typeFlags_{};
	uint8_t enumId_{};
	Map map_{Map::UNKNOWN};
	Subtype subtype_{};

	void updateGamepadSubtype(std::string_view name, uint32_t vendorProductId);
};

class Axis
{
public:
	constexpr Axis() = default;
	Axis(const Device &, AxisId id, float scaler = 1.f);
	void setEmulatesDirectionKeys(const Device &, bool);
	bool emulatesDirectionKeys() const;
	constexpr AxisId id() const { return id_; }
	AxisFlags idBit() const;
	bool update(float pos, Map map, SteadyClockTimePoint time, const Device &, Window &, bool normalized = false);

protected:
	float scaler{};
	AxisKeyEmu keyEmu{};
	AxisId id_{};
};

}
