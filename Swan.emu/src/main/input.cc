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
#include "MainSystem.hh"
#include "MainApp.hh"

namespace EmuEx
{

enum
{
	wsKeyIdxUp = Controls::systemKeyMapStart,
	wsKeyIdxRight,
	wsKeyIdxDown,
	wsKeyIdxLeft,
	wsKeyIdxLeftUp,
	wsKeyIdxRightUp,
	wsKeyIdxRightDown,
	wsKeyIdxLeftDown,
	wsKeyIdxStart,
	wsKeyIdxA,
	wsKeyIdxB,
	wsKeyIdxY1,
	wsKeyIdxY2,
	wsKeyIdxY3,
	wsKeyIdxY4,
	wsKeyIdxATurbo,
	wsKeyIdxBTurbo,
	wsKeyIdxY1Turbo,
	wsKeyIdxY2Turbo,
	wsKeyIdxY3Turbo,
	wsKeyIdxY4Turbo,
	wsKeyIdxANoRotation,
	wsKeyIdxBNoRotation,
	wsKeyIdxY1X1,
	wsKeyIdxY2X2,
	wsKeyIdxY3X3,
	wsKeyIdxY4X4,
};

constexpr std::array<unsigned, 4> dpadButtonCodes
{
	wsKeyIdxUp,
	wsKeyIdxRight,
	wsKeyIdxDown,
	wsKeyIdxLeft,
};

constexpr unsigned centerButtonCodes[]{wsKeyIdxStart};

constexpr unsigned faceButtonCodes[]
{
	wsKeyIdxBNoRotation,
	wsKeyIdxANoRotation,
};

constexpr unsigned oppositeDPadButtonCodes[]
{
	wsKeyIdxY4X4,
	wsKeyIdxY3X3,
	wsKeyIdxY1X1,
	wsKeyIdxY2X2,
};

constexpr unsigned faceButtonCombinedCodes[]
{
	wsKeyIdxBNoRotation,
	wsKeyIdxANoRotation,
	wsKeyIdxY4X4,
	wsKeyIdxY3X3,
	wsKeyIdxY1X1,
	wsKeyIdxY2X2,
};

constexpr std::array gamepadComponents
{
	InputComponentDesc{"D-Pad", dpadButtonCodes, InputComponent::dPad, LB2DO},
	InputComponentDesc{"Face Buttons + Opposite D-Pad Buttons", faceButtonCombinedCodes, InputComponent::button, RB2DO, InputComponentFlagsMask::rowSize2},
	InputComponentDesc{"Face Buttons", faceButtonCodes, InputComponent::button, RB2DO, InputComponentFlagsMask::altConfig},
	InputComponentDesc{"Opposite D-Pad Buttons", oppositeDPadButtonCodes, InputComponent::button, RB2DO, InputComponentFlagsMask::altConfig | InputComponentFlagsMask::staggeredLayout},
	InputComponentDesc{"Start", centerButtonCodes, InputComponent::button, RB2DO},
};

constexpr SystemInputDeviceDesc gamepadDesc{"Gamepad", gamepadComponents};

constexpr FRect gpImageCoords(IRect cellRelBounds)
{
	constexpr FP imageSize{256, 256};
	constexpr int cellSize = 32;
	return (cellRelBounds.relToAbs() * cellSize).as<float>() / imageSize;
}

constexpr AssetDesc virtualControllerAssets[]
{
	// d-pad
	{AssetFileID::gamepadOverlay, gpImageCoords({{}, {4, 4}})},

	// gamepad buttons
	{AssetFileID::gamepadOverlay, gpImageCoords({{4, 0}, {2, 2}})}, // A
	{AssetFileID::gamepadOverlay, gpImageCoords({{6, 0}, {2, 2}})}, // B
	{AssetFileID::gamepadOverlay, gpImageCoords({{4, 2}, {2, 2}})}, // 1
	{AssetFileID::gamepadOverlay, gpImageCoords({{6, 2}, {2, 2}})}, // 2
	{AssetFileID::gamepadOverlay, gpImageCoords({{0, 4}, {2, 2}})}, // 3
	{AssetFileID::gamepadOverlay, gpImageCoords({{2, 4}, {2, 2}})}, // 4
	{AssetFileID::gamepadOverlay, gpImageCoords({{0, 6}, {2, 1}}), {1, 2}}, // start
};

AssetDesc WsApp::vControllerAssetDesc(unsigned key) const
{
	switch(key)
	{
		case 0: return virtualControllerAssets[0];
		case wsKeyIdxANoRotation:
		case wsKeyIdxATurbo:
		case wsKeyIdxA: return virtualControllerAssets[1];
		case wsKeyIdxBNoRotation:
		case wsKeyIdxBTurbo:
		case wsKeyIdxB: return virtualControllerAssets[2];
		case wsKeyIdxY1X1:
		case wsKeyIdxY1Turbo:
		case wsKeyIdxY1: return virtualControllerAssets[3];
		case wsKeyIdxY2X2:
		case wsKeyIdxY2Turbo:
		case wsKeyIdxY2: return virtualControllerAssets[4];
		case wsKeyIdxY3X3:
		case wsKeyIdxY3Turbo:
		case wsKeyIdxY3: return virtualControllerAssets[5];
		case wsKeyIdxY4X4:
		case wsKeyIdxY4Turbo:
		case wsKeyIdxY4: return virtualControllerAssets[6];
		case wsKeyIdxStart: return virtualControllerAssets[7];
		default: return virtualControllerAssets[1];
	}
}

const int EmuSystem::maxPlayers = 1;

enum KeypadMask: unsigned
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

static bool isGamepadButton(unsigned input)
{
	switch(input)
	{
		case wsKeyIdxA ... wsKeyIdxY4X4:
			return true;
		default: return false;
	}
}

InputAction WsSystem::translateInputAction(InputAction action)
{
	if(!isGamepadButton(action.key))
		action.setTurboFlag(false);
	action.key = [&] -> unsigned
	{
		if(isRotated())
		{
			switch(action.key)
			{
				case wsKeyIdxUp: return Y2_BIT;
				case wsKeyIdxRight: return Y3_BIT;
				case wsKeyIdxDown: return Y4_BIT;
				case wsKeyIdxLeft: return Y1_BIT;
				case wsKeyIdxLeftUp: return Y1_BIT | Y2_BIT;
				case wsKeyIdxRightUp: return Y3_BIT | Y2_BIT;
				case wsKeyIdxRightDown: return Y3_BIT | Y4_BIT;
				case wsKeyIdxLeftDown: return Y1_BIT | Y4_BIT;
				case wsKeyIdxY1Turbo: action.setTurboFlag(true); [[fallthrough]];
				case wsKeyIdxY1: return B_BIT;
				case wsKeyIdxY2Turbo: action.setTurboFlag(true); [[fallthrough]];
				case wsKeyIdxY2: return A_BIT;
				case wsKeyIdxY3Turbo: action.setTurboFlag(true); [[fallthrough]];
				case wsKeyIdxY3: return X3_BIT;
				case wsKeyIdxY4Turbo: action.setTurboFlag(true); [[fallthrough]];
				case wsKeyIdxY4: return X2_BIT;
				case wsKeyIdxStart: return START_BIT;
				case wsKeyIdxATurbo: action.setTurboFlag(true); [[fallthrough]];
				case wsKeyIdxA: return X4_BIT;
				case wsKeyIdxBTurbo: action.setTurboFlag(true); [[fallthrough]];
				case wsKeyIdxB: return X1_BIT;
				case wsKeyIdxANoRotation: return A_BIT;
				case wsKeyIdxBNoRotation: return B_BIT;
				case wsKeyIdxY1X1: return X1_BIT;
				case wsKeyIdxY2X2: return X2_BIT;
				case wsKeyIdxY3X3: return X3_BIT;
				case wsKeyIdxY4X4: return X4_BIT;
			}
		}
		else
		{
			switch(action.key)
			{
				case wsKeyIdxUp: return X1_BIT;
				case wsKeyIdxRight: return X2_BIT;
				case wsKeyIdxDown: return X3_BIT;
				case wsKeyIdxLeft: return X4_BIT;
				case wsKeyIdxLeftUp: return X4_BIT | X1_BIT;
				case wsKeyIdxRightUp: return X2_BIT | X1_BIT;
				case wsKeyIdxRightDown: return X2_BIT | X3_BIT;
				case wsKeyIdxLeftDown: return X4_BIT | X3_BIT;
				case wsKeyIdxY1Turbo: action.setTurboFlag(true); [[fallthrough]];
				case wsKeyIdxY1: return Y1_BIT;
				case wsKeyIdxY2Turbo: action.setTurboFlag(true); [[fallthrough]];
				case wsKeyIdxY2: return Y2_BIT;
				case wsKeyIdxY3Turbo: action.setTurboFlag(true); [[fallthrough]];
				case wsKeyIdxY3: return Y3_BIT;
				case wsKeyIdxY4Turbo: action.setTurboFlag(true); [[fallthrough]];
				case wsKeyIdxY4: return Y4_BIT;
				case wsKeyIdxStart: return START_BIT;
				case wsKeyIdxATurbo: action.setTurboFlag(true); [[fallthrough]];
				case wsKeyIdxA: return A_BIT;
				case wsKeyIdxBTurbo: action.setTurboFlag(true); [[fallthrough]];
				case wsKeyIdxB: return B_BIT;
				case wsKeyIdxANoRotation: return A_BIT;
				case wsKeyIdxBNoRotation: return B_BIT;
				case wsKeyIdxY1X1: return Y1_BIT;
				case wsKeyIdxY2X2: return Y2_BIT;
				case wsKeyIdxY3X3: return Y3_BIT;
				case wsKeyIdxY4X4: return Y4_BIT;
			}
		}
		bug_unreachable("invalid key");
	}();
	return action;
}

void WsSystem::handleInputAction(EmuApp *, InputAction a)
{
	inputBuff = IG::setOrClearBits(inputBuff, (uint16_t)a.key, a.state == Input::Action::PUSHED);
}

void WsSystem::clearInputBuffers(EmuInputView &)
{
	inputBuff = {};
}

void WsSystem::setupInput(EmuApp &app)
{
	if(isRotated())
	{
		if(showVGamepadABWhenVertical)
			app.unsetDisabledInputKeys();
		else
			app.setDisabledInputKeys(faceButtonCodes);
	}
	else
	{
		if(showVGamepadYWhenHorizonal)
			app.unsetDisabledInputKeys();
		else
			app.setDisabledInputKeys(oppositeDPadButtonCodes);
	}
}

SystemInputDeviceDesc WsSystem::inputDeviceDesc(int idx) const
{
	return gamepadDesc;
}

}
