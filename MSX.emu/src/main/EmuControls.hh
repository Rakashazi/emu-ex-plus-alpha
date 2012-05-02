#pragma once

#include <util/preprocessor/repeat.h>

namespace EmuControls
{

static const uint categories = 4;
static const uint gamepadKeys = 12;
static const uint colecoKeys = 12;
static const uint kbKeys = 93;
static const uint systemTotalKeys = gameActionKeys + gamepadKeys + colecoKeys + kbKeys;

static const char *gamepadName[gamepadKeys] =
{
		"Up",
		"Right",
		"Down",
		"Left",
		"Left+Up",
		"Right+Up",
		"Right+Down",
		"Left+Down",
		"A",
		"B",
		"Turbo A",
		"Turbo B",
};

static const char *colecoName[colecoKeys] =
{
		"0",
		"1",
		"2",
		"3",
		"4",
		"5",
		"6",
		"7",
		"8",
		"9",
		"*",
		"#",
};

static const char *kbName[kbKeys] =
{
		"Toggle Keyboard",

		"F1",
		"F2",
		"F3",
		"F4",
		"F5",
		"Stop",
		"Cls",
		"Select",
		"Ins",
		"Del",

		"Esc",
		"1  !",
		"2  \"",
		"3  #",
		"4  $",
		"5  %",
		"6  &",
		"7  '",
		"8  (",
		"9  )",
		"0",
		"-  =",
		"^  ~",
		"\\  |",
		"Backspace",

		"Tab",
		"Q",
		"W",
		"E",
		"R",
		"T",
		"Y",
		"U",
		"I",
		"O",
		"P",
		"@  `",
		"[  {",
		"Return",

		"Ctrl",
		"A",
		"S",
		"D",
		"F",
		"G",
		"H",
		"J",
		"K",
		"L",
		";  +",
		":  *",
		"]  }",

		"Left Shift",
		"Z",
		"X",
		"C",
		"V",
		"B",
		"N",
		"M",
		",  <",
		".  >",
		"/  ?",
		"_",
		"Right Shift",

		"Caps",
		"Graph",
		"Cancel",
		"Space",
		"Execute",
		"Code",
		"Pause",

		"Left Arrow",
		"Up Arrow",
		"Down Arrow",
		"Right Arrow",

		"Num 7",
		"Num 8",
		"Num 9",
		"Num Div",
		"Num 4",
		"Num 5",
		"Num 6",
		"Num Mult",
		"Num 1",
		"Num 2",
		"Num 3",
		"Num Sub",
		"Num 0",
		"Num Comma",
		"Num Period",
		"Num Add",
};

typedef uint KeyArray[systemTotalKeys];

// Key Input/Keyboard

#ifdef INPUT_SUPPORTS_KEYBOARD
static const KeyArray webOSKB =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_WEBOS_KB_PROFILE_INIT,

		EMU_CONTROLS_WEBOS_KB_8WAY_DIRECTION_PROFILE_INIT,
		input_asciiKey(','),
		input_asciiKey('m'),
		0,
		0,

		// start Coleco
		PP_ZERO_LIST(12)

		// start MSX keyboard
		input_asciiKey(' '),
};

static const KeyArray genericKB =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_PROFILE_INIT,

		Input::Key::UP,
		Input::Key::RIGHT,
		Input::Key::DOWN,
		Input::Key::LEFT,
		0,
		0,
		0,
		0,
		input_asciiKey('c'),
		input_asciiKey('x'),
		0,
		0,

		// start Coleco
		input_asciiKey('p'),
		input_asciiKey('q'),
		input_asciiKey('w'),
		input_asciiKey('e'),
		input_asciiKey('r'),
		input_asciiKey('t'),
		input_asciiKey('y'),
		input_asciiKey('u'),
		input_asciiKey('i'),
		input_asciiKey('o'),
		input_asciiKey('k'),
		input_asciiKey('d'),

		// start MSX keyboard
		0,
		input_asciiKey('t'), // F1 ... F5
		input_asciiKey('y'),
		input_asciiKey('u'),
		input_asciiKey('i'),
		input_asciiKey('o'),
		PP_ZERO_LIST(6) // 6 - 11
		input_asciiKey('g'), // 1 ... 0
		input_asciiKey('h'),
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		PP_ZERO_LIST(3) // 22 - 24
		Input::Key::BACK_SPACE,
		PP_ZERO_LIST(11) // 26 - 36
		0, // @
		0,
		Input::Key::ENTER,
		0, // CTRL
		PP_ZERO_LIST(12) // 41 ... 52
		0, // Left Shift
		PP_ZERO_LIST(7) // 54 ... 60
		0, // ,
		0, // .
		0, // /
		0,
		0, // Right Shift
		PP_ZERO_LIST(3) // 66 - 68
		input_asciiKey(' '),
		PP_ZERO_LIST(23) // 70 - 92
};

