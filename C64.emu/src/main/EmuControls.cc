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
	"Swap Ports"
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
	KeyCategory("Set Joystick Keys", gamepadName, gamepadKeyOffset),
	KeyCategory("Set Joystick 2 Keys", gamepadName, gamepad2KeyOffset, 1),
	KeyCategory("Set Keyboard Keys", keyboardName, keyboardKeyOffset)
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
			Input::Keycode::PS3::UP,
			Input::Keycode::PS3::RIGHT,
			Input::Keycode::PS3::DOWN,
			Input::Keycode::PS3::LEFT,
			0, 0, 0, 0,
			Input::Keycode::PS3::CROSS,
			Input::Keycode::PS3::CIRCLE,
			Input::Keycode::PS3::TRIANGLE,

			// JS 2
			PP_ZERO_LIST(11)

			// Keyboard
			Input::Keycode::PS3::START,
			Input::Keycode::PS3::SELECT, // F1 ... F8
			0,
			Input::Keycode::PS3::L1,
			0,
			Input::Keycode::PS3::R1,
			0,
			Input::Keycode::PS3::SQUARE,
		}
	},
	{
		Input::Event::MAP_KEYBOARD,
		Input::Device::SUBTYPE_OUYA_CONTROLLER,
		"OUYA Controller",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

			// JS 1
			Input::Keycode::Ouya::UP,
			Input::Keycode::Ouya::RIGHT,
			Input::Keycode::Ouya::DOWN,
			Input::Keycode::Ouya::LEFT,
			0, 0, 0, 0,
			Input::Keycode::Ouya::A,
			Input::Keycode::Ouya::O,
			Input::Keycode::Ouya::Y,

			// JS 2
			PP_ZERO_LIST(11)

			// Keyboard
			Input::Keycode::Ouya::R3,
			Input::Keycode::Ouya::L3, // F1 ... F8
			0,
			Input::Keycode::Ouya::L1,
			0,
			Input::Keycode::Ouya::R1,
			0,
			Input::Keycode::Ouya::U,
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
				Input::Keycode::XperiaPlay::UP,
				Input::Keycode::XperiaPlay::RIGHT,
				Input::Keycode::XperiaPlay::DOWN,
				Input::Keycode::XperiaPlay::LEFT,
				0, 0, 0, 0,
				Input::Keycode::XperiaPlay::CROSS,
				Input::Keycode::XperiaPlay::CIRCLE,
				Input::Keycode::XperiaPlay::TRIANGLE,

				// JS 2
				PP_ZERO_LIST(11)

				// Keyboard
				Input::Keycode::XperiaPlay::START,
				Input::Keycode::XperiaPlay::SELECT, // F1 ... F8
				0,
				Input::Keycode::XperiaPlay::L1,
				0,
				Input::Keycode::XperiaPlay::R1,
				0,
				Input::Keycode::XperiaPlay::SQUARE,
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
				0, 0, 0, 0,
				asciiKey('x'),
				asciiKey('c'),
				0,

				// JS 2
				PP_ZERO_LIST(11)

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
				PP_ZERO_LIST(11)

				// JS 2
				PP_ZERO_LIST(11)

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
			0, 0, 0, 0,
			asciiKey('z'),
			asciiKey('x'),
			Input::Keycode::F10,

			// JS 2
			PP_ZERO_LIST(11)

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
			PP_ZERO_LIST(11)

			// JS 2
			PP_ZERO_LIST(11)

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
	#ifdef CONFIG_MACHINE_PANDORA
	{
		Input::Event::MAP_KEYBOARD,
		Input::Device::SUBTYPE_PANDORA_HANDHELD,
		"Default Pandora",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_OPEN_PANDORA_ALT_MINIMAL_PROFILE_INIT,

			// JS 1
			Input::Keycode::Pandora::UP,
			Input::Keycode::Pandora::RIGHT,
			Input::Keycode::Pandora::DOWN,
			Input::Keycode::Pandora::LEFT,
			0, 0, 0, 0,
			Input::Keycode::Pandora::X,
			Input::Keycode::Pandora::B,
			Input::Keycode::Pandora::Y,

			// JS 2
			PP_ZERO_LIST(11)

			// KB
			Input::Keycode::Pandora::SELECT,

			asciiKey('1'),
			asciiKey('2'),
			asciiKey('3'),
			asciiKey('4'),
			0, // F5 - F8
			0,
			0,
			0,

			0, // Left Arrow
			0, // 1 - 4
			0,
			0,
			0,
			asciiKey('5'),
			asciiKey('6'),
			asciiKey('7'),
			asciiKey('8'),
			asciiKey('9'),
			asciiKey('0'),
			0, // +
			0, // -
			0, // Pound
			0, // CLR HOME
			Input::Keycode::BACK_SPACE,

			0, // CTRL
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

			0, // Run/Stop
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
			0, // :
			0, // ;
			0,
			Input::Keycode::ENTER,

			0, // Commodore
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
			0, // slash
			0, // right shift
			0, // up
			0, // right
			0, // down
			0, // left

			asciiKey(' '),
		}
	},
	#endif
};

