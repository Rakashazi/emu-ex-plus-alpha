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
#include <imagine/util/format.hh>
#include "MainSystem.hh"
#include "MainApp.hh"
#include <vbam/gba/GBA.h>

namespace EmuEx
{

using namespace IG;

enum
{
	gbaKeyIdxUp = Controls::systemKeyMapStart,
	gbaKeyIdxRight,
	gbaKeyIdxDown,
	gbaKeyIdxLeft,
	gbaKeyIdxLeftUp,
	gbaKeyIdxRightUp,
	gbaKeyIdxRightDown,
	gbaKeyIdxLeftDown,
	gbaKeyIdxSelect,
	gbaKeyIdxStart,
	gbaKeyIdxA,
	gbaKeyIdxB,
	gbaKeyIdxL,
	gbaKeyIdxR,
	gbaKeyIdxATurbo,
	gbaKeyIdxBTurbo,
	gbaKeyIdxAB,
	gbaKeyIdxRB,
	gbaKeyIdxLightInc,
	gbaKeyIdxLightDec,
};

constexpr std::array<unsigned, 4> dpadButtonCodes
{
	gbaKeyIdxUp,
	gbaKeyIdxRight,
	gbaKeyIdxDown,
	gbaKeyIdxLeft,
};

constexpr unsigned centerButtonCodes[]
{
	gbaKeyIdxSelect,
	gbaKeyIdxStart,
};

constexpr unsigned faceButtonCodes[]
{
	gbaKeyIdxB,
	gbaKeyIdxA,
};

constexpr unsigned faceButtonLRCodes[]
{
	gbaKeyIdxB,
	gbaKeyIdxL,
	gbaKeyIdxA,
	gbaKeyIdxR,
};

constexpr unsigned lButtonCode[]{gbaKeyIdxL};
constexpr unsigned rButtonCode[]{gbaKeyIdxR};

constexpr std::array gamepadComponents
{
	InputComponentDesc{"D-Pad", dpadButtonCodes, InputComponent::dPad, LB2DO},
	InputComponentDesc{"Face Buttons", faceButtonCodes, InputComponent::button, RB2DO},
	InputComponentDesc{"Face Buttons + Inline L/R", faceButtonLRCodes, InputComponent::button, RB2DO, InputComponentFlagsMask::altConfig},
	InputComponentDesc{"L", lButtonCode, InputComponent::trigger, LB2DO},
	InputComponentDesc{"R", rButtonCode, InputComponent::trigger, RB2DO},
	InputComponentDesc{"Select", {&centerButtonCodes[0], 1}, InputComponent::button, LB2DO},
	InputComponentDesc{"Start", {&centerButtonCodes[1], 1}, InputComponent::button, RB2DO},
	InputComponentDesc{"Select/Start", centerButtonCodes, InputComponent::button, CB2DO, InputComponentFlagsMask::altConfig},
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
	{AssetFileID::gamepadOverlay, gpImageCoords({{4, 2}, {2, 2}})}, // AB
	{AssetFileID::gamepadOverlay, gpImageCoords({{2, 4}, {2, 2}})}, // RB
	{AssetFileID::gamepadOverlay, gpImageCoords({{4, 4}, {2, 2}})}, // L
	{AssetFileID::gamepadOverlay, gpImageCoords({{6, 4}, {2, 2}})}, // R
	{AssetFileID::gamepadOverlay, gpImageCoords({{0, 6}, {2, 1}}), {1, 2}}, // select
	{AssetFileID::gamepadOverlay, gpImageCoords({{0, 7}, {2, 1}}), {1, 2}}, // start

	// functions
	{AssetFileID::gamepadOverlay, gpImageCoords({{2, 6}, {2, 1}}), {1, 2}}, // light sensor +
	{AssetFileID::gamepadOverlay, gpImageCoords({{2, 7}, {2, 1}}), {1, 2}}, // light sensor -
};

AssetDesc GbaApp::vControllerAssetDesc(unsigned key) const
{
	switch(key)
	{
		case 0: return virtualControllerAssets[0];
		case gbaKeyIdxATurbo:
		case gbaKeyIdxA: return virtualControllerAssets[1];
		case gbaKeyIdxBTurbo:
		case gbaKeyIdxB: return virtualControllerAssets[2];
		case gbaKeyIdxAB: return virtualControllerAssets[3];
		case gbaKeyIdxRB: return virtualControllerAssets[4];
		case gbaKeyIdxL: return virtualControllerAssets[5];
		case gbaKeyIdxR: return virtualControllerAssets[6];
		case gbaKeyIdxSelect: return virtualControllerAssets[7];
		case gbaKeyIdxStart: return virtualControllerAssets[8];
		case gbaKeyIdxLightInc: return virtualControllerAssets[9];
		case gbaKeyIdxLightDec: return virtualControllerAssets[10];
		default: return virtualControllerAssets[1];
	}
}

const int EmuSystem::maxPlayers = 1;
constexpr int gbaKeypadBits = 10;
constexpr unsigned gbaKeypadMask = 0x3FF;

enum ActionBits : unsigned
{
	A = bit(0),
	B = bit(1),
	SELECT = bit(2),
	START = bit(3),
	RIGHT = bit(4),
	LEFT = bit(5),
	UP = bit(6),
	DOWN = bit(7),
	R = bit(8),
	L = bit(9),
};

constexpr unsigned lightIncKey = 1;
constexpr unsigned lightDecKey = 2;

static bool isGamepadButton(unsigned input)
{
	switch(input)
	{
		case gbaKeyIdxSelect:
		case gbaKeyIdxStart:
		case gbaKeyIdxATurbo:
		case gbaKeyIdxA:
		case gbaKeyIdxBTurbo:
		case gbaKeyIdxB:
		case gbaKeyIdxL:
		case gbaKeyIdxR:
			return true;
		default: return false;
	}
}

InputAction GbaSystem::translateInputAction(InputAction action)
{
	if(!isGamepadButton(action.key))
		action.setTurboFlag(false);
	action.key = [&] -> unsigned
	{
		switch(action.key)
		{
			case gbaKeyIdxUp: return UP;
			case gbaKeyIdxRight: return RIGHT;
			case gbaKeyIdxDown: return DOWN;
			case gbaKeyIdxLeft: return LEFT;
			case gbaKeyIdxLeftUp: return UP | LEFT;
			case gbaKeyIdxRightUp: return UP | RIGHT;
			case gbaKeyIdxRightDown: return DOWN | RIGHT;
			case gbaKeyIdxLeftDown: return DOWN | LEFT;
			case gbaKeyIdxSelect: return SELECT;
			case gbaKeyIdxStart: return START;
			case gbaKeyIdxATurbo: action.setTurboFlag(true); [[fallthrough]];
			case gbaKeyIdxA: return A;
			case gbaKeyIdxBTurbo: action.setTurboFlag(true); [[fallthrough]];
			case gbaKeyIdxB: return B;
			case gbaKeyIdxL: return L;
			case gbaKeyIdxR: return R;
			case gbaKeyIdxAB: return A | B;
			case gbaKeyIdxRB: return R | B;
			case gbaKeyIdxLightInc: return lightIncKey << gbaKeypadBits;
			case gbaKeyIdxLightDec: return lightDecKey << gbaKeypadBits;
		}
		bug_unreachable("invalid key");
	}();
	return action;
}

void GbaSystem::handleInputAction(EmuApp *app, InputAction a)
{
	if(auto exKey = a.key >> gbaKeypadBits;
		exKey)
	{
		if(a.state == Input::Action::PUSHED && (exKey == lightIncKey || exKey == lightDecKey))
		{
			int darknessChange = exKey == lightDecKey ? 17 : -17;
			darknessLevel = std::clamp(darknessLevel + darknessChange, 0, 0xff);
			if(app)
			{
				app->postMessage(1, false, fmt::format("Light sensor level: {}%", IG::remap(darknessLevel, 0xff, 0, 0, 100)));
			}
		}
	}
	else
	{
		P1 = IG::setOrClearBits(P1, uint16_t(a.key & gbaKeypadMask), a.state != Input::Action::PUSHED);
	}
}

void GbaSystem::clearInputBuffers(EmuInputView &)
{
	P1 = 0x03FF;
	clearSensorValues();
}

SystemInputDeviceDesc GbaSystem::inputDeviceDesc(int idx) const
{
	return gamepadDesc;
}

}
