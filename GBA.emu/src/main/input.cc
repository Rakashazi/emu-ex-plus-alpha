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
#include "MainSystem.hh"
#include <vbam/gba/GBA.h>

namespace EmuEx
{

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
};

const char *EmuSystem::inputFaceBtnName = "A/B/L/R";
const char *EmuSystem::inputCenterBtnName = "Select/Start";
const int EmuSystem::inputFaceBtns = 4;
const int EmuSystem::inputCenterBtns = 2;
int EmuSystem::inputLTriggerIndex = 2;
int EmuSystem::inputRTriggerIndex = 3;
const int EmuSystem::maxPlayers = 1;
std::array<int, EmuSystem::MAX_FACE_BTNS> EmuSystem::vControllerImageMap{1, 0, 2, 3};

namespace GbaKeyStatus
{

using namespace IG;
static const unsigned A = bit(0), B = bit(1),
	SELECT = bit(2), START = bit(3),
	RIGHT = bit(4), LEFT = bit(5), UP = bit(6), DOWN = bit(7),
	R = bit(8), L = bit(9);

}

VController::Map GbaSystem::vControllerMap(int player)
{
	using namespace GbaKeyStatus;
	VController::Map map{};
	map[VController::F_ELEM] = B;
	map[VController::F_ELEM+1] = A;
	map[VController::F_ELEM+2] = L;
	map[VController::F_ELEM+3] = R;

	map[VController::C_ELEM] = SELECT;
	map[VController::C_ELEM+1] = START;

	map[VController::D_ELEM] = UP | LEFT;
	map[VController::D_ELEM+1] = UP;
	map[VController::D_ELEM+2] = UP | RIGHT;
	map[VController::D_ELEM+3] = LEFT;
	map[VController::D_ELEM+5] = RIGHT;
	map[VController::D_ELEM+6] = DOWN | LEFT;
	map[VController::D_ELEM+7] = DOWN;
	map[VController::D_ELEM+8] = DOWN | RIGHT;
	return map;
}

unsigned GbaSystem::translateInputAction(unsigned input, bool &turbo)
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

void GbaSystem::handleInputAction(EmuApp *, InputAction a)
{
	P1 = IG::setOrClearBits(P1, (uint16_t)a.key, a.state != Input::Action::PUSHED);
}

void GbaSystem::clearInputBuffers(EmuInputView &)
{
	P1 = 0x03FF;
}

}
