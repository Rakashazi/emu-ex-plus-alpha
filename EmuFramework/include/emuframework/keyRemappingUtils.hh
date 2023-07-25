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

constexpr Input::Key genericGamepadKeycodeToBitdo(Input::Key k)
{
	using namespace Input;
	switch(k)
	{
		case Keycode::GAME_A: return Keycode::GAME_B;
		case Keycode::GAME_B: return Keycode::GAME_A;
		case Keycode::GAME_X: return Keycode::GAME_Y;
		case Keycode::GAME_Y: return Keycode::GAME_X;
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
		case Keycode::UP: return AppleGC::UP;
		case Keycode::RIGHT: return AppleGC::RIGHT;
		case Keycode::DOWN: return AppleGC::DOWN;
		case Keycode::LEFT: return AppleGC::LEFT;
		case Keycode::GAME_A: return AppleGC::A;
		case Keycode::GAME_B: return AppleGC::B;
		case Keycode::GAME_X: return AppleGC::X;
		case Keycode::GAME_Y: return AppleGC::Y;
		case Keycode::GAME_SELECT: return AppleGC::RSTICK_LEFT;
		case Keycode::GAME_START: return AppleGC::RSTICK_RIGHT;
		case Keycode::GAME_L1: return AppleGC::L1;
		case Keycode::GAME_L2: return AppleGC::L2;
		case Keycode::GAME_R1: return AppleGC::R1;
		case Keycode::GAME_R2: return AppleGC::R2;
		default: return 0;
	}
}
#endif

constexpr Input::Key wiimoteKeycodeToZeemote(Input::Key k)
{
	using namespace Input;
	switch(k)
	{
		case Wiimote::UP: return Zeemote::UP;
		case Wiimote::RIGHT: return Zeemote::RIGHT;
		case Wiimote::DOWN: return Zeemote::DOWN;
		case Wiimote::LEFT: return Zeemote::LEFT;
		case Wiimote::_1: return Zeemote::A;
		case Wiimote::_2: return Zeemote::B;
		case Wiimote::A: return Zeemote::C;
		case Wiimote::B: return Zeemote::POWER;
		default: return 0;
	}
}

constexpr Input::Key genericGamepadKeycodeToWiiCC(Input::Key k)
{
	using namespace Input;
	switch(k)
	{
		case Keycode::UP: return WiiCC::UP;
		case Keycode::RIGHT: return WiiCC::RIGHT;
		case Keycode::DOWN: return WiiCC::DOWN;
		case Keycode::LEFT: return WiiCC::LEFT;
		case Keycode::GAME_A: return WiiCC::B;
		case Keycode::GAME_B: return WiiCC::A;
		case Keycode::GAME_X: return WiiCC::Y;
		case Keycode::GAME_Y: return WiiCC::X;
		case Keycode::GAME_SELECT: return WiiCC::MINUS;
		case Keycode::GAME_START: return WiiCC::PLUS;
		case Keycode::GAME_L1: return WiiCC::L;
		case Keycode::GAME_L2: return WiiCC::ZL;
		case Keycode::GAME_R1: return WiiCC::R;
		case Keycode::GAME_R2: return WiiCC::ZR;
		case Keycode::MENU: return WiiCC::HOME;
		default: return 0;
	}
}

constexpr Input::Key genericGamepadKeycodeToICP(Input::Key k)
{
	using namespace Input;
	switch(k)
	{
		case Keycode::UP: return iControlPad::UP;
		case Keycode::RIGHT: return iControlPad::RIGHT;
		case Keycode::DOWN: return iControlPad::DOWN;
		case Keycode::LEFT: return iControlPad::LEFT;
		case Keycode::GAME_A: return iControlPad::X;
		case Keycode::GAME_B: return iControlPad::B;
		case Keycode::GAME_X: return iControlPad::A;
		case Keycode::GAME_Y: return iControlPad::Y;
		case Keycode::GAME_SELECT: return iControlPad::SELECT;
		case Keycode::GAME_START: return iControlPad::START;
		case Keycode::GAME_L1: return iControlPad::L;
		case Keycode::GAME_R1: return iControlPad::R;
		default: return 0;
	}
}

constexpr Input::Key genericGamepadKeycodeToICade(Input::Key k)
{
	using namespace Input;
	switch(k)
	{
		case Keycode::UP: return ICade::UP;
		case Keycode::RIGHT: return ICade::RIGHT;
		case Keycode::DOWN: return ICade::DOWN;
		case Keycode::LEFT: return ICade::LEFT;
		case Keycode::GAME_A: return ICade::A;
		case Keycode::GAME_B: return ICade::B;
		case Keycode::GAME_X: return ICade::X;
		case Keycode::GAME_Y: return ICade::Y;
		case Keycode::GAME_SELECT: return ICade::SELECT;
		case Keycode::GAME_START: return ICade::START;
		case Keycode::GAME_L1: return ICade::Z;
		case Keycode::GAME_R1: return ICade::C;
		default: return 0;
	}
}

