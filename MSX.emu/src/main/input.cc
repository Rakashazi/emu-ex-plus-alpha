/*  This file is part of MSX.emu.

	MSX.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MSX.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MSX.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include "internal.hh"

extern "C"
{
	#include <blueMSX/Input/InputEvent.h>
}

static const unsigned msxKeyboardKeys = 92;

enum
{
	msxKeyIdxUp = EmuControls::systemKeyMapStart,
	msxKeyIdxRight,
	msxKeyIdxDown,
	msxKeyIdxLeft,
	msxKeyIdxLeftUp,
	msxKeyIdxRightUp,
	msxKeyIdxRightDown,
	msxKeyIdxLeftDown,
	msxKeyIdxJS1Btn,
	msxKeyIdxJS2Btn,
	msxKeyIdxJS1BtnTurbo,
	msxKeyIdxJS2BtnTurbo,

	msxKeyIdxUp2,
	msxKeyIdxRight2,
	msxKeyIdxDown2,
	msxKeyIdxLeft2,
	msxKeyIdxLeftUp2,
	msxKeyIdxRightUp2,
	msxKeyIdxRightDown2,
	msxKeyIdxLeftDown2,
	msxKeyIdxJS1Btn2,
	msxKeyIdxJS2Btn2,
	msxKeyIdxJS1BtnTurbo2,
	msxKeyIdxJS2BtnTurbo2,

	msxKeyIdxColeco0Num,
	msxKeyIdxColeco1Num,
	msxKeyIdxColeco2Num,
	msxKeyIdxColeco3Num,
	msxKeyIdxColeco4Num,
	msxKeyIdxColeco5Num,
	msxKeyIdxColeco6Num,
	msxKeyIdxColeco7Num,
	msxKeyIdxColeco8Num,
	msxKeyIdxColeco9Num,
	msxKeyIdxColecoStar,
	msxKeyIdxColecoHash,

	msxKeyIdxColeco0Num2,
	msxKeyIdxColeco1Num2,
	msxKeyIdxColeco2Num2,
	msxKeyIdxColeco3Num2,
	msxKeyIdxColeco4Num2,
	msxKeyIdxColeco5Num2,
	msxKeyIdxColeco6Num2,
	msxKeyIdxColeco7Num2,
	msxKeyIdxColeco8Num2,
	msxKeyIdxColeco9Num2,
	msxKeyIdxColecoStar2,
	msxKeyIdxColecoHash2,

	msxKeyIdxToggleKb,
	msxKeyIdxKbStart,
	msxKeyIdxKbEnd = msxKeyIdxKbStart + (msxKeyboardKeys - 1)
};

const char *EmuSystem::inputFaceBtnName = "A/B";
const char *EmuSystem::inputCenterBtnName = "Space/KB";
const unsigned EmuSystem::inputFaceBtns = 2;
const unsigned EmuSystem::inputCenterBtns = 2;
const bool EmuSystem::inputHasTriggerBtns = false;
const bool EmuSystem::inputHasRevBtnLayout = false;
bool EmuSystem::inputHasKeyboard = true;
const unsigned EmuSystem::maxPlayers = 2;
extern Machine *machine;

static SysVController::KbMap kbToEventMap
{
	EC_Q, EC_W, EC_E, EC_R, EC_T, EC_Y, EC_U, EC_I, EC_O, EC_P,
	EC_A, EC_S, EC_D, EC_F, EC_G, EC_H, EC_J, EC_K, EC_L, EC_NONE,
	EC_CAPS, EC_Z, EC_X, EC_C, EC_V, EC_B, EC_N, EC_M, EC_BKSPACE, EC_NONE,
	EC_NONE, EC_NONE, EC_NONE, EC_SPACE, EC_SPACE, EC_SPACE, EC_SPACE, EC_CTRL, EC_CTRL, EC_RETURN
};

static SysVController::KbMap kbToEventMap2
{
	EC_F1, EC_F1, EC_F2, EC_F2, EC_F3, EC_F3, EC_F4, EC_F4, EC_F5, EC_F5, // 0-9
	EC_1, EC_2, EC_3, EC_4, EC_5, EC_6, EC_7, EC_8, EC_9, EC_0, // 10-19
	EC_TAB, EC_8 | (EC_LSHIFT << 8), EC_9 | (EC_LSHIFT << 8), EC_3 | (EC_LSHIFT << 8), EC_4 | (EC_LSHIFT << 8), EC_SEMICOL | (EC_LSHIFT << 8), EC_NEG, EC_SEMICOL, EC_ESC, EC_NONE,
	EC_NONE, EC_NONE, EC_NONE, EC_SPACE, EC_SPACE, EC_SPACE, EC_SPACE, EC_PERIOD, EC_PERIOD, EC_RETURN
};

void setupVKeyboardMap(EmuApp &app, unsigned boardType)
{
	if(boardType != BOARD_COLECO)
	{
		iterateTimes(10, i) // 1 - 0
			kbToEventMap2[10 + i] = EC_1 + i;
		kbToEventMap2[23] = EC_3 | (EC_LSHIFT << 8);
	}
	app.updateKeyboardMapping();
}

SysVController::KbMap updateVControllerKeyboardMapping(unsigned mode)
{
	return mode ? kbToEventMap2 : kbToEventMap;
}

void updateVControllerMapping(unsigned player, SysVController::Map &map)
{
	if(machine && machine->board.type == BOARD_COLECO)
	{
		unsigned playerShift = player ? 12 : 0;
		iterateTimes(9, i) // 1 - 9
			kbToEventMap2[10 + i] = EC_COLECO1_1 + i + playerShift;
		kbToEventMap2[19] = EC_COLECO1_0 + playerShift;
		kbToEventMap2[23] = EC_COLECO1_HASH + playerShift;
	}
	map[SysVController::F_ELEM] = player ? EC_JOY2_BUTTON2 : EC_JOY1_BUTTON2;
	map[SysVController::F_ELEM+1] = player ? EC_JOY2_BUTTON1 : EC_JOY1_BUTTON1;

	map[SysVController::C_ELEM] = activeBoardType == BOARD_COLECO ? (player ? EC_COLECO2_STAR : EC_COLECO1_STAR)
																	: EC_SPACE;
	map[SysVController::C_ELEM+1] = EC_KEYCOUNT;

	unsigned up = player ? EC_JOY2_UP : EC_JOY1_UP;
	unsigned down = player ? EC_JOY2_DOWN : EC_JOY1_DOWN;
	unsigned left = player ? EC_JOY2_LEFT : EC_JOY1_LEFT;
	unsigned right = player ? EC_JOY2_RIGHT : EC_JOY1_RIGHT;
	map[SysVController::D_ELEM] = up | (left << 8);
	map[SysVController::D_ELEM+1] = up;
	map[SysVController::D_ELEM+2] = up | (right << 8);
	map[SysVController::D_ELEM+3] = left;
	map[SysVController::D_ELEM+5] = right;
	map[SysVController::D_ELEM+6] = down | (left << 8);
	map[SysVController::D_ELEM+7] = down;
	map[SysVController::D_ELEM+8] = down | (right << 8);
}

unsigned EmuSystem::translateInputAction(unsigned input, bool &turbo)
{
	turbo = 0;
	switch(input)
	{
		case msxKeyIdxUp: return EC_JOY1_UP;
		case msxKeyIdxRight: return EC_JOY1_RIGHT;
		case msxKeyIdxDown: return EC_JOY1_DOWN;
		case msxKeyIdxLeft: return EC_JOY1_LEFT;
		case msxKeyIdxLeftUp: return EC_JOY1_LEFT | (EC_JOY1_UP << 8);
		case msxKeyIdxRightUp: return EC_JOY1_RIGHT | (EC_JOY1_UP << 8);
		case msxKeyIdxRightDown: return EC_JOY1_RIGHT | (EC_JOY1_DOWN << 8);
		case msxKeyIdxLeftDown: return EC_JOY1_LEFT | (EC_JOY1_DOWN << 8);
		case msxKeyIdxJS1BtnTurbo: turbo = 1; [[fallthrough]];
		case msxKeyIdxJS1Btn: return EC_JOY1_BUTTON1;
		case msxKeyIdxJS2BtnTurbo: turbo = 1; [[fallthrough]];
		case msxKeyIdxJS2Btn: return EC_JOY1_BUTTON2;

		case msxKeyIdxUp2: return EC_JOY2_UP;
		case msxKeyIdxRight2: return EC_JOY2_RIGHT;
		case msxKeyIdxDown2: return EC_JOY2_DOWN;
		case msxKeyIdxLeft2: return EC_JOY2_LEFT;
		case msxKeyIdxLeftUp2: return EC_JOY2_LEFT | (EC_JOY2_UP << 8);
		case msxKeyIdxRightUp2: return EC_JOY2_RIGHT | (EC_JOY2_UP << 8);
		case msxKeyIdxRightDown2: return EC_JOY2_RIGHT | (EC_JOY2_DOWN << 8);
		case msxKeyIdxLeftDown2: return EC_JOY2_LEFT | (EC_JOY2_DOWN << 8);
		case msxKeyIdxJS1BtnTurbo2: turbo = 1; [[fallthrough]];
		case msxKeyIdxJS1Btn2: return EC_JOY2_BUTTON1;
		case msxKeyIdxJS2BtnTurbo2: turbo = 1; [[fallthrough]];
		case msxKeyIdxJS2Btn2: return EC_JOY2_BUTTON2;

		case msxKeyIdxColeco0Num ... msxKeyIdxColecoHash :
			return (input - msxKeyIdxColeco0Num) + EC_COLECO1_0;
		case msxKeyIdxColeco0Num2 ... msxKeyIdxColecoHash2 :
			return (input - msxKeyIdxColeco0Num) + EC_COLECO2_0;

		case msxKeyIdxToggleKb: return EC_KEYCOUNT;
		case msxKeyIdxKbStart ... msxKeyIdxKbEnd :
			return (input - msxKeyIdxKbStart) + 1;
		default: bug_unreachable("input == %d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(EmuApp *appPtr, Input::Action action, unsigned emuKey)
{
	auto event1 = emuKey & 0xFF;
	if(event1 == EC_KEYCOUNT)
	{
		if(appPtr && action == Input::Action::PUSHED)
			appPtr->toggleKeyboard();
	}
	else
	{
		assert(event1 < EC_KEYCOUNT);
		eventMap[event1] = action == Input::Action::PUSHED;
		auto event2 = emuKey >> 8;
		if(event2) // extra event for diagonals
		{
			eventMap[event2] = action == Input::Action::PUSHED;
		}
	}
}

void EmuSystem::clearInputBuffers(EmuInputView &)
{
	IG::fill(eventMap);
}
