/*  This file is part of PCE.emu.

	PCE.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	PCE.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with PCE.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include "internal.hh"

enum
{
	pceKeyIdxUp = EmuControls::systemKeyMapStart,
	pceKeyIdxRight,
	pceKeyIdxDown,
	pceKeyIdxLeft,
	pceKeyIdxLeftUp,
	pceKeyIdxRightUp,
	pceKeyIdxRightDown,
	pceKeyIdxLeftDown,
	pceKeyIdxSelect,
	pceKeyIdxRun,
	pceKeyIdxI,
	pceKeyIdxII,
	pceKeyIdxITurbo,
	pceKeyIdxIITurbo,
	pceKeyIdxIII,
	pceKeyIdxIV,
	pceKeyIdxV,
	pceKeyIdxVI
};

const char *EmuSystem::inputFaceBtnName = "I/II";
const char *EmuSystem::inputCenterBtnName = "Select/Run";
const unsigned EmuSystem::inputFaceBtns = 6;
const unsigned EmuSystem::inputCenterBtns = 2;
const bool EmuSystem::inputHasTriggerBtns = false;
const bool EmuSystem::inputHasRevBtnLayout = false;
const unsigned EmuSystem::maxPlayers = 5;
unsigned playerBit = 13;

void updateVControllerMapping(unsigned player, SysVController::Map &map)
{
	using namespace IG;
	unsigned playerMask = player << playerBit;
	map[SysVController::F_ELEM] = bit(0) | playerMask;
	map[SysVController::F_ELEM+1] = bit(1) | playerMask;
	map[SysVController::F_ELEM+2] = bit(8) | playerMask;
	map[SysVController::F_ELEM+3] = bit(9) | playerMask;
	map[SysVController::F_ELEM+4] = bit(10) | playerMask;
	map[SysVController::F_ELEM+5] = bit(11) | playerMask;

	map[SysVController::C_ELEM] = bit(2) | playerMask;
	map[SysVController::C_ELEM+1] = bit(3) | playerMask;

	map[SysVController::D_ELEM] = bit(4) | bit(7) | playerMask;
	map[SysVController::D_ELEM+1] = bit(4) | playerMask;
	map[SysVController::D_ELEM+2] = bit(4) | bit(5) | playerMask;
	map[SysVController::D_ELEM+3] = bit(7) | playerMask;
	map[SysVController::D_ELEM+5] = bit(5) | playerMask;
	map[SysVController::D_ELEM+6] = bit(6) | bit(7) | playerMask;
	map[SysVController::D_ELEM+7] = bit(6) | playerMask;
	map[SysVController::D_ELEM+8] = bit(6) | bit(5) | playerMask;
}

unsigned EmuSystem::translateInputAction(unsigned input, bool &turbo)
{
	turbo = 0;
	assert(input >= pceKeyIdxUp);
	unsigned player = (input - pceKeyIdxUp) / EmuControls::gamepadKeys;
	unsigned playerMask = player << playerBit;
	input -= EmuControls::gamepadKeys * player;
	using namespace IG;
	switch(input)
	{
		case pceKeyIdxUp: return bit(4) | playerMask;
		case pceKeyIdxRight: return bit(5) | playerMask;
		case pceKeyIdxDown: return bit(6) | playerMask;
		case pceKeyIdxLeft: return bit(7) | playerMask;
		case pceKeyIdxLeftUp: return bit(7) | bit(4) | playerMask;
		case pceKeyIdxRightUp: return bit(5) | bit(4) | playerMask;
		case pceKeyIdxRightDown: return bit(5) | bit(6) | playerMask;
		case pceKeyIdxLeftDown: return bit(7) | bit(6) | playerMask;
		case pceKeyIdxSelect: return bit(2) | playerMask;
		case pceKeyIdxRun: return bit(3) | playerMask;
		case pceKeyIdxITurbo: turbo = 1; [[fallthrough]];
		case pceKeyIdxI: return bit(0) | playerMask;
		case pceKeyIdxIITurbo: turbo = 1; [[fallthrough]];
		case pceKeyIdxII: return bit(1) | playerMask;
		case pceKeyIdxIII: return bit(8) | playerMask;
		case pceKeyIdxIV: return bit(9) | playerMask;
		case pceKeyIdxV: return bit(10) | playerMask;
		case pceKeyIdxVI: return bit(11) | playerMask;
		default: bug_unreachable("input == %d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(EmuApp *, Input::Action action, unsigned emuKey)
{
	auto player = emuKey >> playerBit;
	assert(player < maxPlayers);
	inputBuff[player] = IG::setOrClearBits(inputBuff[player], (uint16)emuKey, action == Input::Action::PUSHED);
}

void EmuSystem::clearInputBuffers(EmuInputView &)
{
	inputBuff = {};
	if(option6BtnPad)
	{
		iterateTimes(2, i)
			inputBuff[i] = IG::bit(12);
	}
}
