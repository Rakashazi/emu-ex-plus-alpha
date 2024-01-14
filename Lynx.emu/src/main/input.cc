/*  This file is part of Lynx.emu.

	Lynx.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Lynx.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Lynx.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/keyRemappingUtils.hh>
#include "MainSystem.hh"
#include "MainApp.hh"

void Lynx_SetButtonData(uint32 data);

namespace EmuEx
{

const int EmuSystem::maxPlayers = 1;

enum class LynxKey : KeyCode
{
	Up = 7,
	Right = 6,
	Down = 8,
	Left = 5,
	Option1 = 4,
	Option2 = 3,
	Pause = 9,
	A = 2,
	B = 1,
};

constexpr auto dpadKeyInfo = makeArray<KeyInfo>
(
	LynxKey::Up,
	LynxKey::Right,
	LynxKey::Down,
	LynxKey::Left
);

constexpr std::array optionKeyInfo = makeArray<KeyInfo>
(
	LynxKey::Option1,
	LynxKey::Option2
);

constexpr std::array pauseKeyInfo = makeArray<KeyInfo>(LynxKey::Pause);

constexpr std::array faceKeyInfo = makeArray<KeyInfo>
(
	LynxKey::B,
	LynxKey::A
);

constexpr auto turboFaceKeyInfo = turbo(faceKeyInfo);

constexpr auto gpKeyInfo = concatToArrayNow<dpadKeyInfo, optionKeyInfo, pauseKeyInfo, faceKeyInfo, turboFaceKeyInfo>;

std::span<const KeyCategory> LynxApp::keyCategories()
{
	static constexpr KeyCategory categories[]
	{
		{"Gamepad", gpKeyInfo},
	};
	return categories;
}

std::string_view LynxApp::systemKeyCodeToString(KeyCode c)
{
	switch(LynxKey(c))
	{
		case LynxKey::Up: return "Up";
		case LynxKey::Right: return "Right";
		case LynxKey::Down: return "Down";
		case LynxKey::Left: return "Left";
		case LynxKey::Option1: return "Option 1";
		case LynxKey::Option2: return "Option 2";
		case LynxKey::Pause: return "Pause";
		case LynxKey::A: return "A";
		case LynxKey::B: return "B";
		default: return "";
	}
}

std::span<const KeyConfigDesc> LynxApp::defaultKeyConfigs()
{
	using namespace IG::Input;

	static constexpr std::array pcKeyboardMap
	{
		KeyMapping{LynxKey::Up, Keycode::UP},
		KeyMapping{LynxKey::Right, Keycode::RIGHT},
		KeyMapping{LynxKey::Down, Keycode::DOWN},
		KeyMapping{LynxKey::Left, Keycode::LEFT},
		KeyMapping{LynxKey::Option1, Keycode::A},
		KeyMapping{LynxKey::Option2, Keycode::S},
		KeyMapping{LynxKey::Pause, Keycode::ENTER},
		KeyMapping{LynxKey::A, Keycode::X},
		KeyMapping{LynxKey::B, Keycode::Z},
	};

	static constexpr std::array genericGamepadMap
	{
		KeyMapping{LynxKey::Up, Keycode::UP},
		KeyMapping{LynxKey::Right, Keycode::RIGHT},
		KeyMapping{LynxKey::Down, Keycode::DOWN},
		KeyMapping{LynxKey::Left, Keycode::LEFT},
		KeyMapping{LynxKey::Option1, Keycode::GAME_L1},
		KeyMapping{LynxKey::Option2, Keycode::GAME_R1},
		KeyMapping{LynxKey::Pause, Keycode::GAME_START},
		KeyMapping{LynxKey::A, Keycode::GAME_A},
		KeyMapping{LynxKey::B, Keycode::GAME_X},
	};

	static constexpr std::array wiimoteMap
	{
		KeyMapping{LynxKey::Up, WiimoteKey::UP},
		KeyMapping{LynxKey::Right, WiimoteKey::RIGHT},
		KeyMapping{LynxKey::Down, WiimoteKey::DOWN},
		KeyMapping{LynxKey::Left, WiimoteKey::LEFT},
		KeyMapping{LynxKey::B, WiimoteKey::_1},
		KeyMapping{LynxKey::A, WiimoteKey::_2},
		KeyMapping{LynxKey::Option1, WiimoteKey::A},
		KeyMapping{LynxKey::Option1, WiimoteKey::B},
		KeyMapping{LynxKey::Pause, WiimoteKey::PLUS},
	};

	return genericKeyConfigs<pcKeyboardMap, genericGamepadMap, wiimoteMap>();
}

bool LynxApp::allowsTurboModifier(KeyCode c)
{
	switch(LynxKey(c))
	{
		case LynxKey::B ... LynxKey::A:
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

AssetDesc LynxApp::vControllerAssetDesc(KeyInfo key) const
{
	static constexpr struct VirtualControllerAssets
	{
		AssetDesc dpad{AssetFileID::gamepadOverlay, gpImageCoords({{}, {4, 4}})},

		a{AssetFileID::gamepadOverlay,       gpImageCoords({{4, 0}, {2, 2}})},
		b{AssetFileID::gamepadOverlay,       gpImageCoords({{6, 0}, {2, 2}})},
		blank{AssetFileID::gamepadOverlay,   gpImageCoords({{4, 2}, {2, 2}})},
		ab{AssetFileID::gamepadOverlay,      gpImageCoords({{6, 2}, {2, 2}})},
		pause{AssetFileID::gamepadOverlay,   gpImageCoords({{0, 6}, {2, 1}}), {1, 2}},
		option1{AssetFileID::gamepadOverlay, gpImageCoords({{0, 7}, {2, 1}}), {1, 2}},
		option2{AssetFileID::gamepadOverlay, gpImageCoords({{2, 7}, {2, 1}}), {1, 2}};
	} virtualControllerAssets;

	if(key[0] == 0)
		return virtualControllerAssets.dpad;
	switch(LynxKey(key[0]))
	{
		case LynxKey::A: return virtualControllerAssets.a;
		case LynxKey::B: return virtualControllerAssets.b;
		case LynxKey::Pause: return virtualControllerAssets.pause;
		case LynxKey::Option1: return virtualControllerAssets.option1;
		case LynxKey::Option2: return virtualControllerAssets.option2;
		default: return virtualControllerAssets.blank;
	}
}

static LynxKey rotateDPadKeycode(LynxKey key, Rotation rotation)
{
	if(rotation == Rotation::UP)
		return key;
	switch(key)
	{
		case LynxKey::Up: return rotation == Rotation::RIGHT ? LynxKey::Right : LynxKey::Left;
		case LynxKey::Right: return rotation == Rotation::RIGHT ? LynxKey::Down : LynxKey::Up;
		case LynxKey::Down: return rotation == Rotation::RIGHT ? LynxKey::Left : LynxKey::Right;
		case LynxKey::Left: return rotation == Rotation::RIGHT ? LynxKey::Up : LynxKey::Down;
		default: return key;
	}
}

LynxKey LynxSystem::rotateDPadKeycode(LynxKey key) const
{
	return EmuEx::rotateDPadKeycode(key, contentRotation());
}

void LynxSystem::handleInputAction(EmuApp *, InputAction a)
{
	auto key = rotateDPadKeycode(LynxKey(a.code));
	inputBuff = setOrClearBits(inputBuff, bit(to_underlying(key) - 1), a.isPushed());
	Lynx_SetButtonData(inputBuff);
}

void LynxSystem::clearInputBuffers(EmuInputView &)
{
	inputBuff = {};
	Lynx_SetButtonData(0);
}

SystemInputDeviceDesc LynxSystem::inputDeviceDesc(int idx) const
{
	static constexpr std::array gamepadComponents
	{
		InputComponentDesc{"D-Pad", dpadKeyInfo, InputComponent::dPad, LB2DO},
		InputComponentDesc{"Face Buttons", faceKeyInfo, InputComponent::button, RB2DO},
		InputComponentDesc{"Option Buttons", optionKeyInfo, InputComponent::button, LB2DO},
		InputComponentDesc{"Pause", pauseKeyInfo, InputComponent::button,RB2DO},
	};

	static constexpr SystemInputDeviceDesc gamepadDesc{"Gamepad", gamepadComponents};

	return gamepadDesc;
}

}
