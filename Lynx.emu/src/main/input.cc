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
#include "MainSystem.hh"
#include "MainApp.hh"

void Lynx_SetButtonData(uint32 data);

namespace EmuEx
{

enum
{
	lynxKeyIdxUp = Controls::systemKeyMapStart,
	lynxKeyIdxRight,
	lynxKeyIdxDown,
	lynxKeyIdxLeft,
	lynxKeyIdxLeftUp,
	lynxKeyIdxRightUp,
	lynxKeyIdxRightDown,
	lynxKeyIdxLeftDown,
	lynxKeyIdxOption1,
	lynxKeyIdxOption2,
	lynxKeyIdxPause,
	lynxKeyIdxA,
	lynxKeyIdxB,
	lynxKeyIdxATurbo,
	lynxKeyIdxBTurbo,
	lynxKeyIdxAB,
};

constexpr std::array<unsigned, 4> dpadButtonCodes
{
	lynxKeyIdxUp,
	lynxKeyIdxRight,
	lynxKeyIdxDown,
	lynxKeyIdxLeft,
};

constexpr unsigned optionButtonCodes[]
{
	lynxKeyIdxOption1,
	lynxKeyIdxOption2,
};

constexpr unsigned pauseButtonCode[]{lynxKeyIdxPause};

constexpr unsigned faceButtonCodes[]
{
	lynxKeyIdxB,
	lynxKeyIdxA,
};

constexpr std::array gamepadComponents
{
	InputComponentDesc{"D-Pad", dpadButtonCodes, InputComponent::dPad, LB2DO},
	InputComponentDesc{"Face Buttons", faceButtonCodes, InputComponent::button, RB2DO},
	InputComponentDesc{"Option Buttons", optionButtonCodes, InputComponent::button, LB2DO},
	InputComponentDesc{"Pause", pauseButtonCode, InputComponent::button,RB2DO},
};

constexpr SystemInputDeviceDesc gamepadDesc{"Gamepad", gamepadComponents};

constexpr FRect gpImageCoords(IRect cellRelBounds)
{
	constexpr FP imageSize{256, 256};
	constexpr int cellSize = 32;
	return (cellRelBounds.relToAbs() * cellSize).as<float>() / imageSize;
}

constexpr struct VirtualControllerAssets
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

AssetDesc LynxApp::vControllerAssetDesc(unsigned key) const
{
	switch(key)
	{
		case 0: return virtualControllerAssets.dpad;
		case lynxKeyIdxATurbo:
		case lynxKeyIdxA: return virtualControllerAssets.a;
		case lynxKeyIdxBTurbo:
		case lynxKeyIdxB: return virtualControllerAssets.b;
		case lynxKeyIdxAB: return virtualControllerAssets.ab;
		case lynxKeyIdxPause: return virtualControllerAssets.pause;
		case lynxKeyIdxOption1: return virtualControllerAssets.option1;
		case lynxKeyIdxOption2: return virtualControllerAssets.option2;
		default: return virtualControllerAssets.blank;
	}
}

const int EmuSystem::maxPlayers = 1;

enum KeypadMask: unsigned
{
	PAUSE_BIT = bit(8),
	UP_BIT = bit(6),
	DOWN_BIT = bit(7),
	LEFT_BIT = bit(4),
	RIGHT_BIT = bit(5),
	OPTION1_BIT = bit(3),
	OPTION2_BIT = bit(2),
	B_BIT = bit(1),
	A_BIT = bit(0),
};

static bool isGamepadButton(unsigned input)
{
	switch(input)
	{
		case lynxKeyIdxA ... lynxKeyIdxB:
			return true;
		default: return false;
	}
}

static unsigned rotateDPadKeycode(unsigned key, Rotation rotation)
{
	if(rotation == Rotation::UP)
		return key;
	switch(key)
	{
		case UP_BIT: return rotation == Rotation::RIGHT ? RIGHT_BIT : LEFT_BIT;
		case RIGHT_BIT: return rotation == Rotation::RIGHT ? DOWN_BIT : UP_BIT;
		case DOWN_BIT: return rotation == Rotation::RIGHT ? LEFT_BIT : RIGHT_BIT;
		case LEFT_BIT: return rotation == Rotation::RIGHT ? UP_BIT : DOWN_BIT;
	}
	bug_unreachable("invalid key");
}

unsigned LynxSystem::rotateDPadKeycode(unsigned key) const
{
	return EmuEx::rotateDPadKeycode(key, contentRotation());
}

InputAction LynxSystem::translateInputAction(InputAction action)
{
	if(!isGamepadButton(action.key))
		action.setTurboFlag(false);
	action.key = [&] -> unsigned
	{
		switch(action.key)
		{
			case lynxKeyIdxUp: return rotateDPadKeycode(UP_BIT);
			case lynxKeyIdxRight: return rotateDPadKeycode(RIGHT_BIT);
			case lynxKeyIdxDown: return rotateDPadKeycode(DOWN_BIT);
			case lynxKeyIdxLeft: return rotateDPadKeycode(LEFT_BIT);
			case lynxKeyIdxLeftUp: return rotateDPadKeycode(LEFT_BIT) | rotateDPadKeycode(UP_BIT);
			case lynxKeyIdxRightUp: return rotateDPadKeycode(RIGHT_BIT) | rotateDPadKeycode(UP_BIT);
			case lynxKeyIdxRightDown: return rotateDPadKeycode(RIGHT_BIT) | rotateDPadKeycode(DOWN_BIT);
			case lynxKeyIdxLeftDown: return rotateDPadKeycode(LEFT_BIT) | rotateDPadKeycode(DOWN_BIT);
			case lynxKeyIdxOption1: return OPTION1_BIT;
			case lynxKeyIdxOption2: return OPTION2_BIT;
			case lynxKeyIdxPause: return PAUSE_BIT;
			case lynxKeyIdxATurbo: action.setTurboFlag(true); [[fallthrough]];
			case lynxKeyIdxA: return A_BIT;
			case lynxKeyIdxBTurbo: action.setTurboFlag(true); [[fallthrough]];
			case lynxKeyIdxB: return B_BIT;
			case lynxKeyIdxAB: return A_BIT | B_BIT;
		}
		bug_unreachable("invalid key");
	}();
	return action;
}

void LynxSystem::handleInputAction(EmuApp *, InputAction a)
{
	inputBuff = IG::setOrClearBits(inputBuff, (uint16_t)a.key, a.state == Input::Action::PUSHED);
	Lynx_SetButtonData(inputBuff);
}

void LynxSystem::clearInputBuffers(EmuInputView &)
{
	inputBuff = {};
	Lynx_SetButtonData(0);
}

SystemInputDeviceDesc LynxSystem::inputDeviceDesc(int idx) const
{
	return gamepadDesc;
}

}
