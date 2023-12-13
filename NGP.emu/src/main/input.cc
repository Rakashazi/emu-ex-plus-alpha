/*  This file is part of NGP.emu.

	NGP.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NGP.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NGP.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/keyRemappingUtils.hh>
#include "MainSystem.hh"
#include "MainApp.hh"
#include <ngp/mem.h>

namespace EmuEx
{

const int EmuSystem::maxPlayers = 1;

enum class NgpKey : KeyCode
{
	Up = 1,
	Right = 4,
	Down = 2,
	Left = 3,
	Option = 7,
	A = 5,
	B = 6,
};

constexpr auto dpadKeyInfo = makeArray<KeyInfo>
(
	NgpKey::Up,
	NgpKey::Right,
	NgpKey::Down,
	NgpKey::Left
);

constexpr auto optionKeyInfo = makeArray<KeyInfo>(NgpKey::Option);

constexpr auto faceKeyInfo = makeArray<KeyInfo>
(
	NgpKey::A,
	NgpKey::B
);

constexpr auto turboFaceKeyInfo = turbo(faceKeyInfo);

constexpr auto gpKeyInfo = concatToArrayNow<dpadKeyInfo, optionKeyInfo, faceKeyInfo, turboFaceKeyInfo>;

std::span<const KeyCategory> NgpApp::keyCategories()
{
	static constexpr std::array categories
	{
		KeyCategory{"Gamepad", gpKeyInfo},
	};
	return categories;
}

std::string_view NgpApp::systemKeyCodeToString(KeyCode c)
{
	switch(NgpKey(c))
	{
		case NgpKey::Up: return "Up";
		case NgpKey::Right: return "Right";
		case NgpKey::Down: return "Down";
		case NgpKey::Left: return "Left";
		case NgpKey::Option: return "Option";
		case NgpKey::A: return "A";
		case NgpKey::B: return "B";
		default: return "";
	}
}

std::span<const KeyConfigDesc> NgpApp::defaultKeyConfigs()
{
	using namespace IG::Input;

	static constexpr std::array pcKeyboardMap
	{
		KeyMapping{NgpKey::Up, Keycode::UP},
		KeyMapping{NgpKey::Right, Keycode::RIGHT},
		KeyMapping{NgpKey::Down, Keycode::DOWN},
		KeyMapping{NgpKey::Left, Keycode::LEFT},
		KeyMapping{NgpKey::Option, Keycode::ENTER},
		KeyMapping{NgpKey::B, Keycode::Z},
		KeyMapping{NgpKey::A, Keycode::X},
	};

	static constexpr std::array genericGamepadMap
	{
		KeyMapping{NgpKey::Up, Keycode::UP},
		KeyMapping{NgpKey::Right, Keycode::RIGHT},
		KeyMapping{NgpKey::Down, Keycode::DOWN},
		KeyMapping{NgpKey::Left, Keycode::LEFT},
		KeyMapping{NgpKey::Option, Keycode::GAME_START},
		KeyMapping{NgpKey::B, Keycode::GAME_X},
		KeyMapping{NgpKey::A, Keycode::GAME_A},
	};

	static constexpr std::array wiimoteMap
	{
		KeyMapping{NgpKey::Up, WiimoteKey::UP},
		KeyMapping{NgpKey::Right, WiimoteKey::RIGHT},
		KeyMapping{NgpKey::Down, WiimoteKey::DOWN},
		KeyMapping{NgpKey::Left, WiimoteKey::LEFT},
		KeyMapping{NgpKey::B, WiimoteKey::_1},
		KeyMapping{NgpKey::A, WiimoteKey::_2},
		KeyMapping{NgpKey::Option, WiimoteKey::PLUS},
	};

	return genericKeyConfigs<pcKeyboardMap, genericGamepadMap, wiimoteMap>();
}

bool NgpApp::allowsTurboModifier(KeyCode c)
{
	switch(NgpKey(c))
	{
		case NgpKey::A ... NgpKey::B:
			return true;
		default: return false;
	}
}

constexpr FRect gpImageCoords(IRect cellRelBounds)
{
	constexpr F2Size imageSize{256, 128};
	constexpr int cellSize = 32;
	return (cellRelBounds.relToAbs() * cellSize).as<float>() / imageSize;
}

AssetDesc NgpApp::vControllerAssetDesc(KeyInfo key) const
{
	static constexpr struct VirtualControllerAssets
	{
		AssetDesc dpad{AssetFileID::gamepadOverlay, gpImageCoords({{}, {4, 4}})},

		a{AssetFileID::gamepadOverlay,      gpImageCoords({{4, 0}, {2, 2}})},
		b{AssetFileID::gamepadOverlay,      gpImageCoords({{6, 0}, {2, 2}})},
		option{AssetFileID::gamepadOverlay, gpImageCoords({{4, 2}, {2, 1}}), {1, 2}},

		blank{AssetFileID::gamepadOverlay, gpImageCoords({{6, 2}, {2, 2}})};
	} virtualControllerAssets;

	if(key[0] == 0)
		return virtualControllerAssets.dpad;
	switch(NgpKey(key[0]))
	{
		case NgpKey::A: return virtualControllerAssets.a;
		case NgpKey::B: return virtualControllerAssets.b;
		case NgpKey::Option: return virtualControllerAssets.option;
		default: return virtualControllerAssets.blank;
	}
}

void NgpSystem::handleInputAction(EmuApp *, InputAction a)
{
	inputBuff = setOrClearBits(inputBuff, bit(a.code - 1), a.state == Input::Action::PUSHED);
	MDFN_IEN_NGP::storeB(0x6F82, inputBuff);
}

void NgpSystem::clearInputBuffers(EmuInputView &)
{
	inputBuff = {};
	MDFN_IEN_NGP::storeB(0x6F82, 0);
}

SystemInputDeviceDesc NgpSystem::inputDeviceDesc(int idx) const
{
	static constexpr std::array gamepadComponents
	{
		InputComponentDesc{"D-Pad", dpadKeyInfo, InputComponent::dPad, LB2DO},
		InputComponentDesc{"Face Buttons", faceKeyInfo, InputComponent::button, RB2DO},
		InputComponentDesc{"Option", optionKeyInfo, InputComponent::button, RB2DO},
	};

	static constexpr SystemInputDeviceDesc gamepadDesc{"Gamepad", gamepadComponents};

	return gamepadDesc;
}

}
