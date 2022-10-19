/*  This file is part of NEO.emu.

	NEO.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NEO.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NEO.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuInput.hh>
#include "MainSystem.hh"

extern "C"
{
	#include <gngeo/memory.h>
}

namespace EmuEx
{

enum
{
	neogeoKeyIdxUp = EmuEx::Controls::systemKeyMapStart,
	neogeoKeyIdxRight,
	neogeoKeyIdxDown,
	neogeoKeyIdxLeft,
	neogeoKeyIdxLeftUp,
	neogeoKeyIdxRightUp,
	neogeoKeyIdxRightDown,
	neogeoKeyIdxLeftDown,
	neogeoKeyIdxSelect,
	neogeoKeyIdxStart,
	neogeoKeyIdxA,
	neogeoKeyIdxB,
	neogeoKeyIdxX,
	neogeoKeyIdxY,
	neogeoKeyIdxATurbo,
	neogeoKeyIdxBTurbo,
	neogeoKeyIdxXTurbo,
	neogeoKeyIdxYTurbo,
	neogeoKeyIdxABC,
	neogeoKeyIdxTestSwitch = EmuEx::Controls::systemKeyMapStart + EmuEx::Controls::joystickKeys*2
};

const char *EmuSystem::inputFaceBtnName = "A/B/C/D";
const char *EmuSystem::inputCenterBtnName = "Select/Start";
const int EmuSystem::inputFaceBtns = 4;
const int EmuSystem::inputCenterBtns = 2;
const int EmuSystem::maxPlayers = 2;

namespace NGKey
{

using namespace IG;

static const unsigned COIN1 = bit(0), COIN2 = bit(1), SERVICE = bit(2),
	START1 = bit(0), SELECT1 = bit(1),
	START2 = bit(2), SELECT2 = bit(3),

	UP = bit(0), DOWN = bit(1), LEFT = bit(2), RIGHT = bit(3),
	A = bit(4), B = bit(5), C = bit(6), D = bit(7),

	START_EMU_INPUT = bit(8),
	SELECT_COIN_EMU_INPUT = bit(9),
	SERVICE_EMU_INPUT = bit(10);

}

VController::Map NeoSystem::vControllerMap(int player)
{
	using namespace NGKey;
	unsigned playerMask = player << 11;
	VController::Map map{};
	map[VController::F_ELEM] = A | playerMask;
	map[VController::F_ELEM+1] = B | playerMask;
	map[VController::F_ELEM+2] = C | playerMask;
	map[VController::F_ELEM+3] = D | playerMask;

	map[VController::C_ELEM] = SELECT_COIN_EMU_INPUT | playerMask;
	map[VController::C_ELEM+1] = START_EMU_INPUT | playerMask;

	map[VController::D_ELEM] = UP | LEFT | playerMask;
	map[VController::D_ELEM+1] = UP | playerMask;
	map[VController::D_ELEM+2] = UP | RIGHT | playerMask;
	map[VController::D_ELEM+3] = LEFT | playerMask;
	map[VController::D_ELEM+5] = RIGHT | playerMask;
	map[VController::D_ELEM+6] = DOWN | LEFT | playerMask;
	map[VController::D_ELEM+7] = DOWN | playerMask;
	map[VController::D_ELEM+8] = DOWN | RIGHT | playerMask;
	return map;
}

static bool isGamepadButton(unsigned input)
{
	switch(input)
	{
		case neogeoKeyIdxSelect:
		case neogeoKeyIdxStart:
		case neogeoKeyIdxXTurbo:
		case neogeoKeyIdxX:
		case neogeoKeyIdxYTurbo:
		case neogeoKeyIdxY:
		case neogeoKeyIdxATurbo:
		case neogeoKeyIdxA:
		case neogeoKeyIdxBTurbo:
		case neogeoKeyIdxB:
			return true;
		default: return false;
	}
}

unsigned NeoSystem::translateInputAction(unsigned input, bool &turbo)
{
	if(!isGamepadButton(input))
		turbo = 0;
	using namespace NGKey;
	if(input == neogeoKeyIdxTestSwitch) [[unlikely]]
	{
		return SERVICE_EMU_INPUT;
	}
	assert(input >= neogeoKeyIdxUp);
	unsigned player = (input - neogeoKeyIdxUp) / EmuEx::Controls::joystickKeys;
	unsigned playerMask = player << 11;
	input -= EmuEx::Controls::joystickKeys * player;
	switch(input)
	{
		case neogeoKeyIdxUp: return UP | playerMask;
		case neogeoKeyIdxRight: return RIGHT | playerMask;
		case neogeoKeyIdxDown: return DOWN | playerMask;
		case neogeoKeyIdxLeft: return LEFT | playerMask;
		case neogeoKeyIdxLeftUp: return LEFT | UP | playerMask;
		case neogeoKeyIdxRightUp: return RIGHT | UP | playerMask;
		case neogeoKeyIdxRightDown: return RIGHT | DOWN | playerMask;
		case neogeoKeyIdxLeftDown: return LEFT | DOWN | playerMask;
		case neogeoKeyIdxSelect: return SELECT_COIN_EMU_INPUT | playerMask;
		case neogeoKeyIdxStart: return START_EMU_INPUT | playerMask;
		case neogeoKeyIdxXTurbo: turbo = 1; [[fallthrough]];
		case neogeoKeyIdxX: return C | playerMask;
		case neogeoKeyIdxYTurbo: turbo = 1; [[fallthrough]];
		case neogeoKeyIdxY: return D | playerMask;
		case neogeoKeyIdxATurbo: turbo = 1; [[fallthrough]];
		case neogeoKeyIdxA: return A | playerMask;
		case neogeoKeyIdxBTurbo: turbo = 1; [[fallthrough]];
		case neogeoKeyIdxB: return B | playerMask;
		case neogeoKeyIdxABC: return A | B | C | playerMask;
		default: bug_unreachable("input == %d", input);
	}
	return 0;
}

void NeoSystem::handleInputAction(EmuApp *, InputAction a)
{
	auto player = a.key >> 11;
	bool isPushed = a.state == Input::Action::PUSHED;

	if(a.key & 0xFF) // joystick
	{
		auto &p = player ? memory.intern_p2 : memory.intern_p1;
		// Don't permit simultaneous left + right input, locks up Metal Slug 3
		if(isPushed && (a.key & 0xFF) == NGKey::LEFT)
		{
			p = IG::setBits(p, (Uint8)NGKey::RIGHT);
		}
		else if(isPushed && (a.key & 0xFF) == NGKey::RIGHT)
		{
			p = IG::setBits(p, (Uint8)NGKey::LEFT);
		}
		p = IG::setOrClearBits(p, (Uint8)(a.key & 0xFF), !isPushed);
		return;
	}

	if(a.key & NGKey::SELECT_COIN_EMU_INPUT)
	{
		if(conf.system == SYS_ARCADE)
		{
			unsigned bits = player ? NGKey::COIN2 : NGKey::COIN1;
			memory.intern_coin = IG::setOrClearBits(memory.intern_coin, (Uint8)bits, !isPushed);
		}
		else
		{
			// convert COIN to SELECT
			unsigned bits = player ? NGKey::SELECT2 : NGKey::SELECT1;
			memory.intern_start = IG::setOrClearBits(memory.intern_start, (Uint8)bits, !isPushed);
		}
		return;
	}

	if(a.key & NGKey::START_EMU_INPUT)
	{
		unsigned bits = player ? NGKey::START2 : NGKey::START1;
		memory.intern_start = IG::setOrClearBits(memory.intern_start, (Uint8)bits, !isPushed);
		return;
	}

	if(a.key & NGKey::SERVICE_EMU_INPUT)
	{
		if(isPushed)
			conf.test_switch = 1; // Test Switch is reset to 0 after every frame
		return;
	}
}

void NeoSystem::clearInputBuffers(EmuInputView &)
{
	memory.intern_coin = 0x7;
	memory.intern_start = 0x8F;
	memory.intern_p1 = 0xFF;
	memory.intern_p2 = 0xFF;
}

}
