/*  This file is part of GBC.emu.

	GBC.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	GBC.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with GBC.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include "MainSystem.hh"

namespace EmuEx
{

enum
{
	gbcKeyIdxUp = EmuEx::Controls::systemKeyMapStart,
	gbcKeyIdxRight,
	gbcKeyIdxDown,
	gbcKeyIdxLeft,
	gbcKeyIdxLeftUp,
	gbcKeyIdxRightUp,
	gbcKeyIdxRightDown,
	gbcKeyIdxLeftDown,
	gbcKeyIdxSelect,
	gbcKeyIdxStart,
	gbcKeyIdxA,
	gbcKeyIdxB,
	gbcKeyIdxATurbo,
	gbcKeyIdxBTurbo
};

const char *EmuSystem::inputFaceBtnName = "A/B";
const char *EmuSystem::inputCenterBtnName = "Select/Start";
const int EmuSystem::inputFaceBtns = 2;
const int EmuSystem::inputCenterBtns = 2;
const int EmuSystem::maxPlayers = 1;
std::array<int, EmuSystem::MAX_FACE_BTNS> EmuSystem::vControllerImageMap{1, 0};
GbcInput gbcInput{};

VController::Map GbcSystem::vControllerMap(int player)
{
	using namespace gambatte;
	VController::Map map;
	map[VController::F_ELEM] = InputGetter::B;
	map[VController::F_ELEM+1] = InputGetter::A;

	map[VController::C_ELEM] = InputGetter::SELECT;
	map[VController::C_ELEM+1] = InputGetter::START;

	map[VController::D_ELEM] = InputGetter::UP | InputGetter::LEFT;
	map[VController::D_ELEM+1] = InputGetter::UP;
	map[VController::D_ELEM+2] = InputGetter::UP | InputGetter::RIGHT;
	map[VController::D_ELEM+3] = InputGetter::LEFT;
	map[VController::D_ELEM+5] = InputGetter::RIGHT;
	map[VController::D_ELEM+6] = InputGetter::DOWN | InputGetter::LEFT;
	map[VController::D_ELEM+7] = InputGetter::DOWN;
	map[VController::D_ELEM+8] = InputGetter::DOWN | InputGetter::RIGHT;
	return map;
}

unsigned GbcSystem::translateInputAction(unsigned input, bool &turbo)
{
	using namespace gambatte;
	turbo = 0;
	switch(input)
	{
		case gbcKeyIdxUp: return InputGetter::UP;
		case gbcKeyIdxRight: return InputGetter::RIGHT;
		case gbcKeyIdxDown: return InputGetter::DOWN;
		case gbcKeyIdxLeft: return InputGetter::LEFT;
		case gbcKeyIdxLeftUp: return InputGetter::LEFT | InputGetter::UP;
		case gbcKeyIdxRightUp: return InputGetter::RIGHT | InputGetter::UP;
		case gbcKeyIdxRightDown: return InputGetter::RIGHT | InputGetter::DOWN;
		case gbcKeyIdxLeftDown: return InputGetter::LEFT | InputGetter::DOWN;
		case gbcKeyIdxSelect: return InputGetter::SELECT;
		case gbcKeyIdxStart: return InputGetter::START;
		case gbcKeyIdxATurbo: turbo = 1; [[fallthrough]];
		case gbcKeyIdxA: return InputGetter::A;
		case gbcKeyIdxBTurbo: turbo = 1; [[fallthrough]];
		case gbcKeyIdxB: return InputGetter::B;
		default: bug_unreachable("input == %d", input);
	}
	return 0;
}

void GbcSystem::handleInputAction(EmuApp *, InputAction a)
{
	gbcInput.bits = IG::setOrClearBits(gbcInput.bits, a.key, a.state == Input::Action::PUSHED);
}

void GbcSystem::clearInputBuffers(EmuInputView &)
{
	gbcInput.bits = 0;
}

}
