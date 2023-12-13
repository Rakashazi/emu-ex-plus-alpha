/*  This file is part of NEO.emu.

	NEO.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NEO.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NEO.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuInput.hh>
#include <emuframework/keyRemappingUtils.hh>
#include "MainSystem.hh"
#include "MainApp.hh"

extern "C"
{
	#include <gngeo/memory.h>
}

namespace EmuEx
{

const int EmuSystem::maxPlayers = 2;

enum class NeoKey : KeyCode
{
	Up = 1,
	Right = 4,
	Down = 2,
	Left = 3,
	A = 5,
	B = 6,
	C = 7,
	D = 8,
	Select = 9,
	Start = 10,
	TestSwitch = 11
};

constexpr auto dpadKeyInfo = makeArray<KeyInfo>
(
	NeoKey::Up,
	NeoKey::Right,
	NeoKey::Down,
	NeoKey::Left
);

constexpr auto centerKeyInfo = makeArray<KeyInfo>
(
	NeoKey::Select,
	NeoKey::Start
);

constexpr auto faceKeyInfo = makeArray<KeyInfo>
(
	NeoKey::A,
	NeoKey::B,
	NeoKey::C,
	NeoKey::D
);

constexpr auto turboFaceKeyInfo = turbo(faceKeyInfo);

constexpr std::array comboKeyInfo{KeyInfo{std::array{NeoKey::A, NeoKey::B, NeoKey::C}}};

constexpr auto gpKeyInfo = concatToArrayNow<dpadKeyInfo, centerKeyInfo, faceKeyInfo, turboFaceKeyInfo, comboKeyInfo>;
constexpr auto gp2KeyInfo = transpose(gpKeyInfo, 1);

std::span<const KeyCategory> NeoApp::keyCategories()
{
	static constexpr std::array categories
	{
		KeyCategory{"Gamepad", gpKeyInfo},
		KeyCategory{"Gamepad 2", gp2KeyInfo, 1},
	};
	return categories;
}

std::string_view NeoApp::systemKeyCodeToString(KeyCode c)
{
	switch(NeoKey(c))
	{
		case NeoKey::Up: return "Up";
		case NeoKey::Right: return "Right";
		case NeoKey::Down: return "Down";
		case NeoKey::Left: return "Left";
		case NeoKey::Select: return "Select";
		case NeoKey::Start: return "Start";
		case NeoKey::TestSwitch: return "Test Switch";
		case NeoKey::A: return "A";
		case NeoKey::B: return "B";
		case NeoKey::C: return "C";
		case NeoKey::D: return "D";
		default: return "";
	}
}

std::span<const KeyConfigDesc> NeoApp::defaultKeyConfigs()
{
	using namespace IG::Input;

	static constexpr std::array pcKeyboardMap
	{
		KeyMapping{NeoKey::Up, Keycode::UP},
		KeyMapping{NeoKey::Right, Keycode::RIGHT},
		KeyMapping{NeoKey::Down, Keycode::DOWN},
		KeyMapping{NeoKey::Left, Keycode::LEFT},
		KeyMapping{NeoKey::Select, Keycode::SPACE},
		KeyMapping{NeoKey::Start, Keycode::ENTER},
		KeyMapping{NeoKey::B, Keycode::Z},
		KeyMapping{NeoKey::D, Keycode::X},
		KeyMapping{NeoKey::A, Keycode::A},
		KeyMapping{NeoKey::C, Keycode::S},
	};

	static constexpr std::array genericGamepadMap
	{
		KeyMapping{NeoKey::Up, Keycode::UP},
		KeyMapping{NeoKey::Right, Keycode::RIGHT},
		KeyMapping{NeoKey::Down, Keycode::DOWN},
		KeyMapping{NeoKey::Left, Keycode::LEFT},
		KeyMapping{NeoKey::Select, Keycode::GAME_SELECT},
		KeyMapping{NeoKey::Start, Keycode::GAME_START},
		KeyMapping{NeoKey::B, Keycode::GAME_A},
		KeyMapping{NeoKey::D, Keycode::GAME_B},
		KeyMapping{NeoKey::A, Keycode::GAME_X},
		KeyMapping{NeoKey::C, Keycode::GAME_Y},
	};

	static constexpr std::array wiimoteMap
	{
		KeyMapping{NeoKey::Up, WiimoteKey::UP},
		KeyMapping{NeoKey::Right, WiimoteKey::RIGHT},
		KeyMapping{NeoKey::Down, WiimoteKey::DOWN},
		KeyMapping{NeoKey::Left, WiimoteKey::LEFT},
		KeyMapping{NeoKey::B, WiimoteKey::_1},
		KeyMapping{NeoKey::D, WiimoteKey::_2},
		KeyMapping{NeoKey::A, WiimoteKey::B},
		KeyMapping{NeoKey::C, WiimoteKey::A},
		KeyMapping{NeoKey::Select, WiimoteKey::MINUS},
		KeyMapping{NeoKey::Start, WiimoteKey::PLUS},
	};

	return genericKeyConfigs<pcKeyboardMap, genericGamepadMap, wiimoteMap>();
}

bool NeoApp::allowsTurboModifier(KeyCode c)
{
	switch(NeoKey(c))
	{
		case NeoKey::A ... NeoKey::D:
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

AssetDesc NeoApp::vControllerAssetDesc(KeyInfo key) const
{
	static constexpr struct VirtualControllerAssets
	{
		AssetDesc dpad{AssetFileID::gamepadOverlay, gpImageCoords({{}, {4, 4}})},

		a{AssetFileID::gamepadOverlay,      gpImageCoords({{4, 0}, {2, 2}})},
		b{AssetFileID::gamepadOverlay,      gpImageCoords({{6, 0}, {2, 2}})},
		c{AssetFileID::gamepadOverlay,      gpImageCoords({{4, 2}, {2, 2}})},
		d{AssetFileID::gamepadOverlay,      gpImageCoords({{6, 2}, {2, 2}})},
		abc{AssetFileID::gamepadOverlay,    gpImageCoords({{0, 4}, {2, 2}})},
		select{AssetFileID::gamepadOverlay, gpImageCoords({{0, 6}, {2, 1}}), {1, 2}},
		start{AssetFileID::gamepadOverlay,  gpImageCoords({{0, 7}, {2, 1}}), {1, 2}},
		test{AssetFileID::gamepadOverlay,   gpImageCoords({{2, 6}, {2, 1}}), {1, 2}},

		blank{AssetFileID::gamepadOverlay, gpImageCoords({{2, 4}, {2, 2}})};
	} virtualControllerAssets;

	if(key[0] == 0)
		return virtualControllerAssets.dpad;
	switch(NeoKey(key[0]))
	{
		case NeoKey::A: return NeoKey(key[1]) == NeoKey::B && NeoKey(key[2]) == NeoKey::C ? virtualControllerAssets.abc : virtualControllerAssets.a;
		case NeoKey::B: return virtualControllerAssets.b;
		case NeoKey::C: return virtualControllerAssets.c;
		case NeoKey::D: return virtualControllerAssets.d;
		case NeoKey::Select: return virtualControllerAssets.select;
		case NeoKey::Start: return virtualControllerAssets.start;
		case NeoKey::TestSwitch: return virtualControllerAssets.test;
		default: return virtualControllerAssets.blank;
	}
}

void NeoSystem::handleInputAction(EmuApp *, InputAction a)
{
	auto player = a.flags.deviceId;
	bool isPushed = a.state == Input::Action::PUSHED;
	auto neoKey = NeoKey(a.code);
	if(a.code <= 8) // joystick bit index
	{
		auto &p = player ? memory.intern_p2 : memory.intern_p1;
		// Don't permit simultaneous left + right input, locks up Metal Slug 3
		if(isPushed && neoKey == NeoKey::Left)
		{
			p |= bit(to_underlying(NeoKey::Right) - 1);
		}
		else if(isPushed && neoKey == NeoKey::Right)
		{
			p |= bit(to_underlying(NeoKey::Left) - 1);
		}
		p = setOrClearBits(p, Uint8((bit(a.code - 1))), !isPushed);
	}
	else if(neoKey == NeoKey::Select)
	{
		constexpr unsigned coin1Bit = bit(0), coin2Bit = bit(1),
			select1Bit = bit(1), select2Bit = bit(3);
		if(conf.system == SYS_ARCADE)
		{
			unsigned bits = player ? coin2Bit : coin1Bit;
			memory.intern_coin = setOrClearBits(memory.intern_coin, (Uint8)bits, !isPushed);
		}
		else
		{
			// convert COIN to SELECT
			unsigned bits = player ? select2Bit : select1Bit;
			memory.intern_start = setOrClearBits(memory.intern_start, (Uint8)bits, !isPushed);
		}
	}
	else if(neoKey == NeoKey::Start)
	{
		constexpr unsigned start1Bit = bit(0), start2Bit = bit(2);
		unsigned bits = player ? start2Bit : start1Bit;
		memory.intern_start = IG::setOrClearBits(memory.intern_start, (Uint8)bits, !isPushed);
	}
	else if(neoKey == NeoKey::TestSwitch)
	{
		if(isPushed)
			conf.test_switch = 1; // Test Switch is reset to 0 after every frame
		return;
	}
}

void NeoSystem::clearInputBuffers(EmuInputView &)
{
	memory.intern_coin = 0x7;
	memory.intern_start = 0x8F;
	memory.intern_p1 = 0xFF;
	memory.intern_p2 = 0xFF;
}

SystemInputDeviceDesc NeoSystem::inputDeviceDesc(int idx) const
{
	static constexpr std::array gamepadComponents
	{
		InputComponentDesc{"D-Pad", dpadKeyInfo, InputComponent::dPad, LB2DO},
		InputComponentDesc{"Face Buttons", faceKeyInfo, InputComponent::button, RB2DO, {.staggeredLayout = true}},
		InputComponentDesc{"Select", {&centerKeyInfo[0], 1}, InputComponent::button, LB2DO},
		InputComponentDesc{"Start", {&centerKeyInfo[1], 1}, InputComponent::button, RB2DO},
		InputComponentDesc{"Select/Start", centerKeyInfo, InputComponent::button, CB2DO, {.altConfig = true}},
	};

	static constexpr SystemInputDeviceDesc gamepadDesc{"Gamepad", gamepadComponents};

	return gamepadDesc;
}

}