static const KeyArray genericKBTyping =
{
		0,
		Input::Key::MENU,
		0,
		0,
		Input::Key::SEARCH,
		0,
		0,

		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,

		// start Coleco
		PP_ZERO_LIST(12)

		// start MSX keyboard
		0,
		Input::Key::F1, // F1 ... F5
		Input::Key::F2,
		Input::Key::F3,
		Input::Key::F4,
		Input::Key::F5,

		Input::Key::SCROLL_LOCK, // STOP - DEL
		Input::Key::END,
		Input::Key::HOME,
		Input::Key::INSERT,
		Input::Key::DELETE,

		Input::Key::ESCAPE,

		input_asciiKey('1'), // 1 ... 0
		input_asciiKey('2'),
		input_asciiKey('3'),
		input_asciiKey('4'),
		input_asciiKey('5'),
		input_asciiKey('6'),
		input_asciiKey('7'),
		input_asciiKey('8'),
		input_asciiKey('9'),
		input_asciiKey('0'),
		input_asciiKey('-'),
		input_asciiKey('='),
		input_asciiKey('\\'),
		Input::Key::BACK_SPACE,

		Input::Key::TAB,
		input_asciiKey('q'),
		input_asciiKey('w'),
		input_asciiKey('e'),
		input_asciiKey('r'),
		input_asciiKey('t'),
		input_asciiKey('y'),
		input_asciiKey('u'),
		input_asciiKey('i'),
		input_asciiKey('o'),
		input_asciiKey('p'),
		input_asciiKey('`'), // @
		input_asciiKey('['),
		Input::Key::ENTER,

		Input::Key::LCTRL, // CTRL
		input_asciiKey('a'),
		input_asciiKey('s'),
		input_asciiKey('d'),
		input_asciiKey('f'),
		input_asciiKey('g'),
		input_asciiKey('h'),
		input_asciiKey('j'),
		input_asciiKey('k'),
		input_asciiKey('l'),
		input_asciiKey(';'),
		input_asciiKey('\''),
		input_asciiKey(']'),

		Input::Key::LSHIFT, // Left Shift
		input_asciiKey('z'),
		input_asciiKey('x'),
		input_asciiKey('c'),
		input_asciiKey('v'),
		input_asciiKey('b'),
		input_asciiKey('n'),
		input_asciiKey('m'),
		input_asciiKey(','),
		input_asciiKey('.'),
		input_asciiKey('/'),
		Input::Key::RCTRL,
		Input::Key::RSHIFT, // Right Shift

		Input::Key::CAPS,
		Input::Key::LMETA,
		Input::Key::LALT,
		input_asciiKey(' '),
		Input::Key::RMETA,
		Input::Key::RALT,
		Input::Key::PAUSE,

		Input::Key::LEFT,
		Input::Key::UP,
		Input::Key::DOWN,
		Input::Key::RIGHT,
		PP_ZERO_LIST(15) // 77 - 92
};

#ifdef CONFIG_BASE_ANDROID
static const KeyArray androidNav =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

		Input::Key::UP,
		Input::Key::RIGHT,
		Input::Key::DOWN,
		Input::Key::LEFT,
		0,
		0,
		0,
		0,
		input_asciiKey('c'),
		input_asciiKey('x'),
		0,
		0,

		// start Coleco
		input_asciiKey('p'),
		input_asciiKey('q'),
		input_asciiKey('w'),
		input_asciiKey('e'),
		input_asciiKey('r'),
		input_asciiKey('t'),
		input_asciiKey('y'),
		input_asciiKey('u'),
		input_asciiKey('i'),
		input_asciiKey('o'),
		input_asciiKey('k'),
		input_asciiKey('d'),

		// start MSX keyboard
		0,
		input_asciiKey('t'), // F1 ... F5
		input_asciiKey('y'),
		input_asciiKey('u'),
		input_asciiKey('i'),
		input_asciiKey('o'),
		PP_ZERO_LIST(6) // 6 - 11
		input_asciiKey('g'), // 1 ... 0
		input_asciiKey('h'),
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		PP_ZERO_LIST(3) // 22 - 24
		Input::Key::BACK_SPACE,
		PP_ZERO_LIST(11) // 26 - 36
		0, // @
		0,
		Input::Key::ENTER,
		0, // CTRL
		PP_ZERO_LIST(12) // 41 ... 52
		0, // Left Shift
		PP_ZERO_LIST(7) // 54 ... 60
		0, // ,
		0, // .
		0, // /
		0,
		0, // Right Shift
		PP_ZERO_LIST(3) // 66 - 68
		input_asciiKey(' '),
		PP_ZERO_LIST(23) // 70 - 92
};

