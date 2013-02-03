#include <util/preprocessor/repeat.h>
#include <EmuInput.hh>

namespace EmuControls
{

void transposeKeysForPlayer(KeyConfig::KeyArray &key, uint player)
{
	generic2PlayerTranspose(key, player, 1);
	generic2PlayerTranspose(key, player, 3);
}

static const char *gamepadName[joystickKeys] =
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

static const char *colecoName[colecoNumericKeys] =
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

static const char *keyboardName[kbKeys] =
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

static const uint msxJoystickKeyOffset = gameActionKeys;
static const uint msxJoystick2KeyOffset = msxJoystickKeyOffset + joystickKeys;
static const uint colecoJoystickKeyOffset = msxJoystick2KeyOffset + joystickKeys;
static const uint colecoJoystick2KeyOffset = colecoJoystickKeyOffset + colecoNumericKeys;
static const uint keyboardKeyOffset = colecoJoystick2KeyOffset + colecoNumericKeys;

const KeyCategory category[categories]
{
		EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
		KeyCategory("Joystick", gamepadName, msxJoystickKeyOffset),
		KeyCategory("Joystick 2", gamepadName, msxJoystick2KeyOffset, 1),
		KeyCategory("Coleco Numeric Pad", colecoName, colecoJoystickKeyOffset),
		KeyCategory("Coleco Numeric Pad 2", colecoName, colecoJoystick2KeyOffset, 1),
		KeyCategory("Keyboard", keyboardName, keyboardKeyOffset),
};

#ifdef INPUT_SUPPORTS_KEYBOARD

const KeyConfig defaultKeyProfile[] =
{
#ifdef CONFIG_ENV_WEBOS
	{
			Input::Event::MAP_KEYBOARD,
			"WebOS Keyboard (Joystick use)",
			{
					EMU_CONTROLS_IN_GAME_ACTIONS_WEBOS_KB_PROFILE_INIT,

					// JS 1
					EMU_CONTROLS_WEBOS_KB_8WAY_DIRECTION_PROFILE_INIT,
					asciiKey(','),
					asciiKey('m'),
					0,
					0,

					// JS 2
					PP_ZERO_LIST(12)

					// Coleco 1 & 2
					PP_ZERO_LIST(24)

					// Keyboard
					asciiKey(' '),
			}
	},
#endif
#ifdef CONFIG_BASE_ANDROID
	{
			Input::Event::MAP_KEYBOARD,
			"Android Nav + Keyboard (Joystick use)",
			{
					EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

					// JS 1
					Input::Keycode::UP,
					Input::Keycode::RIGHT,
					Input::Keycode::DOWN,
					Input::Keycode::LEFT,
					0,
					0,
					0,
					0,
					asciiKey('c'),
					asciiKey('x'),
					0,
					0,

					// JS 2
					PP_ZERO_LIST(12)

					// Coleco 1
					asciiKey('p'),
					asciiKey('q'),
					asciiKey('w'),
					asciiKey('e'),
					asciiKey('r'),
					asciiKey('t'),
					asciiKey('y'),
					asciiKey('u'),
					asciiKey('i'),
					asciiKey('o'),
					asciiKey('k'),
					asciiKey('d'),

					// Coleco 2
					PP_ZERO_LIST(12)

					// Keyboard
					0,
					asciiKey('t'), // F1 ... F5
					asciiKey('y'),
					asciiKey('u'),
					asciiKey('i'),
					asciiKey('o'),
					PP_ZERO_LIST(6) // 6 - 11
					asciiKey('g'), // 1 ... 0
					asciiKey('h'),
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					PP_ZERO_LIST(3) // 22 - 24
					Input::Keycode::BACK_SPACE,
					PP_ZERO_LIST(11) // 26 - 36
					0, // @
					0,
					Input::Keycode::ENTER,
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
					asciiKey(' '),
					PP_ZERO_LIST(23) // 70 - 92
			}
	},
	{
			Input::Event::MAP_KEYBOARD,
			"Xperia Play",
			{
					EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

					// JS 1
					Input::Keycode::UP,
					Input::Keycode::RIGHT,
					Input::Keycode::DOWN,
					Input::Keycode::LEFT,
					0,
					0,
					0,
					0,
					Input::Keycode::CENTER,
					Input::Keycode::GAME_B,
					0,
					0,

					// JS 2
					PP_ZERO_LIST(12)

					// Coleco 1
					PP_ZERO_LIST(10)
					Input::Keycode::GAME_Y,
					Input::Keycode::GAME_X,

					// Coleco 2
					PP_ZERO_LIST(12)

					// Keyboard
					Input::Keycode::GAME_START,
					Input::Keycode::GAME_Y, // F1 ... F5
					Input::Keycode::GAME_L1,
					Input::Keycode::GAME_R1,
					Input::Keycode::GAME_X,
					Input::Keycode::GAME_SELECT,
					PP_ZERO_LIST(87) // 26 - 92
			}
	},
	{
			Input::Event::MAP_KEYBOARD,
			"PS3 Controller",
			{
					EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_PS3_GAMEPAD_PROFILE_INIT,

					// JS 1
					Input::Keycode::UP,
					Input::Keycode::RIGHT,
					Input::Keycode::DOWN,
					Input::Keycode::LEFT,
					0,
					0,
					0,
					0,
					Input::Keycode::GAME_X,
					Input::Keycode::GAME_Y,
					0,
					0,

					// JS 2
					PP_ZERO_LIST(12)

					// Coleco 1
					PP_ZERO_LIST(10)
					Input::Keycode::GAME_A,
					Input::Keycode::GAME_B,

					// Coleco 2
					PP_ZERO_LIST(12)

					// Keyboard
					Input::Keycode::GAME_START,
					Input::Keycode::GAME_B, // F1 ... F5
					Input::Keycode::GAME_L1,
					Input::Keycode::GAME_R1,
					Input::Keycode::GAME_A,
					Input::Keycode::GAME_SELECT,
			}
	},
#endif
	{
			Input::Event::MAP_KEYBOARD,
			"Default Keyboard (Joystick use)",
			{
					EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_PROFILE_INIT,

					// JS 1
					Input::Keycode::UP,
					Input::Keycode::RIGHT,
					Input::Keycode::DOWN,
					Input::Keycode::LEFT,
					0,
					0,
					0,
					0,
					asciiKey('c'),
					asciiKey('x'),
					0,
					0,

					// JS 2
					PP_ZERO_LIST(12)

					// Coleco 1
					asciiKey('p'),
					asciiKey('q'),
					asciiKey('w'),
					asciiKey('e'),
					asciiKey('r'),
					asciiKey('t'),
					asciiKey('y'),
					asciiKey('u'),
					asciiKey('i'),
					asciiKey('o'),
					asciiKey('k'),
					asciiKey('d'),

					// Coleco 2
					PP_ZERO_LIST(12)

					// Keyboard
					0,
					asciiKey('t'), // F1 ... F5
					asciiKey('y'),
					asciiKey('u'),
					asciiKey('i'),
					asciiKey('o'),
					PP_ZERO_LIST(6) // 6 - 11
					asciiKey('g'), // 1 ... 0
					asciiKey('h'),
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					PP_ZERO_LIST(3) // 22 - 24
					Input::Keycode::BACK_SPACE,
					PP_ZERO_LIST(11) // 26 - 36
					0, // @
					0,
					Input::Keycode::ENTER,
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
					asciiKey(' '),
					PP_ZERO_LIST(23) // 70 - 92
			}
	},
	{
			Input::Event::MAP_KEYBOARD,
			"Default Keyboard (Typing use)",
			{
					EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_MINIMAL_PROFILE_INIT,

					// JS 1 & 2
					PP_ZERO_LIST(24)

					// Coleco 1 & 2
					PP_ZERO_LIST(24)

					// Keyboard
					0,
					Input::Keycode::F1, // F1 ... F5
					Input::Keycode::F2,
					Input::Keycode::F3,
					Input::Keycode::F4,
					Input::Keycode::F5,

					Input::Keycode::SCROLL_LOCK, // STOP - DEL
					Input::Keycode::END,
					Input::Keycode::HOME,
					Input::Keycode::INSERT,
					Input::Keycode::DELETE,

					Input::Keycode::ESCAPE,

					asciiKey('1'), // 1 ... 0
					asciiKey('2'),
					asciiKey('3'),
					asciiKey('4'),
					asciiKey('5'),
					asciiKey('6'),
					asciiKey('7'),
					asciiKey('8'),
					asciiKey('9'),
					asciiKey('0'),
					asciiKey('-'),
					asciiKey('='),
					asciiKey('\\'),
					Input::Keycode::BACK_SPACE,

					Input::Keycode::TAB,
					asciiKey('q'),
					asciiKey('w'),
					asciiKey('e'),
					asciiKey('r'),
					asciiKey('t'),
					asciiKey('y'),
					asciiKey('u'),
					asciiKey('i'),
					asciiKey('o'),
					asciiKey('p'),
					asciiKey('`'), // @
					asciiKey('['),
					Input::Keycode::ENTER,

					Input::Keycode::LCTRL, // CTRL
					asciiKey('a'),
					asciiKey('s'),
					asciiKey('d'),
					asciiKey('f'),
					asciiKey('g'),
					asciiKey('h'),
					asciiKey('j'),
					asciiKey('k'),
					asciiKey('l'),
					asciiKey(';'),
					asciiKey('\''),
					asciiKey(']'),

					Input::Keycode::LSHIFT, // Left Shift
					asciiKey('z'),
					asciiKey('x'),
					asciiKey('c'),
					asciiKey('v'),
					asciiKey('b'),
					asciiKey('n'),
					asciiKey('m'),
					asciiKey(','),
					asciiKey('.'),
					asciiKey('/'),
					Input::Keycode::RCTRL,
					Input::Keycode::RSHIFT, // Right Shift

					Input::Keycode::CAPS,
					Input::Keycode::LMETA,
					Input::Keycode::LALT,
					asciiKey(' '),
					Input::Keycode::RMETA,
					Input::Keycode::RALT,
					Input::Keycode::PAUSE,

					Input::Keycode::LEFT,
					Input::Keycode::UP,
					Input::Keycode::DOWN,
					Input::Keycode::RIGHT,
					PP_ZERO_LIST(15) // 77 - 92
			}
	}
};

const uint defaultKeyProfiles = sizeofArray(defaultKeyProfile);

#endif

// Wiimote

const KeyConfig defaultWiimoteProfile[] =
{
	{
			Input::Event::MAP_WIIMOTE,
			"Default Wiimote",
			{
					EMU_CONTROLS_IN_GAME_ACTIONS_WIIMOTE_PROFILE_INIT,

					// JS 1
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

					// JS 2
					PP_ZERO_LIST(12)

					// Coleco 1
					PP_ZERO_LIST(11)
					Input::Wiimote::MINUS,

					// Coleco 2
					PP_ZERO_LIST(12)

					// Keyboard
					Input::Wiimote::PLUS,
			}
	},
	{
			Input::Event::MAP_WIIMOTE,
			"Default Classic Controller",
			{
					EMU_CONTROLS_IN_GAME_ACTIONS_WII_CC_PROFILE_INIT,

				// JS 1
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

				// JS 2
				PP_ZERO_LIST(12)

				// Coleco 1
				PP_ZERO_LIST(11)
				Input::Wiimote::MINUS,

				// Coleco 2
				PP_ZERO_LIST(12)

				// Keyboard
				Input::Wiimote::PLUS,
				Input::Wiimote::X, // F1 - F5
				Input::Wiimote::L,
				Input::Wiimote::R,
				Input::Wiimote::A,
				Input::Wiimote::MINUS,
			}
	},
};

const uint defaultWiimoteProfiles = sizeofArray(defaultWiimoteProfile);

// iControlPad

const KeyConfig defaultIControlPadProfile[] =
{
	{
			Input::Event::MAP_ICONTROLPAD,
			"Default iControlPad",
			{
					EMU_CONTROLS_IN_GAME_ACTIONS_ICP_NUBS_PROFILE_INIT,

					// JS 1
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

					// JS 2
					PP_ZERO_LIST(12)

					// Coleco 1 & 2
					PP_ZERO_LIST(24)

					// Keyboard
					Input::iControlPad::START,
					Input::iControlPad::Y, // F1 - F5
					Input::iControlPad::L,
					Input::iControlPad::R,
					Input::iControlPad::B,
					Input::iControlPad::SELECT,
					PP_ZERO_LIST(87) // 6 - 92
			}
	},
};

const uint defaultIControlPadProfiles = sizeofArray(defaultIControlPadProfile);

// iCade

const KeyConfig defaultICadeProfile[] =
{
	{
			Input::Event::MAP_ICADE,
			"Default iCade",
			{
					EMU_CONTROLS_IN_GAME_ACTIONS_UNBINDED_PROFILE_INIT,

					// JS 1
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

					// JS 2
					PP_ZERO_LIST(12)

					// Coleco 1 & 2
					PP_ZERO_LIST(24)

					// Keyboard
					Input::ICade::C,
					Input::ICade::E, // F1 - F5
					Input::ICade::B,
					Input::ICade::D,
					Input::ICade::G,
					Input::ICade::A,
					PP_ZERO_LIST(87) // 6 - 92
			}
	},
};

const uint defaultICadeProfiles = sizeofArray(defaultICadeProfile);

// Zeemote

const KeyConfig defaultZeemoteProfile[] =
{
	{
			Input::Event::MAP_ZEEMOTE,
			"Default Zeemote",
			{
					EMU_CONTROLS_IN_GAME_ACTIONS_UNBINDED_PROFILE_INIT,

					// JS 1
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

					// JS 2
					PP_ZERO_LIST(12)

					// Coleco 1 & 2
					PP_ZERO_LIST(24)

					// Keyboard
					Input::Zeemote::C,
			}
	},
};

const uint defaultZeemoteProfiles = sizeofArray(defaultZeemoteProfile);

};
