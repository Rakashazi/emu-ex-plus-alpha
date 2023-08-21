/*  This file is part of NES.emu.

	NES.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NES.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NES.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/keyRemappingUtils.hh>
#include <imagine/util/math/math.hh>
#include "MainSystem.hh"
#include "MainApp.hh"
#include <fceu/fceu.h>

namespace EmuEx
{

const int EmuSystem::maxPlayers = 4;

enum class NesKey : KeyCode
{
	Up = 5,
	Right = 8,
	Down = 6,
	Left = 7,
	Select = 3,
	Start = 4,
	A = 1,
	B = 2,
};

constexpr auto dpadKeyInfo = makeArray<KeyInfo>
(
	NesKey::Up,
	NesKey::Right,
	NesKey::Down,
	NesKey::Left
);

constexpr auto centerKeyInfo = makeArray<KeyInfo>
(
	NesKey::Select,
	NesKey::Start
);

constexpr auto faceKeyInfo = makeArray<KeyInfo>
(
	NesKey::B,
	NesKey::A
);

constexpr auto turboFaceKeyInfo = turbo(faceKeyInfo);

constexpr std::array comboKeyInfo{KeyInfo{std::array{NesKey::A, NesKey::B}}};

constexpr auto gpKeyInfo = concatToArrayNow<dpadKeyInfo, centerKeyInfo, faceKeyInfo, turboFaceKeyInfo, comboKeyInfo>;
constexpr auto gp2KeyInfo = transpose(gpKeyInfo, 1);
constexpr auto gp3KeyInfo = transpose(gpKeyInfo, 2);
constexpr auto gp4KeyInfo = transpose(gpKeyInfo, 3);

std::span<const KeyCategory> NesApp::keyCategories()
{
	static constexpr std::array categories
	{
		KeyCategory{"Gamepad", gpKeyInfo},
		KeyCategory{"Gamepad 2", gp2KeyInfo, 1},
		KeyCategory{"Gamepad 3", gp2KeyInfo, 2},
		KeyCategory{"Gamepad 4", gp2KeyInfo, 3},
	};
	return categories;
}

std::string_view NesApp::systemKeyCodeToString(KeyCode c)
{
	switch(NesKey(c))
	{
		case NesKey::Up: return "Up";
		case NesKey::Right: return "Right";
		case NesKey::Down: return "Down";
		case NesKey::Left: return "Left";
		case NesKey::Select: return "Select";
		case NesKey::Start: return "Start";
		case NesKey::A: return "A";
		case NesKey::B: return "B";
		default: return "";
	}
}

std::span<const KeyConfigDesc> NesApp::defaultKeyConfigs()
{
	using namespace IG::Input;

	static constexpr std::array pcKeyboardMap
	{
		KeyMapping{NesKey::Up, Keycode::UP},
		KeyMapping{NesKey::Right, Keycode::RIGHT},
		KeyMapping{NesKey::Down, Keycode::DOWN},
		KeyMapping{NesKey::Left, Keycode::LEFT},
		KeyMapping{NesKey::Select, Keycode::SPACE},
		KeyMapping{NesKey::Start, Keycode::ENTER},
		KeyMapping{NesKey::B, Keycode::Z},
		KeyMapping{NesKey::A, Keycode::X},
	};

	static constexpr std::array genericGamepadMap
	{
		KeyMapping{NesKey::Up, Keycode::UP},
		KeyMapping{NesKey::Right, Keycode::RIGHT},
		KeyMapping{NesKey::Down, Keycode::DOWN},
		KeyMapping{NesKey::Left, Keycode::LEFT},
		KeyMapping{NesKey::Select, Keycode::GAME_SELECT},
		KeyMapping{NesKey::Start, Keycode::GAME_START},
		KeyMapping{NesKey::B, Keycode::GAME_X},
		KeyMapping{NesKey::A, Keycode::GAME_A},
	};

	static constexpr std::array wiimoteMap
	{
		KeyMapping{NesKey::Up, Wiimote::UP},
		KeyMapping{NesKey::Right, Wiimote::RIGHT},
		KeyMapping{NesKey::Down, Wiimote::DOWN},
		KeyMapping{NesKey::Left, Wiimote::LEFT},
		KeyMapping{NesKey::B, Wiimote::_1},
		KeyMapping{NesKey::A, Wiimote::_2},
		KeyMapping{NesKey::Select, Wiimote::MINUS},
		KeyMapping{NesKey::Start, Wiimote::PLUS},
	};

	return genericKeyConfigs<pcKeyboardMap, genericGamepadMap, wiimoteMap>();
}

bool NesApp::allowsTurboModifier(KeyCode c)
{
	switch(NesKey(c))
	{
		case NesKey::A ... NesKey::B:
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

AssetDesc NesApp::vControllerAssetDesc(KeyInfo key) const
{
	static constexpr struct VirtualControllerAssets
	{
		AssetDesc dpad{AssetFileID::gamepadOverlay, gpImageCoords({{}, {4, 4}})},

		a{AssetFileID::gamepadOverlay,      gpImageCoords({{4, 0}, {2, 2}})},
		b{AssetFileID::gamepadOverlay,      gpImageCoords({{6, 0}, {2, 2}})},
		select{AssetFileID::gamepadOverlay, gpImageCoords({{4, 2}, {2, 1}}), {1, 2}},
		start{AssetFileID::gamepadOverlay,  gpImageCoords({{4, 3}, {2, 1}}), {1, 2}},
		ab{AssetFileID::gamepadOverlay,     gpImageCoords({{6, 2}, {2, 2}})},

		blank{AssetFileID::gamepadOverlay, gpImageCoords({{0, 4}, {2, 2}})};
	} virtualControllerAssets;

	if(key[0] == 0)
		return virtualControllerAssets.dpad;
	switch(NesKey(key[0]))
	{
		case NesKey::A: return NesKey(key[1]) == NesKey::B ? virtualControllerAssets.ab : virtualControllerAssets.a;
		case NesKey::B: return virtualControllerAssets.b;
		case NesKey::Select: return virtualControllerAssets.select;
		case NesKey::Start: return virtualControllerAssets.start;
		default: return virtualControllerAssets.blank;
	}
}

void NesSystem::connectNESInput(int port, ESI type)
{
	assert(GameInfo);
	if(type == SI_GAMEPAD)
	{
		//logMsg("gamepad to port %d", port);
		FCEUI_SetInput(port, SI_GAMEPAD, &padData, 0);
	}
	else if(type == SI_ZAPPER)
	{
		//logMsg("zapper to port %d", port);
		FCEUI_SetInput(port, SI_ZAPPER, &zapperData, 1);
	}
	else
	{
		FCEUI_SetInput(port, SI_NONE, 0, 0);
	}
}

static unsigned playerInputShift(int player)
{
	switch(player)
	{
		case 1: return 8;
		case 2: return 16;
		case 3: return 24;
	}
	return 0;
}

void NesSystem::handleInputAction(EmuApp *, InputAction a)
{
	int player = a.flags.deviceId;
	auto key = NesKey(a.code);
	auto gpBits = bit(a.code - 1);
	if(GameInfo->type == GIT_NSF && a.isPushed())
	{
		if(key == NesKey::Up)
			FCEUI_NSFChange(10);
		else if(key == NesKey::Down)
			FCEUI_NSFChange(-10);
		else if(key == NesKey::Right)
			FCEUI_NSFChange(1);
		else if(key == NesKey::Left)
			FCEUI_NSFChange(-1);
		else if(key == NesKey::B)
			FCEUI_NSFChange(0);
	}
	else if(GameInfo->type == GIT_VSUNI) // TODO: make coin insert separate key
	{
		if(a.isPushed() && key == NesKey::Start)
			FCEUI_VSUniCoin();
	}
	else if(GameInfo->inputfc == SIFC_HYPERSHOT)
	{
		if(auto hsKey = gpBits & 0x3;
			hsKey)
		{
			hsKey = hsKey == 0x3 ? 0x3 : hsKey ^ 0x3; // swap the 2 bits
			auto hsPlayerInputShift = player == 1 ? 3 : 1;
			fcExtData = IG::setOrClearBits(fcExtData, hsKey << hsPlayerInputShift, a.isPushed());
		}
	}
	padData = setOrClearBits(padData, gpBits << playerInputShift(player), a.isPushed());
}

bool NesSystem::onPointerInputStart(const Input::MotionEvent &e, Input::DragTrackerState, IG::WindowRect gameRect)
{
	if(!usingZapper)
		return false;
	zapperData[2] = 0;
	if(gameRect.overlaps(e.pos()))
	{
		int xRel = e.pos().x - gameRect.x, yRel = e.pos().y - gameRect.y;
		int xNes = IG::remap(xRel, 0, gameRect.xSize(), 0, 256);
		int yNes = IG::remap(yRel, 0, gameRect.ySize(), 0, 224) + 8;
		logMsg("zapper pushed @ %d,%d, on NES %d,%d", e.pos().x, e.pos().y, xNes, yNes);
		zapperData[0] = xNes;
		zapperData[1] = yNes;
		zapperData[2] |= 0x1;
	}
	else // off-screen shot
	{
		zapperData[0] = 0;
		zapperData[1] = 0;
		zapperData[2] |= 0x2;
	}
	return true;
}

bool NesSystem::onPointerInputEnd(const Input::MotionEvent &, Input::DragTrackerState, IG::WindowRect)
{
	if(!usingZapper)
		return false;
	zapperData[2] = 0;
	return true;
}

void NesSystem::clearInputBuffers(EmuInputView &)
{
	IG::fill(zapperData);
	padData = {};
	fcExtData = {};
}

SystemInputDeviceDesc NesSystem::inputDeviceDesc(int idx) const
{
	static constexpr std::array gamepadComponents
	{
		InputComponentDesc{"D-Pad", dpadKeyInfo, InputComponent::dPad, LB2DO},
		InputComponentDesc{"Face Buttons", faceKeyInfo, InputComponent::button, RB2DO},
		InputComponentDesc{"Select", {&centerKeyInfo[0], 1}, InputComponent::button, LB2DO},
		InputComponentDesc{"Start", {&centerKeyInfo[1], 1}, InputComponent::button, RB2DO},
		InputComponentDesc{"Select/Start", centerKeyInfo, InputComponent::button, CB2DO, {.altConfig = true}},
	};

	static constexpr SystemInputDeviceDesc gamepadDesc{"Gamepad", gamepadComponents};

	return gamepadDesc;
}

}

void GetMouseData(uint32 (&d)[3])
{
	// TODO
}
