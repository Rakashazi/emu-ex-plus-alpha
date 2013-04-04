#include <util/preprocessor/repeat.h>
#include <EmuInput.hh>

namespace EmuControls
{

void transposeKeysForPlayer(KeyConfig::KeyArray &key, uint player)
{
	generic2PlayerTranspose(key, player, 1);
}

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
		"Trigger",
		"Trigger Turbo",
};

static const char *keyboardName[kbKeys] =
{
		"Toggle Keyboard",

		"F1",
		"F2",
		"F3",
		"F4",
		"F5",
		"F6",
		"F7",
		"F8",

		"Left Arrow",
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
		"+",
		"-",
		"Pound",
		"Clr Home",
		"Inst Del",

		"Ctrl",
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
		"@",
		"*",
		"Up Arrow",
		"Restore",

		"Run Stop",
		"Shift Lock",
		"A",
		"S",
		"D",
		"F",
		"G",
		"H",
		"J",
		"K",
		"L",
		":  (",
		";  )",
		"=",
		"Return",

		"Commodore Logo",
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
		"Right Shift",
		"Up",
		"Right",
		"Down",
		"Left",

		"Space",
		"Ctrl Lock"
};

static const uint gamepadKeyOffset = gameActionKeys;
static const uint gamepad2KeyOffset = gamepadKeyOffset + gamepadKeys;
static const uint keyboardKeyOffset = gamepad2KeyOffset + gamepadKeys;

const KeyCategory category[categories]
{
		EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
		KeyCategory("Joystick", gamepadName, gamepadKeyOffset),
		KeyCategory("Joystick 2", gamepadName, gamepad2KeyOffset, 1),
		KeyCategory("Keyboard", keyboardName, keyboardKeyOffset)
};

#ifdef INPUT_SUPPORTS_KEYBOARD

const KeyConfig defaultKeyProfile[] =
{
#ifdef CONFIG_ENV_WEBOS
	{
		Input::Event::MAP_KEYBOARD,
		0,
		"WebOS Keyboard",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_WEBOS_KB_PROFILE_INIT,

			EMU_CONTROLS_WEBOS_KB_8WAY_DIRECTION_PROFILE_INIT,
			asciiKey(' '),
			Input::Keycode::ENTER,
			asciiKey(','),
			asciiKey('m'),
			asciiKey('i'),
			asciiKey('o'),
		}
	},
