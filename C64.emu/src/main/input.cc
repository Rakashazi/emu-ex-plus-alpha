/*  This file is part of C64.emu.

	C64.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	C64.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with C64.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include "internal.hh"

enum
{
	c64KeyIdxUp = EmuControls::systemKeyMapStart,
	c64KeyIdxRight,
	c64KeyIdxDown,
	c64KeyIdxLeft,
	c64KeyIdxLeftUp,
	c64KeyIdxRightUp,
	c64KeyIdxRightDown,
	c64KeyIdxLeftDown,
	c64KeyIdxBtn,
	c64KeyIdxBtnTurbo,
	c64KeyIdxSwapPorts,

	c64KeyIdxUp2,
	c64KeyIdxRight2,
	c64KeyIdxDown2,
	c64KeyIdxLeft2,
	c64KeyIdxLeftUp2,
	c64KeyIdxRightUp2,
	c64KeyIdxRightDown2,
	c64KeyIdxLeftDown2,
	c64KeyIdxBtn2,
	c64KeyIdxBtnTurbo2,
	c64KeyIdxSwapPorts2,

	c64KeyToggleKB,

	c64KeyF1,
	c64KeyF2,
	c64KeyF3,
	c64KeyF4,
	c64KeyF5,
	c64KeyF6,
	c64KeyF7,
	c64KeyF8,

	c64KeyLeftArrow,
	c64Key1,
	c64Key2,
	c64Key3,
	c64Key4,
	c64Key5,
	c64Key6,
	c64Key7,
	c64Key8,
	c64Key9,
	c64Key0,
	c64KeyPlus,
	c64KeyMinus,
	c64KeyPound,
	c64KeyClrHome,
	c64KeyInstDel,

	c64KeyCtrl,
	c64KeyQ,
	c64KeyW,
	c64KeyE,
	c64KeyR,
	c64KeyT,
	c64KeyY,
	c64KeyU,
	c64KeyI,
	c64KeyO,
	c64KeyP,
	c64KeyAt,
	c64KeyAsterisk,
	c64KeyUpArrow,
	c64KeyRestore,

	c64KeyRunStop,
	c64KeyShiftLock,
	c64KeyA,
	c64KeyS,
	c64KeyD,
	c64KeyF,
	c64KeyG,
	c64KeyH,
	c64KeyJ,
	c64KeyK,
	c64KeyL,
	c64KeyColon,
	c64KeySemiColon,
	c64KeyEquals,
	c64KeyReturn,

	c64KeyCommodore,
	c64KeyLeftShift,
	c64KeyZ,
	c64KeyX,
	c64KeyC,
	c64KeyV,
	c64KeyB,
	c64KeyN,
	c64KeyM,
	c64KeyComma,
	c64KeyPeriod,
	c64KeySlash,
	c64KeyRightShift,
	c64KeyKbUp,
	c64KeyKbRight,
	c64KeyKbDown,
	c64KeyKbLeft,

	c64KeySpace,
	c64KeyCtrlLock,
};

const char *EmuSystem::inputFaceBtnName = "JS Buttons";
const char *EmuSystem::inputCenterBtnName = "F1/KB";
const uint EmuSystem::inputFaceBtns = 2;
const uint EmuSystem::inputCenterBtns = 2;
const bool EmuSystem::inputHasTriggerBtns = false;
const bool EmuSystem::inputHasRevBtnLayout = false;
bool EmuSystem::inputHasKeyboard = true;
const uint EmuSystem::maxPlayers = 2;

static bool shiftLock = false, ctrlLock = false;

static const uint JOYPAD_FIRE = 0x10,
	JOYPAD_E = 0x08,
	JOYPAD_W = 0x04,
	JOYPAD_S = 0x02,
	JOYPAD_N = 0x01,
	JOYPAD_SW = (JOYPAD_S | JOYPAD_W),
	JOYPAD_SE = (JOYPAD_S | JOYPAD_E),
	JOYPAD_NW = (JOYPAD_N | JOYPAD_W),
	JOYPAD_NE = (JOYPAD_N | JOYPAD_E);

static const uint JS_SHIFT = 16;

static const uint SHIFT_BIT = IG::bit(8);

static constexpr uint mkKeyCode(int row, int col, int shift = 0)
{
	return (shift << 8) | (row << 4) | col;
}

static const uint
KB_NONE = 0x7F,
KB_INST_DEL = mkKeyCode(0,0),
KB_RETURN = mkKeyCode(0,1),
KB_CRSR_LR = mkKeyCode(0,2),
KB_F7 = mkKeyCode(0,3),
KB_F1 = mkKeyCode(0,4),
KB_F3 = mkKeyCode(0,5),
KB_F5 = mkKeyCode(0,6),
KB_CRSR_UD = mkKeyCode(0,7),
KB_3 = mkKeyCode(1,0),
KB_W = mkKeyCode(1,1),
KB_A = mkKeyCode(1,2),
KB_4 = mkKeyCode(1,3),
KB_Z = mkKeyCode(1,4),
KB_S = mkKeyCode(1,5),
KB_E = mkKeyCode(1,6),
KB_5 = mkKeyCode(2,0),
KB_R = mkKeyCode(2,1),
KB_D = mkKeyCode(2,2),
KB_6 = mkKeyCode(2,3),
KB_C = mkKeyCode(2,4),
KB_F = mkKeyCode(2,5),
KB_T = mkKeyCode(2,6),
KB_X = mkKeyCode(2,7),
KB_7 = mkKeyCode(3,0),
KB_Y = mkKeyCode(3,1),
KB_G = mkKeyCode(3,2),
KB_8 = mkKeyCode(3,3),
KB_B = mkKeyCode(3,4),
KB_H = mkKeyCode(3,5),
KB_U = mkKeyCode(3,6),
KB_V = mkKeyCode(3,7),
KB_9 = mkKeyCode(4,0),
KB_I = mkKeyCode(4,1),
KB_J = mkKeyCode(4,2),
KB_0 = mkKeyCode(4,3),
KB_M = mkKeyCode(4,4),
KB_K = mkKeyCode(4,5),
KB_O = mkKeyCode(4,6),
KB_N = mkKeyCode(4,7),
KB_PLUS = mkKeyCode(5,0),
KB_P = mkKeyCode(5,1),
KB_L = mkKeyCode(5,2),
KB_MINUS = mkKeyCode(5,3),
KB_AT_SIGN = mkKeyCode(5,6),
KB_POUND = mkKeyCode(6,0),
KB_ASTERISK = mkKeyCode(6,1),
KB_CLR_HOME = mkKeyCode(6,3),
KB_EQUALS = mkKeyCode(6,5),
KB_UP_ARROW = mkKeyCode(6,6),
KB_1 = mkKeyCode(7,0),
KB_LEFT_ARROW = mkKeyCode(7,1),
KB_2 = mkKeyCode(7,3),
KB_SPACE = mkKeyCode(7,4),
KB_Q = mkKeyCode(7,6),
KB_RUN_STOP = mkKeyCode(7,7),
KB_COLON = mkKeyCode(5,5),
KB_SEMICOLON = mkKeyCode(6,2),
KB_PERIOD = mkKeyCode(5,4),
KB_COMMA = mkKeyCode(5,7),
KB_SLASH = mkKeyCode(6,7),
KB_CTRL = mkKeyCode(7,2),
KB_COMMODORE = mkKeyCode(7,5),
KB_LEFT_SHIFT = mkKeyCode(1,7),
KB_RIGHT_SHIFT = mkKeyCode(6,4),
KB_RESTORE = 0xFF,

// shifted key codes
KBS_F2 = KB_F1 | SHIFT_BIT,
KBS_F4 = KB_F3 | SHIFT_BIT,
KBS_F6 = KB_F5 | SHIFT_BIT,
KBS_F8 = KB_F7 | SHIFT_BIT,

// special function codes
KBEX_SWAP_JS_PORTS = 0xFFFC,
KBEX_CTRL_LOCK = 0xFFFD,
KBEX_SHIFT_LOCK = 0xFFFE,
KBEX_TOGGLE_VKEYBOARD = 0xFFFF
;

static const SysVController::KbMap kbToEventMap
{
	KB_Q, KB_W, KB_E, KB_R, KB_T, KB_Y, KB_U, KB_I, KB_O, KB_P,
	KB_A, KB_S, KB_D, KB_F, KB_G, KB_H, KB_J, KB_K, KB_L, KB_NONE,
	KBEX_SHIFT_LOCK, KB_Z, KB_X, KB_C, KB_V, KB_B, KB_N, KB_M, KB_INST_DEL, KB_NONE,
	KB_NONE, KB_NONE, KB_NONE, KB_SPACE, KB_SPACE, KB_SPACE, KB_SPACE, KB_RUN_STOP, KB_RUN_STOP, KB_RETURN
};

static const SysVController::KbMap kbToEventMapShifted
{
	KB_Q | SHIFT_BIT, KB_W | SHIFT_BIT, KB_E | SHIFT_BIT, KB_R | SHIFT_BIT, KB_T | SHIFT_BIT, KB_Y | SHIFT_BIT, KB_U | SHIFT_BIT, KB_I | SHIFT_BIT, KB_O | SHIFT_BIT, KB_P | SHIFT_BIT,
	KB_A | SHIFT_BIT, KB_S | SHIFT_BIT, KB_D | SHIFT_BIT, KB_F | SHIFT_BIT, KB_G | SHIFT_BIT, KB_H | SHIFT_BIT, KB_J | SHIFT_BIT, KB_K | SHIFT_BIT, KB_L | SHIFT_BIT, KB_NONE,
	KBEX_SHIFT_LOCK, KB_Z | SHIFT_BIT, KB_X | SHIFT_BIT, KB_C | SHIFT_BIT, KB_V | SHIFT_BIT, KB_B | SHIFT_BIT, KB_N | SHIFT_BIT, KB_M | SHIFT_BIT, KB_INST_DEL | SHIFT_BIT, KB_NONE,
	KB_NONE, KB_NONE, KB_NONE, KB_SPACE, KB_SPACE, KB_SPACE, KB_SPACE, KB_RUN_STOP | SHIFT_BIT, KB_RUN_STOP | SHIFT_BIT, KB_RETURN
};

static const SysVController::KbMap kbToEventMap2
{
	KB_F1, KB_F3, KB_F5, KB_F7, KB_AT_SIGN, KB_COMMODORE, KB_CRSR_UD, KB_CRSR_LR, KB_PLUS, KB_MINUS,
	KB_1, KB_2, KB_3, KB_4, KB_5, KB_6, KB_7, KB_8, KB_9, KB_0,
	KB_RESTORE, KB_COLON, KB_SEMICOLON, KB_EQUALS, KB_COMMA, KB_PERIOD, KB_SLASH, KB_ASTERISK, KB_CLR_HOME, KB_NONE,
	KB_NONE, KB_NONE, KB_NONE, KB_SPACE, KB_SPACE, KB_SPACE, KB_SPACE, KB_PERIOD, KBEX_CTRL_LOCK, KB_RETURN
};

static const SysVController::KbMap kbToEventMap2Shifted
{
	KBS_F2, KBS_F4, KBS_F6, KBS_F8, KB_AT_SIGN, KB_COMMODORE, KB_CRSR_UD | SHIFT_BIT, KB_CRSR_LR | SHIFT_BIT, KB_PLUS | SHIFT_BIT, KB_MINUS | SHIFT_BIT,
	KB_1 | SHIFT_BIT, KB_2 | SHIFT_BIT, KB_3 | SHIFT_BIT, KB_4 | SHIFT_BIT, KB_5 | SHIFT_BIT, KB_6 | SHIFT_BIT, KB_7 | SHIFT_BIT, KB_8 | SHIFT_BIT, KB_9 | SHIFT_BIT, KB_0 | SHIFT_BIT,
	KB_RESTORE, KB_COLON | SHIFT_BIT, KB_SEMICOLON | SHIFT_BIT, KB_EQUALS | SHIFT_BIT, KB_COMMA | SHIFT_BIT, KB_PERIOD | SHIFT_BIT, KB_SLASH | SHIFT_BIT, KB_ASTERISK | SHIFT_BIT, KB_CLR_HOME | SHIFT_BIT, KB_NONE,
	KB_NONE, KB_NONE, KB_NONE, KB_SPACE, KB_SPACE, KB_SPACE, KB_SPACE, KB_PERIOD | SHIFT_BIT, KBEX_CTRL_LOCK, KB_RETURN
};

SysVController::KbMap updateVControllerKeyboardMapping(uint mode)
{
	return mode ? (shiftLock ? kbToEventMap2Shifted : kbToEventMap2) : (shiftLock ? kbToEventMapShifted : kbToEventMap);
}

void updateVControllerMapping(uint player, SysVController::Map &map)
{
	const uint p2Bit = player ? IG::bit(5) : 0;
	map[SysVController::F_ELEM] = (JOYPAD_FIRE | p2Bit) << JS_SHIFT;
	map[SysVController::F_ELEM+1] = ((JOYPAD_FIRE | p2Bit) << JS_SHIFT) | SysVController::TURBO_BIT;

	map[SysVController::C_ELEM] = KB_F1;
	map[SysVController::C_ELEM+1] = KBEX_TOGGLE_VKEYBOARD;

	map[SysVController::D_ELEM] = (JOYPAD_NW | p2Bit) << JS_SHIFT;
	map[SysVController::D_ELEM+1] = (JOYPAD_N | p2Bit) << JS_SHIFT;
	map[SysVController::D_ELEM+2] = (JOYPAD_NE | p2Bit) << JS_SHIFT;
	map[SysVController::D_ELEM+3] = (JOYPAD_W | p2Bit) << JS_SHIFT;
	map[SysVController::D_ELEM+5] = (JOYPAD_E | p2Bit) << JS_SHIFT;
	map[SysVController::D_ELEM+6] = (JOYPAD_SW | p2Bit) << JS_SHIFT;
	map[SysVController::D_ELEM+7] = (JOYPAD_S | p2Bit) << JS_SHIFT;
	map[SysVController::D_ELEM+8] = (JOYPAD_SE | p2Bit) << JS_SHIFT;
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	const uint p2Bit = IG::bit(5);
	const uint shiftBit = shiftLock ? SHIFT_BIT : 0;
	switch(input)
	{
		case c64KeyIdxUp: return JOYPAD_N << JS_SHIFT;
		case c64KeyIdxRight: return JOYPAD_E << JS_SHIFT;
		case c64KeyIdxDown: return JOYPAD_S << JS_SHIFT;
		case c64KeyIdxLeft: return JOYPAD_W << JS_SHIFT;
		case c64KeyIdxLeftUp: return JOYPAD_NW << JS_SHIFT;
		case c64KeyIdxRightUp: return JOYPAD_NE << JS_SHIFT;
		case c64KeyIdxRightDown: return JOYPAD_SE << JS_SHIFT;
		case c64KeyIdxLeftDown: return JOYPAD_SW << JS_SHIFT;
		case c64KeyIdxBtn: return JOYPAD_FIRE << JS_SHIFT;
		case c64KeyIdxBtnTurbo: turbo = 1; return JOYPAD_FIRE << JS_SHIFT;
		case c64KeyIdxSwapPorts: return KBEX_SWAP_JS_PORTS;

		case c64KeyIdxUp2: return (JOYPAD_N | p2Bit) << JS_SHIFT;
		case c64KeyIdxRight2: return (JOYPAD_E | p2Bit) << JS_SHIFT;
		case c64KeyIdxDown2: return (JOYPAD_S | p2Bit) << JS_SHIFT;
		case c64KeyIdxLeft2: return (JOYPAD_W | p2Bit) << JS_SHIFT;
		case c64KeyIdxLeftUp2: return (JOYPAD_NW | p2Bit) << JS_SHIFT;
		case c64KeyIdxRightUp2: return (JOYPAD_NE | p2Bit) << JS_SHIFT;
		case c64KeyIdxRightDown2: return (JOYPAD_SE | p2Bit) << JS_SHIFT;
		case c64KeyIdxLeftDown2: return (JOYPAD_SW | p2Bit) << JS_SHIFT;
		case c64KeyIdxBtn2: return (JOYPAD_FIRE | p2Bit) << JS_SHIFT;
		case c64KeyIdxBtnTurbo2: turbo = 1; return (JOYPAD_FIRE | p2Bit) << JS_SHIFT;
		case c64KeyIdxSwapPorts2: return KBEX_SWAP_JS_PORTS;

		case c64KeyToggleKB : return KBEX_TOGGLE_VKEYBOARD;

		case c64KeyF1 : return KB_F1;
		case c64KeyF2 : return KBS_F2;
		case c64KeyF3 : return KB_F3;
		case c64KeyF4 : return KBS_F4;
		case c64KeyF5 : return KB_F5;
		case c64KeyF6 : return KBS_F6;
		case c64KeyF7 : return KB_F7;
		case c64KeyF8 : return KBS_F8;

		case c64KeyLeftArrow : return KB_LEFT_ARROW;
		case c64Key1 : return KB_1;
		case c64Key2 : return KB_2;
		case c64Key3 : return KB_3;
		case c64Key4 : return KB_4;
		case c64Key5 : return KB_5;
		case c64Key6 : return KB_6;
		case c64Key7 : return KB_7;
		case c64Key8 : return KB_8;
		case c64Key9 : return KB_9;
		case c64Key0 : return KB_0;
		case c64KeyPlus : return KB_PLUS;
		case c64KeyMinus : return KB_MINUS;
		case c64KeyPound : return KB_POUND;
		case c64KeyClrHome : return KB_CLR_HOME;
		case c64KeyInstDel : return KB_INST_DEL;

		case c64KeyCtrl : return KB_CTRL;
		case c64KeyQ : return KB_Q | shiftBit;
		case c64KeyW : return KB_W | shiftBit;
		case c64KeyE : return KB_E | shiftBit;
		case c64KeyR : return KB_R | shiftBit;
		case c64KeyT : return KB_T | shiftBit;
		case c64KeyY : return KB_Y | shiftBit;
		case c64KeyU : return KB_U | shiftBit;
		case c64KeyI : return KB_I | shiftBit;
		case c64KeyO : return KB_O | shiftBit;
		case c64KeyP : return KB_P | shiftBit;
		case c64KeyAt : return KB_AT_SIGN;
		case c64KeyAsterisk : return KB_ASTERISK;
		case c64KeyUpArrow : return KB_UP_ARROW;
		case c64KeyRestore : return KB_RESTORE;

		case c64KeyRunStop : return KB_RUN_STOP;
		case c64KeyShiftLock : return KBEX_SHIFT_LOCK;
		case c64KeyA : return KB_A | shiftBit;
		case c64KeyS : return KB_S | shiftBit;
		case c64KeyD : return KB_D | shiftBit;
		case c64KeyF : return KB_F | shiftBit;
		case c64KeyG : return KB_G | shiftBit;
		case c64KeyH : return KB_H | shiftBit;
		case c64KeyJ : return KB_J | shiftBit;
		case c64KeyK : return KB_K | shiftBit;
		case c64KeyL : return KB_L | shiftBit;
		case c64KeyColon : return KB_COLON;
		case c64KeySemiColon : return KB_SEMICOLON;
		case c64KeyEquals : return KB_EQUALS;
		case c64KeyReturn : return KB_RETURN;

		case c64KeyCommodore : return KB_COMMODORE;
		case c64KeyLeftShift : return KB_LEFT_SHIFT;
		case c64KeyZ : return KB_Z | shiftBit;
		case c64KeyX : return KB_X | shiftBit;
		case c64KeyC : return KB_C | shiftBit;
		case c64KeyV : return KB_V | shiftBit;
		case c64KeyB : return KB_B | shiftBit;
		case c64KeyN : return KB_N | shiftBit;
		case c64KeyM : return KB_M | shiftBit;
		case c64KeyComma : return KB_COMMA;
		case c64KeyPeriod : return KB_PERIOD;
		case c64KeySlash : return KB_SLASH;
		case c64KeyRightShift : return KB_RIGHT_SHIFT;
		case c64KeyKbUp : return KB_CRSR_UD | SHIFT_BIT;
		case c64KeyKbRight : return KB_CRSR_LR;
		case c64KeyKbDown : return KB_CRSR_UD;
		case c64KeyKbLeft : return KB_CRSR_LR | SHIFT_BIT;

		case c64KeySpace : return KB_SPACE;
		case c64KeyCtrlLock : return KBEX_CTRL_LOCK;
		default: bug_unreachable("input == %d", input);
	}
	return 0;
}

static void setC64KBKey(int key, bool pushed)
{
	int row = (key >> 4) & 0xF;
	int col = key  & 0xF;
	int shift = (key >> 8) & 0xF;
	auto &keyarr = *plugin.keyarr;
	auto &rev_keyarr = *plugin.rev_keyarr;
	if(pushed)
	{
		keyarr[row] |= 1 << col;
		rev_keyarr[col] |= 1 << row;
		if(shift)
		{
			keyarr[1] |= 1 << 7;
			rev_keyarr[7] |= 1 << 1;
		}
	}
	else
	{
		keyarr[row] &= ~(1 << col);
		rev_keyarr[col] &= ~(1 << row);
		if(shift)
		{
			keyarr[1] &= ~(1 << 7);
			rev_keyarr[7] &= ~(1 << 1);
		}
	}
}

void EmuSystem::handleInputAction(uint state, uint emuKey)
{
	auto &joystick_value = *plugin.joystick_value;
	if(emuKey & 0xFF0000) // Joystick
	{

		uint key = emuKey >> 16;
		uint player = (key & IG::bit(5)) ? 2 : 1;
		if(optionSwapJoystickPorts)
		{
			player = (player == 1) ? 2 : 1;
		}
		//logMsg("js %X p %d", key & 0x1F, player);
		joystick_value[player] = IG::setOrClearBits(joystick_value[player], (BYTE)(key & 0x1F), state == Input::PUSHED);
	}
	else // Keyboard
	{
		// Special Keys
		switch(emuKey)
		{
			case KBEX_SWAP_JS_PORTS:
			{
				if(state == Input::PUSHED)
				{
					EmuSystem::sessionOptionSet();
					if(optionSwapJoystickPorts)
						optionSwapJoystickPorts = 0;
					else
						optionSwapJoystickPorts = 1;
					IG::fillData(*plugin.joystick_value);
					EmuApp::postMessage(1, false, "Swapped Joystick Ports");
				}
				return;
			}
			case KBEX_TOGGLE_VKEYBOARD:
			{
				if(state == Input::PUSHED)
					EmuControls::toggleKeyboard();
				return;
			}
			case KBEX_SHIFT_LOCK:
			{
				if(state == Input::PUSHED)
				{
					shiftLock ^= true;
					EmuControls::updateKeyboardMapping();
				}
				return;
			}
			case KBEX_CTRL_LOCK:
			{
				if(state == Input::PUSHED)
				{
					ctrlLock ^= true;
					setC64KBKey(KB_CTRL, ctrlLock);
				}
				return;
			}
		}
		if(unlikely((emuKey & 0xFF) == KB_NONE))
		{
			return;
		}
		if(unlikely((emuKey & 0xFF) == 0xFF))
		{
			logMsg("pushed restore key");
			EmuApp::syncEmulationThread();
			plugin.machine_set_restore_key(state == Input::PUSHED);
			return;
		}

		setC64KBKey(emuKey, state == Input::PUSHED);
	}
}

void EmuSystem::clearInputBuffers(EmuInputView &)
{
	auto &keyarr = *plugin.keyarr;
	auto &rev_keyarr = *plugin.rev_keyarr;
	auto &joystick_value = *plugin.joystick_value;
	shiftLock = 0;
	ctrlLock = 0;
	IG::fillData(keyarr);
	IG::fillData(rev_keyarr);
	IG::fillData(joystick_value);
}
