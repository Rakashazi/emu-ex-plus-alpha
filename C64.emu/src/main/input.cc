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
	#include "joyport.h"
}

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

	// other virtual & symbolic keys
	c64KeyCtrlLock,
	c64KeyExclam,
	c64KeyQuoteDbl,
	c64KeyNumberSign,
	c64KeyDollar,
	c64KeyPercent,
	c64KeyAmpersand,
	c64KeyParenLeft,
	c64KeyParenRight,
	c64KeyBracketLeft,
	c64KeyBracketRight,
	c64KeyLess,
	c64KeyGreater,
	c64KeyQuestion,
	c64KeyApostrophe,

	c64KeyLastKeyboardKey = c64KeyApostrophe,
};

const char *EmuSystem::inputFaceBtnName = "JS Buttons";
const char *EmuSystem::inputCenterBtnName = "F1/KB";
const unsigned EmuSystem::inputFaceBtns = 2;
const unsigned EmuSystem::inputCenterBtns = 2;
bool EmuSystem::inputHasKeyboard = true;
const unsigned EmuSystem::maxPlayers = 2;

static bool ctrlLock = false;

static constexpr uint8_t KEY_MODE_SHIFT = 28;
static constexpr uint8_t KB_MODE = 0;
static constexpr uint8_t JS_MODE = 1;
static constexpr uint8_t EX_MODE = 2;
static constexpr uint32_t CODE_MASK = 0xFFFFFF;
static constexpr uint32_t KB_SHIFT_BIT = IG::bit(8);
static constexpr uint32_t JS_P2_BIT = IG::bit(5);
static constexpr unsigned shiftedPound = c64KeyPound | IG::bit(8);

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
	KBEX_POS_SHIFT_LOCK = mkEXKeyCode(4),
	KBEX_TOGGLE_VKEYBOARD = mkEXKeyCode(5);

static bool isEmuKeyInKeyboardRange(uint32_t emuKey)
{
	return emuKey >= c64KeyFirstKeyboardKey && emuKey <= c64KeyLastKeyboardKey;
}

VController::KbMap updateVControllerKeyboardMapping(unsigned mode)
{
	static constexpr VController::KbMap kbToEventMap =
	{
		c64KeyQ, c64KeyW, c64KeyE, c64KeyR, c64KeyT, c64KeyY, c64KeyU, c64KeyI, c64KeyO, c64KeyP,
		c64KeyA, c64KeyS, c64KeyD, c64KeyF, c64KeyG, c64KeyH, c64KeyJ, c64KeyK, c64KeyL, KBEX_NONE,
		KBEX_POS_SHIFT_LOCK, c64KeyZ, c64KeyX, c64KeyC, c64KeyV, c64KeyB, c64KeyN, c64KeyM, c64KeyInstDel, KBEX_NONE,
		KBEX_NONE, KBEX_NONE, KBEX_NONE, c64KeySpace, c64KeySpace, c64KeySpace, c64KeySpace, c64KeyRunStop, c64KeyRunStop, c64KeyReturn
	};

	static constexpr VController::KbMap kbToEventMap2 =
	{
		c64KeyF1, c64KeyF3, c64KeyF5, c64KeyF7, c64KeyAt, c64KeyCommodore, c64KeyLeftArrow, c64KeyUpArrow, c64KeyPlus, c64KeyMinus,
		c64Key1, c64Key2, c64Key3, c64Key4, c64Key5, c64Key6, c64Key7, c64Key8, c64Key9, c64Key0,
		KBEX_RESTORE, c64KeyColon, c64KeySemiColon, c64KeyEquals, c64KeyComma, c64KeyPeriod, c64KeySlash, c64KeyAsterisk, c64KeyClrHome, KBEX_NONE,
		KBEX_NONE, KBEX_NONE, KBEX_NONE, c64KeySpace, c64KeySpace, c64KeySpace, c64KeySpace, KBEX_CTRL_LOCK, KBEX_CTRL_LOCK, c64KeyReturn
	};

	return mode ? kbToEventMap2 : kbToEventMap;
}

void updateVControllerMapping(unsigned player, VController::Map &map)
{
	const unsigned p2Bit = player ? JS_P2_BIT : 0;
	map[VController::F_ELEM] = JS_FIRE | p2Bit;
	map[VController::F_ELEM+1] = JS_FIRE | p2Bit | VController::TURBO_BIT;

	map[VController::C_ELEM] = c64KeyF1;
	map[VController::C_ELEM+1] = KBEX_TOGGLE_VKEYBOARD;

	map[VController::D_ELEM] = JS_NW | p2Bit;
	map[VController::D_ELEM+1] = JS_N | p2Bit;
	map[VController::D_ELEM+2] = JS_NE | p2Bit;
	map[VController::D_ELEM+3] = JS_W | p2Bit;
	map[VController::D_ELEM+5] = JS_E | p2Bit;
	map[VController::D_ELEM+6] = JS_SW | p2Bit;
	map[VController::D_ELEM+7] = JS_S | p2Bit;
	map[VController::D_ELEM+8] = JS_SE | p2Bit;
}