static const KeyArray xperiaPlay =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_NO_BACK_PROFILE_INIT,

		Input::Key::UP,
		Input::Key::RIGHT,
		Input::Key::DOWN,
		Input::Key::LEFT,
		0,
		0,
		0,
		0,
		Input::Key::CENTER,
		Input::Key::ESCAPE,
		0,
		0,

		// start Coleco
		PP_ZERO_LIST(10)
		Input::Key::GAME_Y,
		Input::Key::GAME_X,

		// start MSX keyboard
		Input::Key::GAME_START,
		Input::Key::GAME_Y, // F1 ... F5
		Input::Key::GAME_L1,
		Input::Key::GAME_R1,
		Input::Key::GAME_X,
		Input::Key::GAME_SELECT,
		PP_ZERO_LIST(87) // 26 - 92
};
#endif

static const ConstKeyProfile kb[] =
{
#ifdef CONFIG_ENV_WEBOS
	{ "WebOS Keyboard (Game)", webOSKB },
#endif
#ifdef CONFIG_BASE_ANDROID
	{ "Android Nav + Keyboard (Game)", androidNav },
	{ "Xperia Play", xperiaPlay },
#endif
	{ "Standard Keyboard (Game)", genericKB },
	{ "Standard Keyboard (Typing)", genericKBTyping },
};

#endif

// Wiimote

static const KeyArray wiimote =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_WIIMOTE_PROFILE_INIT,

		Input::Wiimote::UP,
		Input::Wiimote::RIGHT,
		Input::Wiimote::DOWN,
		Input::Wiimote::LEFT,
		0,
		0,
		0,
		0,
		Input::Wiimote::_2,
		Input::Wiimote::_1,
		0,
		0,

		// start Coleco
		PP_ZERO_LIST(11)
		Input::Wiimote::MINUS,

		Input::Wiimote::PLUS,
		PP_ZERO_LIST(92) // 1 - 92
};

static const KeyArray wiiCC =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_WII_CC_PROFILE_INIT,

	Input::Wiimote::UP,
	Input::Wiimote::RIGHT,
	Input::Wiimote::DOWN,
	Input::Wiimote::LEFT,
	0,
	0,
	0,
	0,
	Input::Wiimote::B,
	Input::Wiimote::Y,
	0,
	0,

	// start Coleco
	PP_ZERO_LIST(11)
	Input::Wiimote::MINUS,

	Input::Wiimote::PLUS,
	Input::Wiimote::X, // F1 - F5
	Input::Wiimote::L,
	Input::Wiimote::R,
	Input::Wiimote::A,
	Input::Wiimote::MINUS,
};

static const ConstKeyProfile wii[] =
{
		{ "Default Wiimote", wiimote },
		{ "Default Classic Controller", wiiCC }
};

// iControlPad

static const KeyArray iCPDefault =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_ICP_NUBS_PROFILE_INIT,

		Input::iControlPad::UP,
		Input::iControlPad::RIGHT,
		Input::iControlPad::DOWN,
		Input::iControlPad::LEFT,
		0,
		0,
		0,
		0,
		Input::iControlPad::X,
		Input::iControlPad::A,
		0,
		0,

		// start Coleco
		PP_ZERO_LIST(12)

		// start MSX keyboard
		Input::iControlPad::START,
		Input::iControlPad::Y, // F1 - F5
		Input::iControlPad::L,
		Input::iControlPad::R,
		Input::iControlPad::B,
		Input::iControlPad::SELECT,
		PP_ZERO_LIST(87) // 6 - 92
};

static const ConstKeyProfile iCP[] =
{
		{ "Default iControlPad", iCPDefault }
};

// iCade

static const KeyArray iCadeDefault =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_UNBINDED_PROFILE_INIT,

		Input::ICade::UP,
		Input::ICade::RIGHT,
		Input::ICade::DOWN,
		Input::ICade::LEFT,
		0,
		0,
		0,
		0,
		Input::ICade::H,
		Input::ICade::F,
		0,
		0,

		// start Coleco
		PP_ZERO_LIST(12)

		// start MSX keyboard
		Input::ICade::C,
		Input::ICade::E, // F1 - F5
		Input::ICade::B,
		Input::ICade::D,
		Input::ICade::G,
		Input::ICade::A,
		PP_ZERO_LIST(87) // 6 - 92
};

static const ConstKeyProfile iCade[] =
{
		{ "Default iCade", iCadeDefault }
};

// Zeemote

static const KeyArray zeemoteDefaults =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_UNBINDED_PROFILE_INIT,

		Input::Zeemote::UP,
		Input::Zeemote::RIGHT,
		Input::Zeemote::DOWN,
		Input::Zeemote::LEFT,
		0,
		0,
		0,
		0,
		Input::Zeemote::B,
		Input::Zeemote::A,
		0,
		0,

		// start Coleco
		PP_ZERO_LIST(12)
		Input::Zeemote::C,
		PP_ZERO_LIST(92) // 1 - 92
};

static const ConstKeyProfile zeemote[] =
{
		{ "Default Zeemote JS1", zeemoteDefaults }
};

}

