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

const char *EmuSystem::inputFaceBtnName = "A/B/C";
const char *EmuSystem::inputCenterBtnName = "Mode/Start";
const int EmuSystem::inputFaceBtns = 6;
const int EmuSystem::inputCenterBtns = 2;
const int EmuSystem::maxPlayers = 4;

constexpr std::pair<int, bool> setMd6BGamepad[]
{
	{0, true}, {1, true}, {2, true}, {3, true}, {4, true}, {5, true}
};
constexpr std::pair<int, bool> setMdGamepad[]
{
	{0, true}, {1, true}, {2, true}, {3, false}, {4, false}, {5, false}
};
constexpr std::pair<int, bool> setM3Gamepad[]
{
	{0, false}, {1, true}, {2, true}, {3, false}, {4, false}, {5, false}
};
constexpr std::pair<int, bool> enableModeBtn[]{{0, true}};
constexpr std::pair<int, bool> disableModeBtn[]{{0, false}};

VController::Map MdSystem::vControllerMap(int player)
{
	unsigned playerMask = player << 30;
	unsigned playMaskAlt = input.system[1] == SYSTEM_MENACER ? 1 << 30 : playerMask;
	unsigned playMaskAlt2 = input.system[1] == SYSTEM_JUSTIFIER ? 1 << 30 : playerMask;
	VController::Map map{};
	map[VController::F_ELEM] = INPUT_A | playerMask;
	map[VController::F_ELEM+1] = INPUT_B | playMaskAlt;
	map[VController::F_ELEM+2] = INPUT_C | playMaskAlt;
	map[VController::F_ELEM+3] = INPUT_X | playerMask;
	map[VController::F_ELEM+4] = INPUT_Y | playerMask;
	map[VController::F_ELEM+5] = INPUT_Z | playerMask;

	map[VController::C_ELEM] = INPUT_MODE | playerMask;
	map[VController::C_ELEM+1] = INPUT_START | playMaskAlt2;

	map[VController::D_ELEM] = INPUT_UP | INPUT_LEFT | playerMask;
	map[VController::D_ELEM+1] = INPUT_UP | playerMask;
	map[VController::D_ELEM+2] = INPUT_UP | INPUT_RIGHT | playerMask;
	map[VController::D_ELEM+3] = INPUT_LEFT | playerMask;
	map[VController::D_ELEM+5] = INPUT_RIGHT | playerMask;
	map[VController::D_ELEM+6] = INPUT_DOWN | INPUT_LEFT | playerMask;
	map[VController::D_ELEM+7] = INPUT_DOWN | playerMask;
	map[VController::D_ELEM+8] = INPUT_DOWN | INPUT_RIGHT | playerMask;
	return map;
}

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

unsigned MdSystem::translateInputAction(unsigned input, bool &turbo)
{
	if(!isGamepadButton(input))
		turbo = 0;
	assert(input >= mdKeyIdxUp);
	unsigned player = (input - mdKeyIdxUp) / Controls::gamepadKeys;
	unsigned playerMask = player << 30;
	input -= Controls::gamepadKeys * player;
	switch(input)
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
		case mdKeyIdxATurbo: turbo = 1; [[fallthrough]];
		case mdKeyIdxA: return INPUT_A | playerMask;
		case mdKeyIdxBTurbo: turbo = 1; [[fallthrough]];
		case mdKeyIdxB: return INPUT_B | playerMask;
		case mdKeyIdxCTurbo: turbo = 1; [[fallthrough]];
		case mdKeyIdxC: return INPUT_C | playerMask;
		case mdKeyIdxXTurbo: turbo = 1; [[fallthrough]];
		case mdKeyIdxX: return INPUT_X | playerMask;
		case mdKeyIdxYTurbo: turbo = 1; [[fallthrough]];
		case mdKeyIdxY: return INPUT_Y | playerMask;
		case mdKeyIdxZTurbo: turbo = 1; [[fallthrough]];
		case mdKeyIdxZ: return INPUT_Z | playerMask;
		default: bug_unreachable("input == %d", input);
	}
	return 0;
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
	app.applyEnabledFaceButtons(setM3Gamepad);
	app.applyEnabledCenterButtons(disableModeBtn);
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
	app.applyEnabledFaceButtons(option6BtnPad ? setMd6BGamepad : setMdGamepad);
	app.applyEnabledCenterButtons(option6BtnPad ? enableModeBtn : disableModeBtn);
}

void MdSystem::setupInput(EmuApp &app)
{
	if(!hasContent())
	{
		app.applyEnabledFaceButtons(option6BtnPad ? setMd6BGamepad : setMdGamepad);
		app.applyEnabledCenterButtons(option6BtnPad ? enableModeBtn : disableModeBtn);
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
	app.updateVControllerMapping();
}


}
