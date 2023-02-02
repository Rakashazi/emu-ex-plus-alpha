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

constexpr std::array<unsigned, 4> dpadButtonCodes
{
	gbcKeyIdxUp,
	gbcKeyIdxRight,
	gbcKeyIdxDown,
	gbcKeyIdxLeft,
};

constexpr unsigned centerButtonCodes[]
{
	gbcKeyIdxSelect,
	gbcKeyIdxStart,
};

constexpr unsigned faceButtonCodes[]
{
	gbcKeyIdxB,
	gbcKeyIdxA,
};

constexpr std::array gamepadComponents
{
	InputComponentDesc{"D-Pad", dpadButtonCodes, InputComponent::dPad, LB2DO},
	InputComponentDesc{"Face Buttons", faceButtonCodes, InputComponent::button, RB2DO},
	InputComponentDesc{"Select", {&centerButtonCodes[0], 1}, InputComponent::button, LB2DO},
	InputComponentDesc{"Start", {&centerButtonCodes[1], 1}, InputComponent::button, RB2DO},
	InputComponentDesc{"Select/Start", centerButtonCodes, InputComponent::button, CB2DO, InputComponentFlagsMask::altConfig},
};

constexpr SystemInputDeviceDesc gamepadDesc{"Gamepad", gamepadComponents};

const int EmuSystem::inputFaceBtns = 2;
const int EmuSystem::maxPlayers = 1;

static bool isGamepadButton(unsigned input)
{
	switch(input)
	{
		case gbcKeyIdxSelect:
		case gbcKeyIdxStart:
		case gbcKeyIdxATurbo:
		case gbcKeyIdxA:
		case gbcKeyIdxBTurbo:
		case gbcKeyIdxB:
			return true;
		default: return false;
	}
}

InputAction GbcSystem::translateInputAction(InputAction action)
{
	using namespace gambatte;
	if(!isGamepadButton(action.key))
		action.setTurboFlag(false);
	action.key = [&] -> unsigned
	{
		switch(action.key)
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
			case gbcKeyIdxATurbo: action.setTurboFlag(true); [[fallthrough]];
			case gbcKeyIdxA: return InputGetter::A;
			case gbcKeyIdxBTurbo: action.setTurboFlag(true); [[fallthrough]];
			case gbcKeyIdxB: return InputGetter::B;
		}
		bug_unreachable("invalid key");
	}();
	return action;
}

void GbcSystem::handleInputAction(EmuApp *, InputAction a)
{
	gbcInput.bits = IG::setOrClearBits(gbcInput.bits, a.key, a.state == Input::Action::PUSHED);
}

void GbcSystem::clearInputBuffers(EmuInputView &)
{
	gbcInput.bits = 0;
}

VControllerImageIndex GbcSystem::mapVControllerButton(unsigned key) const
{
	using enum VControllerImageIndex;
	switch(key)
	{
		case gbcKeyIdxSelect: return auxButton1;
		case gbcKeyIdxStart: return auxButton2;
		case gbcKeyIdxATurbo:
		case gbcKeyIdxA: return button1;
		case gbcKeyIdxBTurbo:
		case gbcKeyIdxB: return button2;
		default: return button1;
	}
}

SystemInputDeviceDesc GbcSystem::inputDeviceDesc(int idx) const
{
	return gamepadDesc;
}

}
