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

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include "MainSystem.hh"

namespace EmuEx
{

enum
{
	wsKeyIdxUp = Controls::systemKeyMapStart,
	wsKeyIdxRight,
	wsKeyIdxDown,
	wsKeyIdxLeft,
	wsKeyIdxLeftUp,
	wsKeyIdxRightUp,
	wsKeyIdxRightDown,
	wsKeyIdxLeftDown,
	wsKeyIdxStart,
	wsKeyIdxA,
	wsKeyIdxB,
	wsKeyIdxY1,
	wsKeyIdxY2,
	wsKeyIdxY3,
	wsKeyIdxY4,
	wsKeyIdxATurbo,
	wsKeyIdxBTurbo,
	wsKeyIdxY1Turbo,
	wsKeyIdxY2Turbo,
	wsKeyIdxY3Turbo,
	wsKeyIdxY4Turbo,
};

const char *EmuSystem::inputFaceBtnName = "A/B/X1-4/Y1-4";
const char *EmuSystem::inputCenterBtnName = "Start";
const int EmuSystem::inputFaceBtns = 6;
const int EmuSystem::inputCenterBtns = 1;
const int EmuSystem::maxPlayers = 1;
FaceButtonImageMap EmuSystem::vControllerImageMap{0, 4, 3, 1, 5, 2};

enum KeypadMask: unsigned
{
	X1_BIT = bit(0),
	X2_BIT = bit(1),
	X3_BIT = bit(2),
	X4_BIT = bit(3),
	Y1_BIT = bit(4),
	Y2_BIT = bit(5),
	Y3_BIT = bit(6),
	Y4_BIT = bit(7),
	START_BIT = bit(8),
	A_BIT = bit(9),
	B_BIT = bit(10),
};

VController::Map WsSystem::vControllerMap(int player)
{
	VController::Map map{};
	if(isRotated())
	{
		map[VController::F_ELEM] = X4_BIT;
		map[VController::F_ELEM+1] = X3_BIT;
		map[VController::F_ELEM+2] = A_BIT;
		map[VController::F_ELEM+3] = X1_BIT;
		map[VController::F_ELEM+4] = X2_BIT;
		map[VController::F_ELEM+5] = B_BIT;

		map[VController::C_ELEM] = START_BIT;

		map[VController::D_ELEM] = Y2_BIT | Y1_BIT;
		map[VController::D_ELEM+1] = Y2_BIT;
		map[VController::D_ELEM+2] = Y2_BIT | Y3_BIT;
		map[VController::D_ELEM+3] = Y1_BIT;
		map[VController::D_ELEM+5] = Y3_BIT;
		map[VController::D_ELEM+6] = Y4_BIT | Y1_BIT;
		map[VController::D_ELEM+7] = Y4_BIT;
		map[VController::D_ELEM+8] = Y4_BIT | Y3_BIT;
	}
	else
	{
		bool swapAB = !showVGamepadYWhenHorizonal;
		map[VController::F_ELEM] = swapAB ? B_BIT : A_BIT;
		map[VController::F_ELEM+1] = Y3_BIT;
		map[VController::F_ELEM+2] = Y2_BIT;
		map[VController::F_ELEM+3] = swapAB ? A_BIT : B_BIT;
		map[VController::F_ELEM+4] = Y4_BIT;
		map[VController::F_ELEM+5] = Y1_BIT;

		map[VController::C_ELEM] = START_BIT;

		map[VController::D_ELEM] = X1_BIT | X4_BIT;
		map[VController::D_ELEM+1] = X1_BIT;
		map[VController::D_ELEM+2] = X1_BIT | X2_BIT;
		map[VController::D_ELEM+3] = X4_BIT;
		map[VController::D_ELEM+5] = X2_BIT;
		map[VController::D_ELEM+6] = X3_BIT | X4_BIT;
		map[VController::D_ELEM+7] = X3_BIT;
		map[VController::D_ELEM+8] = X3_BIT | X2_BIT;
	}
	return map;
}

static bool isGamepadButton(unsigned input)
{
	switch(input)
	{
		case wsKeyIdxY1Turbo:
		case wsKeyIdxY1:
		case wsKeyIdxY2Turbo:
		case wsKeyIdxY2:
		case wsKeyIdxY3Turbo:
		case wsKeyIdxY3:
		case wsKeyIdxY4Turbo:
		case wsKeyIdxY4:
		case wsKeyIdxStart:
		case wsKeyIdxATurbo:
		case wsKeyIdxA:
		case wsKeyIdxBTurbo:
		case wsKeyIdxB:
			return true;
		default: return false;
	}
}

unsigned WsSystem::translateInputAction(unsigned input, bool &turbo)
{
	if(!isGamepadButton(input))
		turbo = 0;
	if(isRotated())
	{
		switch(input)
		{
			case wsKeyIdxUp: return Y2_BIT;
			case wsKeyIdxRight: return Y3_BIT;
			case wsKeyIdxDown: return Y4_BIT;
			case wsKeyIdxLeft: return Y1_BIT;
			case wsKeyIdxLeftUp: return Y1_BIT | Y2_BIT;
			case wsKeyIdxRightUp: return Y3_BIT | Y2_BIT;
			case wsKeyIdxRightDown: return Y3_BIT | Y4_BIT;
			case wsKeyIdxLeftDown: return Y1_BIT | Y4_BIT;
			case wsKeyIdxY1Turbo: turbo = 1; [[fallthrough]];
			case wsKeyIdxY1: return B_BIT;
			case wsKeyIdxY2Turbo: turbo = 1; [[fallthrough]];
			case wsKeyIdxY2: return A_BIT;
			case wsKeyIdxY3Turbo: turbo = 1; [[fallthrough]];
			case wsKeyIdxY3: return X3_BIT;
			case wsKeyIdxY4Turbo: turbo = 1; [[fallthrough]];
			case wsKeyIdxY4: return X2_BIT;
			case wsKeyIdxStart: return START_BIT;
			case wsKeyIdxATurbo: turbo = 1; [[fallthrough]];
			case wsKeyIdxA: return X4_BIT;
			case wsKeyIdxBTurbo: turbo = 1; [[fallthrough]];
			case wsKeyIdxB: return X1_BIT;
			default: bug_unreachable("input == %d", input);
		}
	}
	else
	{
		switch(input)
		{
			case wsKeyIdxUp: return X1_BIT;
			case wsKeyIdxRight: return X2_BIT;
			case wsKeyIdxDown: return X3_BIT;
			case wsKeyIdxLeft: return X4_BIT;
			case wsKeyIdxLeftUp: return X4_BIT | X1_BIT;
			case wsKeyIdxRightUp: return X2_BIT | X1_BIT;
			case wsKeyIdxRightDown: return X2_BIT | X3_BIT;
			case wsKeyIdxLeftDown: return X4_BIT | X3_BIT;
			case wsKeyIdxY1Turbo: turbo = 1; [[fallthrough]];
			case wsKeyIdxY1: return Y1_BIT;
			case wsKeyIdxY2Turbo: turbo = 1; [[fallthrough]];
			case wsKeyIdxY2: return Y2_BIT;
			case wsKeyIdxY3Turbo: turbo = 1; [[fallthrough]];
			case wsKeyIdxY3: return Y3_BIT;
			case wsKeyIdxY4Turbo: turbo = 1; [[fallthrough]];
			case wsKeyIdxY4: return Y4_BIT;
			case wsKeyIdxStart: return START_BIT;
			case wsKeyIdxATurbo: turbo = 1; [[fallthrough]];
			case wsKeyIdxA: return A_BIT;
			case wsKeyIdxBTurbo: turbo = 1; [[fallthrough]];
			case wsKeyIdxB: return B_BIT;
			default: bug_unreachable("input == %d", input);
		}
	}
	return 0;
}

void WsSystem::handleInputAction(EmuApp *, InputAction a)
{
	inputBuff = IG::setOrClearBits(inputBuff, (uint16_t)a.key, a.state == Input::Action::PUSHED);
}

void WsSystem::clearInputBuffers(EmuInputView &)
{
	inputBuff = {};
}

void WsSystem::setupInput(EmuApp &app)
{
	static constexpr std::pair<int, bool> enableAll[]
	{
		{0, true}, {1, true}, {2, true}, {3, true}, {4, true}, {5, true}
	};
	if(isRotated())
	{
		static constexpr std::pair<int, bool> enableY[]
		{
			{0, true}, {1, true}, {2, false}, {3, true}, {4, true}, {5, false}
		};
		app.setFaceButtonMapping({5, 4, 0, 2, 3, 1});
		app.applyEnabledFaceButtons(showVGamepadABWhenVertical ? enableAll : enableY);
	}
	else
	{
		static constexpr std::pair<int, bool> enableAB[]
		{
			{0, true}, {1, false}, {2, false}, {3, true}, {4, false}, {5, false}
		};
		app.applyEnabledFaceButtons(showVGamepadYWhenHorizonal ? enableAll : enableAB);
		app.setFaceButtonMapping(showVGamepadYWhenHorizonal ? vControllerImageMap
			: FaceButtonImageMap{1, 4, 3, 0, 5, 2}); // A/B ordering
	}
	app.updateVControllerMapping();
}

}