#endif
#ifdef CONFIG_BASE_ANDROID
	KEY_CONFIG_ANDROID_NAV_KEYS,
	{
		Input::Event::MAP_KEYBOARD,
		Input::Device::SUBTYPE_PS3_CONTROLLER,
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
			Input::PS3::CROSS,
			Input::PS3::CIRCLE,

			// JS 2
			PP_ZERO_LIST(10)

			// Keyboard
			Input::Keycode::GAME_START,
			Input::Keycode::GAME_SELECT, // F1 ... F8
			0,
			Input::Keycode::GAME_L1,
			0,
			Input::Keycode::GAME_R1,
			0,
			Input::PS3::SQUARE,
		}
	},
	{
		Input::Event::MAP_KEYBOARD,
		Input::Device::SUBTYPE_OUYA_CONTROLLER,
		"OUYA Controller",
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
			Input::Ouya::A,
			Input::Ouya::O,

			// JS 2
			PP_ZERO_LIST(10)

			// Keyboard
			Input::Keycode::GAME_RIGHT_THUMB,
			Input::Keycode::GAME_LEFT_THUMB, // F1 ... F8
			0,
			Input::Keycode::GAME_L1,
			0,
			Input::Keycode::GAME_R1,
			0,
			Input::Ouya::U,
		}
	},
	#ifdef CONFIG_MACHINE_GENERIC_ARMV7
		{
			Input::Event::MAP_KEYBOARD,
			Input::Device::SUBTYPE_XPERIA_PLAY,
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
				Input::XperiaPlay::CROSS,
				Input::XperiaPlay::CIRCLE,

				// JS 2
				PP_ZERO_LIST(10)

				// Keyboard
				Input::Keycode::GAME_START,
				Input::Keycode::GAME_SELECT, // F1 ... F8
				0,
				Input::Keycode::GAME_L1,
				0,
				Input::Keycode::GAME_R1,
				0,
				Input::XperiaPlay::SQUARE,
			}
		},
		{
			Input::Event::MAP_KEYBOARD,
			Input::Device::SUBTYPE_MOTO_DROID_KEYBOARD,
			"Droid/Milestone Keyboard (w/ Joystick Keys)",
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
				asciiKey('x'),
				asciiKey('c'),

				// JS 2
				PP_ZERO_LIST(10)

				// KB
				Input::Keycode::LALT,

				PP_ZERO_LIST(8) // F1-F8

				0, // Left Arrow
				PP_ZERO_LIST(10) // '1', '2', ... '0'
				0, // +
				0, // =
				0, // Pound
				0, // Clr Home
				Input::Keycode::BACK_SPACE,

				0, // Ctrl
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
				0, // @
				0, // *
				0, // Up Arrow
				0, // Restore

				Input::Keycode::RALT, // Run/Stop
				0, // Shift/Lock
				asciiKey('a'),
				asciiKey('s'),
				asciiKey('d'),
				asciiKey('f'),
				asciiKey('g'),
				asciiKey('h'),
				asciiKey('j'),
				asciiKey('k'),
				0, // l
				0, // :
				0, // ;
				0, // =
				Input::Keycode::ENTER,

				0, // Commodore Logo
				Input::Keycode::LSHIFT,
				asciiKey('z'),
				0, // x
				0, // c
				asciiKey('v'),
				asciiKey('b'),
				asciiKey('n'),
				asciiKey('m'),
				asciiKey(','),
				asciiKey('.'),
				asciiKey('/'),
				Input::Keycode::RSHIFT,
				0, // up
				0, // right
				0, // down
				0, // left

				asciiKey(' '),
			}
		},
		{
			Input::Event::MAP_KEYBOARD,
			Input::Device::SUBTYPE_MOTO_DROID_KEYBOARD,
			"Droid/Milestone Keyboard",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

				// JS 1
				PP_ZERO_LIST(10)

				// JS 2
				PP_ZERO_LIST(10)

				// KB
				Input::Keycode::LALT,

				PP_ZERO_LIST(8) // F1-F8

				0, // Left Arrow
				PP_ZERO_LIST(10) // '1', '2', ... '0'
				0, // +
				0, // =
				0, // Pound
				0, // Clr Home
				Input::Keycode::BACK_SPACE,

				0, // Ctrl
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
				0, // @
				0, // *
				0, // Up Arrow
				0, // Restore

				Input::Keycode::RALT, // Run/Stop
				0, // Shift/Lock
				asciiKey('a'),
				asciiKey('s'),
				asciiKey('d'),
				asciiKey('f'),
				asciiKey('g'),
				asciiKey('h'),
				asciiKey('j'),
				asciiKey('k'),
				0, // l
				0, // :
				0, // ;
				0, // =
				Input::Keycode::ENTER,

				0, // Commodore Logo
				Input::Keycode::LSHIFT,
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
				Input::Keycode::RSHIFT,
				Input::Keycode::UP,
				Input::Keycode::RIGHT,
				Input::Keycode::DOWN,
				Input::Keycode::LEFT,

				asciiKey(' '),
			}
		},
	#endif
#endif
	{
		Input::Event::MAP_KEYBOARD,
		0,
		"PC Keyboard (w/ Joystick Keys)",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_ALT_PROFILE_INIT,

			// JS 1
			Input::Keycode::UP,
			Input::Keycode::RIGHT,
			Input::Keycode::DOWN,
			Input::Keycode::LEFT,
			0,
			0,
			0,
			0,
			asciiKey('z'),
			asciiKey('x'),

			// JS 2
			PP_ZERO_LIST(10)

			// KB
			Input::Keycode::RMETA,

			Input::Keycode::F1,
			Input::Keycode::F2,
			Input::Keycode::F3,
			Input::Keycode::F4,
			Input::Keycode::F5,
			Input::Keycode::F6,
			Input::Keycode::F7,
			Input::Keycode::F8,

			0, // Left Arrow
			asciiKey('1'),
			asciiKey('2'),
			asciiKey('3'),
			asciiKey('4'),
			asciiKey('5'),
			asciiKey('6'),
			asciiKey('7'),
			asciiKey('8'),
			asciiKey('9'),
			asciiKey('0'),
			asciiKey('-'), // +
			asciiKey('='),
			0, // Pound
			Input::Keycode::HOME,
			Input::Keycode::BACK_SPACE,

			asciiKey('\t'),
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
			0, // @
			0, // *
			asciiKey('\\'), // Up Arrow
			Input::Keycode::END, // Restore

			Input::Keycode::PGUP, // Run/Stop
			0, // Shift/Lock
			asciiKey('a'),
			asciiKey('s'),
			asciiKey('d'),
			asciiKey('f'),
			asciiKey('g'),
			asciiKey('h'),
			asciiKey('j'),
			asciiKey('k'),
			0, // l
			asciiKey(';'), // :
			asciiKey('\''), // ;
			0,
			Input::Keycode::ENTER,

			Input::Keycode::LMETA,
			Input::Keycode::LSHIFT,
			0, // z
			0, // x
			asciiKey('c'),
			asciiKey('v'),
			asciiKey('b'),
			asciiKey('n'),
			asciiKey('m'),
			asciiKey(','),
			asciiKey('.'),
			asciiKey('/'),
			Input::Keycode::RSHIFT,
			0, // up
			0, // right
			0, // down
			0, // left

			asciiKey(' '),
		}
	},
	{
		Input::Event::MAP_KEYBOARD,
		0,
		"PC Keyboard",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_MINIMAL_PROFILE_INIT,

			// JS 1
			PP_ZERO_LIST(10)

			// JS 2
			PP_ZERO_LIST(10)

			// KB
			Input::Keycode::RMETA,

			Input::Keycode::F1,
			Input::Keycode::F2,
			Input::Keycode::F3,
			Input::Keycode::F4,
			Input::Keycode::F5,
			Input::Keycode::F6,
			Input::Keycode::F7,
			Input::Keycode::F8,

			asciiKey('`'), // Left Arrow
			asciiKey('1'),
			asciiKey('2'),
			asciiKey('3'),
			asciiKey('4'),
			asciiKey('5'),
			asciiKey('6'),
			asciiKey('7'),
			asciiKey('8'),
			asciiKey('9'),
			asciiKey('0'),
			asciiKey('-'), // +
			asciiKey('='), // -
			0, // Pound
			Input::Keycode::HOME,
			Input::Keycode::BACK_SPACE,

			asciiKey('\t'),
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
			asciiKey('['), // @
			asciiKey(']'), // *
			asciiKey('\\'), // Up Arrow
			Input::Keycode::END, // Restore

			Input::Keycode::PGUP, // Run/Stop
			0, // Shift/Lock
			asciiKey('a'),
			asciiKey('s'),
			asciiKey('d'),
			asciiKey('f'),
			asciiKey('g'),
			asciiKey('h'),
			asciiKey('j'),
			asciiKey('k'),
			asciiKey('l'),
			asciiKey(';'), // :
			asciiKey('\''), // ;
			0,
			Input::Keycode::ENTER,

			Input::Keycode::LMETA,
			Input::Keycode::LSHIFT,
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
			Input::Keycode::RSHIFT,
			Input::Keycode::UP, // up
			Input::Keycode::RIGHT, // right
			Input::Keycode::DOWN, // down
			Input::Keycode::LEFT, // left

			asciiKey(' '),
		}
	},
};

