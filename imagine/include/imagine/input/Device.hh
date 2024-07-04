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

#include <imagine/input/inputDefs.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/utility.h>
#include <imagine/util/variant.hh>
#include <string>
#include <string_view>
#include <span>

#ifdef CONFIG_PACKAGE_X11
#include <imagine/base/x11/XInputDevice.hh>
#include <imagine/input/evdev/EvdevInputDevice.hh>
#elif defined __ANDROID__
#include <imagine/base/android/AndroidInputDevice.hh>
#elif defined CONFIG_OS_IOS
#include <imagine/input/apple/KeyboardDevice.hh>
#endif
#ifdef CONFIG_INPUT_BLUETOOTH
#include <imagine/bluetooth/IControlPad.hh>
#include <imagine/bluetooth/Wiimote.hh>
#include <imagine/bluetooth/Zeemote.hh>
#endif
#ifdef CONFIG_BLUETOOTH_SERVER
#include <imagine/bluetooth/PS3Controller.hh>
#endif
#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
#include <imagine/input/apple/AppleGameDevice.hh>
#endif

namespace IG
{
class ApplicationContext;
}

namespace IG::Input
{

class Axis;

struct KeyNameFlags
{
	bool basicModifiers{};
};

class NullDevice : public BaseDevice {};

using DeviceVariant = std::variant<
#ifdef CONFIG_PACKAGE_X11
XInputDevice,
EvdevInputDevice,
#elif defined __ANDROID__
AndroidInputDevice,
#elif defined CONFIG_OS_IOS
KeyboardDevice,
#endif
#ifdef CONFIG_INPUT_BLUETOOTH
IG::IControlPad,
IG::Wiimote,
IG::WiimoteExtDevice,
IG::Zeemote,
#endif
#ifdef CONFIG_BLUETOOTH_SERVER
IG::PS3Controller,
#endif
#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
AppleGameDevice,
#endif
NullDevice
>;

class Device : public DeviceVariant, public AddVisit
{
public:
	using Subtype = DeviceSubtype;
	using DeviceVariant::DeviceVariant;
	using AddVisit::visit;

	bool hasKeyboard() const { return typeFlags().keyboard; }
	bool hasGamepad() const { return typeFlags().gamepad; }
	bool hasJoystick() const { return typeFlags().joystick; }
	bool isVirtual() const { return typeFlags().virtualInput; }
	bool hasKeys() const { return hasKeyboard() || hasGamepad() || typeFlags().miscKeys; }
	bool isPowerButton() const { return typeFlags().powerButton; }

	constexpr bool isModifierKey(Key k) const
	{
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

	int id() const { return visit([](auto &d){ return d.id_; }); }
	uint8_t enumId() const { return visit([](auto &d){ return d.enumId_; }); }
	void setEnumId(uint8_t id) { visit([&](auto &d){ d.enumId_ = id; }); }
	std::string_view name() const { return visit([](auto &d){ return std::string_view{d.name_}; }); }
	std::string displayName() const;
	static std::string makeDisplayName(std::string_view name, int id);
	Map map() const;
	DeviceTypeFlags typeFlags() const
	{
		return visit([&](auto &d)
		{
			auto flags = d.typeFlags_;
			if(iCadeMode())
				flags.gamepad = true;
			return flags;
		});
	}
	Subtype subtype() const { return visit([](auto &d){ return d.subtype_; }); }
	void setSubtype(Subtype s) { visit([&](auto &d){ d.subtype_ = s; }); }
	bool operator==(Device const&) const = default;
	void setJoystickAxesAsKeys(AxisSetId, bool on);
	bool joystickAxesAsKeys(AxisSetId);
	Axis *motionAxis(AxisId);
	std::string keyString(Key k, KeyNameFlags flags = {}) const;
	static bool anyTypeFlagsPresent(ApplicationContext, DeviceTypeFlags);

	// TODO
	//bool isDisconnectable() { return 0; }
	//void disconnect() { }

	template <class T>
	T &makeAppData(auto &&...args)
	{
		auto &appDataPtr = visit([&](auto &d) -> auto& { return d.appDataPtr; });
		appDataPtr = std::make_shared<T>(IG_forward(args)...);
		return *appData<T>();
	}

	template<class T>
	T *appData() const
	{
		return visit([&](auto &d){ return static_cast<T*>(d.appDataPtr.get()); });
	}

	// optional API
	std::span<Axis> motionAxes()
	{
		return visit([&](auto &d)
		{
			if constexpr(requires {d.motionAxes();})
				return d.motionAxes();
			else
				return std::span<Axis>{};
		});
	}
	const char *keyName(Key k) const;
	void setICadeMode(bool on);
	[[nodiscard]]
	bool iCadeMode() const;
};

}