static unsigned shiftKeycodeSymbolic(unsigned keycode)
{
	switch(keycode)
	{
		case c64KeyEquals: return c64KeyPlus;
		case c64KeySemiColon: return c64KeyColon;
		case c64KeyMinus: return c64KeyLeftArrow;
		case c64KeyComma: return c64KeyLess;
		case c64KeyPeriod: return c64KeyGreater;
		case c64KeySlash: return c64KeyQuestion;
		case c64KeyApostrophe: return c64KeyQuoteDbl;
		case c64KeyPound: return shiftedPound;
		case c64Key1: return c64KeyExclam;
		case c64Key2: return c64KeyAt;
		case c64Key3: return c64KeyNumberSign;
		case c64Key4: return c64KeyDollar;
		case c64Key5: return c64KeyPercent;
		case c64Key7: return c64KeyAmpersand;
		case c64Key8: return c64KeyAsterisk;
		case c64Key9: return c64KeyParenLeft;
		case c64Key0: return c64KeyParenRight;
	}
	return keycode;
}

static unsigned shiftKeycodePositional(unsigned keycode)
{
	switch(keycode)
	{
		case c64KeyColon: return c64KeyBracketLeft;
		case c64KeySemiColon: return c64KeyBracketRight;
		case c64KeyComma: return c64KeyLess;
		case c64KeyPeriod: return c64KeyGreater;
		case c64KeySlash: return c64KeyQuestion;
		case c64KeyPound: return shiftedPound;
		case c64Key1: return c64KeyExclam;
		case c64Key2: return c64KeyQuoteDbl;
		case c64Key3: return c64KeyNumberSign;
		case c64Key4: return c64KeyDollar;
		case c64Key5: return c64KeyPercent;
		case c64Key6: return c64KeyAmpersand;
		case c64Key7: return c64KeyApostrophe;
		case c64Key8: return c64KeyParenLeft;
		case c64Key9: return c64KeyParenRight;
	}
	return keycode;
}

static unsigned shiftKeycode(unsigned keycode, bool positional)
{
	return positional ? shiftKeycodePositional(keycode) : shiftKeycodeSymbolic(keycode);
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
		case c64KeyCtrlLock : return KBEX_CTRL_LOCK;
		default:
		{
			if(!isEmuKeyInKeyboardRange(input))
				return KBEX_NONE;
			return input;
		}
	}
	return 0;
}

static void handleKeyboardInput(unsigned key, Input::Action action, uint32_t metaState = {}, bool positionalShift = {})
{
	logMsg("key:%u %d", key, (int)action);
	int mod{};
	if(metaState & Input::Meta::SHIFT)
	{
		mod |= KBD_MOD_LSHIFT;
		key = shiftKeycode(key, positionalShift);
	}
	if(metaState & Input::Meta::CAPS_LOCK)
	{
		mod |= KBD_MOD_SHIFTLOCK;
	}
	if(action == Input::Action::PUSHED)
		plugin.keyboard_key_pressed(key, mod);
	else
		plugin.keyboard_key_released(key, mod);
}

