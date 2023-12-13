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
#include <emuframework/keyRemappingUtils.hh>
#include "MainApp.hh"

extern "C"
{
	#include <blueMSX/Input/InputEvent.h>
}

namespace EmuEx
{

bool EmuSystem::inputHasKeyboard = true;
const int EmuSystem::maxPlayers = 2;
constexpr int msxKeyboardKeys = 92;
constexpr int EC_TOGGLE_KB = EC_KEYCOUNT;

constexpr auto dpadKeyInfo = makeArray<KeyInfo>
(
	EC_JOY1_UP,
	EC_JOY1_RIGHT,
	EC_JOY1_DOWN,
	EC_JOY1_LEFT
);

constexpr auto shortcutKeyInfo = makeArray<KeyInfo>
(
	EC_SPACE,
	EC_TOGGLE_KB
);

constexpr auto faceKeyInfo = makeArray<KeyInfo>
(
	EC_JOY1_BUTTON1,
	EC_JOY1_BUTTON2
);

constexpr auto turboFaceKeyInfo = turbo(faceKeyInfo);

constexpr auto colecoNumpadKeyInfo = makeArray<KeyInfo>
(
	EC_COLECO1_0,
	EC_COLECO1_1,
	EC_COLECO1_2,
	EC_COLECO1_3,
	EC_COLECO1_4,
	EC_COLECO1_5,
	EC_COLECO1_6,
	EC_COLECO1_7,
	EC_COLECO1_8,
	EC_COLECO1_9,
	EC_COLECO1_STAR,
	EC_COLECO1_HASH
);

constexpr auto keyboardKeyInfo = makeArray<KeyInfo>
(
	EC_TOGGLE_KB,

	// ROW 0
	EC_F1,
	EC_F2,
	EC_F3,
	EC_F4,
	EC_F5,
	EC_STOP,
	EC_CLS,
	EC_SELECT,
	EC_INS,
	EC_DEL,

	// ROW 1
	EC_ESC,
	EC_1,
	EC_2,
	EC_3,
	EC_4,
	EC_5,
	EC_6,
	EC_7,
	EC_8,
	EC_9,
	EC_0,
	EC_NEG,
	EC_CIRCFLX,
	EC_BKSLASH,
	EC_BKSPACE,

	// ROW 2
	EC_TAB,
	EC_Q,
	EC_W,
	EC_E,
	EC_R,
	EC_T,
	EC_Y,
	EC_U,
	EC_I,
	EC_O,
	EC_P,
	EC_AT,
	EC_LBRACK,
	EC_RETURN,

	// ROW 3
	EC_CTRL,
	EC_A,
	EC_S,
	EC_D,
	EC_F,
	EC_G,
	EC_H,
	EC_J,
	EC_K,
	EC_L,
	EC_SEMICOL,
	EC_COLON,
	EC_RBRACK,

	// ROW 4
	EC_LSHIFT,
	EC_Z,
	EC_X,
	EC_C,
	EC_V,
	EC_B,
	EC_N,
	EC_M,
	EC_COMMA,
	EC_PERIOD,
	EC_DIV,
	EC_UNDSCRE,
	EC_RSHIFT,

	// ROW 5
	EC_CAPS,
	EC_GRAPH,
	EC_TORIKE,
	EC_SPACE,
	EC_JIKKOU,
	EC_CODE,
	EC_PAUSE,

	// ARROWS
	EC_LEFT,
	EC_UP,
	EC_DOWN,
	EC_RIGHT,

	// NUMERIC KEYBOARD
	EC_NUM7,
	EC_NUM8,
	EC_NUM9,
	EC_NUMDIV,
	EC_NUM4,
	EC_NUM5,
	EC_NUM6,
	EC_NUMMUL,
	EC_NUM1,
	EC_NUM2,
	EC_NUM3,
	EC_NUMSUB,
	EC_NUM0,
	EC_NUMPER,
	EC_NUMCOM,
	EC_NUMADD,

	// SVI SPECIFIC KEYS
	EC_PRINT
);

constexpr auto jsKeyInfo = concatToArrayNow<dpadKeyInfo, faceKeyInfo, turboFaceKeyInfo>;
constexpr auto js2KeyInfo = transpose(jsKeyInfo, 1);
constexpr auto colecoNumpad2KeyInfo = transpose(colecoNumpadKeyInfo, 1);

std::span<const KeyCategory> MsxApp::keyCategories()
{
	static constexpr std::array categories
	{
		KeyCategory{"Joystick", jsKeyInfo},
		KeyCategory{"Joystick 2", js2KeyInfo, 1},
		KeyCategory{"Coleco Numpad", colecoNumpadKeyInfo},
		KeyCategory{"Coleco Numpad 2", colecoNumpad2KeyInfo, 1},
		KeyCategory{"Keyboard", keyboardKeyInfo},
	};
	return categories;
}

std::string_view MsxApp::systemKeyCodeToString(KeyCode c)
{
	switch(c)
	{
		case EC_JOY1_UP: return "Up";
		case EC_JOY1_RIGHT: return "Right";
		case EC_JOY1_DOWN: return "Down";
		case EC_JOY1_LEFT: return "Left";
		case EC_JOY1_BUTTON1: return "Button 1";
		case EC_JOY1_BUTTON2: return "Button 2";
		case EC_COLECO1_0: return "Coleco 0";
		case EC_COLECO1_1: return "Coleco 1";
		case EC_COLECO1_2: return "Coleco 2";
		case EC_COLECO1_3: return "Coleco 3";
		case EC_COLECO1_4: return "Coleco 4";
		case EC_COLECO1_5: return "Coleco 5";
		case EC_COLECO1_6: return "Coleco 6";
		case EC_COLECO1_7: return "Coleco 7";
		case EC_COLECO1_8: return "Coleco 8";
		case EC_COLECO1_9: return "Coleco 9";
		case EC_COLECO1_STAR: return "Coleco *";
		case EC_COLECO1_HASH: return "Coleco #";
		case EC_TOGGLE_KB: return "Toggle Keyboard";
		case EC_F1: return "F1";
		case EC_F2: return "F2";
		case EC_F3: return "F3";
		case EC_F4: return "F4";
		case EC_F5: return "F5";
		case EC_STOP: return "Stop";
		case EC_CLS: return "Cls";
		case EC_SELECT: return "Select";
		case EC_INS: return "Ins";
		case EC_DEL: return "Del";
		case EC_ESC: return "Esc";
		case EC_1: return "1 ⇧!";
		case EC_2: return "2 ⇧\"";
		case EC_3: return "3 ⇧#";
		case EC_4: return "4 ⇧$";
		case EC_5: return "5 ⇧%";
		case EC_6: return "6 ⇧&";
		case EC_7: return "7 ⇧'";
		case EC_8: return "8 ⇧(";
		case EC_9: return "9 ⇧)";
		case EC_0: return "0";
		case EC_NEG: return "- ⇧=";
		case EC_CIRCFLX: return "^ ⇧~";
		case EC_BKSLASH: return "\\ ⇧|";
		case EC_BKSPACE: return "Backspace";
		case EC_TAB: return "Tab";
		case EC_Q: return "Q";
		case EC_W: return "W";
		case EC_E: return "E";
		case EC_R: return "R";
		case EC_T: return "T";
		case EC_Y: return "Y";
		case EC_U: return "U";
		case EC_I: return "I";
		case EC_O: return "O";
		case EC_P: return "P";
		case EC_AT: return "@ ⇧`";
		case EC_LBRACK: return "[ ⇧{";
		case EC_RETURN: return "Return";
		case EC_CTRL: return "Ctrl";
		case EC_A: return "A";
		case EC_S: return "S";
		case EC_D: return "D";
		case EC_F: return "F";
		case EC_G: return "G";
		case EC_H: return "H";
		case EC_J: return "J";
		case EC_K: return "K";
		case EC_L: return "L";
		case EC_SEMICOL: return "; ⇧+";
		case EC_COLON: return ": ⇧*";
		case EC_RBRACK: return "] ⇧}";
		case EC_LSHIFT: return "Left Shift";
		case EC_Z: return "Z";
		case EC_X: return "X";
		case EC_C: return "C";
		case EC_V: return "V";
		case EC_B: return "B";
		case EC_N: return "N";
		case EC_M: return "M";
		case EC_COMMA: return ", ⇧<";
		case EC_PERIOD: return ". ⇧>";
		case EC_DIV: return "/ ⇧?";
		case EC_UNDSCRE: return "_";
		case EC_RSHIFT: return "Right Shift";
		case EC_CAPS: return "Caps";
		case EC_GRAPH: return "Graph";
		case EC_TORIKE: return "Cancel";
		case EC_SPACE: return "Space";
		case EC_JIKKOU: return "Execute";
		case EC_CODE: return "Code";
		case EC_PAUSE: return "Pause";
		case EC_UP: return "Up Arrow";
		case EC_RIGHT: return "Right Arrow";
		case EC_DOWN: return "Down Arrow";
		case EC_LEFT: return "Left Arrow";
		case EC_NUM0: return "Num 0";
		case EC_NUM1: return "Num 1";
		case EC_NUM2: return "Num 2";
		case EC_NUM3: return "Num 3";
		case EC_NUM4: return "Num 4";
		case EC_NUM5: return "Num 5";
		case EC_NUM6: return "Num 6";
		case EC_NUM7: return "Num 7";
		case EC_NUM8: return "Num 8";
		case EC_NUM9: return "Num 9";
		case EC_NUMDIV: return "Num Div";
		case EC_NUMMUL: return "Num Mult";
		case EC_NUMSUB: return "Num Sub";
		case EC_NUMPER: return "Num Period";
		case EC_NUMCOM: return "Num Comma";
		case EC_NUMADD: return "Num Add";
		case EC_PRINT: return "Print";
		default: return "";
	}
}

std::span<const KeyConfigDesc> MsxApp::defaultKeyConfigs()
{
	using namespace IG::Input;

	static constexpr std::array pcKeyboardMap
	{
		KeyMapping{EC_JOY1_UP, Keycode::UP},
		KeyMapping{EC_JOY1_RIGHT, Keycode::RIGHT},
		KeyMapping{EC_JOY1_DOWN, Keycode::DOWN},
		KeyMapping{EC_JOY1_LEFT, Keycode::LEFT},
		KeyMapping{EC_JOY1_BUTTON1, Keycode::LCTRL},
		KeyMapping{EC_JOY1_BUTTON2, Keycode::LALT},
		KeyMapping{EC_F1, Keycode::F1},
		KeyMapping{EC_F2, Keycode::F2},
		KeyMapping{EC_F3, Keycode::F3},
		KeyMapping{EC_F4, Keycode::F4},
		KeyMapping{EC_F5, Keycode::F5},
		KeyMapping{EC_1, Keycode::_1},
		KeyMapping{EC_2, Keycode::_2},
		KeyMapping{EC_3, Keycode::_3},
		KeyMapping{EC_4, Keycode::_4},
		KeyMapping{EC_5, Keycode::_5},
		KeyMapping{EC_6, Keycode::_6},
		KeyMapping{EC_7, Keycode::_7},
		KeyMapping{EC_8, Keycode::_8},
		KeyMapping{EC_9, Keycode::_9},
		KeyMapping{EC_0, Keycode::_0},
		KeyMapping{EC_NEG, Keycode::MINUS},
		KeyMapping{EC_BKSLASH, Keycode::BACKSLASH},
		KeyMapping{EC_BKSPACE, Keycode::BACK_SPACE},
		KeyMapping{EC_CTRL, Keycode::RCTRL},
		KeyMapping{EC_Q, Keycode::Q},
		KeyMapping{EC_W, Keycode::W},
		KeyMapping{EC_E, Keycode::E},
		KeyMapping{EC_R, Keycode::R},
		KeyMapping{EC_T, Keycode::T},
		KeyMapping{EC_Y, Keycode::Y},
		KeyMapping{EC_U, Keycode::U},
		KeyMapping{EC_I, Keycode::I},
		KeyMapping{EC_O, Keycode::O},
		KeyMapping{EC_P, Keycode::P},
		KeyMapping{EC_STOP, Keycode::END},
		KeyMapping{EC_PAUSE, Keycode::PAUSE},
		KeyMapping{EC_CAPS, Keycode::CAPS},
		KeyMapping{EC_A, Keycode::A},
		KeyMapping{EC_S, Keycode::S},
		KeyMapping{EC_D, Keycode::D},
		KeyMapping{EC_F, Keycode::F},
		KeyMapping{EC_G, Keycode::G},
		KeyMapping{EC_H, Keycode::H},
		KeyMapping{EC_J, Keycode::J},
		KeyMapping{EC_K, Keycode::K},
		KeyMapping{EC_L, Keycode::L},
		KeyMapping{EC_SEMICOL, Keycode::SEMICOLON},
		KeyMapping{EC_RETURN, Keycode::ENTER},
		KeyMapping{EC_TAB, Keycode::TAB},
		KeyMapping{EC_LSHIFT, Keycode::LSHIFT},
		KeyMapping{EC_Z, Keycode::Z},
		KeyMapping{EC_X, Keycode::X},
		KeyMapping{EC_C, Keycode::C},
		KeyMapping{EC_V, Keycode::V},
		KeyMapping{EC_B, Keycode::B},
		KeyMapping{EC_N, Keycode::N},
		KeyMapping{EC_M, Keycode::M},
		KeyMapping{EC_COMMA, Keycode::COMMA},
		KeyMapping{EC_PERIOD, Keycode::PERIOD},
		KeyMapping{EC_DIV, Keycode::SLASH},
		KeyMapping{EC_RSHIFT, Keycode::RSHIFT},
		KeyMapping{EC_SPACE, Keycode::SPACE},
		KeyMapping{EC_LBRACK, Keycode::LEFT_BRACKET},
		KeyMapping{EC_RBRACK, Keycode::RIGHT_BRACKET},
	};

	static constexpr std::array genericGamepadMap
	{
		KeyMapping{EC_JOY1_UP, Keycode::UP},
		KeyMapping{EC_JOY1_RIGHT, Keycode::RIGHT},
		KeyMapping{EC_JOY1_DOWN, Keycode::DOWN},
		KeyMapping{EC_JOY1_LEFT, Keycode::LEFT},
		KeyMapping{EC_JOY1_BUTTON1, Keycode::GAME_A},
		KeyMapping{EC_JOY1_BUTTON2, Keycode::GAME_X},
		KeyMapping{EC_F1, Keycode::GAME_Y},
		KeyMapping{EC_F2, Keycode::GAME_L1},
		KeyMapping{EC_F3, Keycode::GAME_R1},
		KeyMapping{EC_F4, Keycode::GAME_B},
		KeyMapping{EC_F5, Keycode::GAME_SELECT},
	};

	static constexpr std::array wiimoteMap
	{
		KeyMapping{EC_JOY1_UP, WiimoteKey::UP},
		KeyMapping{EC_JOY1_RIGHT, WiimoteKey::RIGHT},
		KeyMapping{EC_JOY1_DOWN, WiimoteKey::DOWN},
		KeyMapping{EC_JOY1_LEFT, WiimoteKey::LEFT},
		KeyMapping{EC_JOY1_BUTTON1, WiimoteKey::_1},
		KeyMapping{EC_JOY1_BUTTON2, WiimoteKey::_2},
		KeyMapping{EC_F2, WiimoteKey::MINUS},
		KeyMapping{EC_F1, WiimoteKey::PLUS},
	};

	return genericKeyConfigs<pcKeyboardMap, genericGamepadMap, wiimoteMap>();
}

bool MsxApp::allowsTurboModifier(KeyCode c)
{
	switch(c)
	{
		case EC_JOY1_BUTTON1 ... EC_JOY1_BUTTON2:
			return true;
		default: return false;
	}
}

constexpr FRect gpImageCoords(IRect cellRelBounds)
{
	constexpr F2Size imageSize{512, 256};
	constexpr int cellSize = 32;
	return (cellRelBounds.relToAbs() * cellSize).as<float>() / imageSize;
}

AssetDesc MsxApp::vControllerAssetDesc(KeyInfo key) const
{
	static constexpr struct VirtualControllerAssets
	{
		AssetDesc dpad{AssetFileID::gamepadOverlay, gpImageCoords({{}, {4, 4}})},

		a{AssetFileID::gamepadOverlay, gpImageCoords({{4, 0}, {2, 2}})},
		b{AssetFileID::gamepadOverlay, gpImageCoords({{6, 0}, {2, 2}})},

		zero{AssetFileID::gamepadOverlay,  gpImageCoords({{8,  0}, {2, 2}})},
		one{AssetFileID::gamepadOverlay,   gpImageCoords({{10, 0}, {2, 2}})},
		two{AssetFileID::gamepadOverlay,   gpImageCoords({{12, 0}, {2, 2}})},
		three{AssetFileID::gamepadOverlay, gpImageCoords({{14, 0}, {2, 2}})},
		four{AssetFileID::gamepadOverlay,  gpImageCoords({{4,  2}, {2, 2}})},
		five{AssetFileID::gamepadOverlay,  gpImageCoords({{6,  2}, {2, 2}})},
		six{AssetFileID::gamepadOverlay,   gpImageCoords({{8,  2}, {2, 2}})},
		seven{AssetFileID::gamepadOverlay, gpImageCoords({{10, 2}, {2, 2}})},
		eight{AssetFileID::gamepadOverlay, gpImageCoords({{12, 2}, {2, 2}})},
		nine{AssetFileID::gamepadOverlay,  gpImageCoords({{14, 2}, {2, 2}})},
		star{AssetFileID::gamepadOverlay,  gpImageCoords({{0,  4}, {2, 2}})},
		pound{AssetFileID::gamepadOverlay, gpImageCoords({{2,  4}, {2, 2}})},

		kb{AssetFileID::gamepadOverlay,    gpImageCoords({{0, 6}, {2, 1}}), {1, 2}},
		space{AssetFileID::gamepadOverlay, gpImageCoords({{0, 7}, {2, 1}}), {1, 2}},

		blank{AssetFileID::gamepadOverlay, gpImageCoords({{4, 4}, {2, 2}})};
	} virtualControllerAssets;

	static_assert(offsetof(VirtualControllerAssets, zero) + 11 * sizeof(AssetDesc) == offsetof(VirtualControllerAssets, pound),
		"keyboard assets must be in sequence");

	if(key[0] == 0)
		return virtualControllerAssets.dpad;
	switch(key[0])
	{
		case EC_JOY1_BUTTON1: return virtualControllerAssets.a;
		case EC_JOY1_BUTTON2: return virtualControllerAssets.b;
		case EC_COLECO1_0 ... EC_COLECO1_HASH:
			return (&virtualControllerAssets.zero)[key[0] - EC_COLECO1_0];
		case EC_TOGGLE_KB: return virtualControllerAssets.kb;
		case EC_SPACE: return virtualControllerAssets.space;
		default: return virtualControllerAssets.blank;
	}
}

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
	EC_TAB, std::array{EC_8, EC_LSHIFT}, std::array{EC_9, EC_LSHIFT}, std::array{EC_3, EC_LSHIFT}, std::array{EC_4, EC_LSHIFT}, std::array{EC_SEMICOL, EC_LSHIFT}, EC_NEG, EC_SEMICOL, EC_ESC, EC_NONE,
	EC_NONE, EC_NONE, EC_NONE, EC_SPACE, EC_SPACE, EC_SPACE, EC_SPACE, EC_PERIOD, EC_PERIOD, EC_RETURN
};

