/*  This file is part of Swan.emu.

	Swan.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Swan.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Swan.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/keyRemappingUtils.hh>
#include "MainSystem.hh"
#include "MainApp.hh"

namespace MDFN_IEN_WSWAN
{
extern uint16 WSButtonStatus;
}

namespace EmuEx
{

const int EmuSystem::maxPlayers = 1;
constexpr KeyCode altCodeBit = 0x10;

enum class SwanKey : KeyCode
{
	Up = 1,
	Right = 2,
	Down = 3,
	Left = 4,
	Start = 9,
	A = 10,
	B = 11,
	Y1 = 5,
	Y2 = 6,
	Y3 = 7,
	Y4 = 8,
	ANoRotation = altCodeBit | 10,
	BNoRotation = altCodeBit | 11,
	Y1X1 = altCodeBit | 1,
	Y2X2 = altCodeBit | 2,
	Y3X3 = altCodeBit | 3,
	Y4X4 = altCodeBit | 4,
};

constexpr auto dpadKeyInfo = makeArray<KeyInfo>
(
	SwanKey::Up,
	SwanKey::Right,
	SwanKey::Down,
	SwanKey::Left
);

constexpr auto centerKeyInfo = makeArray<KeyInfo>(SwanKey::Start);

constexpr auto faceKeyInfo = makeArray<KeyInfo>
(
	SwanKey::BNoRotation,
	SwanKey::ANoRotation
);

constexpr auto turboFaceKeyInfo = turbo(faceKeyInfo);

constexpr auto oppositeDPadKeyInfo = makeArray<KeyInfo>
(
	SwanKey::Y4X4,
	SwanKey::Y3X3,
	SwanKey::Y1X1,
	SwanKey::Y2X2
);

constexpr auto faceButtonCombinedCodes = makeArray<KeyInfo>
(
	SwanKey::BNoRotation,
	SwanKey::ANoRotation,
	SwanKey::Y4X4,
	SwanKey::Y3X3,
	SwanKey::Y1X1,
	SwanKey::Y2X2
);

constexpr auto allFaceKeyInfo = makeArray<KeyInfo>
(
	SwanKey::A,
	SwanKey::B,
	SwanKey::Y1,
	SwanKey::Y2,
	SwanKey::Y4,
	SwanKey::Y3,
	SwanKey::BNoRotation,
	SwanKey::ANoRotation,
	SwanKey::Y4X4,
	SwanKey::Y3X3,
	SwanKey::Y1X1,
	SwanKey::Y2X2
);

constexpr auto gpKeyInfo = concatToArrayNow<dpadKeyInfo, centerKeyInfo, allFaceKeyInfo, turboFaceKeyInfo>;

std::span<const KeyCategory> WsApp::keyCategories()
{
	static constexpr KeyCategory categories[]
	{
		{"Gamepad", gpKeyInfo},
	};
	return categories;
}

std::string_view WsApp::systemKeyCodeToString(KeyCode c)
{
	switch(SwanKey(c))
	{
		case SwanKey::Up: return "Up X1 ↷ Y2";
		case SwanKey::Right: return "Right X2 ↷ Y3";
		case SwanKey::Down: return "Down X3 ↷ Y4";
		case SwanKey::Left: return "Left X4 ↷ Y1";
		case SwanKey::Start: return "Start";
		case SwanKey::A: return "A ↷ X4";
		case SwanKey::B: return "B ↷ X1";
		case SwanKey::Y1: return "Y1 ↷ B";
		case SwanKey::Y2: return "Y2 ↷ A";
		case SwanKey::Y3: return "Y3 ↷ X3";
		case SwanKey::Y4: return "Y4 ↷ X2";
		case SwanKey::ANoRotation: return "A";
		case SwanKey::BNoRotation: return "B";
		case SwanKey::Y1X1: return "Y1 ↷ X1";
		case SwanKey::Y2X2: return "Y2 ↷ X2";
		case SwanKey::Y3X3: return "Y3 ↷ X3";
		case SwanKey::Y4X4: return "Y4 ↷ X4";
		default: return "";
	}
}

std::span<const KeyConfigDesc> WsApp::defaultKeyConfigs()
{
	using namespace IG::Input;

	static constexpr std::array pcKeyboardMap
	{
		KeyMapping{SwanKey::Up, Keycode::UP},
		KeyMapping{SwanKey::Right, Keycode::RIGHT},
		KeyMapping{SwanKey::Down, Keycode::DOWN},
		KeyMapping{SwanKey::Left, Keycode::LEFT},
		KeyMapping{SwanKey::Start, Keycode::ENTER},
		KeyMapping{SwanKey::A, Keycode::X},
		KeyMapping{SwanKey::B, Keycode::Z},
		KeyMapping{SwanKey::Y1, Keycode::Q},
		KeyMapping{SwanKey::Y2, Keycode::W},
		KeyMapping{SwanKey::Y3, Keycode::S},
		KeyMapping{SwanKey::Y4, Keycode::A},
	};

	static constexpr std::array genericGamepadMap
	{
		KeyMapping{SwanKey::Up, Keycode::UP},
		KeyMapping{SwanKey::Right, Keycode::RIGHT},
		KeyMapping{SwanKey::Down, Keycode::DOWN},
		KeyMapping{SwanKey::Left, Keycode::LEFT},
		KeyMapping{SwanKey::Start, Keycode::GAME_START},
		KeyMapping{SwanKey::A, Keycode::GAME_A},
		KeyMapping{SwanKey::B, Keycode::GAME_X},
		KeyMapping{SwanKey::Y1, Keycode::GAME_L1},
		KeyMapping{SwanKey::Y2, Keycode::GAME_R1},
		KeyMapping{SwanKey::Y3, Keycode::GAME_B},
		KeyMapping{SwanKey::Y4, Keycode::GAME_Y},
	};

	static constexpr std::array wiimoteMap
	{
		KeyMapping{SwanKey::Up, WiimoteKey::UP},
		KeyMapping{SwanKey::Right, WiimoteKey::RIGHT},
		KeyMapping{SwanKey::Down, WiimoteKey::DOWN},
		KeyMapping{SwanKey::Left, WiimoteKey::LEFT},
		KeyMapping{SwanKey::B, WiimoteKey::_1},
		KeyMapping{SwanKey::A, WiimoteKey::_2},
		KeyMapping{SwanKey::Start, WiimoteKey::PLUS},
	};

	return genericKeyConfigs<pcKeyboardMap, genericGamepadMap, wiimoteMap>();
}

bool WsApp::allowsTurboModifier(KeyCode c)
{
	switch(SwanKey(c))
	{
		case SwanKey::A ... SwanKey::B:
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

AssetDesc WsApp::vControllerAssetDesc(KeyInfo key) const
{
	static constexpr struct VirtualControllerAssets
	{
		AssetDesc dpad{AssetFileID::gamepadOverlay, gpImageCoords({{}, {4, 4}})},

		a{AssetFileID::gamepadOverlay,     gpImageCoords({{4, 0}, {2, 2}})},
		b{AssetFileID::gamepadOverlay,     gpImageCoords({{6, 0}, {2, 2}})},
		d1{AssetFileID::gamepadOverlay,    gpImageCoords({{4, 2}, {2, 2}})},
		d2{AssetFileID::gamepadOverlay,    gpImageCoords({{6, 2}, {2, 2}})},
		d3{AssetFileID::gamepadOverlay,    gpImageCoords({{0, 4}, {2, 2}})},
		d4{AssetFileID::gamepadOverlay,    gpImageCoords({{2, 4}, {2, 2}})},
		start{AssetFileID::gamepadOverlay, gpImageCoords({{0, 6}, {2, 1}}), {1, 2}},

		blank{AssetFileID::gamepadOverlay, gpImageCoords({{4, 4}, {2, 2}})};
	} virtualControllerAssets;

	if(key[0] == 0)
		return virtualControllerAssets.dpad;
	switch(SwanKey(key[0]))
	{
		case SwanKey::ANoRotation:
		case SwanKey::A: return virtualControllerAssets.a;
		case SwanKey::BNoRotation:
		case SwanKey::B: return virtualControllerAssets.b;
		case SwanKey::Y1X1:
		case SwanKey::Y1: return virtualControllerAssets.d1;
		case SwanKey::Y2X2:
		case SwanKey::Y2: return virtualControllerAssets.d2;
		case SwanKey::Y3X3:
		case SwanKey::Y3: return virtualControllerAssets.d3;
		case SwanKey::Y4X4:
		case SwanKey::Y4: return virtualControllerAssets.d4;
		case SwanKey::Start: return virtualControllerAssets.start;
		default: return virtualControllerAssets.blank;
	}
}

enum KeypadMask: uint16_t
{
	X1_BIT = bit(0),
	X2_BIT = bit(1),
	X3_BIT = bit(2),
	X4_BIT = bit(3),
	Y1_BIT = bit(4),
	Y2_BIT = bit(5),
	Y3_BIT = bit(6),
	Y4_BIT = bit(7),
	START_BIT = bit(8),
	A_BIT = bit(9),
	B_BIT = bit(10),
};

void WsSystem::handleInputAction(EmuApp *, InputAction a)
{
	using namespace MDFN_IEN_WSWAN;
	auto gpBits = [&] -> uint16_t
	{
		if(isRotated())
		{
			switch(SwanKey(a.code))
			{
				case SwanKey::Up: return Y2_BIT;
				case SwanKey::Right: return Y3_BIT;
				case SwanKey::Down: return Y4_BIT;
				case SwanKey::Left: return Y1_BIT;
				case SwanKey::Y1: return B_BIT;
				case SwanKey::Y2: return A_BIT;
				case SwanKey::Y3: return X3_BIT;
				case SwanKey::Y4: return X2_BIT;
				case SwanKey::Start: return START_BIT;
				case SwanKey::A: return X4_BIT;
				case SwanKey::B: return X1_BIT;
				case SwanKey::ANoRotation: return A_BIT;
				case SwanKey::BNoRotation: return B_BIT;
				case SwanKey::Y1X1: return X1_BIT;
				case SwanKey::Y2X2: return X2_BIT;
				case SwanKey::Y3X3: return X3_BIT;
				case SwanKey::Y4X4: return X4_BIT;
			}
		}
		else
		{
			switch(SwanKey(a.code))
			{
				case SwanKey::Up: return X1_BIT;
				case SwanKey::Right: return X2_BIT;
				case SwanKey::Down: return X3_BIT;
				case SwanKey::Left: return X4_BIT;
				case SwanKey::Y1: return Y1_BIT;
				case SwanKey::Y2: return Y2_BIT;
				case SwanKey::Y3: return Y3_BIT;
				case SwanKey::Y4: return Y4_BIT;
				case SwanKey::Start: return START_BIT;
				case SwanKey::A: return A_BIT;
				case SwanKey::B: return B_BIT;
				case SwanKey::ANoRotation: return A_BIT;
				case SwanKey::BNoRotation: return B_BIT;
				case SwanKey::Y1X1: return Y1_BIT;
				case SwanKey::Y2X2: return Y2_BIT;
				case SwanKey::Y3X3: return Y3_BIT;
				case SwanKey::Y4X4: return Y4_BIT;
			}
		}
		bug_unreachable("invalid key");
	}();
	WSButtonStatus = setOrClearBits(WSButtonStatus, gpBits, a.isPushed());
}

void WsSystem::clearInputBuffers(EmuInputView &)
{
	MDFN_IEN_WSWAN::WSButtonStatus = {};
}

void WsSystem::setupInput(EmuApp &app)
{
	constexpr std::array faceKeyInfo{KeyCode(SwanKey::BNoRotation), KeyCode(SwanKey::ANoRotation)};
	constexpr std::array oppositeDPadKeyInfo{KeyCode(SwanKey::Y4X4), KeyCode(SwanKey::Y3X3),
		KeyCode(SwanKey::Y1X1), KeyCode(SwanKey::Y2X2)};
	if(isRotated())
	{
		if(showVGamepadABWhenVertical)
			app.unsetDisabledInputKeys();
		else
			app.setDisabledInputKeys(faceKeyInfo);
	}
	else
	{
		if(showVGamepadYWhenHorizonal)
			app.unsetDisabledInputKeys();
		else
			app.setDisabledInputKeys(oppositeDPadKeyInfo);
	}
}

SystemInputDeviceDesc WsSystem::inputDeviceDesc(int idx) const
{
	static constexpr std::array gamepadComponents
	{
		InputComponentDesc{"D-Pad", dpadKeyInfo, InputComponent::dPad, LB2DO},
		InputComponentDesc{"Face Buttons + Opposite D-Pad Buttons", faceButtonCombinedCodes, InputComponent::button, RB2DO, {.rowSize = 2}},
		InputComponentDesc{"Face Buttons", faceKeyInfo, InputComponent::button, RB2DO, {.altConfig = true}},
		InputComponentDesc{"Opposite D-Pad Buttons", oppositeDPadKeyInfo, InputComponent::button, RB2DO, {.altConfig = true, .staggeredLayout = true}},
		InputComponentDesc{"Start", centerKeyInfo, InputComponent::button, RB2DO},
	};

	static constexpr SystemInputDeviceDesc gamepadDesc{"Gamepad", gamepadComponents};

	return gamepadDesc;
}

}
