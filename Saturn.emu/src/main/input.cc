/*  This file is part of Saturn.emu.

	Saturn.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Saturn.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Saturn.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/keyRemappingUtils.hh>
#include "MainSystem.hh"
#include "MainApp.hh"

namespace EmuEx
{

const int EmuSystem::maxPlayers = 2;

enum class SaturnKey : KeyCode
{
	Up = 1,
	Right,
	Down,
	Left,
	Start,
	A,
	B,
	C,
	X,
	Y,
	Z,
	L,
	R,
};

constexpr auto dpadButtonCodes = makeArray<KeyInfo>
(
	SaturnKey::Up,
	SaturnKey::Right,
	SaturnKey::Down,
	SaturnKey::Left
);

constexpr auto centerButtonCodes = makeArray<KeyInfo>(SaturnKey::Start);

constexpr auto faceButtonCodes = makeArray<KeyInfo>
(
	SaturnKey::A,
	SaturnKey::B,
	SaturnKey::C,
	SaturnKey::X,
	SaturnKey::Y,
	SaturnKey::Z
);

constexpr auto gpKeyInfo = concatToArrayNow<dpadButtonCodes, centerButtonCodes, faceButtonCodes>;
constexpr auto gp2KeyInfo = transpose(gpKeyInfo, 1);

std::span<const KeyCategory> SaturnApp::keyCategories()
{
	static constexpr std::array categories
	{
		KeyCategory{"Gamepad", gpKeyInfo},
		KeyCategory{"Gamepad 2", gp2KeyInfo, 1},
	};
	return categories;
}

std::string_view SaturnApp::systemKeyCodeToString(KeyCode c)
{
	switch(SaturnKey(c))
	{
		case SaturnKey::Up: return "Up";
		case SaturnKey::Right: return "Right";
		case SaturnKey::Down: return "Down";
		case SaturnKey::Left: return "Left";
		case SaturnKey::Start: return "Start";
		case SaturnKey::A: return "A";
		case SaturnKey::B: return "B";
		case SaturnKey::C: return "C";
		case SaturnKey::X: return "X";
		case SaturnKey::Y: return "Y";
		case SaturnKey::Z: return "Z";
		default: return "";
	}
}

std::span<const KeyConfigDesc> SaturnApp::defaultKeyConfigs()
{
	using namespace IG::Input;

	static constexpr std::array pcKeyboardMap
	{
		KeyMapping{SaturnKey::Up, Keycode::UP},
		KeyMapping{SaturnKey::Right, Keycode::RIGHT},
		KeyMapping{SaturnKey::Down, Keycode::DOWN},
		KeyMapping{SaturnKey::Left, Keycode::LEFT},
		KeyMapping{SaturnKey::Start, Keycode::ENTER},
		KeyMapping{SaturnKey::A, Keycode::Z},
		KeyMapping{SaturnKey::B, Keycode::X},
		KeyMapping{SaturnKey::C, Keycode::C},
		KeyMapping{SaturnKey::X, Keycode::A},
		KeyMapping{SaturnKey::Y, Keycode::S},
		KeyMapping{SaturnKey::Z, Keycode::D},
	};

	static constexpr std::array genericGamepadMap
	{
		KeyMapping{SaturnKey::Up, Keycode::UP},
		KeyMapping{SaturnKey::Right, Keycode::RIGHT},
		KeyMapping{SaturnKey::Down, Keycode::DOWN},
		KeyMapping{SaturnKey::Left, Keycode::LEFT},
		KeyMapping{SaturnKey::Start, Keycode::GAME_START},
		KeyMapping{SaturnKey::A, Keycode::GAME_X},
		KeyMapping{SaturnKey::B, Keycode::GAME_A},
		KeyMapping{SaturnKey::C, Keycode::GAME_B},
		KeyMapping{SaturnKey::X, Keycode::GAME_L1},
		KeyMapping{SaturnKey::Y, Keycode::GAME_Y},
		KeyMapping{SaturnKey::Z, Keycode::GAME_R1},
	};

	static constexpr std::array wiimoteMap
	{
		KeyMapping{SaturnKey::Up, Wiimote::UP},
		KeyMapping{SaturnKey::Right, Wiimote::RIGHT},
		KeyMapping{SaturnKey::Down, Wiimote::DOWN},
		KeyMapping{SaturnKey::Left, Wiimote::LEFT},
		KeyMapping{SaturnKey::A, Wiimote::_1},
		KeyMapping{SaturnKey::B, Wiimote::_2},
		KeyMapping{SaturnKey::C, Wiimote::B},
		KeyMapping{SaturnKey::Start, Wiimote::PLUS},
	};

	return genericKeyConfigs<pcKeyboardMap, genericGamepadMap, wiimoteMap>();
}

bool SaturnApp::allowsTurboModifier(KeyCode c)
{
	switch(SaturnKey(c))
	{
		case SaturnKey::A ... SaturnKey::Z:
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

AssetDesc SaturnApp::vControllerAssetDesc(KeyInfo key) const
{
	static constexpr AssetDesc virtualControllerAssets[]
	{
		// d-pad
		{AssetFileID::gamepadOverlay, gpImageCoords({{}, {4, 4}})},

		// gamepad buttons
		{AssetFileID::gamepadOverlay, gpImageCoords({{4, 0}, {2, 2}})}, // A
		{AssetFileID::gamepadOverlay, gpImageCoords({{6, 0}, {2, 2}})}, // B
		{AssetFileID::gamepadOverlay, gpImageCoords({{4, 2}, {2, 2}})}, // C
		{AssetFileID::gamepadOverlay, gpImageCoords({{6, 2}, {2, 2}})}, // X
		{AssetFileID::gamepadOverlay, gpImageCoords({{0, 4}, {2, 2}})}, // Y
		{AssetFileID::gamepadOverlay, gpImageCoords({{2, 4}, {2, 2}})}, // Z
		{AssetFileID::gamepadOverlay, gpImageCoords({{4, 4}, {2, 2}})}, // L
		{AssetFileID::gamepadOverlay, gpImageCoords({{6, 4}, {2, 2}})}, // R
		{AssetFileID::gamepadOverlay, gpImageCoords({{0, 6}, {2, 1}}), {1, 2}}, // start
	};

	if(key[0] == 0)
		return virtualControllerAssets[0];
	switch(SaturnKey(key[0]))
	{
		case SaturnKey::A: return virtualControllerAssets[1];
		case SaturnKey::B: return virtualControllerAssets[2];
		case SaturnKey::C: return virtualControllerAssets[3];
		case SaturnKey::X: return virtualControllerAssets[4];
		case SaturnKey::Y: return virtualControllerAssets[5];
		case SaturnKey::Z: return virtualControllerAssets[6];
		case SaturnKey::L: return virtualControllerAssets[7];
		case SaturnKey::R: return virtualControllerAssets[8];
		case SaturnKey::Start: return virtualControllerAssets[9];
		default: return virtualControllerAssets[1];
	}
}

void SaturnSystem::handleInputAction(EmuApp *, InputAction a)
{
	auto player = a.flags.deviceId;
	PerPad_struct *p = (player == 1) ? pad[1] : pad[0];
	bool pushed = a.state == Input::Action::PUSHED;
	switch(SaturnKey(a.code))
	{
		case SaturnKey::Up: if(pushed) PerPadUpPressed(p); else PerPadUpReleased(p); break;
		case SaturnKey::Right: if(pushed) PerPadRightPressed(p); else PerPadRightReleased(p); break;
		case SaturnKey::Down: if(pushed) PerPadDownPressed(p); else PerPadDownReleased(p); break;
		case SaturnKey::Left: if(pushed) PerPadLeftPressed(p); else PerPadLeftReleased(p); break;
		case SaturnKey::Start: if(pushed) PerPadStartPressed(p); else PerPadStartReleased(p); break;
		case SaturnKey::X: if(pushed) PerPadXPressed(p); else PerPadXReleased(p); break;
		case SaturnKey::Y: if(pushed) PerPadYPressed(p); else PerPadYReleased(p); break;
		case SaturnKey::Z: if(pushed) PerPadZPressed(p); else PerPadZReleased(p); break;
		case SaturnKey::A: if(pushed) PerPadAPressed(p); else PerPadAReleased(p); break;
		case SaturnKey::B: if(pushed) PerPadBPressed(p); else PerPadBReleased(p); break;
		case SaturnKey::C: if(pushed) PerPadCPressed(p); else PerPadCReleased(p); break;
		case SaturnKey::L: if(pushed) PerPadLTriggerPressed(p); else PerPadLTriggerReleased(p); break;
		case SaturnKey::R: if(pushed) PerPadRTriggerPressed(p); else PerPadRTriggerReleased(p); break;
		default: bug_unreachable("input == %d", a.code);
	}
}

void SaturnSystem::clearInputBuffers(EmuInputView &)
{
	PerPortReset();
	pad[0] = PerPadAdd(&PORTDATA1);
	pad[1] = PerPadAdd(&PORTDATA2);
}

SystemInputDeviceDesc SaturnSystem::inputDeviceDesc(int idx) const
{
	static constexpr std::array gamepadComponents
	{
		InputComponentDesc{"D-Pad", dpadButtonCodes, InputComponent::dPad, LB2DO},
		InputComponentDesc{"Face Buttons", faceButtonCodes, InputComponent::button, RB2DO},
		InputComponentDesc{"Start", centerButtonCodes, InputComponent::button, RB2DO},
	};

	static constexpr SystemInputDeviceDesc gamepadDesc{"Gamepad", gamepadComponents};

	return gamepadDesc;
}

}
