/*  This file is part of 2600.emu.

	2600.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	2600.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with 2600.emu.  If not, see <http://www.gnu.org/licenses/> */

// TODO: Some Stella types collide with MacTypes.h
#define BytePtr BytePtrMac
#define Debugger DebuggerMac
#include <emuframework/EmuApp.hh>
#undef BytePtr
#undef Debugger
#include "internal.hh"

enum
{
	vcsKeyIdxUp = EmuControls::systemKeyMapStart,
	vcsKeyIdxRight,
	vcsKeyIdxDown,
	vcsKeyIdxLeft,
	vcsKeyIdxLeftUp,
	vcsKeyIdxRightUp,
	vcsKeyIdxRightDown,
	vcsKeyIdxLeftDown,
	vcsKeyIdxJSBtn,
	vcsKeyIdxJSBtnTurbo,

	vcsKeyIdxUp2,
	vcsKeyIdxRight2,
	vcsKeyIdxDown2,
	vcsKeyIdxLeft2,
	vcsKeyIdxLeftUp2,
	vcsKeyIdxRightUp2,
	vcsKeyIdxRightDown2,
	vcsKeyIdxLeftDown2,
	vcsKeyIdxJSBtn2,
	vcsKeyIdxJSBtnTurbo2,

	vcsKeyIdxSelect,
	vcsKeyIdxReset,
	vcsKeyIdxP1Diff,
	vcsKeyIdxP2Diff,
	vcsKeyIdxColorBW,
	vcsKeyIdxKeyboard1Base,
	vcsKeyIdxKeyboard2Base = vcsKeyIdxKeyboard1Base + 12,
};

const char *EmuSystem::inputFaceBtnName = "JS Buttons";
const char *EmuSystem::inputCenterBtnName = "Select/Reset";
const uint EmuSystem::inputFaceBtns = 2;
const uint EmuSystem::inputCenterBtns = 2;
const bool EmuSystem::inputHasTriggerBtns = false;
const bool EmuSystem::inputHasRevBtnLayout = false;
const uint EmuSystem::maxPlayers = 2;

void EmuSystem::clearInputBuffers(EmuInputView &)
{
	Event &ev = osystem.eventHandler().event();
	ev.clear();

	ev.set(Event::ConsoleLeftDiffB, p1DiffB);
	ev.set(Event::ConsoleLeftDiffA, !p1DiffB);
	ev.set(Event::ConsoleRightDiffB, p2DiffB);
	ev.set(Event::ConsoleRightDiffA, !p2DiffB);
	ev.set(Event::ConsoleColor, vcsColor);
	ev.set(Event::ConsoleBlackWhite, !vcsColor);
}

