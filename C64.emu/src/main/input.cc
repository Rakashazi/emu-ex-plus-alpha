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

extern "C"
{
	#include "kbd.h"
}

enum shift_type {
    NO_SHIFT = 0,             /* Key is not shifted. Keys will be deshifted,
                                 no other flags will be checked */

    VIRTUAL_SHIFT     = (1 << 0), /* The key needs a shift on the real machine. */
    LEFT_SHIFT        = (1 << 1), /* Key is left shift. */
    RIGHT_SHIFT       = (1 << 2), /* Key is right shift. */
    ALLOW_SHIFT       = (1 << 3), /* Allow key to be shifted. */
    DESHIFT_SHIFT     = (1 << 4), /* Although SHIFT might be pressed, do not
                                 press shift on the real machine. */
    ALLOW_OTHER       = (1 << 5), /* Allow another key code to be assigned if
                                 SHIFT is pressed. */
    SHIFT_LOCK        = (1 << 6), /* Key is shift lock on the real machine */
    MAP_MOD_SHIFT     = (1 << 7), /* Key requires SHIFT to be pressed on host */

    ALT_MAP           = (1 << 8), /* Key is used for an alternative keyboard mapping (x128) */

    MAP_MOD_RIGHT_ALT = (1 << 9), /* Key requires right ALT (Alt-gr) to be pressed on host */
    MAP_MOD_CTRL     = (1 << 10), /* Key requires control to be pressed on host */

    VIRTUAL_CBM      = (1 << 11), /* The key is combined with CBM on the emulated machine */
    VIRTUAL_CTRL     = (1 << 12), /* The key is combined with CTRL on the emulated machine */

    LEFT_CBM         = (1 << 13), /* Key is CBM on the real machine */
    LEFT_CTRL        = (1 << 14)  /* Key is CTRL on the real machine */
};

struct keyboard_conv_t {
    signed long sym;
    int row;
    int column;
    enum shift_type shift;
    char *comment;
};

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

	c64KeyFirstKeyboardKey,
	c64KeyF1 = c64KeyFirstKeyboardKey,
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
	c64KeyLastKeyboardKey = c64KeySpace,
	c64KeyCtrlLock,
};

const char *EmuSystem::inputFaceBtnName = "JS Buttons";
const char *EmuSystem::inputCenterBtnName = "F1/KB";
const unsigned EmuSystem::inputFaceBtns = 2;
const unsigned EmuSystem::inputCenterBtns = 2;
const bool EmuSystem::inputHasTriggerBtns = false;
const bool EmuSystem::inputHasRevBtnLayout = false;
bool EmuSystem::inputHasKeyboard = true;
const unsigned EmuSystem::maxPlayers = 2;

static bool shiftLock = false, ctrlLock = false;

static constexpr uint8_t KEY_MODE_SHIFT = 28;
static constexpr uint8_t KB_MODE = 0;
static constexpr uint8_t JS_MODE = 1;
static constexpr uint8_t EX_MODE = 2;
static constexpr uint32_t CODE_MASK = 0xFFFFFF;
static constexpr uint32_t KB_SHIFT_BIT = IG::bit(8);
static constexpr uint32_t JS_P2_BIT = IG::bit(5);
static constexpr uint8_t KB_KEYS = (c64KeyLastKeyboardKey - c64KeyFirstKeyboardKey) + 1;

static constexpr uint32_t mkKBKeyCode(uint32_t row, uint32_t col, bool shift = false)
{
	return (shift << 8) | (row << 4) | col;
}

static constexpr uint32_t mkJSKeyCode(uint32_t jsBits)
{
	return (JS_MODE << KEY_MODE_SHIFT) | jsBits;
}

static constexpr uint32_t mkEXKeyCode(uint32_t code)
{
	return (EX_MODE << KEY_MODE_SHIFT) | code;
}

