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
#include <emuframework/keyRemappingUtils.hh>
#include "MainSystem.hh"
#include "MainApp.hh"
#include <imagine/logger/logger.h>

extern "C"
{
	#include "kbd.h"
	#include "joyport.h"
}

namespace EmuEx
{

constexpr SystemLogger log{"C64.emu"};
bool EmuSystem::inputHasKeyboard = true;
const int EmuSystem::maxPlayers = 2;

enum class C64Key : KeyCode
{
	Up = 1,
	Right,
	Down,
	Left,
	JSTrigger,

	SwapJSPorts,
	ToggleKB,

	KeyboardFirstEnum,
	KeyboardF1 = KeyboardFirstEnum,
	KeyboardF2,
	KeyboardF3,
	KeyboardF4,
	KeyboardF5,
	KeyboardF6,
	KeyboardF7,
	KeyboardF8,

	KeyboardLeftArrow,
	Keyboard1,
	Keyboard2,
	Keyboard3,
	Keyboard4,
	Keyboard5,
	Keyboard6,
	Keyboard7,
	Keyboard8,
	Keyboard9,
	Keyboard0,
	KeyboardPlus,
	KeyboardMinus,
	KeyboardPound,
	KeyboardClrHome,
	KeyboardInstDel,

	KeyboardCtrl,
	KeyboardQ,
	KeyboardW,
	KeyboardE,
	KeyboardR,
	KeyboardT,
	KeyboardY,
	KeyboardU,
	KeyboardI,
	KeyboardO,
	KeyboardP,
	KeyboardAt,
	KeyboardAsterisk,
	KeyboardUpArrow,
	KeyboardRestore,

	KeyboardRunStop,
	KeyboardShiftLock,
	KeyboardA,
	KeyboardS,
	KeyboardD,
	KeyboardF,
	KeyboardG,
	KeyboardH,
	KeyboardJ,
	KeyboardK,
	KeyboardL,
	KeyboardColon,
	KeyboardSemiColon,
	KeyboardEquals,
	KeyboardReturn,

	KeyboardCommodore,
	KeyboardLeftShift,
	KeyboardZ,
	KeyboardX,
	KeyboardC,
	KeyboardV,
	KeyboardB,
	KeyboardN,
	KeyboardM,
	KeyboardComma,
	KeyboardPeriod,
	KeyboardSlash,
	KeyboardRightShift,
	KeyboardUp,
	KeyboardRight,
	KeyboardDown,
	KeyboardLeft,

	KeyboardSpace,

	// other virtual & symbolic keys
	KeyboardCtrlLock,
	KeyboardExclam,
	KeyboardQuoteDbl,
	KeyboardNumberSign,
	KeyboardDollar,
	KeyboardPercent,
	KeyboardAmpersand,
	KeyboardParenLeft,
	KeyboardParenRight,
	KeyboardBracketLeft,
	KeyboardBracketRight,
	KeyboardLess,
	KeyboardGreater,
	KeyboardQuestion,
	KeyboardApostrophe,

