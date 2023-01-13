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

constexpr std::array<unsigned, 4> dpadButtonCodes
{
	pceKeyIdxUp,
	pceKeyIdxRight,
	pceKeyIdxDown,
	pceKeyIdxLeft,
};

constexpr unsigned centerButtonCodes[]
{
	pceKeyIdxSelect,
	pceKeyIdxRun,
};

constexpr unsigned faceButtonCodes[]
{
	pceKeyIdxIII,
	pceKeyIdxII,
	pceKeyIdxI,
	pceKeyIdxIV,
	pceKeyIdxV,
	pceKeyIdxVI,
};

constexpr std::array gamepadComponents
{
	InputComponentDesc{"D-Pad", dpadButtonCodes, InputComponent::dPad, LB2DO},
	InputComponentDesc{"Center Buttons", centerButtonCodes, InputComponent::button, CB2DO},
	InputComponentDesc{"Face Buttons", faceButtonCodes, InputComponent::button, RB2DO}
};

constexpr SystemInputDeviceDesc gamepadDesc{"Gamepad", gamepadComponents};

const int EmuSystem::inputFaceBtns = 6;
const int EmuSystem::maxPlayers = 5;
unsigned playerBit = 13;

static bool isGamepadButton(unsigned input)
{
	switch(input)
	{
		case pceKeyIdxSelect:
		case pceKeyIdxRun:
		case pceKeyIdxITurbo:
		case pceKeyIdxI:
		case pceKeyIdxIITurbo:
		case pceKeyIdxII:
		case pceKeyIdxIII:
		case pceKeyIdxIV:
		case pceKeyIdxV:
		case pceKeyIdxVI:
			return true;
		default: return false;
	}
}

InputAction PceSystem::translateInputAction(InputAction action)
{
	if(!isGamepadButton(action.key))
		action.setTurboFlag(false);
	assert(action.key >= pceKeyIdxUp);
	unsigned player = (action.key - pceKeyIdxUp) / Controls::gamepadKeys;
	unsigned playerMask = player << playerBit;
	action.key -= Controls::gamepadKeys * player;
	action.key = [&] -> unsigned
	{
		switch(action.key)
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
			case pceKeyIdxITurbo: action.setTurboFlag(true); [[fallthrough]];
			case pceKeyIdxI: return bit(0) | playerMask;
			case pceKeyIdxIITurbo: action.setTurboFlag(true); [[fallthrough]];
			case pceKeyIdxII: return bit(1) | playerMask;
			case pceKeyIdxIII: return bit(8) | playerMask;
			case pceKeyIdxIV: return bit(9) | playerMask;
			case pceKeyIdxV: return bit(10) | playerMask;
			case pceKeyIdxVI: return bit(11) | playerMask;
		}
		bug_unreachable("invalid key");
	}();
	return action;
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
		for(auto &padData : std::span{inputBuff.data(), 2})
			padData = IG::bit(12);
	}
}

void set6ButtonPadEnabled(EmuApp &app, bool on)
{
	if(on)
	{
		app.unsetDisabledInputKeys();
	}
	else
	{
		static constexpr unsigned extraCodes[]{pceKeyIdxIII, pceKeyIdxIV, pceKeyIdxV, pceKeyIdxVI};
		app.setDisabledInputKeys(extraCodes);
	}
}

VControllerImageIndex PceSystem::mapVControllerButton(unsigned key) const
{
	using enum VControllerImageIndex;
	switch(key)
	{
		case pceKeyIdxSelect: return auxButton1;
		case pceKeyIdxRun: return auxButton2;
		case pceKeyIdxITurbo:
		case pceKeyIdxI: return button1;
		case pceKeyIdxIITurbo:
		case pceKeyIdxII: return button2;
		case pceKeyIdxIII: return button3;
		case pceKeyIdxIV: return button4;
		case pceKeyIdxV: return button5;
		case pceKeyIdxVI: return button6;
		default: return button1;
	}
}

SystemInputDeviceDesc PceSystem::inputDeviceDesc(int idx) const
{
	return gamepadDesc;
}

}