// joystick codes
static constexpr uint32_t JS_FIRE = mkJSKeyCode(0x10),
	JS_E = mkJSKeyCode(0x08),
	JS_W = mkJSKeyCode(0x04),
	JS_S = mkJSKeyCode(0x02),
	JS_N = mkJSKeyCode(0x01),
	JS_SW = mkJSKeyCode(JS_S | JS_W),
	JS_SE = mkJSKeyCode(JS_S | JS_E),
	JS_NW = mkJSKeyCode(JS_N | JS_W),
	JS_NE = mkJSKeyCode(JS_N | JS_E);

// special function codes
static constexpr uint32_t
	KBEX_NONE = mkEXKeyCode(0),
	KBEX_RESTORE = mkEXKeyCode(1),
	KBEX_SWAP_JS_PORTS = mkEXKeyCode(2),
	KBEX_CTRL_LOCK = mkEXKeyCode(3),
	KBEX_SHIFT_LOCK = mkEXKeyCode(4),
	KBEX_TOGGLE_VKEYBOARD = mkEXKeyCode(5);

static std::array<uint32_t, KB_KEYS> c64KeyMap{};
static SysVController::KbMap kbToEventMap{};
static SysVController::KbMap kbToEventMapShifted{};
static SysVController::KbMap kbToEventMap2{};
static SysVController::KbMap kbToEventMap2Shifted{};

static bool isEmuKeyInKeyboardRange(uint32_t emuKey)
{
	return emuKey >= c64KeyFirstKeyboardKey && emuKey <= c64KeyLastKeyboardKey;
}

static uint32_t keyboardCodeFromEmuKey(uint32_t emuKey)
{
	assumeExpr(isEmuKeyInKeyboardRange(emuKey));
	return c64KeyMap[emuKey - c64KeyFirstKeyboardKey];
}

SysVController::KbMap updateVControllerKeyboardMapping(unsigned mode)
{
	return mode ? (shiftLock ? kbToEventMap2Shifted : kbToEventMap2) : (shiftLock ? kbToEventMapShifted : kbToEventMap);
}

void updateVControllerMapping(unsigned player, SysVController::Map &map)
{
	const unsigned p2Bit = player ? JS_P2_BIT : 0;
	map[SysVController::F_ELEM] = JS_FIRE | p2Bit;
	map[SysVController::F_ELEM+1] = JS_FIRE | p2Bit | SysVController::TURBO_BIT;

	map[SysVController::C_ELEM] = keyboardCodeFromEmuKey(c64KeyF1);
	map[SysVController::C_ELEM+1] = KBEX_TOGGLE_VKEYBOARD;

	map[SysVController::D_ELEM] = JS_NW | p2Bit;
	map[SysVController::D_ELEM+1] = JS_N | p2Bit;
	map[SysVController::D_ELEM+2] = JS_NE | p2Bit;
	map[SysVController::D_ELEM+3] = JS_W | p2Bit;
	map[SysVController::D_ELEM+5] = JS_E | p2Bit;
	map[SysVController::D_ELEM+6] = JS_SW | p2Bit;
	map[SysVController::D_ELEM+7] = JS_S | p2Bit;
	map[SysVController::D_ELEM+8] = JS_SE | p2Bit;
}

