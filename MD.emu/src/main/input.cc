/*  This file is part of MD.emu.

	MD.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MD.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MD.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include <imagine/util/math/math.hh>
#include "MainSystem.hh"
#include "MainApp.hh"
#include "input.h"
#include "system.h"
#include "loadrom.h"
#include "md_cart.h"
#include "io_ctrl.h"

namespace EmuEx
{

enum
{
	mdKeyIdxUp = Controls::systemKeyMapStart,
	mdKeyIdxRight,
	mdKeyIdxDown,
	mdKeyIdxLeft,
	mdKeyIdxLeftUp,
	mdKeyIdxRightUp,
	mdKeyIdxRightDown,
	mdKeyIdxLeftDown,
	mdKeyIdxMode,
	mdKeyIdxStart,
	mdKeyIdxA,
	mdKeyIdxB,
	mdKeyIdxC,
	mdKeyIdxX,
	mdKeyIdxY,
	mdKeyIdxZ,
	mdKeyIdxATurbo,
	mdKeyIdxBTurbo,
	mdKeyIdxCTurbo,
	mdKeyIdxXTurbo,
	mdKeyIdxYTurbo,
	mdKeyIdxZTurbo
};

constexpr std::array<unsigned, 4> dpadButtonCodes
{
	mdKeyIdxUp,
	mdKeyIdxRight,
	mdKeyIdxDown,
	mdKeyIdxLeft,
};

constexpr unsigned centerButtonCodes[]
{
	mdKeyIdxMode,
	mdKeyIdxStart,
};

constexpr unsigned faceButtonCodes[]
{
	mdKeyIdxA,
	mdKeyIdxB,
	mdKeyIdxC,
	mdKeyIdxX,
	mdKeyIdxY,
	mdKeyIdxZ,
};

constexpr std::array gamepadComponents
{
	InputComponentDesc{"D-Pad", dpadButtonCodes, InputComponent::dPad, LB2DO},
	InputComponentDesc{"Face Buttons", faceButtonCodes, InputComponent::button, RB2DO},
	InputComponentDesc{"Mode", {&centerButtonCodes[0], 1}, InputComponent::button, LB2DO},
	InputComponentDesc{"Start", {&centerButtonCodes[1], 1}, InputComponent::button, RB2DO},
	InputComponentDesc{"Mode/Start", centerButtonCodes, InputComponent::button, CB2DO, InputComponentFlagsMask::altConfig},
};

constexpr SystemInputDeviceDesc gamepadDesc{"Gamepad", gamepadComponents};

constexpr FRect gpImageCoords(IRect cellRelBounds)
{
	constexpr F2Size imageSize{256, 256};
	constexpr int cellSize = 32;
	return (cellRelBounds.relToAbs() * cellSize).as<float>() / imageSize;
}

constexpr struct VirtualControllerAssets
{
	AssetDesc dpad{AssetFileID::gamepadOverlay, gpImageCoords({{}, {4, 4}})},

	a{AssetFileID::gamepadOverlay,     gpImageCoords({{4, 0}, {2, 2}})},
	b{AssetFileID::gamepadOverlay,     gpImageCoords({{6, 0}, {2, 2}})},
	c{AssetFileID::gamepadOverlay,     gpImageCoords({{4, 2}, {2, 2}})},
	x{AssetFileID::gamepadOverlay,     gpImageCoords({{6, 2}, {2, 2}})},
	y{AssetFileID::gamepadOverlay,     gpImageCoords({{0, 4}, {2, 2}})},
	z{AssetFileID::gamepadOverlay,     gpImageCoords({{2, 4}, {2, 2}})},
	mode{AssetFileID::gamepadOverlay,  gpImageCoords({{0, 6}, {2, 1}}), {1, 2}},
	start{AssetFileID::gamepadOverlay, gpImageCoords({{0, 7}, {2, 1}}), {1, 2}},

	blank{AssetFileID::gamepadOverlay, gpImageCoords({{4, 4}, {2, 2}})};
} virtualControllerAssets;

AssetDesc MdApp::vControllerAssetDesc(unsigned key) const
{
	switch(key)
	{
		case 0: return virtualControllerAssets.dpad;
		case mdKeyIdxATurbo:
		case mdKeyIdxA: return virtualControllerAssets.a;
		case mdKeyIdxBTurbo:
		case mdKeyIdxB: return virtualControllerAssets.b;
		case mdKeyIdxCTurbo:
		case mdKeyIdxC: return virtualControllerAssets.c;
		case mdKeyIdxXTurbo:
		case mdKeyIdxX: return virtualControllerAssets.x;
		case mdKeyIdxYTurbo:
		case mdKeyIdxY: return virtualControllerAssets.y;
		case mdKeyIdxZTurbo:
		case mdKeyIdxZ: return virtualControllerAssets.z;
		case mdKeyIdxMode: return virtualControllerAssets.mode;
		case mdKeyIdxStart: return virtualControllerAssets.start;
		default: return virtualControllerAssets.blank;
	}
}

const int EmuSystem::maxPlayers = 4;

constexpr unsigned m3MissingCodes[]{mdKeyIdxMode, mdKeyIdxA, mdKeyIdxX, mdKeyIdxY, mdKeyIdxZ};
constexpr unsigned md6BtnExtraCodes[]{mdKeyIdxMode, mdKeyIdxX, mdKeyIdxY, mdKeyIdxZ};

static bool isGamepadButton(unsigned input)
{
	switch(input)
	{
		case mdKeyIdxMode:
		case mdKeyIdxStart:
		case mdKeyIdxATurbo:
		case mdKeyIdxA:
		case mdKeyIdxBTurbo:
		case mdKeyIdxB:
		case mdKeyIdxCTurbo:
		case mdKeyIdxC:
		case mdKeyIdxXTurbo:
		case mdKeyIdxX:
		case mdKeyIdxYTurbo:
		case mdKeyIdxY:
		case mdKeyIdxZTurbo:
		case mdKeyIdxZ:
			return true;
		default: return false;
	}
}

InputAction MdSystem::translateInputAction(InputAction action)
{
	if(!isGamepadButton(action.key))
		action.setTurboFlag(false);
	assert(action.key >= mdKeyIdxUp);
	unsigned player = (action.key - mdKeyIdxUp) / Controls::gamepadKeys;
	unsigned playerMask = player << 30;
	action.key -= Controls::gamepadKeys * player;
	if((input.system[1] == SYSTEM_MENACER && (action.key == mdKeyIdxB || action.key == mdKeyIdxC)) ||
		(input.system[1] == SYSTEM_JUSTIFIER && (action.key == mdKeyIdxStart)))
	{
		playerMask = 1 << 30;
	}
	action.key = [&] -> unsigned
	{
		switch(action.key)
		{
			case mdKeyIdxUp: return INPUT_UP | playerMask;
			case mdKeyIdxRight: return INPUT_RIGHT | playerMask;
			case mdKeyIdxDown: return INPUT_DOWN | playerMask;
			case mdKeyIdxLeft: return INPUT_LEFT | playerMask;
			case mdKeyIdxLeftUp: return INPUT_LEFT | INPUT_UP | playerMask;
			case mdKeyIdxRightUp: return INPUT_RIGHT | INPUT_UP | playerMask;
			case mdKeyIdxRightDown: return INPUT_RIGHT | INPUT_DOWN | playerMask;
			case mdKeyIdxLeftDown: return INPUT_LEFT | INPUT_DOWN | playerMask;
			case mdKeyIdxMode: return INPUT_MODE | playerMask;
			case mdKeyIdxStart: return INPUT_START | playerMask;
			case mdKeyIdxATurbo: action.setTurboFlag(true); [[fallthrough]];
			case mdKeyIdxA: return INPUT_A | playerMask;
			case mdKeyIdxBTurbo: action.setTurboFlag(true); [[fallthrough]];
			case mdKeyIdxB: return INPUT_B | playerMask;
			case mdKeyIdxCTurbo: action.setTurboFlag(true); [[fallthrough]];
			case mdKeyIdxC: return INPUT_C | playerMask;
			case mdKeyIdxXTurbo: action.setTurboFlag(true); [[fallthrough]];
			case mdKeyIdxX: return INPUT_X | playerMask;
			case mdKeyIdxYTurbo: action.setTurboFlag(true); [[fallthrough]];
			case mdKeyIdxY: return INPUT_Y | playerMask;
			case mdKeyIdxZTurbo: action.setTurboFlag(true); [[fallthrough]];
			case mdKeyIdxZ: return INPUT_Z | playerMask;
		}
		bug_unreachable("invalid key");
	}();
	return action;
}

void MdSystem::handleInputAction(EmuApp *, InputAction a)
{
	auto player = a.key >> 30; // player is encoded in upper 2 bits of input code
	assert(player <= 4);
	uint16 &padData = input.pad[playerIdxMap[player]];
	padData = IG::setOrClearBits(padData, (uint16)a.key, a.state == Input::Action::PUSHED);
}

static void updateGunPos(IG::WindowRect gameRect, const Input::MotionEvent &e, int idx)
{
	if(gameRect.overlaps(e.pos()))
	{
		int xRel = e.pos().x - gameRect.x, yRel = e.pos().y - gameRect.y;
		input.analog[idx][0] = IG::remap(xRel, 0, gameRect.xSize(), 0, bitmap.viewport.w);
		input.analog[idx][1] = IG::remap(yRel, 0, gameRect.ySize(), 0, bitmap.viewport.h);
	}
	else
	{
		// offscreen
		input.analog[idx][0] = input.analog[idx][1] = -1000;
	}
}

bool MdSystem::onPointerInputStart(const Input::MotionEvent &e, Input::DragTrackerState, IG::WindowRect gameRect)
{
	if(input.dev[gunDevIdx] != DEVICE_LIGHTGUN)
		return false;
	updateGunPos(gameRect, e, gunDevIdx);
	input.pad[gunDevIdx] |= INPUT_A;
	logMsg("gun pushed @ %d,%d, on MD %d,%d", e.pos().x, e.pos().y, input.analog[gunDevIdx][0], input.analog[gunDevIdx][1]);
	return true;
}

bool MdSystem::onPointerInputUpdate(const Input::MotionEvent &e, Input::DragTrackerState dragState,
	Input::DragTrackerState prevDragState, IG::WindowRect gameRect)
{
	if(input.dev[gunDevIdx] != DEVICE_LIGHTGUN)
		return false;
	updateGunPos(gameRect, e, gunDevIdx);
	return true;
}

bool MdSystem::onPointerInputEnd(const Input::MotionEvent &e, Input::DragTrackerState, IG::WindowRect)
{
	if(input.dev[gunDevIdx] != DEVICE_LIGHTGUN)
		return false;
	input.pad[gunDevIdx] = IG::clearBits(input.pad[gunDevIdx], (uint16)INPUT_A);
	return true;
}

void MdSystem::clearInputBuffers(EmuInputView &)
{
	IG::fill(input.pad);
	for(auto &analog : input.analog)
	{
		IG::fill(analog);
	}
}

const char *mdInputSystemToStr(uint8 system)
{
	switch(system)
	{
		case NO_SYSTEM: return "unconnected";
		case SYSTEM_MD_GAMEPAD: return "gamepad";
		case SYSTEM_MS_GAMEPAD: return "sms gamepad";
		case SYSTEM_MOUSE: return "mouse";
		case SYSTEM_MENACER: return "menacer";
		case SYSTEM_JUSTIFIER: return "justifier";
		case SYSTEM_TEAMPLAYER: return "team-player";
		case SYSTEM_LIGHTPHASER: return "light-phaser";
		default : return "unknown";
	}
}

static bool inputPortWasAutoSetByGame(unsigned port)
{
	return old_system[port] != -1;
}

void MdSystem::setupSmsInput(EmuApp &app)
{
	// first port may be set in rom loading code
	if(!input.system[0])
		input.system[0] = SYSTEM_MS_GAMEPAD;
	input.system[1] = SYSTEM_MS_GAMEPAD;
	io_init();
	app.setDisabledInputKeys(m3MissingCodes);
	for(auto i : iotaCount(2))
	{
		logMsg("attached %s to port %d", mdInputSystemToStr(input.system[i]), i);
	}
	gunDevIdx = 0;
	auto &vCtrl = app.defaultVController();
	if(input.dev[0] == DEVICE_LIGHTGUN && vCtrl.inputPlayer() != 1)
	{
		savedVControllerPlayer = vCtrl.inputPlayer();
		vCtrl.setInputPlayer(1);
	}
}

void MdSystem::setupMdInput(EmuApp &app)
{
	if(cart.special & HW_J_CART)
	{
		input.system[0] = input.system[1] = SYSTEM_MD_GAMEPAD;
		playerIdxMap[2] = 5;
		playerIdxMap[3] = 6;
	}
	else if(optionMultiTap)
	{
		input.system[0] = SYSTEM_TEAMPLAYER;
		input.system[1] = 0;

		playerIdxMap[1] = 1;
		playerIdxMap[2] = 2;
		playerIdxMap[3] = 3;
	}
	else
	{
		for(auto i : iotaCount(2))
		{
			if(mdInputPortDev[i] == -1) // user didn't specify device, go with auto settings
			{
				if(!inputPortWasAutoSetByGame(i))
					input.system[i] = SYSTEM_MD_GAMEPAD;
				else
				{
					logMsg("input port %d set by game detection", i);
					input.system[i] = old_system[i];
				}
			}
			else
				input.system[i] = mdInputPortDev[i];
			logMsg("attached %s to port %d%s", mdInputSystemToStr(input.system[i]), i, mdInputPortDev[i] == -1 ? " (auto)" : "");
		}
	}
	io_init();
	gunDevIdx = 4;
	if(option6BtnPad)
		app.unsetDisabledInputKeys();
	else
		app.setDisabledInputKeys(md6BtnExtraCodes);
}

void MdSystem::setupInput(EmuApp &app)
{
	if(!hasContent())
	{
		if(option6BtnPad)
			app.unsetDisabledInputKeys();
		else
			app.setDisabledInputKeys(md6BtnExtraCodes);
		return;
	}
	if(savedVControllerPlayer != -1)
	{
		app.defaultVController().setInputPlayer(std::exchange(savedVControllerPlayer, -1));
	}
	IG::fill(playerIdxMap);
	playerIdxMap[0] = 0;
	playerIdxMap[1] = 4;

	unsigned mdPad = option6BtnPad ? DEVICE_PAD6B : DEVICE_PAD3B;
	for(auto i : iotaCount(4))
		config.input[i].padtype = mdPad;

	if(system_hw == SYSTEM_PBC)
	{
		setupSmsInput(app);
	}
	else
	{
		setupMdInput(app);
	}
}

SystemInputDeviceDesc MdSystem::inputDeviceDesc(int idx) const
{
	return gamepadDesc;
}


}
