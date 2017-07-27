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

#include <neopop.h>
#include <emuframework/EmuApp.hh>

enum
{
	ngpKeyIdxUp = EmuControls::systemKeyMapStart,
	ngpKeyIdxRight,
	ngpKeyIdxDown,
	ngpKeyIdxLeft,
	ngpKeyIdxLeftUp,
	ngpKeyIdxRightUp,
	ngpKeyIdxRightDown,
	ngpKeyIdxLeftDown,
	ngpKeyIdxOption,
	ngpKeyIdxA,
	ngpKeyIdxB,
	ngpKeyIdxATurbo,
	ngpKeyIdxBTurbo
};

const char *EmuSystem::inputFaceBtnName = "A/B";
const char *EmuSystem::inputCenterBtnName = "Option";
const uint EmuSystem::inputFaceBtns = 2;
const uint EmuSystem::inputCenterBtns = 1;
const bool EmuSystem::inputHasTriggerBtns = false;
const bool EmuSystem::inputHasRevBtnLayout = true;
const uint EmuSystem::maxPlayers = 1;

static const uint ctrlUpBit = 0x01, ctrlDownBit = 0x02, ctrlLeftBit = 0x04, ctrlRightBit = 0x08,
		ctrlABit = 0x10, ctrlBBit = 0x20, ctrlOptionBit = 0x40;

void updateVControllerMapping(uint player, SysVController::Map &map)
{
	map[SysVController::F_ELEM] = ctrlABit;
	map[SysVController::F_ELEM+1] = ctrlBBit;

	map[SysVController::C_ELEM] = ctrlOptionBit;

	map[SysVController::D_ELEM] = ctrlUpBit | ctrlLeftBit;
	map[SysVController::D_ELEM+1] = ctrlUpBit;
	map[SysVController::D_ELEM+2] = ctrlUpBit | ctrlRightBit;
	map[SysVController::D_ELEM+3] = ctrlLeftBit;
	map[SysVController::D_ELEM+5] = ctrlRightBit;
	map[SysVController::D_ELEM+6] = ctrlDownBit | ctrlLeftBit;
	map[SysVController::D_ELEM+7] = ctrlDownBit;
	map[SysVController::D_ELEM+8] = ctrlDownBit | ctrlRightBit;
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	switch(input)
	{
		case ngpKeyIdxUp: return ctrlUpBit;
		case ngpKeyIdxRight: return ctrlRightBit;
		case ngpKeyIdxDown: return ctrlDownBit;
		case ngpKeyIdxLeft: return ctrlLeftBit;
		case ngpKeyIdxLeftUp: return ctrlLeftBit | ctrlUpBit;
		case ngpKeyIdxRightUp: return ctrlRightBit | ctrlUpBit;
		case ngpKeyIdxRightDown: return ctrlRightBit | ctrlDownBit;
		case ngpKeyIdxLeftDown: return ctrlLeftBit | ctrlDownBit;
		case ngpKeyIdxOption: return ctrlOptionBit;
		case ngpKeyIdxATurbo: turbo = 1; [[fallthrough]];
		case ngpKeyIdxA: return ctrlABit;
		case ngpKeyIdxBTurbo: turbo = 1; [[fallthrough]];
		case ngpKeyIdxB: return ctrlBBit;
		default: bug_branch("%d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(uint state, uint emuKey)
{
	uchar &ctrlBits = ram[0x6F82];
	ctrlBits = IG::setOrClearBits(ctrlBits, (uchar)emuKey, state == Input::PUSHED);
}

void EmuSystem::clearInputBuffers(EmuInputView &)
{
	ram[0x6F82] = 0;
}