constexpr Input::Key genericGamepadKeycodeToPS3(Input::Key k)
{
	using namespace Input;
	switch(k)
	{
		case Keycode::UP: return PS3::UP;
		case Keycode::RIGHT: return PS3::RIGHT;
		case Keycode::DOWN: return PS3::DOWN;
		case Keycode::LEFT: return PS3::LEFT;
		case Keycode::GAME_A: return PS3::CROSS;
		case Keycode::GAME_B: return PS3::CIRCLE;
		case Keycode::GAME_X: return PS3::SQUARE;
		case Keycode::GAME_Y: return PS3::TRIANGLE;
		case Keycode::GAME_SELECT: return PS3::SELECT;
		case Keycode::GAME_START: return PS3::START;
		case Keycode::GAME_L1: return PS3::L1;
		case Keycode::GAME_L2: return PS3::L2;
		case Keycode::GAME_R1: return PS3::R1;
		case Keycode::GAME_R2: return PS3::R2;
		case Keycode::MENU: return PS3::PS;
		default: return 0;
	}
}

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

template<const auto &pcKeyboardBaseMap, const auto &genericGamepadBaseMap, const auto &wiimoteBaseMap>
constexpr std::span<const KeyConfigDesc> genericKeyConfigs()
{
	using namespace IG::Input;

	static constexpr auto pcKeyboardMap = concatToArrayNow<genericKeyboardAppKeyCodeMap, pcKeyboardBaseMap>;

	#ifdef CONFIG_INPUT_GAMEPAD_DEVICES
	static constexpr auto genericGamepadMap = concatToArrayNow<genericGamepadAppKeyCodeMap, genericGamepadBaseMap>;
	static constexpr auto ps3GamepadMap = transformMappedKeys(genericGamepadMap, genericGamepadKeycodeToPS3HID);
	static constexpr auto ouyaGamepadMap = transformMappedKeys(genericGamepadMap, genericGamepadKeycodeToOuya);
	static constexpr auto eightBitdoGamepadMap = transformMappedKeys(genericGamepadMap, genericGamepadKeycodeToBitdo);
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
	static constexpr std::array wiiCCMap = transformMappedKeys(genericGamepadMap, genericGamepadKeycodeToWiiCC);
	static constexpr std::array icpMap = transformMappedKeys(genericGamepadMap, genericGamepadKeycodeToICP);
	static constexpr std::array iCadeMap = transformMappedKeys(genericGamepadMap, genericGamepadKeycodeToICade);
	static constexpr std::array ps3Map = transformMappedKeys(genericGamepadMap, genericGamepadKeycodeToPS3);
	static constexpr std::array zeemoteMap = transformMappedKeys(wiimoteBaseMap, wiimoteKeycodeToZeemote);

	static constexpr std::array configs
	{
		KeyConfigDesc{Map::SYSTEM, "PC Keyboard", pcKeyboardMap},
		#ifdef CONFIG_INPUT_GAMEPAD_DEVICES
		KeyConfigDesc{Map::SYSTEM, DeviceSubtype::GENERIC_GAMEPAD, "Generic Gamepad", genericGamepadMap},
		KeyConfigDesc{Map::SYSTEM, DeviceSubtype::PS3_CONTROLLER, "PS3 Controller", ps3GamepadMap},
		KeyConfigDesc{Map::SYSTEM, DeviceSubtype::OUYA_CONTROLLER, "OUYA Controller", ouyaGamepadMap},
		KeyConfigDesc{Map::SYSTEM, DeviceSubtype::_8BITDO_SF30_PRO, "8Bitdo SF30 Pro", eightBitdoGamepadMap},
		KeyConfigDesc{Map::SYSTEM, DeviceSubtype::_8BITDO_SN30_PRO_PLUS, "8BitDo SN30 Pro+", eightBitdoGamepadMap},
		#endif
		#if defined(__ANDROID__) && __ARM_ARCH == 7
		KeyConfigDesc{Map::SYSTEM, DeviceSubtype::XPERIA_PLAY,	"Xperia Play", xperiaPlayGamepadMap},
		#endif
		#ifdef CONFIG_MACHINE_PANDORA
		KeyConfigDesc{Map::SYSTEM, DeviceSubtype::PANDORA_HANDHELD, "Pandora Keys", pandoraKeysMap},
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		KeyConfigDesc{Map::APPLE_GAME_CONTROLLER,"Default", appleGamepadMap},
		#endif
		#ifdef CONFIG_INPUT_BLUETOOTH
		KeyConfigDesc{Map::WIIMOTE, "Default", wiimoteMap},
		KeyConfigDesc{Map::WII_CC, "Default", wiiCCMap},
		KeyConfigDesc{Map::ICONTROLPAD, "Default", icpMap},
		KeyConfigDesc{Map::ZEEMOTE, "Default", zeemoteMap},
		#endif
		KeyConfigDesc{Map::ICADE, "Default", iCadeMap},
		#ifdef CONFIG_BLUETOOTH_SERVER
		KeyConfigDesc{Map::PS3PAD, "Default", ps3Map},
		#endif
	};
	return configs;
}

}