void updateVControllerMapping(uint player, SysVController::Map &map)
{
	uint playerShift = player ? 7 : 0;
	map[SysVController::F_ELEM] = Event::JoystickZeroFire + playerShift;
	map[SysVController::F_ELEM+1] = (Event::JoystickZeroFire + playerShift) | SysVController::TURBO_BIT;

	map[SysVController::C_ELEM] = Event::ConsoleSelect;
	map[SysVController::C_ELEM+1] = Event::ConsoleReset;

	map[SysVController::D_ELEM] = (((uint)Event::JoystickZeroUp) + playerShift)
																| (((uint)Event::JoystickZeroLeft + playerShift) << 8);
	map[SysVController::D_ELEM+1] = Event::JoystickZeroUp + playerShift; // up
	map[SysVController::D_ELEM+2] = ((uint)Event::JoystickZeroUp  + playerShift)
																	| (((uint)Event::JoystickZeroRight + playerShift) << 8);
	map[SysVController::D_ELEM+3] = Event::JoystickZeroLeft + playerShift; // left
	map[SysVController::D_ELEM+5] = Event::JoystickZeroRight + playerShift; // right
	map[SysVController::D_ELEM+6] = ((uint)Event::JoystickZeroDown + playerShift)
																	| (((uint)Event::JoystickZeroLeft + playerShift) << 8);
	map[SysVController::D_ELEM+7] = Event::JoystickZeroDown + playerShift; // down
	map[SysVController::D_ELEM+8] = ((uint)Event::JoystickZeroDown + playerShift)
																	| (((uint)Event::JoystickZeroRight + playerShift) << 8);
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	switch(input)
	{
		case vcsKeyIdxUp: return Event::JoystickZeroUp;
		case vcsKeyIdxRight: return Event::JoystickZeroRight;
		case vcsKeyIdxDown: return Event::JoystickZeroDown;
		case vcsKeyIdxLeft: return Event::JoystickZeroLeft;
		case vcsKeyIdxLeftUp: return Event::JoystickZeroLeft | (Event::JoystickZeroUp << 8);
		case vcsKeyIdxRightUp: return Event::JoystickZeroRight | (Event::JoystickZeroUp << 8);
		case vcsKeyIdxRightDown: return Event::JoystickZeroRight | (Event::JoystickZeroDown << 8);
		case vcsKeyIdxLeftDown: return Event::JoystickZeroLeft | (Event::JoystickZeroDown << 8);
		case vcsKeyIdxJSBtnTurbo: turbo = 1; [[fallthrough]];
		case vcsKeyIdxJSBtn: return Event::JoystickZeroFire;

		case vcsKeyIdxUp2: return Event::JoystickOneUp;
		case vcsKeyIdxRight2: return Event::JoystickOneRight;
		case vcsKeyIdxDown2: return Event::JoystickOneDown;
		case vcsKeyIdxLeft2: return Event::JoystickOneLeft;
		case vcsKeyIdxLeftUp2: return Event::JoystickOneLeft | (Event::JoystickOneUp << 8);
		case vcsKeyIdxRightUp2: return Event::JoystickOneRight | (Event::JoystickOneUp << 8);
		case vcsKeyIdxRightDown2: return Event::JoystickOneRight | (Event::JoystickOneDown << 8);
		case vcsKeyIdxLeftDown2: return Event::JoystickOneLeft | (Event::JoystickOneDown << 8);
		case vcsKeyIdxJSBtnTurbo2: turbo = 1; [[fallthrough]];
		case vcsKeyIdxJSBtn2: return Event::JoystickOneFire;

		case vcsKeyIdxSelect: return Event::ConsoleSelect;
		case vcsKeyIdxP1Diff: return Event::Combo1; // toggle P1 diff
		case vcsKeyIdxP2Diff: return Event::Combo2; // toggle P2 diff
		case vcsKeyIdxColorBW: return Event::Combo3; // toggle Color/BW
		case vcsKeyIdxReset: return Event::ConsoleReset;
		case vcsKeyIdxKeyboard1Base ... vcsKeyIdxKeyboard1Base + 11:
			return Event::KeyboardZero1 + (input - vcsKeyIdxKeyboard1Base);
		case vcsKeyIdxKeyboard2Base ... vcsKeyIdxKeyboard2Base + 11:
			return Event::KeyboardOne1 + (input - vcsKeyIdxKeyboard2Base);
		default: bug_branch("%d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(uint state, uint emuKey)
{
	auto &ev = osystem.eventHandler().event();
	uint event1 = emuKey & 0xFF;

	//logMsg("got key %d", emuKey);

	switch(event1)
	{
		bcase Event::Combo1:
			if(state != Input::PUSHED)
				break;
			p1DiffB ^= true;
			EmuApp::postMessage(1, false, p1DiffB ? "P1 Difficulty -> B" : "P1 Difficulty -> A");
			ev.set(Event::ConsoleLeftDiffB, p1DiffB);
			ev.set(Event::ConsoleLeftDiffA, !p1DiffB);
		bcase Event::Combo2:
			if(state != Input::PUSHED)
				break;
			p2DiffB ^= true;
			EmuApp::postMessage(1, false, p2DiffB ? "P2 Difficulty -> B" : "P2 Difficulty -> A");
			ev.set(Event::ConsoleRightDiffB, p2DiffB);
			ev.set(Event::ConsoleRightDiffA, !p2DiffB);
		bcase Event::Combo3:
			if(state != Input::PUSHED)
				break;
			vcsColor ^= true;
			EmuApp::postMessage(1, false, vcsColor ? "Color Switch -> Color" : "Color Switch -> B&W");
			ev.set(Event::ConsoleColor, vcsColor);
			ev.set(Event::ConsoleBlackWhite, !vcsColor);
		bcase Event::JoystickZeroFire5: // TODO: add turbo support for on-screen controls to framework
			ev.set(Event::Type(Event::JoystickZeroFire), state == Input::PUSHED);
			if(state == Input::PUSHED)
				turboActions.addEvent(Event::JoystickZeroFire);
			else
				turboActions.removeEvent(Event::JoystickZeroFire);
		bcase Event::JoystickOneFire5: // TODO: add turbo support for on-screen controls to framework
			ev.set(Event::Type(Event::JoystickOneFire), state == Input::PUSHED);
			if(state == Input::PUSHED)
				turboActions.addEvent(Event::JoystickOneFire);
			else
				turboActions.removeEvent(Event::JoystickOneFire);
		bcase Event::KeyboardZero1 ... Event::KeyboardOnePound:
			ev.set(Event::Type(event1), state == Input::PUSHED);
		bdefault:
			ev.set(Event::Type(event1), state == Input::PUSHED);
			uint event2 = emuKey >> 8;
			if(event2) // extra event for diagonals
			{
				ev.set(Event::Type(event2), state == Input::PUSHED);
			}
	}
}