unsigned EmuSystem::translateInputAction(unsigned input, bool &turbo)
{
	turbo = 0;
	switch(input)
	{
		case c64KeyIdxUp: return JS_N;
		case c64KeyIdxRight: return JS_E;
		case c64KeyIdxDown: return JS_S;
		case c64KeyIdxLeft: return JS_W;
		case c64KeyIdxLeftUp: return JS_NW;
		case c64KeyIdxRightUp: return JS_NE;
		case c64KeyIdxRightDown: return JS_SE;
		case c64KeyIdxLeftDown: return JS_SW;
		case c64KeyIdxBtn: return JS_FIRE;
		case c64KeyIdxBtnTurbo: turbo = 1; return JS_FIRE;
		case c64KeyIdxSwapPorts: return KBEX_SWAP_JS_PORTS;

		case c64KeyIdxUp2: return JS_N | JS_P2_BIT;
		case c64KeyIdxRight2: return JS_E | JS_P2_BIT;
		case c64KeyIdxDown2: return JS_S | JS_P2_BIT;
		case c64KeyIdxLeft2: return JS_W | JS_P2_BIT;
		case c64KeyIdxLeftUp2: return JS_NW | JS_P2_BIT;
		case c64KeyIdxRightUp2: return JS_NE | JS_P2_BIT;
		case c64KeyIdxRightDown2: return JS_SE | JS_P2_BIT;
		case c64KeyIdxLeftDown2: return JS_SW | JS_P2_BIT;
		case c64KeyIdxBtn2: return JS_FIRE | JS_P2_BIT;
		case c64KeyIdxBtnTurbo2: turbo = 1; return JS_FIRE | JS_P2_BIT;
		case c64KeyIdxSwapPorts2: return KBEX_SWAP_JS_PORTS;

		case c64KeyToggleKB : return KBEX_TOGGLE_VKEYBOARD;
		case c64KeyRestore : return KBEX_RESTORE;
		case c64KeyShiftLock : return KBEX_SHIFT_LOCK;
		case c64KeyCtrlLock : return KBEX_CTRL_LOCK;
		default:
		{
			if(!isEmuKeyInKeyboardRange(input))
				return KBEX_NONE;
			const uint32_t shift = shiftLock ? KB_SHIFT_BIT : 0;
			return keyboardCodeFromEmuKey(input) | shift;
		}
	}
	return 0;
}

static void setC64KBKey(uint32_t key, bool pushed)
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

void EmuSystem::handleInputAction(EmuApp *app, Input::Action action, unsigned emuKey)
{
	switch(emuKey >> KEY_MODE_SHIFT)
	{
		bcase JS_MODE:
		{
			auto &joystick_value = *plugin.joystick_value;
			auto key = emuKey & 0x1F;
			auto player = (emuKey & IG::bit(5)) ? 2 : 1;
			if(optionSwapJoystickPorts)
			{
				player = (player == 1) ? 2 : 1;
			}
			//logMsg("js %X p %d", key, player);
			joystick_value[player] = IG::setOrClearBits(joystick_value[player], (uint8_t)key, action == Input::Action::PUSHED);
		}
		bcase KB_MODE:
		{
			auto key = emuKey & CODE_MASK;
			setC64KBKey(key, action == Input::Action::PUSHED);
		}
		bcase EX_MODE:
		{
			switch(emuKey)
			{
				bcase KBEX_SWAP_JS_PORTS:
				{
					if(action == Input::Action::PUSHED)
					{
						EmuSystem::sessionOptionSet();
						if(optionSwapJoystickPorts)
							optionSwapJoystickPorts = 0;
						else
							optionSwapJoystickPorts = 1;
						IG::fill(*plugin.joystick_value);
						if(app)
							app->postMessage(1, false, "Swapped Joystick Ports");
					}
				}
				bcase KBEX_TOGGLE_VKEYBOARD:
				{
					if(app && action == Input::Action::PUSHED)
						app->toggleKeyboard();
				}
				bcase KBEX_SHIFT_LOCK:
				{
					if(app && action == Input::Action::PUSHED)
					{
						shiftLock ^= true;
						setC64KBKey(keyboardCodeFromEmuKey(c64KeyLeftShift), shiftLock);
						app->updateKeyboardMapping();
					}
				}
				bcase KBEX_CTRL_LOCK:
				{
					if(action == Input::Action::PUSHED)
					{
						ctrlLock ^= true;
						setC64KBKey(keyboardCodeFromEmuKey(c64KeyCtrl), ctrlLock);
					}
				}
				bcase KBEX_RESTORE:
				{
					if(app)
					{
						logMsg("pushed restore key");
						app->syncEmulationThread();
						plugin.machine_set_restore_key(action == Input::Action::PUSHED);
					}
				}
			}
		}
	}
}

void EmuSystem::clearInputBuffers(EmuInputView &)
{
	auto &keyarr = *plugin.keyarr;
	auto &rev_keyarr = *plugin.rev_keyarr;
	auto &joystick_value = *plugin.joystick_value;
	shiftLock = false;
	ctrlLock = false;
	IG::fill(keyarr);
	IG::fill(rev_keyarr);
	IG::fill(joystick_value);
}

