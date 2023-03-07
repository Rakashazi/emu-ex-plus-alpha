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
#include "MainSystem.hh"
#include "MainApp.hh"

namespace EmuEx
{

enum
{
	ssKeyIdxUp = Controls::systemKeyMapStart,
	ssKeyIdxRight,
	ssKeyIdxDown,
	ssKeyIdxLeft,
	ssKeyIdxLeftUp,
	ssKeyIdxRightUp,
	ssKeyIdxRightDown,
	ssKeyIdxLeftDown,
	ssKeyIdxStart,
	ssKeyIdxA,
	ssKeyIdxB,
	ssKeyIdxC,
	ssKeyIdxX,
	ssKeyIdxY,
	ssKeyIdxZ,
	ssKeyIdxL,
	ssKeyIdxR,
	ssKeyIdxATurbo,
	ssKeyIdxBTurbo,
	ssKeyIdxCTurbo,
	ssKeyIdxXTurbo,
	ssKeyIdxYTurbo,
	ssKeyIdxZTurbo,
	ssKeyIdxLastGamepad = ssKeyIdxZTurbo
};

constexpr std::array<unsigned, 4> dpadButtonCodes
{
	ssKeyIdxUp,
	ssKeyIdxRight,
	ssKeyIdxDown,
	ssKeyIdxLeft,
};

constexpr unsigned centerButtonCodes[]{ssKeyIdxStart};

constexpr unsigned faceButtonCodes[]
{
	ssKeyIdxA,
	ssKeyIdxB,
	ssKeyIdxC,
	ssKeyIdxX,
	ssKeyIdxY,
	ssKeyIdxZ,
};

constexpr std::array gamepadComponents
{
	InputComponentDesc{"D-Pad", dpadButtonCodes, InputComponent::dPad, LB2DO},
	InputComponentDesc{"Face Buttons", faceButtonCodes, InputComponent::button, RB2DO},
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
	{AssetFileID::gamepadOverlay, gpImageCoords({{4, 2}, {2, 2}})}, // C
	{AssetFileID::gamepadOverlay, gpImageCoords({{6, 2}, {2, 2}})}, // X
	{AssetFileID::gamepadOverlay, gpImageCoords({{0, 4}, {2, 2}})}, // Y
	{AssetFileID::gamepadOverlay, gpImageCoords({{2, 4}, {2, 2}})}, // Z
	{AssetFileID::gamepadOverlay, gpImageCoords({{4, 4}, {2, 2}})}, // L
	{AssetFileID::gamepadOverlay, gpImageCoords({{6, 4}, {2, 2}})}, // R
	{AssetFileID::gamepadOverlay, gpImageCoords({{0, 6}, {2, 1}}), {1, 2}}, // start
};

AssetDesc SaturnApp::vControllerAssetDesc(unsigned key) const
{
	switch(key)
	{
		case 0: return virtualControllerAssets[0];
		case ssKeyIdxATurbo:
		case ssKeyIdxA: return virtualControllerAssets[1];
		case ssKeyIdxBTurbo:
		case ssKeyIdxB: return virtualControllerAssets[2];
		case ssKeyIdxCTurbo:
		case ssKeyIdxC: return virtualControllerAssets[3];
		case ssKeyIdxXTurbo:
		case ssKeyIdxX: return virtualControllerAssets[4];
		case ssKeyIdxYTurbo:
		case ssKeyIdxY: return virtualControllerAssets[5];
		case ssKeyIdxZTurbo:
		case ssKeyIdxZ: return virtualControllerAssets[6];
		case ssKeyIdxL: return virtualControllerAssets[7];
		case ssKeyIdxR: return virtualControllerAssets[8];
		case ssKeyIdxStart: return virtualControllerAssets[9];
		default: return virtualControllerAssets[1];
	}
}

const int EmuSystem::maxPlayers = 2;

InputAction SaturnSystem::translateInputAction(InputAction action)
{
	switch(action.key)
	{
		case ssKeyIdxXTurbo:
		case ssKeyIdxYTurbo:
		case ssKeyIdxZTurbo:
		case ssKeyIdxATurbo:
		case ssKeyIdxBTurbo:
		case ssKeyIdxCTurbo:
		case ssKeyIdxXTurbo + Controls::gamepadKeys:
		case ssKeyIdxYTurbo + Controls::gamepadKeys:
		case ssKeyIdxZTurbo + Controls::gamepadKeys:
		case ssKeyIdxATurbo + Controls::gamepadKeys:
		case ssKeyIdxBTurbo + Controls::gamepadKeys:
		case ssKeyIdxCTurbo + Controls::gamepadKeys:
			action.setTurboFlag(true); [[fallthrough]];
		default: return action;
	}
}

void SaturnSystem::handleInputAction(EmuApp *, InputAction a)
{
	unsigned player = 0;
	if(a.key > ssKeyIdxLastGamepad)
	{
		player = 1;
		a.key -= Controls::gamepadKeys;
	}
	PerPad_struct *p = (player == 1) ? pad[1] : pad[0];
	bool pushed = a.state == Input::Action::PUSHED;
	switch(a.key)
	{
		case ssKeyIdxUp: if(pushed) PerPadUpPressed(p); else PerPadUpReleased(p); break;
		case ssKeyIdxRight: if(pushed) PerPadRightPressed(p); else PerPadRightReleased(p); break;
		case ssKeyIdxDown: if(pushed) PerPadDownPressed(p); else PerPadDownReleased(p); break;
		case ssKeyIdxLeft: if(pushed) PerPadLeftPressed(p); else PerPadLeftReleased(p); break;
		case ssKeyIdxLeftUp: if(pushed) { PerPadLeftPressed(p); PerPadUpPressed(p); }
			else { PerPadLeftReleased(p); PerPadUpReleased(p); } break;
		case ssKeyIdxRightUp: if(pushed) { PerPadRightPressed(p); PerPadUpPressed(p); }
			else { PerPadRightReleased(p); PerPadUpReleased(p); } break;
		case ssKeyIdxRightDown: if(pushed) { PerPadRightPressed(p); PerPadDownPressed(p); }
			else { PerPadRightReleased(p); PerPadDownReleased(p); } break;
		case ssKeyIdxLeftDown: if(pushed) { PerPadLeftPressed(p); PerPadDownPressed(p); }
			else { PerPadLeftReleased(p); PerPadDownReleased(p); } break;
		case ssKeyIdxStart: if(pushed) PerPadStartPressed(p); else PerPadStartReleased(p); break;
		case ssKeyIdxXTurbo:
		case ssKeyIdxX: if(pushed) PerPadXPressed(p); else PerPadXReleased(p); break;
		case ssKeyIdxYTurbo:
		case ssKeyIdxY: if(pushed) PerPadYPressed(p); else PerPadYReleased(p); break;
		case ssKeyIdxZTurbo:
		case ssKeyIdxZ: if(pushed) PerPadZPressed(p); else PerPadZReleased(p); break;
		case ssKeyIdxATurbo:
		case ssKeyIdxA: if(pushed) PerPadAPressed(p); else PerPadAReleased(p); break;
		case ssKeyIdxBTurbo:
		case ssKeyIdxB: if(pushed) PerPadBPressed(p); else PerPadBReleased(p); break;
		case ssKeyIdxCTurbo:
		case ssKeyIdxC: if(pushed) PerPadCPressed(p); else PerPadCReleased(p); break;
		case ssKeyIdxL: if(pushed) PerPadLTriggerPressed(p); else PerPadLTriggerReleased(p); break;
		case ssKeyIdxR: if(pushed) PerPadRTriggerPressed(p); else PerPadRTriggerReleased(p); break;
		default: bug_unreachable("input == %d", a.key);
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
	return gamepadDesc;
}

}
