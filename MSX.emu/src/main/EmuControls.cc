#include <imagine/util/preprocessor/repeat.h>
#include <emuframework/EmuInput.hh>

namespace EmuControls
{

const uint categories = 6;
static const uint joystickKeys = 12;
static const uint colecoNumericKeys = 12;
static const uint kbKeys = 93;
const uint systemTotalKeys = gameActionKeys + joystickKeys*2 + colecoNumericKeys*2 + kbKeys;

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

const KeyCategory category[MAX_CATEGORIES]
{
	EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
	{"Set Joystick Keys", gamepadName, msxJoystickKeyOffset},
	{"Set Joystick 2 Keys", gamepadName, msxJoystick2KeyOffset, 1},
	{"Set Coleco Numpad Keys", colecoName, colecoJoystickKeyOffset},
	{"Set Coleco Numpad 2 Keys", colecoName, colecoJoystick2KeyOffset, 1},
	{"Set Keyboard Keys", keyboardName, keyboardKeyOffset},
};

const KeyConfig defaultKeyProfile[] =
{
	#ifdef CONFIG_BASE_ANDROID
	KEY_CONFIG_ANDROID_NAV_KEYS,
	#endif
	{
		Event::MAP_SYSTEM,
		0,
		"PC Keyboard (w/ Joystick Keys)",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_PROFILE_INIT,

			// JS 1
			Keycode::UP,
			Keycode::RIGHT,
			Keycode::DOWN,
			Keycode::LEFT,
			0, 0, 0, 0,
			Keycode::C,
			Keycode::X,
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Coleco 1
			Keycode::P,
			Keycode::Q,
			Keycode::W,
			Keycode::E,
			Keycode::R,
			Keycode::T,
			Keycode::Y,
			Keycode::U,
			Keycode::I,
			Keycode::O,
			Keycode::K,
			Keycode::D,

			// Coleco 2
			PP_ZERO_LIST(12)

			// Keyboard
			0,
			Keycode::T, // F1 ... F5
			Keycode::Y,
			Keycode::U,
			Keycode::I,
			Keycode::O,
			PP_ZERO_LIST(6) // 6 - 11
			Keycode::G, // 1 ... 0
			Keycode::H,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			PP_ZERO_LIST(3) // 22 - 24
			Keycode::BACK_SPACE,
			PP_ZERO_LIST(11) // 26 - 36
			0, // @
			0,
			Keycode::ENTER,
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
			Keycode::SPACE,
			PP_ZERO_LIST(23) // 70 - 92
		}
	},
	{
		Event::MAP_SYSTEM,
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
			Keycode::F1, // F1 ... F5
			Keycode::F2,
			Keycode::F3,
			Keycode::F4,
			Keycode::F5,

			Keycode::SCROLL_LOCK, // STOP - DEL
			Keycode::END,
			Keycode::HOME,
			Keycode::INSERT,
			Keycode::DELETE,

			Keycode::ESCAPE,

			Keycode::_1, // 1 ... 0
			Keycode::_2,
			Keycode::_3,
			Keycode::_4,
			Keycode::_5,
			Keycode::_6,
			Keycode::_7,
			Keycode::_8,
			Keycode::_9,
			Keycode::_0,
			Keycode::MINUS,
			Keycode::EQUALS,
			Keycode::BACKSLASH,
			Keycode::BACK_SPACE,

			Keycode::TAB,
			Keycode::Q,
			Keycode::W,
			Keycode::E,
			Keycode::R,
			Keycode::T,
			Keycode::Y,
			Keycode::U,
			Keycode::I,
			Keycode::O,
			Keycode::P,
			Keycode::GRAVE, // @
			Keycode::LEFT_BRACKET,
			Keycode::ENTER,

			Keycode::LCTRL, // CTRL
			Keycode::A,
			Keycode::S,
			Keycode::D,
			Keycode::F,
			Keycode::G,
			Keycode::H,
			Keycode::J,
			Keycode::K,
			Keycode::L,
			Keycode::SEMICOLON,
			Keycode::APOSTROPHE,
			Keycode::RIGHT_BRACKET,

			Keycode::LSHIFT, // Left Shift
			Keycode::Z,
			Keycode::X,
			Keycode::C,
			Keycode::V,
			Keycode::B,
			Keycode::N,
			Keycode::M,
			Keycode::COMMA,
			Keycode::PERIOD,
			Keycode::SLASH,
			Keycode::RCTRL,
			Keycode::RSHIFT, // Right Shift

			Keycode::CAPS,
			Keycode::LSUPER,
			Keycode::LALT,
			Keycode::SPACE,
			Keycode::RSUPER,
			Keycode::RALT,
			Keycode::PAUSE,

			Keycode::LEFT,
			Keycode::UP,
			Keycode::DOWN,
			Keycode::RIGHT,
			PP_ZERO_LIST(15) // 77 - 92
		}
	},
	#ifdef CONFIG_INPUT_GAMEPAD_DEVICES
	{
		Event::MAP_SYSTEM,
		Device::SUBTYPE_GENERIC_GAMEPAD,
		"Generic Gamepad",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_GENERIC_GAMEPAD_PROFILE_INIT,

			Keycode::UP,
			Keycode::RIGHT,
			Keycode::DOWN,
			Keycode::LEFT,
			0, 0, 0, 0,
			Keycode::GAME_A,
			Keycode::GAME_X,
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Coleco 1
			PP_ZERO_LIST(10)
			Keycode::GAME_B,
			Keycode::GAME_Y,

			// Coleco 2
			PP_ZERO_LIST(12)

			// Keyboard
			Keycode::GAME_START,
			Keycode::GAME_Y, // F1 ... F5
			Keycode::GAME_L1,
			Keycode::GAME_R1,
			Keycode::GAME_B,
			Keycode::GAME_SELECT,
			PP_ZERO_LIST(87) // 26 - 92
		}
	},
	#endif
	#ifdef CONFIG_ENV_WEBOS
	{
		Event::MAP_SYSTEM,
		0,
		"WebOS Keyboard (Joystick use)",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_WEBOS_KB_PROFILE_INIT,

			// JS 1
			EMU_CONTROLS_WEBOS_KB_8WAY_DIRECTION_PROFILE_INIT,
			Keycode::COMMA,
			Keycode::M,
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Coleco 1 & 2
			PP_ZERO_LIST(24)

			// Keyboard
			Keycode::SPACE,
		}
	},
	#endif
	#ifdef CONFIG_BASE_ANDROID
	{
		Event::MAP_SYSTEM,
		Device::SUBTYPE_PS3_CONTROLLER,
		"PS3 Controller",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_PS3_GAMEPAD_PROFILE_INIT,

			// JS 1
			Keycode::PS3::UP,
			Keycode::PS3::RIGHT,
			Keycode::PS3::DOWN,
			Keycode::PS3::LEFT,
			0, 0, 0, 0,
			Keycode::PS3::CROSS,
			Keycode::PS3::SQUARE,
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Coleco 1
			PP_ZERO_LIST(10)
			Keycode::PS3::CIRCLE,
			Keycode::PS3::TRIANGLE,

			// Coleco 2
			PP_ZERO_LIST(12)

			// Keyboard
			Keycode::PS3::START,
			Keycode::PS3::TRIANGLE, // F1 ... F5
			Keycode::PS3::L1,
			Keycode::PS3::R1,
			Keycode::PS3::CIRCLE,
			Keycode::PS3::SELECT,
		}
	},
	{
		Event::MAP_SYSTEM,
		Device::SUBTYPE_OUYA_CONTROLLER,
		"OUYA Controller",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_OUYA_PROFILE_INIT,

			// JS 1
			Keycode::Ouya::UP,
			Keycode::Ouya::RIGHT,
			Keycode::Ouya::DOWN,
			Keycode::Ouya::LEFT,
			0, 0, 0, 0,
			Keycode::Ouya::O,
			Keycode::Ouya::U,
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Coleco 1
			PP_ZERO_LIST(10)
			Keycode::Ouya::A,
			Keycode::Ouya::Y,

			// Coleco 2
			PP_ZERO_LIST(12)

			// Keyboard
			Keycode::Ouya::R3,
			Keycode::Ouya::Y, // F1 ... F5
			Keycode::Ouya::L1,
			Keycode::Ouya::R1,
			Keycode::Ouya::A,
			Keycode::Ouya::L3,
		}
	},
		#ifdef CONFIG_MACHINE_GENERIC_ARMV7
		{
			Event::MAP_SYSTEM,
			Device::SUBTYPE_XPERIA_PLAY,
			"Xperia Play",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

				// JS 1
				Keycode::XperiaPlay::UP,
				Keycode::XperiaPlay::RIGHT,
				Keycode::XperiaPlay::DOWN,
				Keycode::XperiaPlay::LEFT,
				0, 0, 0, 0,
				Keycode::XperiaPlay::CROSS,
				Keycode::XperiaPlay::SQUARE,
				0,
				0,

				// JS 2
				PP_ZERO_LIST(12)

				// Coleco 1
				PP_ZERO_LIST(10)
				Keycode::XperiaPlay::CIRCLE,
				Keycode::XperiaPlay::TRIANGLE,

				// Coleco 2
				PP_ZERO_LIST(12)

				// Keyboard
				Keycode::XperiaPlay::START,
				Keycode::XperiaPlay::TRIANGLE, // F1 ... F5
				Keycode::XperiaPlay::L1,
				Keycode::XperiaPlay::R1,
				Keycode::XperiaPlay::CIRCLE,
				Keycode::XperiaPlay::SELECT,
				PP_ZERO_LIST(87) // 26 - 92
			}
		},
		{
			Event::MAP_SYSTEM,
			Device::SUBTYPE_MOTO_DROID_KEYBOARD,
			"Droid/Milestone Keyboard (w/ Joystick Keys)",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

				// JS 1
				Keycode::UP,
				Keycode::RIGHT,
				Keycode::DOWN,
				Keycode::LEFT,
				0, 0, 0, 0,
				Keycode::C,
				Keycode::X,
				0,
				0,

				// JS 2
				PP_ZERO_LIST(12)

				// Coleco 1
				Keycode::P,
				Keycode::Q,
				Keycode::W,
				Keycode::E,
				Keycode::R,
				Keycode::T,
				Keycode::Y,
				Keycode::U,
				Keycode::I,
				Keycode::O,
				Keycode::K,
				Keycode::D,

				// Coleco 2
				PP_ZERO_LIST(12)

				// Keyboard
				0,
				Keycode::T, // F1 ... F5
				Keycode::Y,
				Keycode::U,
				Keycode::I,
				Keycode::O,
				PP_ZERO_LIST(6) // 6 - 11
				Keycode::G, // 1 ... 0
				Keycode::H,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				PP_ZERO_LIST(3) // 22 - 24
				Keycode::BACK_SPACE,
				PP_ZERO_LIST(11) // 26 - 36
				0, // @
				0,
				Keycode::ENTER,
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
				Keycode::SPACE,
				PP_ZERO_LIST(23) // 70 - 92
			}
		},
		{
			Event::MAP_SYSTEM,
			Device::SUBTYPE_NVIDIA_SHIELD,
			"NVidia Shield",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_NVIDIA_SHIELD_PROFILE_INIT,

				// JS 1
				Keycode::UP,
				Keycode::RIGHT,
				Keycode::DOWN,
				Keycode::LEFT,
				0, 0, 0, 0,
				Keycode::GAME_A,
				Keycode::GAME_X,
				0,
				0,

				// JS 2
				PP_ZERO_LIST(12)

				// Coleco 1
				PP_ZERO_LIST(10)
				Keycode::GAME_B,
				Keycode::GAME_Y,

				// Coleco 2
				PP_ZERO_LIST(12)

				// Keyboard
				Keycode::GAME_START,
				Keycode::GAME_Y, // F1 ... F5
				Keycode::GAME_L1,
				Keycode::GAME_R1,
				Keycode::GAME_B,
				Keycode::GAME_LEFT_THUMB,
				PP_ZERO_LIST(87) // 26 - 92
			}
		},
		#endif
	#endif
	#ifdef CONFIG_MACHINE_PANDORA
	{
		Event::MAP_SYSTEM,
		Device::SUBTYPE_PANDORA_HANDHELD,
		"Default Pandora",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_PANDORA_ALT_MINIMAL_PROFILE_INIT,

			// JS 1
			Keycode::Pandora::UP,
			Keycode::Pandora::RIGHT,
			Keycode::Pandora::DOWN,
			Keycode::Pandora::LEFT,
			0, 0, 0, 0,
			Keycode::Pandora::X,
			Keycode::Pandora::A,
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Coleco 1 & 2
			PP_ZERO_LIST(24)

			// Keyboard
			0,
			Keycode::F1, // F1 ... F5
			Keycode::F2,
			Keycode::F3,
			Keycode::F4,
			Keycode::F5,

			Keycode::SCROLL_LOCK, // STOP - DEL
			Keycode::END,
			Keycode::HOME,
			Keycode::INSERT,
			Keycode::DELETE,

			Keycode::ESCAPE,

			Keycode::_1, // 1 ... 0
			Keycode::_2,
			Keycode::_3,
			Keycode::_4,
			Keycode::_5,
			Keycode::_6,
			Keycode::_7,
			Keycode::_8,
			Keycode::_9,
			Keycode::_0,
			Keycode::MINUS,
			Keycode::EQUALS,
			Keycode::BACKSLASH,
			Keycode::BACK_SPACE,

			Keycode::TAB,
			Keycode::Q,
			Keycode::W,
			Keycode::E,
			Keycode::R,
			Keycode::T,
			Keycode::Y,
			Keycode::U,
			Keycode::I,
			Keycode::O,
			Keycode::P,
			Keycode::GRAVE, // @
			Keycode::LEFT_BRACKET,
			Keycode::ENTER,

			Keycode::LCTRL, // CTRL
			Keycode::A,
			Keycode::S,
			Keycode::D,
			Keycode::F,
			Keycode::G,
			Keycode::H,
			Keycode::J,
			Keycode::K,
			Keycode::L,
			Keycode::SEMICOLON,
			Keycode::APOSTROPHE,
			Keycode::RIGHT_BRACKET,

			Keycode::LSHIFT, // Left Shift
			Keycode::Z,
			Keycode::X,
			Keycode::C,
			Keycode::V,
			Keycode::B,
			Keycode::N,
			Keycode::M,
			Keycode::COMMA,
			Keycode::PERIOD,
			Keycode::SLASH,
			Keycode::RCTRL,
			Keycode::RSHIFT, // Right Shift

			Keycode::CAPS,
			Keycode::LSUPER,
			Keycode::LALT,
			Keycode::SPACE,
			Keycode::RSUPER,
			Keycode::RALT,
			Keycode::PAUSE,

			Keycode::LEFT,
			Keycode::UP,
			Keycode::DOWN,
			Keycode::RIGHT,
			PP_ZERO_LIST(15) // 77 - 92
		}
	},
	#endif
};