void EmuSystem::handleInputAction(EmuApp *app, Input::Action action, unsigned emuKey, uint32_t metaState)
{
	bool positionalShift{};
	if(app)
	{
		if(app->defaultVController().keyboard().shiftIsActive())
		{
			metaState |= Input::Meta::SHIFT;
			positionalShift = true;
		}
	}
	switch(emuKey >> KEY_MODE_SHIFT)
	{
		bcase JS_MODE:
		{
			auto key = emuKey & 0x1F;
			if(optionSwapJoystickPorts == JoystickMode::KEYBOARD)
			{
				if(key & JS_E)
					handleKeyboardInput(c64KeyKbRight, action, metaState, positionalShift);
				else if(key & JS_W)
					handleKeyboardInput(c64KeyKbLeft, action, metaState, positionalShift);
				if(key & JS_N)
					handleKeyboardInput(c64KeyKbUp, action, metaState, positionalShift);
				else if(key & JS_S)
					handleKeyboardInput(c64KeyKbDown, action, metaState, positionalShift);
				if(key & JS_FIRE)
					handleKeyboardInput(c64KeyPound, action, metaState, positionalShift);
			}
			else
			{
				auto &joystick_value = *plugin.joystick_value;
				auto player = (emuKey & IG::bit(5)) ? 2 : 1;
				if(optionSwapJoystickPorts == JoystickMode::SWAPPED)
				{
					player = (player == 1) ? 2 : 1;
				}
				//logMsg("js %X p %d", key, player);
				joystick_value[player] = IG::setOrClearBits(joystick_value[player], (uint8_t)key, action == Input::Action::PUSHED);
			}
		}
		bcase KB_MODE:
		{
			auto key = emuKey & CODE_MASK;
			handleKeyboardInput(key, action, metaState, positionalShift);
		}
		bcase EX_MODE:
		{
			switch(emuKey)
			{
				bcase KBEX_SWAP_JS_PORTS:
				{
					if(action == Input::Action::PUSHED && optionSwapJoystickPorts != JoystickMode::KEYBOARD)
					{
						EmuSystem::sessionOptionSet();
						if(optionSwapJoystickPorts == JoystickMode::SWAPPED)
							optionSwapJoystickPorts = JoystickMode::NORMAL;
						else
							optionSwapJoystickPorts = JoystickMode::SWAPPED;
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
				bcase KBEX_POS_SHIFT_LOCK:
				{
					if(app && action == Input::Action::PUSHED)
					{
						bool active = app->defaultVController().keyboard().toggleShiftActive();
						//logMsg("positional shift:%d", active);
						handleKeyboardInput(c64KeyLeftShift, active ? Input::Action::PUSHED : Input::Action::RELEASED);
					}
				}
				bcase KBEX_CTRL_LOCK:
				{
					if(action == Input::Action::PUSHED)
					{
						ctrlLock ^= true;
						handleKeyboardInput(c64KeyCtrl, ctrlLock ? Input::Action::PUSHED : Input::Action::RELEASED);
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

void EmuSystem::clearInputBuffers(EmuInputView &inputView)
{
	ctrlLock = false;
	auto &joystick_value = *plugin.joystick_value;
	IG::fill(joystick_value);
	plugin.keyboard_key_clear();
}

void EmuSystem::onVKeyboardShown(VControllerKeyboard &kb, bool shown)
{
	if(!shown)
	{
		if(ctrlLock)
		{
			ctrlLock = false;
			handleKeyboardInput(c64KeyCtrl, Input::Action::RELEASED);
		}
		if(kb.shiftIsActive())
		{
			kb.setShiftActive(false);
			handleKeyboardInput(c64KeyLeftShift, Input::Action::RELEASED);
		}
	}
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
	else if(string_equal(keyname, "exclam")) { return c64KeyExclam; }
	else if(string_equal(keyname, "quotedbl")) { return c64KeyQuoteDbl; }
	else if(string_equal(keyname, "numbersign")) { return c64KeyNumberSign; }
	else if(string_equal(keyname, "dollar")) { return c64KeyDollar; }
	else if(string_equal(keyname, "percent")) { return c64KeyPercent; }
	else if(string_equal(keyname, "ampersand")) { return c64KeyAmpersand; }
	else if(string_equal(keyname, "parenleft")) { return c64KeyParenLeft; }
	else if(string_equal(keyname, "parenright")) { return c64KeyParenRight; }
	else if(string_equal(keyname, "bracketleft")) { return c64KeyBracketLeft; }
	else if(string_equal(keyname, "bracketright")) { return c64KeyBracketRight; }
	else if(string_equal(keyname, "less")) { return c64KeyLess; }
	else if(string_equal(keyname, "greater")) { return c64KeyGreater; }
	else if(string_equal(keyname, "question")) { return c64KeyQuestion; }
	else if(string_equal(keyname, "apostrophe")) { return c64KeyApostrophe; }
	else if(string_equal(keyname, "Caps_Lock")) { return c64KeyShiftLock; }
	else if(string_equal(keyname, "bar")) { return shiftedPound; }
	//logWarn("unknown keyname:%s", keyname);
	return 0;
}

void setJoystickMode(JoystickMode mode)
{
	optionSwapJoystickPorts = mode;
	if(mode == JoystickMode::KEYBOARD)
	{
		setIntResource("JoyPort1Device", JOYPORT_ID_NONE);
		setIntResource("JoyPort2Device", JOYPORT_ID_NONE);
	}
	else
	{
		setIntResource("JoyPort1Device", JOYPORT_ID_JOYSTICK);
		setIntResource("JoyPort2Device", JOYPORT_ID_JOYSTICK);
	}
}
