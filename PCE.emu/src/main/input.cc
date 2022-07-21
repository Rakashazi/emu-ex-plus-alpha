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
#include "MainSystem.hh"

namespace EmuEx
{

enum
{
	pceKeyIdxUp = Controls::systemKeyMapStart,
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
const int EmuSystem::inputFaceBtns = 6;
const int EmuSystem::inputCenterBtns = 2;
const int EmuSystem::maxPlayers = 5;
std::array<int, EmuSystem::MAX_FACE_BTNS> EmuSystem::vControllerImageMap{2, 1, 0, 3, 4, 5};
unsigned playerBit = 13;

VController::Map PceSystem::vControllerMap(int player)
{
	unsigned playerMask = player << playerBit;
	VController::Map map{};
	map[VController::F_ELEM] = bit(8) | playerMask;
	map[VController::F_ELEM+1] = bit(1) | playerMask;
	map[VController::F_ELEM+2] = bit(0) | playerMask;
	map[VController::F_ELEM+3] = bit(9) | playerMask;
	map[VController::F_ELEM+4] = bit(10) | playerMask;
	map[VController::F_ELEM+5] = bit(11) | playerMask;

	map[VController::C_ELEM] = bit(2) | playerMask;
	map[VController::C_ELEM+1] = bit(3) | playerMask;

	map[VController::D_ELEM] = bit(4) | bit(7) | playerMask;
	map[VController::D_ELEM+1] = bit(4) | playerMask;
	map[VController::D_ELEM+2] = bit(4) | bit(5) | playerMask;
	map[VController::D_ELEM+3] = bit(7) | playerMask;
	map[VController::D_ELEM+5] = bit(5) | playerMask;
	map[VController::D_ELEM+6] = bit(6) | bit(7) | playerMask;
	map[VController::D_ELEM+7] = bit(6) | playerMask;
	map[VController::D_ELEM+8] = bit(6) | bit(5) | playerMask;
	return map;
}

unsigned PceSystem::translateInputAction(unsigned input, bool &turbo)
{
	turbo = 0;
	assert(input >= pceKeyIdxUp);
	unsigned player = (input - pceKeyIdxUp) / Controls::gamepadKeys;
	unsigned playerMask = player << playerBit;
	input -= Controls::gamepadKeys * player;
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

void PceSystem::handleInputAction(EmuApp *, InputAction a)
{
	auto player = a.key >> playerBit;
	assumeExpr(player < maxPlayers);
	inputBuff[player] = IG::setOrClearBits(inputBuff[player], (uint16)a.key, a.state == Input::Action::PUSHED);
}

void PceSystem::clearInputBuffers(EmuInputView &)
{
	inputBuff = {};
	if(option6BtnPad)
	{
		for(auto i : iotaCount(2))
			inputBuff[i] = IG::bit(12);
	}
}

void set6ButtonPadEnabled(EmuApp &app, bool on)
{
	static constexpr std::pair<int, bool> enable6Btn[]{{0, true}, {3, true}, {4, true}, {5, true}};
	static constexpr std::pair<int, bool> disable6Btn[]{{0, false}, {3, false}, {4, false}, {5, false}};
	app.applyEnabledFaceButtons(on ? enable6Btn : disable6Btn);
}

}