const uint defaultKeyProfiles = sizeofArray(defaultKeyProfile);

#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER

const KeyConfig defaultAppleGCProfile[] =
{
	{
		Event::MAP_APPLE_GAME_CONTROLLER,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_APPLEGC_PROFILE_INIT,

			// JS 1
			AppleGC::UP,
			AppleGC::RIGHT,
			AppleGC::DOWN,
			AppleGC::LEFT,
			0, 0, 0, 0,
			AppleGC::A,
			AppleGC::X,
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Coleco 1
			PP_ZERO_LIST(10)
			AppleGC::B,
			AppleGC::Y,

			// Coleco 2
			PP_ZERO_LIST(12)

			// Keyboard
			AppleGC::RSTICK_RIGHT,
			AppleGC::Y, // F1 ... F5
			AppleGC::L1,
			AppleGC::R1,
			AppleGC::B,
			AppleGC::RSTICK_LEFT,
			PP_ZERO_LIST(87) // 26 - 92
		}
	},
};

const uint defaultAppleGCProfiles = sizeofArray(defaultAppleGCProfile);

#endif

// Wiimote

const KeyConfig defaultWiimoteProfile[] =
{
	{
		Event::MAP_WIIMOTE,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_WIIMOTE_PROFILE_INIT,

			// JS 1
			Wiimote::UP,
			Wiimote::RIGHT,
			Wiimote::DOWN,
			Wiimote::LEFT,
			0, 0, 0, 0,
			Wiimote::_2,
			Wiimote::_1,
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Coleco 1
			PP_ZERO_LIST(11)
			Wiimote::MINUS,

			// Coleco 2
			PP_ZERO_LIST(12)

			// Keyboard
			Wiimote::PLUS,
		}
	},
};

