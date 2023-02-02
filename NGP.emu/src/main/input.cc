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
	ngpKeyIdxUp = Controls::systemKeyMapStart,
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

constexpr std::array<unsigned, 4> dpadButtonCodes
{
	ngpKeyIdxUp,
	ngpKeyIdxRight,
	ngpKeyIdxDown,
	ngpKeyIdxLeft,
};

constexpr unsigned optionButtonCode[]{ngpKeyIdxOption};

constexpr unsigned faceButtonCodes[]
{
	ngpKeyIdxA,
	ngpKeyIdxB,
};

constexpr std::array gamepadComponents
{
	InputComponentDesc{"D-Pad", dpadButtonCodes, InputComponent::dPad, LB2DO},
	InputComponentDesc{"Face Buttons", faceButtonCodes, InputComponent::button, RB2DO},
	InputComponentDesc{"Option", optionButtonCode, InputComponent::button, RB2DO},
};

constexpr SystemInputDeviceDesc gamepadDesc{"Gamepad", gamepadComponents};

const int EmuSystem::inputFaceBtns = 2;
const int EmuSystem::maxPlayers = 1;

constexpr unsigned ctrlUpBit = 0x01, ctrlDownBit = 0x02, ctrlLeftBit = 0x04, ctrlRightBit = 0x08,
		ctrlABit = 0x10, ctrlBBit = 0x20, ctrlOptionBit = 0x40;

static bool isGamepadButton(unsigned input)
{
	switch(input)
	{
		case ngpKeyIdxOption:
		case ngpKeyIdxATurbo:
		case ngpKeyIdxA:
		case ngpKeyIdxBTurbo:
		case ngpKeyIdxB:
			return true;
		default: return false;
	}
}

InputAction NgpSystem::translateInputAction(InputAction action)
{
	if(!isGamepadButton(action.key))
		action.setTurboFlag(false);
	action.key = [&] -> unsigned
	{
		switch(action.key)
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
			case ngpKeyIdxATurbo: action.setTurboFlag(true); [[fallthrough]];
			case ngpKeyIdxA: return ctrlABit;
			case ngpKeyIdxBTurbo: action.setTurboFlag(true); [[fallthrough]];
			case ngpKeyIdxB: return ctrlBBit;
		}
		bug_unreachable("invalid key");
	}();
	return action;
}

void NgpSystem::handleInputAction(EmuApp *, InputAction a)
{
	inputBuff = IG::setOrClearBits(inputBuff, (uint8_t)a.key, a.state == Input::Action::PUSHED);
}

void NgpSystem::clearInputBuffers(EmuInputView &)
{
	inputBuff = {};
}

VControllerImageIndex NgpSystem::mapVControllerButton(unsigned key) const
{
	using enum VControllerImageIndex;
	switch(key)
	{
		case ngpKeyIdxOption: return auxButton1;
		case ngpKeyIdxATurbo:
		case ngpKeyIdxA: return button1;
		case ngpKeyIdxBTurbo:
		case ngpKeyIdxB: return button2;
		default: return button1;
	}
}

SystemInputDeviceDesc NgpSystem::inputDeviceDesc(int idx) const
{
	return gamepadDesc;
}

}
