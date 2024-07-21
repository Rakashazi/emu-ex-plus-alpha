/*  This file is part of PCE.emu.

	PCE.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	PCE.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with PCE.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/keyRemappingUtils.hh>
#include "MainSystem.hh"
#include "MainApp.hh"

namespace EmuEx
{

const int EmuSystem::maxPlayers = 5;

enum class PceKey : KeyCode
{
	Up = 5,
	Right = 6,
	Down = 7,
	Left = 8,
	Select = 3,
	Run = 4,
	I = 1,
	II = 2,
	III = 9,
	IV = 10,
	V = 11,
	VI = 12
};

constexpr auto dpadKeyInfo = makeArray<KeyInfo>
(
	PceKey::Up,
	PceKey::Right,
	PceKey::Down,
	PceKey::Left
);

constexpr auto centerKeyInfo = makeArray<KeyInfo>
(
	PceKey::Select,
	PceKey::Run
);

constexpr auto faceKeyInfo = makeArray<KeyInfo>
(
	PceKey::III,
	PceKey::II,
	PceKey::I,
	PceKey::IV,
	PceKey::V,
	PceKey::VI
);

constexpr auto turboFaceKeyInfo = turbo(faceKeyInfo);

constexpr auto gpKeyInfo = concatToArrayNow<dpadKeyInfo, centerKeyInfo, faceKeyInfo, turboFaceKeyInfo>;
constexpr auto gp2KeyInfo = transpose(gpKeyInfo, 1);
constexpr auto gp3KeyInfo = transpose(gpKeyInfo, 2);
constexpr auto gp4KeyInfo = transpose(gpKeyInfo, 3);
constexpr auto gp5KeyInfo = transpose(gpKeyInfo, 4);

std::span<const KeyCategory> PceApp::keyCategories()
{
	static constexpr std::array categories
	{
		KeyCategory{"Gamepad", gpKeyInfo},
		KeyCategory{"Gamepad 2", gp2KeyInfo, 1},
		KeyCategory{"Gamepad 3", gp3KeyInfo, 2},
		KeyCategory{"Gamepad 4", gp4KeyInfo, 3},
		KeyCategory{"Gamepad 5", gp5KeyInfo, 4},
	};
	return categories;
}

std::string_view PceApp::systemKeyCodeToString(KeyCode c)
{
	switch(PceKey(c))
	{
		case PceKey::Up: return "Up";
		case PceKey::Right: return "Right";
		case PceKey::Down: return "Down";
		case PceKey::Left: return "Left";
		case PceKey::Select: return "Select";
		case PceKey::Run: return "Run";
		case PceKey::I: return "I";
		case PceKey::II: return "II";
		case PceKey::III: return "III";
		case PceKey::IV: return "IV";
		case PceKey::V: return "V";
		case PceKey::VI: return "VI";
		default: return "";
	}
}

std::span<const KeyConfigDesc> PceApp::defaultKeyConfigs()
{
	using namespace IG::Input;

	static constexpr std::array pcKeyboardMap
	{
		KeyMapping{PceKey::Up, Keycode::UP},
		KeyMapping{PceKey::Right, Keycode::RIGHT},
		KeyMapping{PceKey::Down, Keycode::DOWN},
		KeyMapping{PceKey::Left, Keycode::LEFT},
		KeyMapping{PceKey::Select, Keycode::SPACE},
		KeyMapping{PceKey::Run, Keycode::ENTER},
		KeyMapping{PceKey::I, Keycode::Z},
		KeyMapping{PceKey::II, Keycode::X},
		KeyMapping{PceKey::III, Keycode::C},
		KeyMapping{PceKey::IV, Keycode::A},
		KeyMapping{PceKey::V, Keycode::S},
		KeyMapping{PceKey::VI, Keycode::D},
	};

	static constexpr std::array genericGamepadMap
	{
		KeyMapping{PceKey::Up, Keycode::UP},
		KeyMapping{PceKey::Right, Keycode::RIGHT},
		KeyMapping{PceKey::Down, Keycode::DOWN},
		KeyMapping{PceKey::Left, Keycode::LEFT},
		KeyMapping{PceKey::Select, Keycode::GAME_SELECT},
		KeyMapping{PceKey::Run, Keycode::GAME_START},
		KeyMapping{PceKey::I, Keycode::GAME_X},
		KeyMapping{PceKey::II, Keycode::GAME_A},
		KeyMapping{PceKey::III, Keycode::GAME_B},
		KeyMapping{PceKey::IV, Keycode::GAME_L1},
		KeyMapping{PceKey::V, Keycode::GAME_Y},
		KeyMapping{PceKey::VI, Keycode::GAME_R1},
	};

	static constexpr std::array wiimoteMap
	{
		KeyMapping{PceKey::Up, WiimoteKey::UP},
		KeyMapping{PceKey::Right, WiimoteKey::RIGHT},
		KeyMapping{PceKey::Down, WiimoteKey::DOWN},
		KeyMapping{PceKey::Left, WiimoteKey::LEFT},
		KeyMapping{PceKey::I, WiimoteKey::_1},
		KeyMapping{PceKey::II, WiimoteKey::_2},
		KeyMapping{PceKey::Select, WiimoteKey::MINUS},
		KeyMapping{PceKey::Run, WiimoteKey::PLUS},
	};

	return genericKeyConfigs<pcKeyboardMap, genericGamepadMap, wiimoteMap>();
}

bool PceApp::allowsTurboModifier(KeyCode c)
{
	switch(PceKey(c))
	{
		case PceKey::I ... PceKey::II:
		case PceKey::III ... PceKey::VI:
			return true;
		default: return false;
	}
}

constexpr FRect gpImageCoords(IRect cellRelBounds)
{
	constexpr F2Size imageSize{256, 256};
	constexpr int cellSize = 32;
	return (cellRelBounds.relToAbs() * cellSize).as<float>() / imageSize;
}

AssetDesc PceApp::vControllerAssetDesc(KeyInfo key) const
{
	constexpr struct VirtualControllerAssets
	{
		AssetDesc dpad{AssetFileID::gamepadOverlay, gpImageCoords({{}, {4, 4}})},

		i{AssetFileID::gamepadOverlay,      gpImageCoords({{4, 0}, {2, 2}})},
		ii{AssetFileID::gamepadOverlay,     gpImageCoords({{6, 0}, {2, 2}})},
		iii{AssetFileID::gamepadOverlay,    gpImageCoords({{4, 2}, {2, 2}})},
		iv{AssetFileID::gamepadOverlay,     gpImageCoords({{6, 2}, {2, 2}})},
		v{AssetFileID::gamepadOverlay,      gpImageCoords({{0, 4}, {2, 2}})},
		vi{AssetFileID::gamepadOverlay,     gpImageCoords({{2, 4}, {2, 2}})},
		select{AssetFileID::gamepadOverlay, gpImageCoords({{0, 6}, {2, 1}}), {1, 2}},
		run{AssetFileID::gamepadOverlay,    gpImageCoords({{0, 7}, {2, 1}}), {1, 2}},

		blank{AssetFileID::gamepadOverlay, gpImageCoords({{4, 4}, {2, 2}})};
	} virtualControllerAssets;

	if(key[0] == 0)
		return virtualControllerAssets.dpad;
	switch(PceKey(key[0]))
	{
		case PceKey::I: return virtualControllerAssets.i;
		case PceKey::II: return virtualControllerAssets.ii;
		case PceKey::III: return virtualControllerAssets.iii;
		case PceKey::IV: return virtualControllerAssets.iv;
		case PceKey::V: return virtualControllerAssets.v;
		case PceKey::VI: return virtualControllerAssets.vi;
		case PceKey::Select: return virtualControllerAssets.select;
		case PceKey::Run: return virtualControllerAssets.run;
		default: return virtualControllerAssets.blank;
	}
}

void PceSystem::handleInputAction(EmuApp *, InputAction a)
{
	auto player = a.flags.deviceId;
	assumeExpr(player < maxPlayers);
	inputBuff[player] = setOrClearBits(inputBuff[player], bit(a.code - 1), a.state == Input::Action::PUSHED);
}

void PceSystem::clearInputBuffers(EmuInputView &)
{
	inputBuff = {};
	if(option6BtnPad)
	{
		for(auto &padData : std::span{inputBuff.data(), 2})
			padData = IG::bit(12);
	}
}

void set6ButtonPadEnabled(EmuApp &app, bool on)
{
	if(on)
	{
		app.unsetDisabledInputKeys();
	}
	else
	{
		static constexpr std::array extraCodes{KeyCode(PceKey::III), KeyCode(PceKey::IV), KeyCode(PceKey::V), KeyCode(PceKey::VI)};
		app.setDisabledInputKeys(extraCodes);
	}
}

SystemInputDeviceDesc PceSystem::inputDeviceDesc(int idx) const
{
	static constexpr std::array gamepadComponents
	{
		InputComponentDesc{"D-Pad", dpadKeyInfo, InputComponent::dPad, LB2DO},
		InputComponentDesc{"Face Buttons", faceKeyInfo, InputComponent::button, RB2DO},
		InputComponentDesc{"Select", {&centerKeyInfo[0], 1}, InputComponent::button, LB2DO},
		InputComponentDesc{"Run", {&centerKeyInfo[1], 1}, InputComponent::button, RB2DO},
		InputComponentDesc{"Select/Run", centerKeyInfo, InputComponent::button, CB2DO, {.altConfig = true}},
	};

	static constexpr SystemInputDeviceDesc gamepadDesc{"Gamepad", gamepadComponents};

	return gamepadDesc;
}

}