const uint defaultWiimoteProfiles = sizeofArray(defaultWiimoteProfile);

const KeyConfig defaultWiiCCProfile[] =
{
	{
		Event::MAP_WII_CC,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_WII_CC_PROFILE_INIT,

			// JS 1
			WiiCC::UP,
			WiiCC::RIGHT,
			WiiCC::DOWN,
			WiiCC::LEFT,
			0, 0, 0, 0,
			WiiCC::B,
			WiiCC::Y,
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Coleco 1
			PP_ZERO_LIST(11)
			WiiCC::MINUS,

			// Coleco 2
			PP_ZERO_LIST(12)

			// Keyboard
			WiiCC::PLUS,
			WiiCC::X, // F1 - F5
			WiiCC::L,
			WiiCC::R,
			WiiCC::A,
			WiiCC::MINUS,
		}
	},
};

const uint defaultWiiCCProfiles = sizeofArray(defaultWiiCCProfile);

// iControlPad

const KeyConfig defaultIControlPadProfile[] =
{
	{
		Event::MAP_ICONTROLPAD,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ICP_NUBS_PROFILE_INIT,

			// JS 1
			iControlPad::UP,
			iControlPad::RIGHT,
			iControlPad::DOWN,
			iControlPad::LEFT,
			0, 0, 0, 0,
			iControlPad::X,
			iControlPad::A,
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Coleco 1 & 2
			PP_ZERO_LIST(24)

			// Keyboard
			iControlPad::START,
			iControlPad::Y, // F1 - F5
			iControlPad::L,
			iControlPad::R,
			iControlPad::B,
			iControlPad::SELECT,
			PP_ZERO_LIST(87) // 6 - 92
		}
	},
};