const uint defaultKeyProfiles = sizeofArray(defaultKeyProfile);

#endif

#ifdef CONFIG_INPUT_EVDEV

const KeyConfig defaultEvdevProfile[] =
{
	{
		Input::Event::MAP_EVDEV,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_UNBINDED_PROFILE_INIT,

			// JS 1
			Input::Evdev::UP,
			Input::Evdev::RIGHT,
			Input::Evdev::DOWN,
			Input::Evdev::LEFT,
			0, 0, 0, 0,
			Input::Evdev::GAME_A,
			Input::Evdev::GAME_B,
			Input::Evdev::GAME_Y,

			// JS 2
			PP_ZERO_LIST(11)
		}
	},
};

const uint defaultEvdevProfiles = sizeofArray(defaultEvdevProfile);

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
			0, 0, 0, 0,
			Input::Wiimote::_2,
			Input::Wiimote::_1,
			0,

			// JS 2
			PP_ZERO_LIST(11)

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
			0, 0, 0, 0,
			Input::WiiCC::B,
			Input::WiiCC::A,
			Input::WiiCC::X,

			// JS 2
			PP_ZERO_LIST(11)

			// KB
			Input::WiiCC::PLUS,
			Input::WiiCC::MINUS, // F1 ... F8
			0,
			Input::WiiCC::L,
			0,
			Input::WiiCC::R,
			0,
			Input::WiiCC::Y,
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
			0, 0, 0, 0,
			Input::iControlPad::X,
			Input::iControlPad::B,
			Input::iControlPad::Y,

			// JS 2
			PP_ZERO_LIST(11)

			// Keyboard
			Input::iControlPad::START,
			Input::iControlPad::SELECT, // F1 ... F8
			0,
			Input::iControlPad::L,
			0,
			Input::iControlPad::R,
			0,
			Input::iControlPad::A,
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
			0, 0, 0, 0,
			Input::ICade::H,
			Input::ICade::F,
			0,

			// JS 2
			PP_ZERO_LIST(11)

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
			0, 0, 0, 0,
			Input::Zeemote::B,
			Input::Zeemote::A,

			// JS 2
			PP_ZERO_LIST(11)

			// Keyboard
			Input::Zeemote::C,
		}
	},
};

const uint defaultZeemoteProfiles = sizeofArray(defaultZeemoteProfile);

// PS3

const KeyConfig defaultPS3Profile[] =
{
	{
		Input::Event::MAP_PS3PAD,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_PS3PAD_PROFILE_INIT,

			// JS 1
			Input::PS3::UP,
			Input::PS3::RIGHT,
			Input::PS3::DOWN,
			Input::PS3::LEFT,
			0, 0, 0, 0,
			Input::PS3::CROSS,
			Input::PS3::CIRCLE,
			Input::PS3::TRIANGLE,

			// JS 2
			PP_ZERO_LIST(11)

			// Keyboard
			Input::PS3::START,
			Input::PS3::SELECT, // F1 ... F8
			0,
			Input::PS3::L1,
			0,
			Input::PS3::R1,
			0,
			Input::PS3::SQUARE,
		}
	},
};

const uint defaultPS3Profiles = sizeofArray(defaultPS3Profile);

};
