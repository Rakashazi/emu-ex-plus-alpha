/*  This file is part of GBA.emu.

	GBA.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	GBA.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with GBA.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/keyRemappingUtils.hh>
#include "MainSystem.hh"
#include "MainApp.hh"
#include <core/gba/gba.h>
#include <core/gba/gbaGlobals.h>
#include <format>

namespace EmuEx
{

using namespace IG;

const int EmuSystem::maxPlayers = 1;

enum class GbaKey : KeyCode
{
	Up = 7,
	Right = 5,
	Down = 8,
	Left = 6,
	Select = 3,
	Start = 4,
	A = 1,
	B = 2,
	L = 10,
	R = 9,

	// Special key codes
	LightInc = 11,
	LightDec,
};

constexpr auto dpadKeyInfo = makeArray<KeyInfo>
(
	GbaKey::Up,
	GbaKey::Right,
	GbaKey::Down,
	GbaKey::Left
);

constexpr auto centerKeyInfo = makeArray<KeyInfo>
(
	GbaKey::Select,
	GbaKey::Start
);

constexpr auto faceKeyInfo = makeArray<KeyInfo>
(
	GbaKey::B,
	GbaKey::A
);

constexpr auto turboFaceKeyInfo = turbo(faceKeyInfo);

constexpr auto faceLRKeyInfo = makeArray<KeyInfo>
(
	GbaKey::B,
	GbaKey::L,
	GbaKey::A,
	GbaKey::R
);

constexpr std::array comboKeyInfo
{
	KeyInfo{std::array{GbaKey::A, GbaKey::B}},
	KeyInfo{std::array{GbaKey::R, GbaKey::B}}
};

constexpr auto lKeyInfo = makeArray<KeyInfo>(GbaKey::L);
constexpr auto rKeyInfo = makeArray<KeyInfo>(GbaKey::R);

constexpr auto gpKeyInfo = concatToArrayNow<dpadKeyInfo, centerKeyInfo, faceLRKeyInfo, turboFaceKeyInfo, comboKeyInfo>;

std::span<const KeyCategory> GbaApp::keyCategories()
{
	static constexpr std::array categories
	{
		KeyCategory{"Gamepad", gpKeyInfo},
	};
	return categories;
}

std::string_view GbaApp::systemKeyCodeToString(KeyCode c)
{
	switch(GbaKey(c))
	{
		case GbaKey::Up: return "Up";
		case GbaKey::Right: return "Right";
		case GbaKey::Down: return "Down";
		case GbaKey::Left: return "Left";
		case GbaKey::Select: return "Select";
		case GbaKey::Start: return "Start";
		case GbaKey::A: return "A";
		case GbaKey::B: return "B";
		case GbaKey::L: return "L";
		case GbaKey::R: return "R";
		case GbaKey::LightInc: return "Light Sensor Level +";
		case GbaKey::LightDec: return "Light Sensor Level -";
		default: return "";
	}
}

std::span<const KeyConfigDesc> GbaApp::defaultKeyConfigs()
{
	using namespace IG::Input;

	static constexpr std::array pcKeyboardMap
	{
		KeyMapping{GbaKey::Up, Keycode::UP},
		KeyMapping{GbaKey::Right, Keycode::RIGHT},
		KeyMapping{GbaKey::Down, Keycode::DOWN},
		KeyMapping{GbaKey::Left, Keycode::LEFT},
		KeyMapping{GbaKey::Select, Keycode::SPACE},
		KeyMapping{GbaKey::Start, Keycode::ENTER},
		KeyMapping{GbaKey::A, Keycode::X},
		KeyMapping{GbaKey::B, Keycode::Z},
		KeyMapping{GbaKey::L, Keycode::A},
		KeyMapping{GbaKey::R, Keycode::S},
	};

	static constexpr std::array genericGamepadMap
	{
		KeyMapping{GbaKey::Up, Keycode::UP},
		KeyMapping{GbaKey::Right, Keycode::RIGHT},
		KeyMapping{GbaKey::Down, Keycode::DOWN},
		KeyMapping{GbaKey::Left, Keycode::LEFT},
		KeyMapping{GbaKey::Select, Keycode::GAME_SELECT},
		KeyMapping{GbaKey::Start, Keycode::GAME_START},
		KeyMapping{GbaKey::A, Keycode::GAME_A},
		KeyMapping{GbaKey::B, Keycode::GAME_X},
		KeyMapping{GbaKey::L, Keycode::GAME_L1},
		KeyMapping{GbaKey::R, Keycode::GAME_R1},
	};

	static constexpr std::array wiimoteMap
	{
		KeyMapping{GbaKey::Up, WiimoteKey::UP},
		KeyMapping{GbaKey::Right, WiimoteKey::RIGHT},
		KeyMapping{GbaKey::Down, WiimoteKey::DOWN},
		KeyMapping{GbaKey::Left, WiimoteKey::LEFT},
		KeyMapping{GbaKey::B, WiimoteKey::_1},
		KeyMapping{GbaKey::A, WiimoteKey::_2},
		KeyMapping{GbaKey::L, WiimoteKey::B},
		KeyMapping{GbaKey::R, WiimoteKey::A},
		KeyMapping{GbaKey::Select, WiimoteKey::MINUS},
		KeyMapping{GbaKey::Start, WiimoteKey::PLUS},
	};

	return genericKeyConfigs<pcKeyboardMap, genericGamepadMap, wiimoteMap>();
}

bool GbaApp::allowsTurboModifier(KeyCode c)
{
	switch(GbaKey(c))
	{
		case GbaKey::A ... GbaKey::R:
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

AssetDesc GbaApp::vControllerAssetDesc(KeyInfo key) const
{
	static constexpr struct VirtualControllerAssets
	{
		AssetDesc dpad{AssetFileID::gamepadOverlay, gpImageCoords({{}, {4, 4}})},

		a{AssetFileID::gamepadOverlay,      gpImageCoords({{4, 0}, {2, 2}})},
		b{AssetFileID::gamepadOverlay,      gpImageCoords({{6, 0}, {2, 2}})},
		ab{AssetFileID::gamepadOverlay,     gpImageCoords({{4, 2}, {2, 2}})},
		rb{AssetFileID::gamepadOverlay,     gpImageCoords({{2, 4}, {2, 2}})},
		l{AssetFileID::gamepadOverlay,      gpImageCoords({{4, 4}, {2, 2}})},
		r{AssetFileID::gamepadOverlay,      gpImageCoords({{6, 4}, {2, 2}})},
		select{AssetFileID::gamepadOverlay, gpImageCoords({{0, 6}, {2, 1}}), {1, 2}},
		start{AssetFileID::gamepadOverlay,  gpImageCoords({{0, 7}, {2, 1}}), {1, 2}},

		increaseLight{AssetFileID::gamepadOverlay, gpImageCoords({{2, 6}, {2, 1}}), {1, 2}},
		decreaseLight{AssetFileID::gamepadOverlay, gpImageCoords({{2, 7}, {2, 1}}), {1, 2}},

		blank{AssetFileID::gamepadOverlay, gpImageCoords({{6, 2}, {2, 2}})};
	} virtualControllerAssets;

	if(key[0] == 0)
		return virtualControllerAssets.dpad;
	switch(GbaKey(key[0]))
	{
		case GbaKey::A: return GbaKey(key[1]) == GbaKey::B ? virtualControllerAssets.ab : virtualControllerAssets.a;
		case GbaKey::B: return virtualControllerAssets.b;
		case GbaKey::L: return virtualControllerAssets.l;
		case GbaKey::R: return GbaKey(key[1]) == GbaKey::B ? virtualControllerAssets.rb : virtualControllerAssets.r;
		case GbaKey::Select: return virtualControllerAssets.select;
		case GbaKey::Start: return virtualControllerAssets.start;
		case GbaKey::LightInc: return virtualControllerAssets.increaseLight;
		case GbaKey::LightDec: return virtualControllerAssets.decreaseLight;
		default: return virtualControllerAssets.blank;
	}
}

void GbaSystem::handleInputAction(EmuApp *app, InputAction a)
{
	auto key = GbaKey(a.code);
	switch(key)
	{
		case GbaKey::LightInc:
		case GbaKey::LightDec:
		{
			int darknessChange = key == GbaKey::LightDec ? 17 : -17;
			darknessLevel = std::clamp(darknessLevel + darknessChange, 0, 0xff);
			if(app)
			{
				app->postMessage(1, false, std::format("Light sensor level: {}%", remap(darknessLevel, 0xff, 0, 0, 100)));
			}
			break;
		}
		default:
			P1 = setOrClearBits(P1, bit(a.code - 1), !a.isPushed());
			break;
	}
}

void GbaSystem::clearInputBuffers(EmuInputView &)
{
	P1 = 0x03FF;
	clearSensorValues();
}

SystemInputDeviceDesc GbaSystem::inputDeviceDesc(int idx) const
{
	static constexpr std::array gamepadComponents
	{
		InputComponentDesc{"D-Pad", dpadKeyInfo, InputComponent::dPad, LB2DO},
		InputComponentDesc{"Face Buttons", faceKeyInfo, InputComponent::button, RB2DO},
		InputComponentDesc{"Face Buttons + Inline L/R", faceLRKeyInfo, InputComponent::button, RB2DO, {.altConfig = true}},
		InputComponentDesc{"L", lKeyInfo, InputComponent::trigger, LB2DO},
		InputComponentDesc{"R", rKeyInfo, InputComponent::trigger, RB2DO},
		InputComponentDesc{"Select", {&centerKeyInfo[0], 1}, InputComponent::button, LB2DO},
		InputComponentDesc{"Start", {&centerKeyInfo[1], 1}, InputComponent::button, RB2DO},
		InputComponentDesc{"Select/Start", centerKeyInfo, InputComponent::button, CB2DO, {.altConfig = true}},
	};

	static constexpr SystemInputDeviceDesc gamepadDesc{"Gamepad", gamepadComponents};

	return gamepadDesc;
}

}
