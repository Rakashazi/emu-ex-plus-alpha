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

#include <emuframework/EmuInput.hh>
#include "MainApp.hh"

extern "C"
{
	#include <blueMSX/Input/InputEvent.h>
}

namespace EmuEx
{

static const unsigned msxKeyboardKeys = 92;

enum
{
	msxKeyIdxUp = EmuEx::Controls::systemKeyMapStart,
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

constexpr std::array<unsigned, 4> dpadButtonCodes
{
	msxKeyIdxUp,
	msxKeyIdxRight,
	msxKeyIdxDown,
	msxKeyIdxLeft,
};

constexpr unsigned shortcutButtonCodes[]
{
	msxKeyIdxKbStart + EC_SPACE,
	msxKeyIdxToggleKb,
};

constexpr unsigned jsButtonCodes[]
{
	msxKeyIdxJS1Btn,
	msxKeyIdxJS2Btn,
};

constexpr std::array jsComponents
{
	InputComponentDesc{"D-Pad", dpadButtonCodes, InputComponent::dPad, LB2DO},
	InputComponentDesc{"Space & Keyboard Toggle", shortcutButtonCodes, InputComponent::button, CB2DO},
	InputComponentDesc{"Joystick Button", jsButtonCodes, InputComponent::button, RB2DO}
};

constexpr SystemInputDeviceDesc jsDesc{"Joystick", jsComponents};

const int EmuSystem::inputFaceBtns = 2;
bool EmuSystem::inputHasKeyboard = true;
const int EmuSystem::maxPlayers = 2;

static VController::KbMap kbToEventMap
{
	EC_Q, EC_W, EC_E, EC_R, EC_T, EC_Y, EC_U, EC_I, EC_O, EC_P,
	EC_A, EC_S, EC_D, EC_F, EC_G, EC_H, EC_J, EC_K, EC_L, EC_NONE,
	EC_CAPS, EC_Z, EC_X, EC_C, EC_V, EC_B, EC_N, EC_M, EC_BKSPACE, EC_NONE,
	EC_NONE, EC_NONE, EC_NONE, EC_SPACE, EC_SPACE, EC_SPACE, EC_SPACE, EC_CTRL, EC_CTRL, EC_RETURN
};

static VController::KbMap kbToEventMap2
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
		for(auto i : iotaCount(10)) // 1 - 0
			kbToEventMap2[10 + i] = EC_1 + i;
		kbToEventMap2[23] = EC_3 | (EC_LSHIFT << 8);
	}
	else
	{
		for(auto i : iotaCount(9)) // 1 - 9
			kbToEventMap2[10 + i] = EC_COLECO1_1 + i;
		kbToEventMap2[19] = EC_COLECO1_0;
		kbToEventMap2[23] = EC_COLECO1_HASH;
	}
	app.updateKeyboardMapping();
}

VController::KbMap MsxSystem::vControllerKeyboardMap(VControllerKbMode mode)
{
	return mode == VControllerKbMode::LAYOUT_2 ? kbToEventMap2 : kbToEventMap;
}

static bool isJoystickButton(unsigned input)
{
	switch(input)
	{
		case msxKeyIdxJS1BtnTurbo:
		case msxKeyIdxJS1Btn:
		case msxKeyIdxJS2BtnTurbo:
		case msxKeyIdxJS2Btn:
		case msxKeyIdxJS1BtnTurbo2:
		case msxKeyIdxJS1Btn2:
		case msxKeyIdxJS2BtnTurbo2:
		case msxKeyIdxJS2Btn2:
			return true;
		default: return false;
	}
}

