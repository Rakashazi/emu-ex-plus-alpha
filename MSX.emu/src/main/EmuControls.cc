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
	KeyCategory("Set Joystick Keys", gamepadName, msxJoystickKeyOffset),
	KeyCategory("Set Joystick 2 Keys", gamepadName, msxJoystick2KeyOffset, 1),
	KeyCategory("Set Coleco Numpad Keys", colecoName, colecoJoystickKeyOffset),
	KeyCategory("Set Coleco Numpad 2 Keys", colecoName, colecoJoystick2KeyOffset, 1),
	KeyCategory("Set Keyboard Keys", keyboardName, keyboardKeyOffset),
};

#ifdef INPUT_SUPPORTS_KEYBOARD

const KeyConfig defaultKeyProfile[] =
{
	#ifdef CONFIG_ENV_WEBOS
	{
		Input::Event::MAP_KEYBOARD,
		0,
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
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Coleco 1
			PP_ZERO_LIST(10)
			Input::Keycode::PS3::TRIANGLE,
			Input::Keycode::PS3::SQUARE,

			// Coleco 2
			PP_ZERO_LIST(12)

			// Keyboard
			Input::Keycode::PS3::START,
			Input::Keycode::PS3::TRIANGLE, // F1 ... F5
			Input::Keycode::PS3::L1,
			Input::Keycode::PS3::R1,
			Input::Keycode::PS3::SQUARE,
			Input::Keycode::PS3::SELECT,
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
			Input::Keycode::Ouya::O,
			Input::Keycode::Ouya::A,
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Coleco 1
			PP_ZERO_LIST(10)
			Input::Keycode::Ouya::U,
			Input::Keycode::Ouya::Y,

			// Coleco 2
			PP_ZERO_LIST(12)

			// Keyboard
			Input::Keycode::Ouya::R3,
			Input::Keycode::Ouya::Y, // F1 ... F5
			Input::Keycode::Ouya::L1,
			Input::Keycode::Ouya::R1,
			Input::Keycode::Ouya::U,
			Input::Keycode::Ouya::L3,
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
				0,
				0,

				// JS 2
				PP_ZERO_LIST(12)

				// Coleco 1
				PP_ZERO_LIST(10)
				Input::Keycode::XperiaPlay::TRIANGLE,
				Input::Keycode::XperiaPlay::SQUARE,

				// Coleco 2
				PP_ZERO_LIST(12)

				// Keyboard
				Input::Keycode::XperiaPlay::START,
				Input::Keycode::XperiaPlay::TRIANGLE, // F1 ... F5
				Input::Keycode::XperiaPlay::L1,
				Input::Keycode::XperiaPlay::R1,
				Input::Keycode::XperiaPlay::SQUARE,
				Input::Keycode::XperiaPlay::SELECT,
				PP_ZERO_LIST(87) // 26 - 92
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
		#endif
	#endif
	{
		Input::Event::MAP_KEYBOARD,
		0,
		"PC Keyboard (w/ Joystick Keys)",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_PROFILE_INIT,

			// JS 1
			Input::Keycode::UP,
			Input::Keycode::RIGHT,
			Input::Keycode::DOWN,
			Input::Keycode::LEFT,
			0, 0, 0, 0,
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
		0,
		"PC Keyboard",
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

			// JS 1
			Input::Wiimote::UP,
			Input::Wiimote::RIGHT,
			Input::Wiimote::DOWN,
			Input::Wiimote::LEFT,
			0, 0, 0, 0,
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

			// JS 1
			Input::WiiCC::UP,
			Input::WiiCC::RIGHT,
			Input::WiiCC::DOWN,
			Input::WiiCC::LEFT,
			0, 0, 0, 0,
			Input::WiiCC::B,
			Input::WiiCC::Y,
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Coleco 1
			PP_ZERO_LIST(11)
			Input::WiiCC::MINUS,

			// Coleco 2
			PP_ZERO_LIST(12)

			// Keyboard
			Input::WiiCC::PLUS,
			Input::WiiCC::X, // F1 - F5
			Input::WiiCC::L,
			Input::WiiCC::R,
			Input::WiiCC::A,
			Input::WiiCC::MINUS,
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

			// JS 1
			Input::iControlPad::UP,
			Input::iControlPad::RIGHT,
			Input::iControlPad::DOWN,
			Input::iControlPad::LEFT,
			0, 0, 0, 0,
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
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_UNBINDED_PROFILE_INIT,

			// JS 1
			Input::ICade::UP,
			Input::ICade::RIGHT,
			Input::ICade::DOWN,
			Input::ICade::LEFT,
			0, 0, 0, 0,
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
		0,
		"Default",
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
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Coleco 1
			PP_ZERO_LIST(10)
			Input::PS3::TRIANGLE,
			Input::PS3::SQUARE,

			// Coleco 2
			PP_ZERO_LIST(12)

			// Keyboard
			Input::PS3::START,
			Input::PS3::TRIANGLE, // F1 ... F5
			Input::PS3::L1,
			Input::PS3::R1,
			Input::PS3::SQUARE,
			Input::PS3::SELECT,
		}
	},
};

const uint defaultPS3Profiles = sizeofArray(defaultPS3Profile);

};