void updateKeyMappingArray(EmuApp &app)
{
	auto keyconvmap = *plugin.keyconvmap;
	iterateTimes(150, i)
	{
		auto e = keyconvmap[i];
		if(!isEmuKeyInKeyboardRange(e.sym))
			continue;
		uint32_t key = mkKBKeyCode(e.row, e.column, e.shift & VIRTUAL_SHIFT);
		c64KeyMap[e.sym - c64KeyFirstKeyboardKey] = key;
		//logMsg("mapped input code 0x%X -> 0x%X", (unsigned)e.sym, key);
	}

	app.updateVControllerMapping();

	const uint32_t KB_Q = keyboardCodeFromEmuKey(c64KeyQ),
		KB_W = keyboardCodeFromEmuKey(c64KeyW),
		KB_E = keyboardCodeFromEmuKey(c64KeyE),
		KB_R = keyboardCodeFromEmuKey(c64KeyR),
		KB_T = keyboardCodeFromEmuKey(c64KeyT),
		KB_Y = keyboardCodeFromEmuKey(c64KeyY),
		KB_U = keyboardCodeFromEmuKey(c64KeyU),
		KB_I = keyboardCodeFromEmuKey(c64KeyI),
		KB_O = keyboardCodeFromEmuKey(c64KeyO),
		KB_P = keyboardCodeFromEmuKey(c64KeyP),
		KB_A = keyboardCodeFromEmuKey(c64KeyA),
		KB_S = keyboardCodeFromEmuKey(c64KeyS),
		KB_D = keyboardCodeFromEmuKey(c64KeyD),
		KB_F = keyboardCodeFromEmuKey(c64KeyF),
		KB_G = keyboardCodeFromEmuKey(c64KeyG),
		KB_H = keyboardCodeFromEmuKey(c64KeyH),
		KB_J = keyboardCodeFromEmuKey(c64KeyJ),
		KB_K = keyboardCodeFromEmuKey(c64KeyK),
		KB_L = keyboardCodeFromEmuKey(c64KeyL),
		KB_Z = keyboardCodeFromEmuKey(c64KeyZ),
		KB_X = keyboardCodeFromEmuKey(c64KeyX),
		KB_C = keyboardCodeFromEmuKey(c64KeyC),
		KB_V = keyboardCodeFromEmuKey(c64KeyV),
		KB_B = keyboardCodeFromEmuKey(c64KeyB),
		KB_N = keyboardCodeFromEmuKey(c64KeyN),
		KB_M = keyboardCodeFromEmuKey(c64KeyM),
		KB_INST_DEL = keyboardCodeFromEmuKey(c64KeyInstDel),
		KB_SPACE = keyboardCodeFromEmuKey(c64KeySpace),
		KB_RUN_STOP = keyboardCodeFromEmuKey(c64KeyRunStop),
		KB_RETURN = keyboardCodeFromEmuKey(c64KeyReturn),
		KB_F1 = keyboardCodeFromEmuKey(c64KeyF1),
		KB_F2 = keyboardCodeFromEmuKey(c64KeyF2),
		KB_F3 = keyboardCodeFromEmuKey(c64KeyF3),
		KB_F4 = keyboardCodeFromEmuKey(c64KeyF4),
		KB_F5 = keyboardCodeFromEmuKey(c64KeyF5),
		KB_F6 = keyboardCodeFromEmuKey(c64KeyF6),
		KB_F7 = keyboardCodeFromEmuKey(c64KeyF7),
		KB_F8 = keyboardCodeFromEmuKey(c64KeyF8),
		KB_AT_SIGN = keyboardCodeFromEmuKey(c64KeyAt),
		KB_COMMODORE = keyboardCodeFromEmuKey(c64KeyCommodore),
		KB_CRSR_UD = keyboardCodeFromEmuKey(c64KeyUpArrow),
		KB_CRSR_LR = keyboardCodeFromEmuKey(c64KeyLeftArrow),
		KB_PLUS = keyboardCodeFromEmuKey(c64KeyPlus),
		KB_MINUS = keyboardCodeFromEmuKey(c64KeyMinus),
		KB_1 = keyboardCodeFromEmuKey(c64Key1),
		KB_2 = keyboardCodeFromEmuKey(c64Key2),
		KB_3 = keyboardCodeFromEmuKey(c64Key3),
		KB_4 = keyboardCodeFromEmuKey(c64Key4),
		KB_5 = keyboardCodeFromEmuKey(c64Key5),
		KB_6 = keyboardCodeFromEmuKey(c64Key6),
		KB_7 = keyboardCodeFromEmuKey(c64Key7),
		KB_8 = keyboardCodeFromEmuKey(c64Key8),
		KB_9 = keyboardCodeFromEmuKey(c64Key9),
		KB_0 = keyboardCodeFromEmuKey(c64Key0),
		KB_COLON = keyboardCodeFromEmuKey(c64KeyColon),
		KB_SEMICOLON = keyboardCodeFromEmuKey(c64KeySemiColon),
		KB_EQUALS = keyboardCodeFromEmuKey(c64KeyEquals),
		KB_COMMA = keyboardCodeFromEmuKey(c64KeyComma),
		KB_PERIOD = keyboardCodeFromEmuKey(c64KeyPeriod),
		KB_SLASH = keyboardCodeFromEmuKey(c64KeySlash),
		KB_ASTERISK = keyboardCodeFromEmuKey(c64KeyAsterisk),
		KB_CLR_HOME = keyboardCodeFromEmuKey(c64KeyClrHome);

	kbToEventMap =
	{
		KB_Q, KB_W, KB_E, KB_R, KB_T, KB_Y, KB_U, KB_I, KB_O, KB_P,
		KB_A, KB_S, KB_D, KB_F, KB_G, KB_H, KB_J, KB_K, KB_L, KBEX_NONE,
		KBEX_SHIFT_LOCK, KB_Z, KB_X, KB_C, KB_V, KB_B, KB_N, KB_M, KB_INST_DEL, KBEX_NONE,
		KBEX_NONE, KBEX_NONE, KBEX_NONE, KB_SPACE, KB_SPACE, KB_SPACE, KB_SPACE, KB_RUN_STOP, KB_RUN_STOP, KB_RETURN
	};

	kbToEventMapShifted =
	{
		KB_Q | KB_SHIFT_BIT, KB_W | KB_SHIFT_BIT, KB_E | KB_SHIFT_BIT, KB_R | KB_SHIFT_BIT, KB_T | KB_SHIFT_BIT, KB_Y | KB_SHIFT_BIT, KB_U | KB_SHIFT_BIT, KB_I | KB_SHIFT_BIT, KB_O | KB_SHIFT_BIT, KB_P | KB_SHIFT_BIT,
		KB_A | KB_SHIFT_BIT, KB_S | KB_SHIFT_BIT, KB_D | KB_SHIFT_BIT, KB_F | KB_SHIFT_BIT, KB_G | KB_SHIFT_BIT, KB_H | KB_SHIFT_BIT, KB_J | KB_SHIFT_BIT, KB_K | KB_SHIFT_BIT, KB_L | KB_SHIFT_BIT, KBEX_NONE,
		KBEX_SHIFT_LOCK, KB_Z | KB_SHIFT_BIT, KB_X | KB_SHIFT_BIT, KB_C | KB_SHIFT_BIT, KB_V | KB_SHIFT_BIT, KB_B | KB_SHIFT_BIT, KB_N | KB_SHIFT_BIT, KB_M | KB_SHIFT_BIT, KB_INST_DEL | KB_SHIFT_BIT, KBEX_NONE,
		KBEX_NONE, KBEX_NONE, KBEX_NONE, KB_SPACE, KB_SPACE, KB_SPACE, KB_SPACE, KB_RUN_STOP | KB_SHIFT_BIT, KB_RUN_STOP | KB_SHIFT_BIT, KB_RETURN
	};

	kbToEventMap2 =
	{
		KB_F1, KB_F3, KB_F5, KB_F7, KB_AT_SIGN, KB_COMMODORE, KB_CRSR_UD, KB_CRSR_LR, KB_PLUS, KB_MINUS,
		KB_1, KB_2, KB_3, KB_4, KB_5, KB_6, KB_7, KB_8, KB_9, KB_0,
		KBEX_RESTORE, KB_COLON, KB_SEMICOLON, KB_EQUALS, KB_COMMA, KB_PERIOD, KB_SLASH, KB_ASTERISK, KB_CLR_HOME, KBEX_NONE,
		KBEX_NONE, KBEX_NONE, KBEX_NONE, KB_SPACE, KB_SPACE, KB_SPACE, KB_SPACE, KB_PERIOD, KBEX_CTRL_LOCK, KB_RETURN
	};

	kbToEventMap2Shifted =
	{
		KB_F2, KB_F4, KB_F6, KB_F8, KB_AT_SIGN, KB_COMMODORE, KB_CRSR_UD | KB_SHIFT_BIT, KB_CRSR_LR | KB_SHIFT_BIT, KB_PLUS | KB_SHIFT_BIT, KB_MINUS | KB_SHIFT_BIT,
		KB_1 | KB_SHIFT_BIT, KB_2 | KB_SHIFT_BIT, KB_3 | KB_SHIFT_BIT, KB_4 | KB_SHIFT_BIT, KB_5 | KB_SHIFT_BIT, KB_6 | KB_SHIFT_BIT, KB_7 | KB_SHIFT_BIT, KB_8 | KB_SHIFT_BIT, KB_9 | KB_SHIFT_BIT, KB_0 | KB_SHIFT_BIT,
		KBEX_RESTORE, KB_COLON | KB_SHIFT_BIT, KB_SEMICOLON | KB_SHIFT_BIT, KB_EQUALS | KB_SHIFT_BIT, KB_COMMA | KB_SHIFT_BIT, KB_PERIOD | KB_SHIFT_BIT, KB_SLASH | KB_SHIFT_BIT, KB_ASTERISK | KB_SHIFT_BIT, KB_CLR_HOME | KB_SHIFT_BIT, KBEX_NONE,
		KBEX_NONE, KBEX_NONE, KBEX_NONE, KB_SPACE, KB_SPACE, KB_SPACE, KB_SPACE, KB_PERIOD | KB_SHIFT_BIT, KBEX_CTRL_LOCK, KB_RETURN
	};

	app.updateKeyboardMapping();
}

