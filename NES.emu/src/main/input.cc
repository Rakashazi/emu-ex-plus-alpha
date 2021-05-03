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
#include <imagine/util/math/space.hh>
#include "internal.hh"
#include <fceu/fceu.h>

enum
{
	nesKeyIdxUp = EmuControls::systemKeyMapStart,
	nesKeyIdxRight,
	nesKeyIdxDown,
	nesKeyIdxLeft,
	nesKeyIdxLeftUp,
	nesKeyIdxRightUp,
	nesKeyIdxRightDown,
	nesKeyIdxLeftDown,
	nesKeyIdxSelect,
	nesKeyIdxStart,
	nesKeyIdxA,
	nesKeyIdxB,
	nesKeyIdxATurbo,
	nesKeyIdxBTurbo,
	nesKeyIdxAB,
};

const char *EmuSystem::inputFaceBtnName = "A/B";
const char *EmuSystem::inputCenterBtnName = "Select/Start";
const unsigned EmuSystem::inputFaceBtns = 2;
const unsigned EmuSystem::inputCenterBtns = 2;
const bool EmuSystem::inputHasTriggerBtns = false;
const bool EmuSystem::inputHasRevBtnLayout = false;
const unsigned EmuSystem::maxPlayers = 4;
static uint32 padData = 0;
uint32 zapperData[3]{};
bool usingZapper = false;

void connectNESInput(int port, ESI type)
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

void GetMouseData(uint32 (&d)[3])
{
	// TODO
}

#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
void updateVControllerMapping(unsigned player, SysVController::Map &map)
{
	using namespace IG;
	unsigned playerMask = player << 8;
	map[SysVController::F_ELEM] = bit(0) | playerMask;
	map[SysVController::F_ELEM+1] = bit(1) | playerMask;

	map[SysVController::C_ELEM] = bit(2) | playerMask;
	map[SysVController::C_ELEM+1] = bit(3) | playerMask;

	map[SysVController::D_ELEM] = bit(4) | bit(6) | playerMask;
	map[SysVController::D_ELEM+1] = bit(4) | playerMask;
	map[SysVController::D_ELEM+2] = bit(4) | bit(7) | playerMask;
	map[SysVController::D_ELEM+3] = bit(6) | playerMask;
	map[SysVController::D_ELEM+5] = bit(7) | playerMask;
	map[SysVController::D_ELEM+6] = bit(5) | bit(6) | playerMask;
	map[SysVController::D_ELEM+7] = bit(5) | playerMask;
	map[SysVController::D_ELEM+8] = bit(5) | bit(7) | playerMask;
}
#endif

static unsigned playerInputShift(unsigned player)
{
	switch(player)
	{
		case 1: return 8;
		case 2: return 16;
		case 3: return 24;
	}
	return 0;
}

unsigned EmuSystem::translateInputAction(unsigned input, bool &turbo)
{
	using namespace IG;
	turbo = 0;
	assert(input >= nesKeyIdxUp);
	unsigned player = (input - nesKeyIdxUp) / EmuControls::gamepadKeys;
	unsigned playerMask = player << 8;
	input -= EmuControls::gamepadKeys * player;
	switch(input)
	{
		case nesKeyIdxUp: return bit(4) | playerMask;
		case nesKeyIdxRight: return bit(7) | playerMask;
		case nesKeyIdxDown: return bit(5) | playerMask;
		case nesKeyIdxLeft: return bit(6) | playerMask;
		case nesKeyIdxLeftUp: return bit(6) | bit(4) | playerMask;
		case nesKeyIdxRightUp: return bit(7) | bit(4) | playerMask;
		case nesKeyIdxRightDown: return bit(7) | bit(5) | playerMask;
		case nesKeyIdxLeftDown: return bit(6) | bit(5) | playerMask;
		case nesKeyIdxSelect: return bit(2) | playerMask;
		case nesKeyIdxStart: return bit(3) | playerMask;
		case nesKeyIdxATurbo: turbo = 1; [[fallthrough]];
		case nesKeyIdxA: return bit(0) | playerMask;
		case nesKeyIdxBTurbo: turbo = 1; [[fallthrough]];
		case nesKeyIdxB: return bit(1) | playerMask;
		case nesKeyIdxAB: return bit(0) | bit(1) | playerMask;
		default: bug_unreachable("input == %d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(EmuApp *, Input::Action action, unsigned emuKey)
{
	unsigned player = emuKey >> 8;
	auto key = emuKey & 0xFF;
	if(GameInfo->type==GIT_VSUNI) // TODO: make coin insert separate key
	{
		if(action == Input::Action::PUSHED && key == IG::bit(3))
			FCEUI_VSUniCoin();
	}
	padData = IG::setOrClearBits(padData, key << playerInputShift(player), action == Input::Action::PUSHED);
}

bool EmuSystem::handlePointerInputEvent(Input::Event e, IG::WindowRect gameRect)
{
	if(!usingZapper)
		return false;
	if(e.pushed())
	{
		zapperData[2] = 0;
		if(gameRect.overlaps(e.pos()))
		{
			int xRel = e.pos().x - gameRect.x, yRel = e.pos().y - gameRect.y;
			int xNes = IG::scalePointRange((float)xRel, (float)gameRect.xSize(), (float)256.);
			int yNes = IG::scalePointRange((float)yRel, (float)gameRect.ySize(), (float)224.) + 8;
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
	}
	else if(e.released())
	{
		zapperData[2] = 0;
	}
	return true;
}

void EmuSystem::clearInputBuffers(EmuInputView &)
{
	IG::fill(zapperData);
	padData = {};
}