	KeyboardLastEnum = KeyboardApostrophe,
};

constexpr auto specialFunctionKeyInfo = makeArray<KeyInfo>
(
	C64Key::SwapJSPorts,
	C64Key::ToggleKB
);

constexpr auto dpadKeyInfo = makeArray<KeyInfo>
(
	C64Key::Up,
	C64Key::Right,
	C64Key::Down,
	C64Key::Left
);

constexpr auto triggerKeyInfo = makeArray<KeyInfo>(C64Key::JSTrigger);
constexpr auto turboTriggerKeyInfo = turbo(triggerKeyInfo);

constexpr auto shortcutKeyInfo = makeArray<KeyInfo>
(
	C64Key::KeyboardF1,
	C64Key::ToggleKB
);

constexpr auto kbKeyInfo = makeArray<KeyInfo>
(
	C64Key::KeyboardF1,
	C64Key::KeyboardF2,
	C64Key::KeyboardF3,
	C64Key::KeyboardF4,
	C64Key::KeyboardF5,
	C64Key::KeyboardF6,
	C64Key::KeyboardF7,
	C64Key::KeyboardF8,
	C64Key::Keyboard1,
	C64Key::Keyboard2,
	C64Key::Keyboard3,
	C64Key::Keyboard4,
	C64Key::Keyboard5,
	C64Key::Keyboard6,
	C64Key::Keyboard7,
	C64Key::Keyboard8,
	C64Key::Keyboard9,
	C64Key::Keyboard0,
	C64Key::KeyboardExclam,
	C64Key::KeyboardAt,
	C64Key::KeyboardNumberSign,
	C64Key::KeyboardDollar,
	C64Key::KeyboardPercent,
	C64Key::KeyboardAmpersand,
	C64Key::KeyboardAsterisk,
	C64Key::KeyboardParenLeft,
	C64Key::KeyboardParenRight,
	C64Key::KeyboardPlus,
	C64Key::KeyboardMinus,
	C64Key::KeyboardLeftArrow,
	C64Key::KeyboardPound,
	C64Key::KeyboardClrHome,
	C64Key::KeyboardInstDel,
	C64Key::KeyboardCtrl,
	C64Key::KeyboardQ,
	C64Key::KeyboardW,
	C64Key::KeyboardE,
	C64Key::KeyboardR,
	C64Key::KeyboardT,
	C64Key::KeyboardY,
	C64Key::KeyboardU,
	C64Key::KeyboardI,
	C64Key::KeyboardO,
	C64Key::KeyboardP,
	C64Key::KeyboardUpArrow,
	C64Key::KeyboardRestore,
	C64Key::KeyboardRunStop,
	C64Key::KeyboardShiftLock,
	C64Key::KeyboardA,
	C64Key::KeyboardS,
	C64Key::KeyboardD,
	C64Key::KeyboardF,
	C64Key::KeyboardG,
	C64Key::KeyboardH,
	C64Key::KeyboardJ,
	C64Key::KeyboardK,
	C64Key::KeyboardL,
	C64Key::KeyboardSemiColon,
	C64Key::KeyboardColon,
	C64Key::KeyboardEquals,
	C64Key::KeyboardReturn,
	C64Key::KeyboardCommodore,
	C64Key::KeyboardLeftShift,
	C64Key::KeyboardZ,
	C64Key::KeyboardX,
	C64Key::KeyboardC,
	C64Key::KeyboardV,
	C64Key::KeyboardB,
	C64Key::KeyboardN,
	C64Key::KeyboardM,
	C64Key::KeyboardComma,
	C64Key::KeyboardPeriod,
	C64Key::KeyboardSlash,
	C64Key::KeyboardApostrophe,
	C64Key::KeyboardLess,
	C64Key::KeyboardGreater,
	C64Key::KeyboardQuestion,
	C64Key::KeyboardQuoteDbl,
	C64Key::KeyboardRightShift,
	C64Key::KeyboardUp,
	C64Key::KeyboardRight,
	C64Key::KeyboardDown,
	C64Key::KeyboardLeft,
	C64Key::KeyboardSpace,
	C64Key::KeyboardCtrlLock,
	C64Key::KeyboardBracketLeft,
	C64Key::KeyboardBracketRight
);

constexpr auto jsKeyInfo = concatToArrayNow<dpadKeyInfo, triggerKeyInfo, turboTriggerKeyInfo>;
constexpr auto js2KeyInfo = transpose(jsKeyInfo, 1);

std::span<const KeyCategory> C64App::keyCategories()
{
	static constexpr KeyCategory categories[]
	{
		{"Joystick", jsKeyInfo},
		{"Joystick 2", js2KeyInfo, 1},
		{"Special Functions", specialFunctionKeyInfo},
		{"Keyboard", kbKeyInfo},
	};
	return categories;
}

std::string_view C64App::systemKeyCodeToString(KeyCode c)
{
	switch(C64Key(c))
	{
		case C64Key::Up: return "Up";
		case C64Key::Right: return "Right";
		case C64Key::Down: return "Down";
		case C64Key::Left: return "Left";
		case C64Key::JSTrigger: return "Trigger";
		case C64Key::SwapJSPorts: return "Swap Ports";
		case C64Key::ToggleKB: return "Toggle Keyboard";
		case C64Key::KeyboardF1: return "F1";
		case C64Key::KeyboardF2: return "F2";
		case C64Key::KeyboardF3: return "F3";
		case C64Key::KeyboardF4: return "F4";
		case C64Key::KeyboardF5: return "F5";
		case C64Key::KeyboardF6: return "F6";
		case C64Key::KeyboardF7: return "F7";
		case C64Key::KeyboardF8: return "F8";
		case C64Key::KeyboardLeftArrow: return "←";
		case C64Key::Keyboard1: return "1";
		case C64Key::Keyboard2: return "2";
		case C64Key::Keyboard3: return "3";
		case C64Key::Keyboard4: return "4";
		case C64Key::Keyboard5: return "5";
		case C64Key::Keyboard6: return "6";
		case C64Key::Keyboard7: return "7";
		case C64Key::Keyboard8: return "8";
		case C64Key::Keyboard9: return "9";
		case C64Key::Keyboard0: return "0";
		case C64Key::KeyboardPlus: return "+";
		case C64Key::KeyboardMinus: return "-";
		case C64Key::KeyboardPound: return "£";
		case C64Key::KeyboardClrHome: return "Clr Home";
		case C64Key::KeyboardInstDel: return "Inst Del";
		case C64Key::KeyboardCtrl: return "Ctrl";
		case C64Key::KeyboardQ: return "Q";
		case C64Key::KeyboardW: return "W";
		case C64Key::KeyboardE: return "E";
		case C64Key::KeyboardR: return "R";
		case C64Key::KeyboardT: return "T";
		case C64Key::KeyboardY: return "Y";
		case C64Key::KeyboardU: return "U";
		case C64Key::KeyboardI: return "I";
		case C64Key::KeyboardO: return "O";
		case C64Key::KeyboardP: return "P";
		case C64Key::KeyboardAt: return "@";
		case C64Key::KeyboardAsterisk: return "*";
		case C64Key::KeyboardUpArrow: return "↑";
		case C64Key::KeyboardRestore: return "Restore";
		case C64Key::KeyboardRunStop: return "Run Stop";
		case C64Key::KeyboardShiftLock: return "Shift Lock";
		case C64Key::KeyboardA: return "A";
		case C64Key::KeyboardS: return "S";
		case C64Key::KeyboardD: return "D";
		case C64Key::KeyboardF: return "F";
		case C64Key::KeyboardG: return "G";
		case C64Key::KeyboardH: return "H";
		case C64Key::KeyboardJ: return "J";
		case C64Key::KeyboardK: return "K";
		case C64Key::KeyboardL: return "L";
		case C64Key::KeyboardColon: return ":";
		case C64Key::KeyboardSemiColon: return ";";
		case C64Key::KeyboardEquals: return "=";
		case C64Key::KeyboardReturn: return "Return";
		case C64Key::KeyboardCommodore: return "Commodore Logo";
		case C64Key::KeyboardLeftShift: return "Left Shift";
		case C64Key::KeyboardZ: return "Z";
		case C64Key::KeyboardX: return "X";
		case C64Key::KeyboardC: return "C";
		case C64Key::KeyboardV: return "V";
		case C64Key::KeyboardB: return "B";
		case C64Key::KeyboardN: return "N";
		case C64Key::KeyboardM: return "M";
		case C64Key::KeyboardComma: return ",";
		case C64Key::KeyboardPeriod: return ".";
		case C64Key::KeyboardSlash: return "/";
		case C64Key::KeyboardRightShift: return "Right Shift";
		case C64Key::KeyboardUp: return "Up";
		case C64Key::KeyboardRight: return "Right";
		case C64Key::KeyboardDown: return "Down";
		case C64Key::KeyboardLeft: return "Left";
		case C64Key::KeyboardSpace: return "Space";
		case C64Key::KeyboardCtrlLock: return "Ctrl Lock";
		case C64Key::KeyboardExclam: return "!";
		case C64Key::KeyboardQuoteDbl: return "\"";
		case C64Key::KeyboardNumberSign: return "#";
		case C64Key::KeyboardDollar: return "$";
		case C64Key::KeyboardPercent: return "%";
		case C64Key::KeyboardAmpersand: return "&";
		case C64Key::KeyboardParenLeft: return "(";
		case C64Key::KeyboardParenRight: return ")";
		case C64Key::KeyboardBracketLeft: return "[";
		case C64Key::KeyboardBracketRight: return "]";
		case C64Key::KeyboardLess: return "<";
		case C64Key::KeyboardGreater: return ">";
		case C64Key::KeyboardQuestion: return "?";
		case C64Key::KeyboardApostrophe: return "'";
		default: return "";
	}
}

std::span<const KeyConfigDesc> C64App::defaultKeyConfigs()
{
	using namespace IG::Input;

	static constexpr std::array pcKeyboardMap
	{
		KeyMapping{C64Key::Up, Keycode::UP},
		KeyMapping{C64Key::Right, Keycode::RIGHT},
		KeyMapping{C64Key::Down, Keycode::DOWN},
		KeyMapping{C64Key::Left, Keycode::LEFT},
		KeyMapping{C64Key::JSTrigger, Keycode::LALT},
		KeyMapping{C64Key::SwapJSPorts, Keycode::F10},
		KeyMapping{C64Key::KeyboardF1, Keycode::F1},
		KeyMapping{C64Key::KeyboardF2, Keycode::F2},
		KeyMapping{C64Key::KeyboardF3, Keycode::F3},
		KeyMapping{C64Key::KeyboardF4, Keycode::F4},
		KeyMapping{C64Key::KeyboardF5, Keycode::F5},
		KeyMapping{C64Key::KeyboardF6, Keycode::F6},
		KeyMapping{C64Key::KeyboardF7, Keycode::F7},
		KeyMapping{C64Key::KeyboardF8, Keycode::F8},
		KeyMapping{C64Key::Keyboard1, Keycode::_1},
		KeyMapping{C64Key::Keyboard2, Keycode::_2},
		KeyMapping{C64Key::Keyboard3, Keycode::_3},
		KeyMapping{C64Key::Keyboard4, Keycode::_4},
		KeyMapping{C64Key::Keyboard5, Keycode::_5},
		KeyMapping{C64Key::Keyboard6, Keycode::_6},
		KeyMapping{C64Key::Keyboard7, Keycode::_7},
		KeyMapping{C64Key::Keyboard8, Keycode::_8},
		KeyMapping{C64Key::Keyboard9, Keycode::_9},
		KeyMapping{C64Key::Keyboard0, Keycode::_0},
		KeyMapping{C64Key::KeyboardExclam, {Keycode::LSHIFT, Keycode::_1}},
		KeyMapping{C64Key::KeyboardAt, {Keycode::LSHIFT, Keycode::_2}},
		KeyMapping{C64Key::KeyboardNumberSign, {Keycode::LSHIFT, Keycode::_3}},
		KeyMapping{C64Key::KeyboardDollar, {Keycode::LSHIFT, Keycode::_4}},
		KeyMapping{C64Key::KeyboardPercent, {Keycode::LSHIFT, Keycode::_5}},
		KeyMapping{C64Key::KeyboardAmpersand, {Keycode::LSHIFT, Keycode::_7}},
		KeyMapping{C64Key::KeyboardAsterisk, {Keycode::LSHIFT, Keycode::_8}},
		KeyMapping{C64Key::KeyboardParenLeft, {Keycode::LSHIFT, Keycode::_9}},
		KeyMapping{C64Key::KeyboardParenRight, {Keycode::LSHIFT, Keycode::_0}},
		KeyMapping{C64Key::KeyboardMinus, Keycode::MINUS},
		KeyMapping{C64Key::KeyboardLeftArrow, {Keycode::LSHIFT, Keycode::MINUS}},
		KeyMapping{C64Key::KeyboardPound, Keycode::BACKSLASH},
		KeyMapping{C64Key::KeyboardClrHome, Keycode::HOME},
		KeyMapping{C64Key::KeyboardInstDel, Keycode::BACK_SPACE},
		KeyMapping{C64Key::KeyboardCtrl, Keycode::LCTRL},
		KeyMapping{C64Key::KeyboardQ, Keycode::Q},
		KeyMapping{C64Key::KeyboardW, Keycode::W},
		KeyMapping{C64Key::KeyboardE, Keycode::E},
		KeyMapping{C64Key::KeyboardR, Keycode::R},
		KeyMapping{C64Key::KeyboardT, Keycode::T},
		KeyMapping{C64Key::KeyboardY, Keycode::Y},
		KeyMapping{C64Key::KeyboardU, Keycode::U},
		KeyMapping{C64Key::KeyboardI, Keycode::I},
		KeyMapping{C64Key::KeyboardO, Keycode::O},
		KeyMapping{C64Key::KeyboardP, Keycode::P},
		KeyMapping{C64Key::KeyboardUpArrow, Keycode::PGDOWN},
		KeyMapping{C64Key::KeyboardRestore, Keycode::END},
		KeyMapping{C64Key::KeyboardRunStop, Keycode::PAUSE},
		KeyMapping{C64Key::KeyboardShiftLock, Keycode::CAPS},
		KeyMapping{C64Key::KeyboardA, Keycode::A},
		KeyMapping{C64Key::KeyboardS, Keycode::S},
		KeyMapping{C64Key::KeyboardD, Keycode::D},
		KeyMapping{C64Key::KeyboardF, Keycode::F},
		KeyMapping{C64Key::KeyboardG, Keycode::G},
		KeyMapping{C64Key::KeyboardH, Keycode::H},
		KeyMapping{C64Key::KeyboardJ, Keycode::J},
		KeyMapping{C64Key::KeyboardK, Keycode::K},
		KeyMapping{C64Key::KeyboardL, Keycode::L},
		KeyMapping{C64Key::KeyboardSemiColon, Keycode::SEMICOLON},
		KeyMapping{C64Key::KeyboardColon, {Keycode::LSHIFT, Keycode::SEMICOLON}},
		KeyMapping{C64Key::KeyboardEquals, Keycode::EQUALS},
		KeyMapping{C64Key::KeyboardPlus, {Keycode::LSHIFT, Keycode::EQUALS}},
		KeyMapping{C64Key::KeyboardReturn, Keycode::ENTER},
		KeyMapping{C64Key::KeyboardCommodore, Keycode::TAB},
		KeyMapping{C64Key::KeyboardLeftShift, Keycode::LSHIFT},
		KeyMapping{C64Key::KeyboardZ, Keycode::Z},
		KeyMapping{C64Key::KeyboardX, Keycode::X},
		KeyMapping{C64Key::KeyboardC, Keycode::C},
		KeyMapping{C64Key::KeyboardV, Keycode::V},
		KeyMapping{C64Key::KeyboardB, Keycode::B},
		KeyMapping{C64Key::KeyboardN, Keycode::N},
		KeyMapping{C64Key::KeyboardM, Keycode::M},
		KeyMapping{C64Key::KeyboardComma, Keycode::COMMA},
		KeyMapping{C64Key::KeyboardLess, {Keycode::LSHIFT, Keycode::COMMA}},
		KeyMapping{C64Key::KeyboardPeriod, Keycode::PERIOD},
		KeyMapping{C64Key::KeyboardGreater, {Keycode::LSHIFT, Keycode::PERIOD}},
		KeyMapping{C64Key::KeyboardSlash, Keycode::SLASH},
		KeyMapping{C64Key::KeyboardQuestion, {Keycode::LSHIFT, Keycode::SLASH}},
		KeyMapping{C64Key::KeyboardRightShift, Keycode::RSHIFT},
		KeyMapping{C64Key::KeyboardSpace, Keycode::SPACE},
		KeyMapping{C64Key::KeyboardCtrlLock, Keycode::INSERT},
		KeyMapping{C64Key::KeyboardBracketLeft, Keycode::LEFT_BRACKET},
		KeyMapping{C64Key::KeyboardBracketRight, Keycode::RIGHT_BRACKET},
		KeyMapping{C64Key::KeyboardApostrophe, Keycode::APOSTROPHE},
		KeyMapping{C64Key::KeyboardQuoteDbl, {Keycode::LSHIFT, Keycode::APOSTROPHE}},
	};

	static constexpr std::array genericGamepadMap
	{
		KeyMapping{C64Key::Up, Keycode::UP},
		KeyMapping{C64Key::Right, Keycode::RIGHT},
		KeyMapping{C64Key::Down, Keycode::DOWN},
		KeyMapping{C64Key::Left, Keycode::LEFT},
		KeyMapping{C64Key::JSTrigger, Keycode::GAME_A},
		KeyMapping{C64Key::SwapJSPorts, Keycode::GAME_R1},
		KeyMapping{C64Key::ToggleKB, Keycode::GAME_START},
		KeyMapping{C64Key::KeyboardF1, Keycode::GAME_SELECT},
		KeyMapping{C64Key::KeyboardF3, Keycode::GAME_B},
		KeyMapping{C64Key::KeyboardF5, Keycode::GAME_Y},
		KeyMapping{C64Key::KeyboardF7, Keycode::GAME_L1},
	};

	static constexpr std::array wiimoteMap
	{
		KeyMapping{C64Key::Up, WiimoteKey::UP},
		KeyMapping{C64Key::Right, WiimoteKey::RIGHT},
		KeyMapping{C64Key::Down, WiimoteKey::DOWN},
		KeyMapping{C64Key::Left, WiimoteKey::LEFT},
		KeyMapping{C64Key::JSTrigger, WiimoteKey::_1},
		KeyMapping{C64Key::KeyboardF1, WiimoteKey::PLUS},
	};

	return genericKeyConfigs<pcKeyboardMap, genericGamepadMap, wiimoteMap>();
}

bool C64App::allowsTurboModifier(KeyCode c)
{
	return C64Key(c) == C64Key::JSTrigger;
}

constexpr FRect gpImageCoords(IRect cellRelBounds)
{
	constexpr F2Size imageSize{256, 128};
	constexpr int cellSize = 32;
	return (cellRelBounds.relToAbs() * cellSize).as<float>() / imageSize;
}

AssetDesc C64App::vControllerAssetDesc(KeyInfo key) const
{
	static constexpr struct VirtualControllerAssets
	{
		AssetDesc dpad{AssetFileID::gamepadOverlay, gpImageCoords({{}, {4, 4}})},

		jsBtn{AssetFileID::gamepadOverlay, gpImageCoords({{4, 0}, {2, 2}})},

		kb{AssetFileID::gamepadOverlay,          gpImageCoords({{6, 0}, {2, 1}}), {1, 2}},
		swapJsPorts{AssetFileID::gamepadOverlay, gpImageCoords({{6, 1}, {2, 1}}), {1, 2}},

		f1{AssetFileID::gamepadOverlay, gpImageCoords({{4, 2}, {2, 1}}), {1, 2}},
		f3{AssetFileID::gamepadOverlay, gpImageCoords({{6, 2}, {2, 1}}), {1, 2}},
		f5{AssetFileID::gamepadOverlay, gpImageCoords({{4, 3}, {2, 1}}), {1, 2}},
		f7{AssetFileID::gamepadOverlay, gpImageCoords({{6, 3}, {2, 1}}), {1, 2}};
	} virtualControllerAssets;

	if(key[0] == 0)
		return virtualControllerAssets.dpad;
	switch(C64Key(key[0]))
	{
		case C64Key::KeyboardF1: return virtualControllerAssets.f1;
		case C64Key::KeyboardF3: return virtualControllerAssets.f3;
		case C64Key::KeyboardF5: return virtualControllerAssets.f5;
		case C64Key::KeyboardF7: return virtualControllerAssets.f7;
		case C64Key::ToggleKB: return virtualControllerAssets.kb;
		case C64Key::SwapJSPorts: return virtualControllerAssets.swapJsPorts;
		default: return virtualControllerAssets.jsBtn;
	}
}

VController::KbMap C64System::vControllerKeyboardMap(VControllerKbMode mode)
{
	static constexpr VController::KbMap kbToEventMap =
	{
		KeyCode(C64Key::KeyboardQ), KeyCode(C64Key::KeyboardW), KeyCode(C64Key::KeyboardE), KeyCode(C64Key::KeyboardR), KeyCode(C64Key::KeyboardT), KeyCode(C64Key::KeyboardY), KeyCode(C64Key::KeyboardU), KeyCode(C64Key::KeyboardI), KeyCode(C64Key::KeyboardO), KeyCode(C64Key::KeyboardP),
		KeyCode(C64Key::KeyboardA), KeyCode(C64Key::KeyboardS), KeyCode(C64Key::KeyboardD), KeyCode(C64Key::KeyboardF), KeyCode(C64Key::KeyboardG), KeyCode(C64Key::KeyboardH), KeyCode(C64Key::KeyboardJ), KeyCode(C64Key::KeyboardK), KeyCode(C64Key::KeyboardL), 0,
		KeyCode(C64Key::KeyboardShiftLock), KeyCode(C64Key::KeyboardZ), KeyCode(C64Key::KeyboardX), KeyCode(C64Key::KeyboardC), KeyCode(C64Key::KeyboardV), KeyCode(C64Key::KeyboardB), KeyCode(C64Key::KeyboardN), KeyCode(C64Key::KeyboardM), KeyCode(C64Key::KeyboardInstDel), 0,
		0, 0, 0, KeyCode(C64Key::KeyboardSpace), KeyCode(C64Key::KeyboardSpace), KeyCode(C64Key::KeyboardSpace), KeyCode(C64Key::KeyboardSpace), KeyCode(C64Key::KeyboardRunStop), KeyCode(C64Key::KeyboardRunStop), KeyCode(C64Key::KeyboardReturn)
	};

	static constexpr VController::KbMap kbToEventMap2 =
	{
		KeyCode(C64Key::KeyboardF1), KeyCode(C64Key::KeyboardF3), KeyCode(C64Key::KeyboardF5), KeyCode(C64Key::KeyboardF7), KeyCode(C64Key::KeyboardAt), KeyCode(C64Key::KeyboardCommodore), KeyCode(C64Key::KeyboardLeftArrow), KeyCode(C64Key::KeyboardUpArrow), KeyCode(C64Key::KeyboardPlus), KeyCode(C64Key::KeyboardMinus),
		KeyCode(C64Key::Keyboard1), KeyCode(C64Key::Keyboard2), KeyCode(C64Key::Keyboard3), KeyCode(C64Key::Keyboard4), KeyCode(C64Key::Keyboard5), KeyCode(C64Key::Keyboard6), KeyCode(C64Key::Keyboard7), KeyCode(C64Key::Keyboard8), KeyCode(C64Key::Keyboard9), KeyCode(C64Key::Keyboard0),
		KeyCode(C64Key::KeyboardRestore), KeyCode(C64Key::KeyboardColon), KeyCode(C64Key::KeyboardSemiColon), KeyCode(C64Key::KeyboardEquals), KeyCode(C64Key::KeyboardComma), KeyCode(C64Key::KeyboardPeriod), KeyCode(C64Key::KeyboardSlash), KeyCode(C64Key::KeyboardAsterisk), KeyCode(C64Key::KeyboardClrHome), 0,
		0, 0, 0, KeyCode(C64Key::KeyboardSpace), KeyCode(C64Key::KeyboardSpace), KeyCode(C64Key::KeyboardSpace), KeyCode(C64Key::KeyboardSpace), KeyCode(C64Key::KeyboardCtrlLock), KeyCode(C64Key::KeyboardCtrlLock), KeyCode(C64Key::KeyboardReturn)
	};

	return mode == VControllerKbMode::LAYOUT_2 ? kbToEventMap2 : kbToEventMap;
}

static KeyCode shiftKeycodePositional(C64Key keycode)
{
	switch(keycode)
	{
		case C64Key::KeyboardColon: return KeyCode(C64Key::KeyboardBracketLeft);
		case C64Key::KeyboardSemiColon: return KeyCode(C64Key::KeyboardBracketRight);
		case C64Key::KeyboardComma: return KeyCode(C64Key::KeyboardLess);
		case C64Key::KeyboardPeriod: return KeyCode(C64Key::KeyboardGreater);
		case C64Key::KeyboardSlash: return KeyCode(C64Key::KeyboardQuestion);
		case C64Key::Keyboard1: return KeyCode(C64Key::KeyboardExclam);
		case C64Key::Keyboard2: return KeyCode(C64Key::KeyboardQuoteDbl);
		case C64Key::Keyboard3: return KeyCode(C64Key::KeyboardNumberSign);
		case C64Key::Keyboard4: return KeyCode(C64Key::KeyboardDollar);
		case C64Key::Keyboard5: return KeyCode(C64Key::KeyboardPercent);
		case C64Key::Keyboard6: return KeyCode(C64Key::KeyboardAmpersand);
		case C64Key::Keyboard7: return KeyCode(C64Key::KeyboardApostrophe);
		case C64Key::Keyboard8: return KeyCode(C64Key::KeyboardParenLeft);
		case C64Key::Keyboard9: return KeyCode(C64Key::KeyboardParenRight);
		default: return KeyCode(keycode);
	}
}

void C64System::handleKeyboardInput(InputAction a, bool positionalShift)
{
	int mod{};
	if(a.metaState & Input::Meta::SHIFT)
	{
		mod |= KBD_MOD_LSHIFT;
		if(positionalShift)
			a.code = shiftKeycodePositional(C64Key(a.code));
	}
	if(a.metaState & Input::Meta::CAPS_LOCK)
	{
		mod |= KBD_MOD_SHIFTLOCK;
	}
	plugin.keyboard_key_pressed_direct(a.code, mod, a.isPushed());
}

void C64System::handleInputAction(EmuApp *app, InputAction a)
{
	bool positionalShift{};
	if(app)
	{
		if(app->defaultVController().keyboard().shiftIsActive())
		{
			a.metaState |= Input::Meta::SHIFT;
			positionalShift = true;
		}
	}
	auto key = C64Key(a.code);
	switch(key)
	{
		case C64Key::Up ... C64Key::JSTrigger:
		{
			if(effectiveJoystickMode == JoystickMode::Keyboard)
			{
				if(key == C64Key::Right)
					handleKeyboardInput({KeyCode(C64Key::KeyboardRight), {}, a.state, a.metaState}, positionalShift);
				else if(key == C64Key::Left)
					handleKeyboardInput({KeyCode(C64Key::KeyboardLeft), {}, a.state, a.metaState}, positionalShift);
				else if(key == C64Key::Up)
					handleKeyboardInput({KeyCode(C64Key::KeyboardUp), {}, a.state, a.metaState}, positionalShift);
				else if(key == C64Key::Down)
					handleKeyboardInput({KeyCode(C64Key::KeyboardDown), {}, a.state, a.metaState}, positionalShift);
				else if(key == C64Key::JSTrigger)
					handleKeyboardInput({KeyCode(C64Key::KeyboardPound), {}, a.state, a.metaState}, positionalShift);
			}
			else
			{
				auto &joystick_value = *plugin.joystick_value;
				auto player = a.flags.deviceId;
				if(effectiveJoystickMode == JoystickMode::Port2)
				{
					player = (player == 1) ? 0 : 1;
				}
				auto jsBits = [&] -> uint16_t
				{
					static constexpr uint16_t JS_FIRE = 0x10,
						JS_E = 0x08,
						JS_W = 0x04,
						JS_S = 0x02,
						JS_N = 0x01;
					switch(key)
					{
						case C64Key::Up: return JS_N;
						case C64Key::Right: return JS_E;
						case C64Key::Down: return JS_S;
						case C64Key::Left: return JS_W;
						case C64Key::JSTrigger: return JS_FIRE;
						default: bug_unreachable();
					}
				}();
				joystick_value[player] = IG::setOrClearBits(joystick_value[player], jsBits, a.isPushed());
			}
			break;
		}
		case C64Key::SwapJSPorts:
		{
			if(a.isPushed() && effectiveJoystickMode != JoystickMode::Keyboard)
			{
				EmuSystem::sessionOptionSet();
				if(effectiveJoystickMode == JoystickMode::Port2)
					joystickMode = JoystickMode::Port1;
				else
					joystickMode = JoystickMode::Port2;
				effectiveJoystickMode = joystickMode;
				IG::fill(*plugin.joystick_value);
				if(app)
					app->postMessage(1, false, "Swapped Joystick Ports");
			}
			break;
		}
		case C64Key::ToggleKB:
		{
			if(app && a.state == Input::Action::PUSHED)
				app->inputManager.toggleKeyboard();
			break;
		}
		case C64Key::KeyboardRestore:
		{
			if(app)
			{
				log.info("pushed restore key");
				auto emuThreadResumer = app->suspendEmulationThread();
				plugin.machine_set_restore_key(a.state == Input::Action::PUSHED);
			}
			break;
		}
		case C64Key::KeyboardCtrlLock:
		{
			if(a.isPushed())
			{
				ctrlLock ^= true;
				handleKeyboardInput({KeyCode(C64Key::KeyboardCtrl), {}, ctrlLock ? Input::Action::PUSHED : Input::Action::RELEASED});
			}
			break;
		}
		case C64Key::KeyboardShiftLock:
		{
			if(app && a.isPushed())
			{
				bool active = app->defaultVController().keyboard().toggleShiftActive();
				//log.debug("positional shift:{}", active);
				handleKeyboardInput({KeyCode(C64Key::KeyboardLeftShift), {}, active ? Input::Action::PUSHED : Input::Action::RELEASED});
			}
			break;
		}
		default:
		{
			handleKeyboardInput({a.code, {}, a.state, a.metaState}, positionalShift);
			break;
		}
	}
}

void C64System::clearInputBuffers(EmuInputView &inputView)
{
	ctrlLock = false;
	auto &joystick_value = *plugin.joystick_value;
	IG::fill(joystick_value);
	plugin.keyboard_key_clear();
}

void C64System::onVKeyboardShown(VControllerKeyboard &kb, bool shown)
{
	if(!shown)
	{
		if(ctrlLock)
		{
			ctrlLock = false;
			handleKeyboardInput({KeyCode(C64Key::KeyboardCtrl), {}, Input::Action::RELEASED});
		}
		if(kb.shiftIsActive())
		{
			kb.setShiftActive(false);
			handleKeyboardInput({KeyCode(C64Key::KeyboardLeftShift), {}, Input::Action::RELEASED});
		}
	}
}

void C64System::setJoystickMode(JoystickMode mode)
{
	joystickMode = mode;
	effectiveJoystickMode = mode == JoystickMode::Auto ? defaultJoystickMode : mode;
	updateJoystickDevices();
}

void C64System::updateJoystickDevices()
{
	enterCPUTrap();
	if(effectiveJoystickMode == JoystickMode::Keyboard)
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

SystemInputDeviceDesc C64System::inputDeviceDesc(int idx) const
{
	static constexpr std::array jsComponents
	{
		InputComponentDesc{"D-Pad", dpadKeyInfo, InputComponent::dPad, LB2DO},
		InputComponentDesc{"Joystick Button", triggerKeyInfo, InputComponent::button, RB2DO},
		InputComponentDesc{"F1 & Keyboard Toggle", shortcutKeyInfo, InputComponent::button, RB2DO, {.rowSize = 1}},
	};

	static constexpr SystemInputDeviceDesc jsDesc{"Joystick", jsComponents};

	return jsDesc;
}

}

signed long kbd_arch_keyname_to_keynum(char *keynamePtr)
{
	using namespace EmuEx;
	//log.debug("kbd_arch_keyname_to_keynum({})", keyname);
	std::string_view keyname{keynamePtr};
	if(keyname == "F1") { return long(C64Key::KeyboardF1); }
	else if(keyname == "F2") { return long(C64Key::KeyboardF2); }
	else if(keyname == "F3") { return long(C64Key::KeyboardF3); }
	else if(keyname == "F4") { return long(C64Key::KeyboardF4); }
	else if(keyname == "F5") { return long(C64Key::KeyboardF5); }
	else if(keyname == "F6") { return long(C64Key::KeyboardF6); }
	else if(keyname == "F7") { return long(C64Key::KeyboardF7); }
	else if(keyname == "F8") { return long(C64Key::KeyboardF8); }
	else if(keyname == "End") { return long(C64Key::KeyboardLeftArrow); }
	else if(keyname == "1") { return long(C64Key::Keyboard1); }
	else if(keyname == "2") { return long(C64Key::Keyboard2); }
	else if(keyname == "3") { return long(C64Key::Keyboard3); }
	else if(keyname == "4") { return long(C64Key::Keyboard4); }
	else if(keyname == "5") { return long(C64Key::Keyboard5); }
	else if(keyname == "6") { return long(C64Key::Keyboard6); }
	else if(keyname == "7") { return long(C64Key::Keyboard7); }
	else if(keyname == "8") { return long(C64Key::Keyboard8); }
	else if(keyname == "9") { return long(C64Key::Keyboard9); }
	else if(keyname == "0") { return long(C64Key::Keyboard0); }
	else if(keyname == "plus") { return long(C64Key::KeyboardPlus); }
	else if(keyname == "minus") { return long(C64Key::KeyboardMinus); }
	else if(keyname == "sterling") { return long(C64Key::KeyboardPound); }
	else if(keyname == "Home") { return long(C64Key::KeyboardClrHome); }
	else if(keyname == "BackSpace") { return long(C64Key::KeyboardInstDel); }
	else if(keyname == "Control_L") { return long(C64Key::KeyboardCtrl); }
	else if(keyname == "q") { return long(C64Key::KeyboardQ); }
	else if(keyname == "w") { return long(C64Key::KeyboardW); }
	else if(keyname == "e") { return long(C64Key::KeyboardE); }
	else if(keyname == "r") { return long(C64Key::KeyboardR); }
	else if(keyname == "t") { return long(C64Key::KeyboardT); }
	else if(keyname == "y") { return long(C64Key::KeyboardY); }
	else if(keyname == "u") { return long(C64Key::KeyboardU); }
	else if(keyname == "i") { return long(C64Key::KeyboardI); }
	else if(keyname == "o") { return long(C64Key::KeyboardO); }
	else if(keyname == "p") { return long(C64Key::KeyboardP); }
	else if(keyname == "at") { return long(C64Key::KeyboardAt); }
	else if(keyname == "asterisk") { return long(C64Key::KeyboardAsterisk); }
	else if(keyname == "Page_Down") { return long(C64Key::KeyboardUpArrow); }
	else if(keyname == "Escape") { return long(C64Key::KeyboardRunStop); }
	else if(keyname == "a") { return long(C64Key::KeyboardA); }
	else if(keyname == "s") { return long(C64Key::KeyboardS); }
	else if(keyname == "d") { return long(C64Key::KeyboardD); }
	else if(keyname == "f") { return long(C64Key::KeyboardF); }
	else if(keyname == "g") { return long(C64Key::KeyboardG); }
	else if(keyname == "h") { return long(C64Key::KeyboardH); }
	else if(keyname == "j") { return long(C64Key::KeyboardJ); }
	else if(keyname == "k") { return long(C64Key::KeyboardK); }
	else if(keyname == "l") { return long(C64Key::KeyboardL); }
	else if(keyname == "colon") { return long(C64Key::KeyboardColon); }
	else if(keyname == "semicolon") { return long(C64Key::KeyboardSemiColon); }
	else if(keyname == "equal") { return long(C64Key::KeyboardEquals); }
	else if(keyname == "Return") { return long(C64Key::KeyboardReturn); }
	else if(keyname == "Tab") { return long(C64Key::KeyboardCommodore); }
	else if(keyname == "Shift_L") { return long(C64Key::KeyboardLeftShift); }
	else if(keyname == "z") { return long(C64Key::KeyboardZ); }
	else if(keyname == "x") { return long(C64Key::KeyboardX); }
	else if(keyname == "c") { return long(C64Key::KeyboardC); }
	else if(keyname == "v") { return long(C64Key::KeyboardV); }
	else if(keyname == "b") { return long(C64Key::KeyboardB); }
	else if(keyname == "n") { return long(C64Key::KeyboardN); }
	else if(keyname == "m") { return long(C64Key::KeyboardM); }
	else if(keyname == "comma") { return long(C64Key::KeyboardComma); }
	else if(keyname == "period") { return long(C64Key::KeyboardPeriod); }
	else if(keyname == "slash") { return long(C64Key::KeyboardSlash); }
	else if(keyname == "Shift_R") { return long(C64Key::KeyboardRightShift); }
	else if(keyname == "Up") { return long(C64Key::KeyboardUp); }
	else if(keyname == "Right") { return long(C64Key::KeyboardRight); }
	else if(keyname == "Down") { return long(C64Key::KeyboardDown); }
	else if(keyname == "Left") { return long(C64Key::KeyboardLeft); }
	else if(keyname == "space") { return long(C64Key::KeyboardSpace); }
	else if(keyname == "exclam") { return long(C64Key::KeyboardExclam); }
	else if(keyname == "quotedbl") { return long(C64Key::KeyboardQuoteDbl); }
	else if(keyname == "numbersign") { return long(C64Key::KeyboardNumberSign); }
	else if(keyname == "dollar") { return long(C64Key::KeyboardDollar); }
	else if(keyname == "percent") { return long(C64Key::KeyboardPercent); }
	else if(keyname == "ampersand") { return long(C64Key::KeyboardAmpersand); }
	else if(keyname == "parenleft") { return long(C64Key::KeyboardParenLeft); }
	else if(keyname == "parenright") { return long(C64Key::KeyboardParenRight); }
	else if(keyname == "bracketleft") { return long(C64Key::KeyboardBracketLeft); }
	else if(keyname == "bracketright") { return long(C64Key::KeyboardBracketRight); }
	else if(keyname == "less") { return long(C64Key::KeyboardLess); }
	else if(keyname == "greater") { return long(C64Key::KeyboardGreater); }
	else if(keyname == "question") { return long(C64Key::KeyboardQuestion); }
	else if(keyname == "apostrophe") { return long(C64Key::KeyboardApostrophe); }
	else if(keyname == "Caps_Lock") { return long(C64Key::KeyboardShiftLock); }
	//logWarn("unknown keyname:%s", keyname.data());
	return 0;
}
