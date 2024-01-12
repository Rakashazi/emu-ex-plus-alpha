#pragma once

/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/AppKeyCode.hh>
#include <imagine/input/inputDefs.hh>
#include <imagine/input/bluetoothInputDefs.hh>
#include <ranges>

namespace EmuEx
{

using namespace IG;

#ifdef CONFIG_INPUT_GAMEPAD_DEVICES
constexpr Input::Key genericGamepadKeycodeToPS3HID(Input::Key k)
{
	using namespace Input;
	switch(k)
	{
		case Keycode::GAME_A: return Keycode::PS3::CROSS;
		case Keycode::GAME_B: return Keycode::PS3::CIRCLE;
		case Keycode::GAME_X: return Keycode::PS3::SQUARE;
		case Keycode::GAME_Y: return Keycode::PS3::TRIANGLE;
		default: return k;
	}
}

constexpr Input::Key genericGamepadKeycodeToOuya(Input::Key k)
{
	using namespace Input;
	switch(k)
	{
		case Keycode::GAME_SELECT: return Keycode::Ouya::L3;
		case Keycode::GAME_START:  return Keycode::Ouya::R3;
		default: return k;
	}
}
#endif

#if defined(__ANDROID__) && __ARM_ARCH == 7
constexpr Input::Key genericGamepadKeycodeToXperiaPlay(Input::Key k)
{
	using namespace Input;
	switch(k)
	{
		case Keycode::GAME_A: return Keycode::XperiaPlay::CROSS;
		case Keycode::GAME_B: return Keycode::XperiaPlay::CIRCLE;
		case Keycode::GAME_X: return Keycode::XperiaPlay::SQUARE;
		case Keycode::GAME_Y: return Keycode::XperiaPlay::TRIANGLE;
		default: return k;
	}
}
#endif

#ifdef CONFIG_MACHINE_PANDORA
constexpr Input::Key genericGamepadKeycodeToPandora(Input::Key k)
{
	using namespace Input;
	switch(k)
	{
		case Keycode::GAME_A: return Keycode::Pandora::X;
		case Keycode::GAME_B: return Keycode::Pandora::B;
		case Keycode::GAME_X: return Keycode::Pandora::A;
		case Keycode::GAME_Y: return Keycode::Pandora::Y;
		case Keycode::GAME_SELECT: return Keycode::Pandora::SELECT;
		case Keycode::GAME_START: return Keycode::Pandora::START;
		case Keycode::GAME_L1: return Keycode::Pandora::L;
		case Keycode::GAME_R1: return Keycode::Pandora::R;
		default: return k;
	}
}
#endif

#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
constexpr Input::Key genericGamepadKeycodeToAppleGamepad(Input::Key k)
{
	using namespace Input;
	switch(k)
	{
		case Keycode::GAME_SELECT: return AppleGC::RSTICK_LEFT;
		case Keycode::GAME_START: return AppleGC::RSTICK_RIGHT;
		default: return k;
	}
}
#endif

constexpr auto transformMappedKeys(auto map, auto &&func)
{
	for(auto &[code, keys] : map)
		keys[0] = func(keys[0]);
	return map;
}

constexpr auto transpose(std::ranges::range auto keyInfo, uint8_t deviceId)
{
	for(auto &k : keyInfo)
		k.flags.deviceId = deviceId;
	return keyInfo;
}

constexpr auto transpose(KeyInfo keyInfo, uint8_t deviceId)
{
	keyInfo.flags.deviceId = deviceId;
	return keyInfo;
}

constexpr auto turbo(std::ranges::range auto keyInfo)
{
	for(auto &k : keyInfo)
		k.flags.turbo = 1;
	return keyInfo;
}

template<const auto &pcKeyboardBaseMap, const auto &genericGamepadBaseMap, const auto &wiimoteBaseMap,
	const auto &gamepadAppKeyCodeMap = genericGamepadAppKeyCodeMap>
constexpr std::span<const KeyConfigDesc> genericKeyConfigs()
{
	using namespace IG::Input;

	static constexpr auto pcKeyboardMap = concatToArrayNow<genericKeyboardAppKeyCodeMap, pcKeyboardBaseMap>;

	static constexpr auto genericGamepadMap = concatToArrayNow<gamepadAppKeyCodeMap, genericGamepadBaseMap>;
	#ifdef CONFIG_INPUT_GAMEPAD_DEVICES
	static constexpr auto ps3GamepadMap = transformMappedKeys(genericGamepadMap, genericGamepadKeycodeToPS3HID);
	static constexpr auto ouyaGamepadMap = transformMappedKeys(genericGamepadMap, genericGamepadKeycodeToOuya);
	#endif

	#if defined(__ANDROID__) && __ARM_ARCH == 7
	static constexpr std::array xperiaPlayGamepadMap = transformMappedKeys(genericGamepadMap, genericGamepadKeycodeToXperiaPlay);
	#endif

	#ifdef CONFIG_MACHINE_PANDORA
	static constexpr std::array pandoraKeysMap = transformMappedKeys(genericGamepadMap, genericGamepadKeycodeToPandora);
	#endif

	#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
	static constexpr std::array appleGamepadMap = transformMappedKeys(genericGamepadMap, genericGamepadKeycodeToAppleGamepad);
	#endif

	static constexpr std::array wiimoteMap = concatToArrayNow<genericWiimoteAppKeyCodeMap, wiimoteBaseMap>;

	static constexpr std::array configs
	{
		KeyConfigDesc{Map::SYSTEM, "PC Keyboard", pcKeyboardMap},
		#ifdef CONFIG_INPUT_GAMEPAD_DEVICES
		KeyConfigDesc{Map::SYSTEM, DeviceSubtype::GENERIC_GAMEPAD, "Generic Gamepad", genericGamepadMap},
		KeyConfigDesc{Map::SYSTEM, DeviceSubtype::PS3_CONTROLLER, "PS3 Controller", ps3GamepadMap},
		KeyConfigDesc{Map::SYSTEM, DeviceSubtype::OUYA_CONTROLLER, "OUYA Controller", ouyaGamepadMap},
		#endif
		#if defined(__ANDROID__) && __ARM_ARCH == 7
		KeyConfigDesc{Map::SYSTEM, DeviceSubtype::XPERIA_PLAY, "Xperia Play", xperiaPlayGamepadMap},
		#endif
		#ifdef CONFIG_MACHINE_PANDORA
		KeyConfigDesc{Map::SYSTEM, DeviceSubtype::PANDORA_HANDHELD, "Pandora Keys", pandoraKeysMap},
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		KeyConfigDesc{Map::APPLE_GAME_CONTROLLER, "Default", appleGamepadMap},
		#endif
		#ifdef CONFIG_INPUT_BLUETOOTH
		KeyConfigDesc{Map::WIIMOTE, "Default", wiimoteMap},
		KeyConfigDesc{Map::WII_CC, "Default", genericGamepadMap},
		KeyConfigDesc{Map::ICONTROLPAD, "Default", genericGamepadMap},
		KeyConfigDesc{Map::ZEEMOTE, "Default", wiimoteMap},
		#endif
		#ifdef CONFIG_BLUETOOTH_SERVER
		KeyConfigDesc{Map::PS3PAD, "Default", genericGamepadMap},
		#endif
	};
	return configs;
}

}