InputAction MsxSystem::translateInputAction(InputAction action)
{
	if(!isJoystickButton(action.key))
		action.setTurboFlag(false);
	action.key = [&] -> unsigned
	{
		switch(action.key)
		{
			case msxKeyIdxUp: return EC_JOY1_UP;
			case msxKeyIdxRight: return EC_JOY1_RIGHT;
			case msxKeyIdxDown: return EC_JOY1_DOWN;
			case msxKeyIdxLeft: return EC_JOY1_LEFT;
			case msxKeyIdxLeftUp: return EC_JOY1_LEFT | (EC_JOY1_UP << 8);
			case msxKeyIdxRightUp: return EC_JOY1_RIGHT | (EC_JOY1_UP << 8);
			case msxKeyIdxRightDown: return EC_JOY1_RIGHT | (EC_JOY1_DOWN << 8);
			case msxKeyIdxLeftDown: return EC_JOY1_LEFT | (EC_JOY1_DOWN << 8);
			case msxKeyIdxJS1BtnTurbo: action.setTurboFlag(true); [[fallthrough]];
			case msxKeyIdxJS1Btn: return EC_JOY1_BUTTON1;
			case msxKeyIdxJS2BtnTurbo: action.setTurboFlag(true); [[fallthrough]];
			case msxKeyIdxJS2Btn: return EC_JOY1_BUTTON2;

			case msxKeyIdxUp2: return EC_JOY2_UP;
			case msxKeyIdxRight2: return EC_JOY2_RIGHT;
			case msxKeyIdxDown2: return EC_JOY2_DOWN;
			case msxKeyIdxLeft2: return EC_JOY2_LEFT;
			case msxKeyIdxLeftUp2: return EC_JOY2_LEFT | (EC_JOY2_UP << 8);
			case msxKeyIdxRightUp2: return EC_JOY2_RIGHT | (EC_JOY2_UP << 8);
			case msxKeyIdxRightDown2: return EC_JOY2_RIGHT | (EC_JOY2_DOWN << 8);
			case msxKeyIdxLeftDown2: return EC_JOY2_LEFT | (EC_JOY2_DOWN << 8);
			case msxKeyIdxJS1BtnTurbo2: action.setTurboFlag(true); [[fallthrough]];
			case msxKeyIdxJS1Btn2: return EC_JOY2_BUTTON1;
			case msxKeyIdxJS2BtnTurbo2: action.setTurboFlag(true); [[fallthrough]];
			case msxKeyIdxJS2Btn2: return EC_JOY2_BUTTON2;

			case msxKeyIdxColeco0Num ... msxKeyIdxColecoHash :
				return (action.key - msxKeyIdxColeco0Num) + EC_COLECO1_0;
			case msxKeyIdxColeco0Num2 ... msxKeyIdxColecoHash2 :
				return (action.key - msxKeyIdxColeco0Num) + EC_COLECO2_0;

			case msxKeyIdxToggleKb: return EC_KEYCOUNT;
			case msxKeyIdxKbStart ... msxKeyIdxKbEnd :
				if(activeBoardType == BOARD_COLECO)
				{
					return EC_COLECO1_STAR;
				}
				else
				{
					return (action.key - msxKeyIdxKbStart) + 1;
				}
		}
		bug_unreachable("invalid key");
	}();
	return action;
}

void MsxSystem::handleInputAction(EmuApp *appPtr, InputAction a)
{
	auto event1 = a.key & 0xFF;
	bool isPushed = a.state == Input::Action::PUSHED;
	if(event1 == EC_KEYCOUNT)
	{
		if(appPtr && isPushed)
			appPtr->toggleKeyboard();
	}
	else
	{
		assert(event1 < EC_KEYCOUNT);
		eventMap[event1] = isPushed;
		auto event2 = a.key >> 8;
		if(event2) // extra event for diagonals
		{
			eventMap[event2] = isPushed;
		}
	}
}

void MsxSystem::clearInputBuffers(EmuInputView &)
{
	IG::fill(eventMap);
}

VControllerImageIndex MsxSystem::mapVControllerButton(unsigned key) const
{
	using enum VControllerImageIndex;
	switch(key)
	{
		case msxKeyIdxKbStart + EC_SPACE: return auxButton1;
		case msxKeyIdxToggleKb: return auxButton1;
		case msxKeyIdxJS1Btn:
		case msxKeyIdxJS1BtnTurbo: return button1;
		case msxKeyIdxJS2Btn:
		case msxKeyIdxJS2BtnTurbo: return button2;
		default: return button1;
	}
}

SystemInputDeviceDesc MsxSystem::inputDeviceDesc(int idx) const
{
	return jsDesc;
}

}