void setupVKeyboardMap(EmuApp &app, unsigned boardType)
{
	if(boardType != BOARD_COLECO)
	{
		for(auto i : iotaCount(10)) // 1 - 0
			kbToEventMap2[10 + i] = EC_1 + i;
		kbToEventMap2[23] = std::array{EC_3, EC_LSHIFT};
	}
	else
	{
		for(auto i : iotaCount(9)) // 1 - 9
			kbToEventMap2[10 + i] = EC_COLECO1_1 + i;
		kbToEventMap2[19] = EC_COLECO1_0;
		kbToEventMap2[23] = EC_COLECO1_HASH;
	}
	app.inputManager.updateKeyboardMapping();
}

VController::KbMap MsxSystem::vControllerKeyboardMap(VControllerKbMode mode)
{
	return mode == VControllerKbMode::LAYOUT_2 ? kbToEventMap2 : kbToEventMap;
}

void MsxSystem::handleInputAction(EmuApp *appPtr, InputAction a)
{
	if(a.code == EC_KEYCOUNT)
	{
		if(appPtr && a.isPushed())
			appPtr->inputManager.toggleKeyboard();
	}
	else
	{
		assert(a.code < EC_KEYCOUNT);
		auto keyIdx = [&] -> int
		{
			bool isPort1 = a.flags.deviceId == 0;
			switch(a.code)
			{
				case EC_JOY1_UP ... EC_JOY1_BUTTON2:
					return isPort1 ? a.code : a.code + 10;
				case EC_COLECO1_0 ... EC_COLECO1_HASH:
					return isPort1 ? a.code : a.code + 20;
				default:
					return activeBoardType == BOARD_COLECO ? EC_COLECO1_STAR : a.code;
			}
		}();
		eventMap[keyIdx] = a.isPushed();
	}
}

void MsxSystem::clearInputBuffers(EmuInputView &)
{
	fill(eventMap);
}

SystemInputDeviceDesc MsxSystem::inputDeviceDesc(int idx) const
{
	static constexpr std::array jsComponents
	{
		InputComponentDesc{"D-Pad", dpadKeyInfo, InputComponent::dPad, LB2DO},
		InputComponentDesc{"Joystick Buttons", faceKeyInfo, InputComponent::button, RB2DO},
		InputComponentDesc{"Space & Keyboard Toggle", shortcutKeyInfo, InputComponent::button, RB2DO, {.rowSize = 1}},
	};

	static constexpr SystemInputDeviceDesc jsDesc{"Joystick", jsComponents};

	return jsDesc;
}

}
