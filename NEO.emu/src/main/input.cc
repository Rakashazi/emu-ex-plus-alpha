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

constexpr std::array<unsigned, 4> dpadButtonCodes
{
	neogeoKeyIdxUp,
	neogeoKeyIdxRight,
	neogeoKeyIdxDown,
	neogeoKeyIdxLeft,
};

constexpr unsigned centerButtonCodes[]
{
	neogeoKeyIdxSelect,
	neogeoKeyIdxStart,
};

constexpr unsigned faceButtonCodes[]
{
	neogeoKeyIdxB,
	neogeoKeyIdxY,
	neogeoKeyIdxA,
	neogeoKeyIdxX,
};

constexpr std::array gamepadComponents
{
	InputComponentDesc{"D-Pad", dpadButtonCodes, InputComponent::dPad, LB2DO},
	InputComponentDesc{"Face Buttons", faceButtonCodes, InputComponent::button, RB2DO, InputComponentFlagsMask::staggeredLayout},
	InputComponentDesc{"Select", {&centerButtonCodes[0], 1}, InputComponent::button, LB2DO},
	InputComponentDesc{"Start", {&centerButtonCodes[1], 1}, InputComponent::button, RB2DO},
	InputComponentDesc{"Select/Start", centerButtonCodes, InputComponent::button, CB2DO, InputComponentFlagsMask::altConfig},
};

constexpr SystemInputDeviceDesc gamepadDesc{"Gamepad", gamepadComponents};

const int EmuSystem::inputFaceBtns = 4;
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

InputAction NeoSystem::translateInputAction(InputAction action)
{
	if(!isGamepadButton(action.key))
		action.setTurboFlag(false);
	using namespace NGKey;
	if(action.key == neogeoKeyIdxTestSwitch) [[unlikely]]
	{
		action.key = SERVICE_EMU_INPUT;
		return action;
	}
	assert(action.key >= neogeoKeyIdxUp);
	unsigned player = (action.key - neogeoKeyIdxUp) / EmuEx::Controls::joystickKeys;
	unsigned playerMask = player << 11;
	action.key -= EmuEx::Controls::joystickKeys * player;
	action.key = [&] -> unsigned
	{
		switch(action.key)
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
			case neogeoKeyIdxXTurbo: action.setTurboFlag(true); [[fallthrough]];
			case neogeoKeyIdxX: return C | playerMask;
			case neogeoKeyIdxYTurbo: action.setTurboFlag(true); [[fallthrough]];
			case neogeoKeyIdxY: return D | playerMask;
			case neogeoKeyIdxATurbo: action.setTurboFlag(true); [[fallthrough]];
			case neogeoKeyIdxA: return A | playerMask;
			case neogeoKeyIdxBTurbo: action.setTurboFlag(true); [[fallthrough]];
			case neogeoKeyIdxB: return B | playerMask;
			case neogeoKeyIdxABC: return A | B | C | playerMask;
		}
		bug_unreachable("invalid key");
	}();
	return action;
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
			p |= (Uint8)NGKey::RIGHT;
		}
		else if(isPushed && (a.key & 0xFF) == NGKey::RIGHT)
		{
			p |= (Uint8)NGKey::LEFT;
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

VControllerImageIndex NeoSystem::mapVControllerButton(unsigned key) const
{
	using enum VControllerImageIndex;
	switch(key)
	{
		case neogeoKeyIdxSelect: return auxButton1;
		case neogeoKeyIdxStart: return auxButton2;
		case neogeoKeyIdxATurbo:
		case neogeoKeyIdxA: return button1;
		case neogeoKeyIdxBTurbo:
		case neogeoKeyIdxB: return button2;
		case neogeoKeyIdxXTurbo:
		case neogeoKeyIdxX: return button3;
		case neogeoKeyIdxYTurbo:
		case neogeoKeyIdxY: return button4;
		default: return button1;
	}
}

SystemInputDeviceDesc NeoSystem::inputDeviceDesc(int idx) const
{
	return gamepadDesc;
}
}