const uint defaultKeyProfiles = sizeofArray(defaultKeyProfile);

#endif

// Wiimote

const KeyConfig defaultWiimoteProfile[] =
{
	{
		Input::Event::MAP_WIIMOTE,
		0,
		"Default",
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

			// JS 2
			PP_ZERO_LIST(10)

			// KB
			Input::Wiimote::PLUS,

			Input::Wiimote::MINUS, // F1
		}
	},
};

const uint defaultWiimoteProfiles = sizeofArray(defaultWiimoteProfile);

const KeyConfig defaultWiiCCProfile[] =
{
	{
		Input::Event::MAP_WII_CC,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_WII_CC_PROFILE_INIT,

			Input::WiiCC::UP,
			Input::WiiCC::RIGHT,
			Input::WiiCC::DOWN,
			Input::WiiCC::LEFT,
			0,
			0,
			0,
			0,
			Input::WiiCC::B,
			Input::WiiCC::Y,

			// JS 2
			PP_ZERO_LIST(10)

			// KB
			Input::WiiCC::PLUS,

			Input::WiiCC::MINUS, // F1 - F5
			Input::WiiCC::L,
			Input::WiiCC::R,
			Input::WiiCC::X,
			Input::WiiCC::A,
		}
	},
};

const uint defaultWiiCCProfiles = sizeofArray(defaultWiiCCProfile);

// iControlPad

const KeyConfig defaultIControlPadProfile[] =
{
	{
		Input::Event::MAP_ICONTROLPAD,
		0,
		"Default",
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

			// JS 2
			PP_ZERO_LIST(10)

			// Keyboard
			Input::iControlPad::START,
			Input::iControlPad::SELECT, // F1 - F5
			Input::iControlPad::L,
			Input::iControlPad::R,
			Input::iControlPad::Y,
			Input::iControlPad::B,
		}
	},
};

const uint defaultIControlPadProfiles = sizeofArray(defaultIControlPadProfile);

// iCade

const KeyConfig defaultICadeProfile[] =
{
	{
		Input::Event::MAP_ICADE,
		0,
		"Default",
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

			// JS 2
			PP_ZERO_LIST(10)

			// Keyboard
			Input::ICade::C,
			Input::ICade::E, // F1 - F5
			Input::ICade::B,
			Input::ICade::D,
			Input::ICade::G,
			Input::ICade::A,
		}
	},
};

const uint defaultICadeProfiles = sizeofArray(defaultICadeProfile);

// Zeemote

const KeyConfig defaultZeemoteProfile[] =
{
	{
		Input::Event::MAP_ZEEMOTE,
		0,
		"Default",
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

			// JS 2
			PP_ZERO_LIST(10)

			// Keyboard
			Input::Zeemote::C,
		}
	},
};

const uint defaultZeemoteProfiles = sizeofArray(defaultZeemoteProfile);

};