signed long kbd_arch_keyname_to_keynum(char *keyname)
{
	//logMsg("kbd_arch_keyname_to_keynum(%s)", keyname);
	if(string_equal(keyname, "F1")) { return c64KeyF1; }
	else if(string_equal(keyname, "F2")) { return c64KeyF2; }
	else if(string_equal(keyname, "F3")) { return c64KeyF3; }
	else if(string_equal(keyname, "F4")) { return c64KeyF4; }
	else if(string_equal(keyname, "F5")) { return c64KeyF5; }
	else if(string_equal(keyname, "F6")) { return c64KeyF6; }
	else if(string_equal(keyname, "F7")) { return c64KeyF7; }
	else if(string_equal(keyname, "F8")) { return c64KeyF8; }
	else if(string_equal(keyname, "underscore")) { return c64KeyLeftArrow; }
	else if(string_equal(keyname, "1")) { return c64Key1; }
	else if(string_equal(keyname, "2")) { return c64Key2; }
	else if(string_equal(keyname, "3")) { return c64Key3; }
	else if(string_equal(keyname, "4")) { return c64Key4; }
	else if(string_equal(keyname, "5")) { return c64Key5; }
	else if(string_equal(keyname, "6")) { return c64Key6; }
	else if(string_equal(keyname, "7")) { return c64Key7; }
	else if(string_equal(keyname, "8")) { return c64Key8; }
	else if(string_equal(keyname, "9")) { return c64Key9; }
	else if(string_equal(keyname, "0")) { return c64Key0; }
	else if(string_equal(keyname, "plus")) { return c64KeyPlus; }
	else if(string_equal(keyname, "minus")) { return c64KeyMinus; }
	else if(string_equal(keyname, "sterling")) { return c64KeyPound; }
	else if(string_equal(keyname, "Home")) { return c64KeyClrHome; }
	else if(string_equal(keyname, "BackSpace")) { return c64KeyInstDel; }
	else if(string_equal(keyname, "Control_L")) { return c64KeyCtrl; }
	else if(string_equal(keyname, "q")) { return c64KeyQ; }
	else if(string_equal(keyname, "w")) { return c64KeyW; }
	else if(string_equal(keyname, "e")) { return c64KeyE; }
	else if(string_equal(keyname, "r")) { return c64KeyR; }
	else if(string_equal(keyname, "t")) { return c64KeyT; }
	else if(string_equal(keyname, "y")) { return c64KeyY; }
	else if(string_equal(keyname, "u")) { return c64KeyU; }
	else if(string_equal(keyname, "i")) { return c64KeyI; }
	else if(string_equal(keyname, "o")) { return c64KeyO; }
	else if(string_equal(keyname, "p")) { return c64KeyP; }
	else if(string_equal(keyname, "at")) { return c64KeyAt; }
	else if(string_equal(keyname, "asterisk")) { return c64KeyAsterisk; }
	else if(string_equal(keyname, "Page_Down")) { return c64KeyUpArrow; }
	else if(string_equal(keyname, "Escape")) { return c64KeyRunStop; }
	else if(string_equal(keyname, "a")) { return c64KeyA; }
	else if(string_equal(keyname, "s")) { return c64KeyS; }
	else if(string_equal(keyname, "d")) { return c64KeyD; }
	else if(string_equal(keyname, "f")) { return c64KeyF; }
	else if(string_equal(keyname, "g")) { return c64KeyG; }
	else if(string_equal(keyname, "h")) { return c64KeyH; }
	else if(string_equal(keyname, "j")) { return c64KeyJ; }
	else if(string_equal(keyname, "k")) { return c64KeyK; }
	else if(string_equal(keyname, "l")) { return c64KeyL; }
	else if(string_equal(keyname, "colon")) { return c64KeyColon; }
	else if(string_equal(keyname, "semicolon")) { return c64KeySemiColon; }
	else if(string_equal(keyname, "equal")) { return c64KeyEquals; }
	else if(string_equal(keyname, "Return")) { return c64KeyReturn; }
	else if(string_equal(keyname, "Tab")) { return c64KeyCommodore; }
	else if(string_equal(keyname, "Shift_L")) { return c64KeyLeftShift; }
	else if(string_equal(keyname, "z")) { return c64KeyZ; }
	else if(string_equal(keyname, "x")) { return c64KeyX; }
	else if(string_equal(keyname, "c")) { return c64KeyC; }
	else if(string_equal(keyname, "v")) { return c64KeyV; }
	else if(string_equal(keyname, "b")) { return c64KeyB; }
	else if(string_equal(keyname, "n")) { return c64KeyN; }
	else if(string_equal(keyname, "m")) { return c64KeyM; }
	else if(string_equal(keyname, "comma")) { return c64KeyComma; }
	else if(string_equal(keyname, "period")) { return c64KeyPeriod; }
	else if(string_equal(keyname, "slash")) { return c64KeySlash; }
	else if(string_equal(keyname, "Shift_R")) { return c64KeyRightShift; }
	else if(string_equal(keyname, "Up")) { return c64KeyKbUp; }
	else if(string_equal(keyname, "Right")) { return c64KeyKbRight; }
	else if(string_equal(keyname, "Down")) { return c64KeyKbDown; }
	else if(string_equal(keyname, "Left")) { return c64KeyKbLeft; }
	else if(string_equal(keyname, "space")) { return c64KeySpace; }
	return 0;
}