const uint defaultIControlPadProfiles = sizeofArray(defaultIControlPadProfile);

// iCade

const KeyConfig defaultICadeProfile[] =
{
	{
		Event::MAP_ICADE,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_UNBINDED_PROFILE_INIT,

			// JS 1
			ICade::UP,
			ICade::RIGHT,
			ICade::DOWN,
			ICade::LEFT,
			0, 0, 0, 0,
			ICade::A,
			ICade::X,
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Coleco 1 & 2
			PP_ZERO_LIST(24)

			// Keyboard
			ICade::START,
			ICade::Y, // F1 ... F5
			ICade::Z,
			ICade::C,
			ICade::B,
			ICade::SELECT,
			PP_ZERO_LIST(87) // 6 - 92
		}
	},
};

const uint defaultICadeProfiles = sizeofArray(defaultICadeProfile);

// Zeemote

const KeyConfig defaultZeemoteProfile[] =
{
	{
		Event::MAP_ZEEMOTE,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_UNBINDED_PROFILE_INIT,

			// JS 1
			Zeemote::UP,
			Zeemote::RIGHT,
			Zeemote::DOWN,
			Zeemote::LEFT,
			0,
			0,
			0,
			0,
			Zeemote::B,
			Zeemote::A,
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Coleco 1 & 2
			PP_ZERO_LIST(24)

			// Keyboard
			Zeemote::C,
		}
	},
};

const uint defaultZeemoteProfiles = sizeofArray(defaultZeemoteProfile);

// PS3

const KeyConfig defaultPS3Profile[] =
{
	{
		Event::MAP_PS3PAD,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_PS3PAD_PROFILE_INIT,

			// JS 1
			PS3::UP,
			PS3::RIGHT,
			PS3::DOWN,
			PS3::LEFT,
			0, 0, 0, 0,
			PS3::CROSS,
			PS3::SQUARE,
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Coleco 1
			PP_ZERO_LIST(10)
			PS3::CIRCLE,
			PS3::TRIANGLE,

			// Coleco 2
			PP_ZERO_LIST(12)

			// Keyboard
			PS3::START,
			PS3::TRIANGLE, // F1 ... F5
			PS3::L1,
			PS3::R1,
			PS3::CIRCLE,
			PS3::SELECT,
		}
	},
};

const uint defaultPS3Profiles = sizeofArray(defaultPS3Profile);

};
