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
#include <imagine/util/math/space.hh>
#include "internal.hh"
#include "input.h"
#include "system.h"

enum
{
	mdKeyIdxUp = EmuControls::systemKeyMapStart,
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
const unsigned EmuSystem::inputFaceBtns = 6;
const unsigned EmuSystem::inputCenterBtns = 2;
const bool EmuSystem::inputHasTriggerBtns = false;
const bool EmuSystem::inputHasRevBtnLayout = true;
const unsigned EmuSystem::maxPlayers = 4;
unsigned playerIdxMap[4]{};

void updateVControllerMapping(unsigned player, SysVController::Map &map)
{
	unsigned playerMask = player << 30;
	map[SysVController::F_ELEM] = INPUT_A | playerMask;
	map[SysVController::F_ELEM+1] = INPUT_B | playerMask;
	map[SysVController::F_ELEM+2] = INPUT_C | playerMask;
	map[SysVController::F_ELEM+3] = INPUT_X | playerMask;
	map[SysVController::F_ELEM+4] = INPUT_Y | playerMask;
	map[SysVController::F_ELEM+5] = INPUT_Z | playerMask;

	map[SysVController::C_ELEM] = INPUT_MODE | playerMask;
	map[SysVController::C_ELEM+1] = INPUT_START | playerMask;

	map[SysVController::D_ELEM] = INPUT_UP | INPUT_LEFT | playerMask;
	map[SysVController::D_ELEM+1] = INPUT_UP | playerMask;
	map[SysVController::D_ELEM+2] = INPUT_UP | INPUT_RIGHT | playerMask;
	map[SysVController::D_ELEM+3] = INPUT_LEFT | playerMask;
	map[SysVController::D_ELEM+5] = INPUT_RIGHT | playerMask;
	map[SysVController::D_ELEM+6] = INPUT_DOWN | INPUT_LEFT | playerMask;
	map[SysVController::D_ELEM+7] = INPUT_DOWN | playerMask;
	map[SysVController::D_ELEM+8] = INPUT_DOWN | INPUT_RIGHT | playerMask;
}

unsigned EmuSystem::translateInputAction(unsigned input, bool &turbo)
{
	turbo = 0;
	assert(input >= mdKeyIdxUp);
	unsigned player = (input - mdKeyIdxUp) / EmuControls::gamepadKeys;
	unsigned playerMask = player << 30;
	input -= EmuControls::gamepadKeys * player;
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

void EmuSystem::handleInputAction(EmuApp *, Input::Action action, unsigned emuKey)
{
	auto player = emuKey >> 30; // player is encoded in upper 2 bits of input code
	assert(player <= 4);
	uint16 &padData = input.pad[playerIdxMap[player]];
	padData = IG::setOrClearBits(padData, (uint16)emuKey, action == Input::Action::PUSHED);
}

bool EmuSystem::handlePointerInputEvent(Input::Event e, IG::WindowRect gameRect)
{
	const int gunDevIdx = 4;
	if(input.dev[gunDevIdx] != DEVICE_LIGHTGUN)
		return false;
	if(gameRect.overlaps(e.pos()))
	{
		int xRel = e.pos().x - gameRect.x, yRel = e.pos().y - gameRect.y;
		input.analog[gunDevIdx][0] = IG::scalePointRange((float)xRel, (float)gameRect.xSize(), (float)bitmap.viewport.w);
		input.analog[gunDevIdx][1] = IG::scalePointRange((float)yRel, (float)gameRect.ySize(), (float)bitmap.viewport.h);
	}
	if(e.pushed())
	{
		input.pad[gunDevIdx] |= INPUT_A;
		logMsg("gun pushed @ %d,%d, on MD %d,%d", e.pos().x, e.pos().y, input.analog[gunDevIdx][0], input.analog[gunDevIdx][1]);
	}
	else if(e.released())
	{
		input.pad[gunDevIdx] = IG::clearBits(input.pad[gunDevIdx], (uint16)INPUT_A);
	}
	return true;
}

void EmuSystem::clearInputBuffers(EmuInputView &)
{
	IG::fill(input.pad);
	for(auto &analog : input.analog)
	{
		IG::fill(analog);
	}
}
