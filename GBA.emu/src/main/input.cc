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
#include "internal.hh"
#include <vbam/gba/GBA.h>

enum
{
	gbaKeyIdxUp = EmuControls::systemKeyMapStart,
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
};

const char *EmuSystem::inputFaceBtnName = "A/B";
const char *EmuSystem::inputCenterBtnName = "Select/Start";
const unsigned EmuSystem::inputFaceBtns = 4;
const unsigned EmuSystem::inputCenterBtns = 2;
const bool EmuSystem::inputHasTriggerBtns = true;
const bool EmuSystem::inputHasRevBtnLayout = false;
const unsigned EmuSystem::maxPlayers = 1;

namespace GbaKeyStatus
{

using namespace IG;
static const unsigned A = bit(0), B = bit(1),
	SELECT = bit(2), START = bit(3),
	RIGHT = bit(4), LEFT = bit(5), UP = bit(6), DOWN = bit(7),
	R = bit(8), L = bit(9);

}

static unsigned ptrInputToSysButton(int input)
{
	using namespace GbaKeyStatus;
	switch(input)
	{
		case SysVController::F_ELEM: return A;
		case SysVController::F_ELEM+1: return B;
		case SysVController::F_ELEM+2: return L;
		case SysVController::F_ELEM+3: return R;

		case SysVController::C_ELEM: return SELECT;
		case SysVController::C_ELEM+1: return START;

		case SysVController::D_ELEM: return UP | LEFT;
		case SysVController::D_ELEM+1: return UP;
		case SysVController::D_ELEM+2: return UP | RIGHT;
		case SysVController::D_ELEM+3: return LEFT;
		case SysVController::D_ELEM+5: return RIGHT;
		case SysVController::D_ELEM+6: return DOWN | LEFT;
		case SysVController::D_ELEM+7: return DOWN;
		case SysVController::D_ELEM+8: return DOWN | RIGHT;
		default: bug_unreachable("input == %d", input); return 0;
	}
}

void updateVControllerMapping(unsigned player, SysVController::Map &map)
{
	using namespace GbaKeyStatus;
	map[SysVController::F_ELEM] = A;
	map[SysVController::F_ELEM+1] = B;
	map[SysVController::F_ELEM+2] = L;
	map[SysVController::F_ELEM+3] = R;

	map[SysVController::C_ELEM] = SELECT;
	map[SysVController::C_ELEM+1] = START;

	map[SysVController::D_ELEM] = UP | LEFT;
	map[SysVController::D_ELEM+1] = UP;
	map[SysVController::D_ELEM+2] = UP | RIGHT;
	map[SysVController::D_ELEM+3] = LEFT;
	map[SysVController::D_ELEM+5] = RIGHT;
	map[SysVController::D_ELEM+6] = DOWN | LEFT;
	map[SysVController::D_ELEM+7] = DOWN;
	map[SysVController::D_ELEM+8] = DOWN | RIGHT;
}

unsigned EmuSystem::translateInputAction(unsigned input, bool &turbo)
{
	using namespace GbaKeyStatus;
	turbo = 0;
	switch(input)
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
		case gbaKeyIdxATurbo: turbo = 1; [[fallthrough]];
		case gbaKeyIdxA: return A;
		case gbaKeyIdxBTurbo: turbo = 1; [[fallthrough]];
		case gbaKeyIdxB: return B;
		case gbaKeyIdxL: return L;
		case gbaKeyIdxR: return R;
		case gbaKeyIdxAB: return A | B;
		case gbaKeyIdxRB: return R | B;
		default: bug_unreachable("input == %d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(EmuApp *, Input::Action action, unsigned emuKey)
{
	P1 = IG::setOrClearBits(P1, (uint16_t)emuKey, action != Input::Action::PUSHED);
}

void EmuSystem::clearInputBuffers(EmuInputView &)
{
	P1 = 0x03FF;
}
