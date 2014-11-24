#include <imagine/util/preprocessor/repeat.h>
#include <emuframework/EmuInput.hh>

namespace EmuControls
{

const uint categories = 4;
static const uint gamepadKeys = 11;
static const uint kbKeys = 74;
const uint systemTotalKeys = gameActionKeys + gamepadKeys*2 + kbKeys;

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

const KeyCategory category[MAX_CATEGORIES]
{
	EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
	{"Set Joystick Keys", gamepadName, gamepadKeyOffset},
	{"Set Joystick 2 Keys", gamepadName, gamepad2KeyOffset, true},
	{"Set Keyboard Keys", keyboardName, keyboardKeyOffset}
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
			EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_ALT_PROFILE_INIT,

			// JS 1
			Keycode::UP,
			Keycode::RIGHT,
			Keycode::DOWN,
			Keycode::LEFT,
			0, 0, 0, 0,
			Keycode::Z,
			Keycode::X,
			Keycode::F10,

			// JS 2
			PP_ZERO_LIST(11)

			// KB
			Keycode::RSUPER,

			Keycode::F1,
			Keycode::F2,
			Keycode::F3,
			Keycode::F4,
			Keycode::F5,
			Keycode::F6,
			Keycode::F7,
			Keycode::F8,

			0, // Left Arrow
			Keycode::_1,
			Keycode::_2,
			Keycode::_3,
			Keycode::_4,
			Keycode::_5,
			Keycode::_6,
			Keycode::_7,
			Keycode::_8,
			Keycode::_9,
			Keycode::_0,
			Keycode::MINUS, // +
			Keycode::EQUALS,
			0, // Pound
			Keycode::HOME,
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
			0, // @
			0, // *
			Keycode::BACKSLASH, // Up Arrow
			Keycode::END, // Restore

			Keycode::PGUP, // Run/Stop
			0, // Shift/Lock
			Keycode::A,
			Keycode::S,
			Keycode::D,
			Keycode::F,
			Keycode::G,
			Keycode::H,
			Keycode::J,
			Keycode::K,
			0, // l
			Keycode::SEMICOLON, // :
			Keycode::APOSTROPHE, // ;
			0,
			Keycode::ENTER,

			Keycode::LSUPER,
			Keycode::LSHIFT,
			0, // z
			0, // x
			Keycode::C,
			Keycode::V,
			Keycode::B,
			Keycode::N,
			Keycode::M,
			Keycode::COMMA,
			Keycode::PERIOD,
			Keycode::SLASH,
			Keycode::RSHIFT,
			0, // up
			0, // right
			0, // down
			0, // left

			Keycode::SPACE,
		}
	},
	{
		Event::MAP_SYSTEM,
		0,
		"PC Keyboard",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_MINIMAL_PROFILE_INIT,

			// JS 1
			PP_ZERO_LIST(11)

			// JS 2
			PP_ZERO_LIST(11)

			// KB
			Keycode::RSUPER,

			Keycode::F1,
			Keycode::F2,
			Keycode::F3,
			Keycode::F4,
			Keycode::F5,
			Keycode::F6,
			Keycode::F7,
			Keycode::F8,

			Keycode::GRAVE, // Left Arrow
			Keycode::_1,
			Keycode::_2,
			Keycode::_3,
			Keycode::_4,
			Keycode::_5,
			Keycode::_6,
			Keycode::_7,
			Keycode::_8,
			Keycode::_9,
			Keycode::_0,
			Keycode::MINUS, // +
			Keycode::EQUALS, // -
			0, // Pound
			Keycode::HOME,
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
			Keycode::LEFT_BRACKET, // @
			Keycode::RIGHT_BRACKET, // *
			Keycode::BACKSLASH, // Up Arrow
			Keycode::END, // Restore

			Keycode::PGUP, // Run/Stop
			0, // Shift/Lock
			Keycode::A,
			Keycode::S,
			Keycode::D,
			Keycode::F,
			Keycode::G,
			Keycode::H,
			Keycode::J,
			Keycode::K,
			Keycode::L,
			Keycode::SEMICOLON, // :
			Keycode::APOSTROPHE, // ;
			0,
			Keycode::ENTER,

			Keycode::LSUPER,
			Keycode::LSHIFT,
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
			Keycode::RSHIFT,
			Keycode::UP, // up
			Keycode::RIGHT, // right
			Keycode::DOWN, // down
			Keycode::LEFT, // left

			Keycode::SPACE,
		}
	},
	#ifdef CONFIG_INPUT_GAMEPAD_DEVICES
	{
		Event::MAP_SYSTEM,
		Device::SUBTYPE_GENERIC_GAMEPAD,
		"Generic Gamepad",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_GENERIC_GAMEPAD_PROFILE_INIT,

			// JS 1
			Keycode::UP,
			Keycode::RIGHT,
			Keycode::DOWN,
			Keycode::LEFT,
			0, 0, 0, 0,
			Keycode::GAME_A,
			Keycode::GAME_X,
			Keycode::GAME_Y,

			// JS 2
			PP_ZERO_LIST(11)

			// Keyboard
			Keycode::GAME_START,
			Keycode::GAME_SELECT, // F1 ... F8
			0,
			Keycode::GAME_L1,
			0,
			Keycode::GAME_R1,
			0,
			Keycode::GAME_B,
		}
	},
	#endif
	#ifdef CONFIG_ENV_WEBOS
	{
		Event::MAP_SYSTEM,
		0,
		"WebOS Keyboard",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_WEBOS_KB_PROFILE_INIT,

			EMU_CONTROLS_WEBOS_KB_8WAY_DIRECTION_PROFILE_INIT,
			Keycode::SPACE,
			Keycode::ENTER,
			Keycode::COMMA,
			Keycode::M,
			Keycode::I,
			Keycode::O,
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
			Keycode::PS3::TRIANGLE,

			// JS 2
			PP_ZERO_LIST(11)

			// Keyboard
			Keycode::PS3::START,
			Keycode::PS3::SELECT, // F1 ... F8
			0,
			Keycode::PS3::L1,
			0,
			Keycode::PS3::R1,
			0,
			Keycode::PS3::CIRCLE,
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
			Keycode::Ouya::Y,

			// JS 2
			PP_ZERO_LIST(11)

			// Keyboard
			Keycode::Ouya::R3,
			Keycode::Ouya::L3, // F1 ... F8
			0,
			Keycode::Ouya::L1,
			0,
			Keycode::Ouya::R1,
			0,
			Keycode::Ouya::A,
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
				Keycode::XperiaPlay::TRIANGLE,

				// JS 2
				PP_ZERO_LIST(11)

				// Keyboard
				Keycode::XperiaPlay::START,
				Keycode::XperiaPlay::SELECT, // F1 ... F8
				0,
				Keycode::XperiaPlay::L1,
				0,
				Keycode::XperiaPlay::R1,
				0,
				Keycode::XperiaPlay::CIRCLE,
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
				Keycode::X,
				Keycode::C,
				0,

				// JS 2
				PP_ZERO_LIST(11)

				// KB
				Keycode::LALT,

				PP_ZERO_LIST(8) // F1-F8

				0, // Left Arrow
				PP_ZERO_LIST(10) // '1', '2', ... '0'
				0, // +
				0, // =
				0, // Pound
				0, // Clr Home
				Keycode::BACK_SPACE,

				0, // Ctrl
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
				0, // @
				0, // *
				0, // Up Arrow
				0, // Restore

				Keycode::RALT, // Run/Stop
				0, // Shift/Lock
				Keycode::A,
				Keycode::S,
				Keycode::D,
				Keycode::F,
				Keycode::G,
				Keycode::H,
				Keycode::J,
				Keycode::K,
				0, // l
				0, // :
				0, // ;
				0, // =
				Keycode::ENTER,

				0, // Commodore Logo
				Keycode::LSHIFT,
				Keycode::Z,
				0, // x
				0, // c
				Keycode::V,
				Keycode::B,
				Keycode::N,
				Keycode::M,
				Keycode::COMMA,
				Keycode::PERIOD,
				Keycode::SLASH,
				Keycode::RSHIFT,
				0, // up
				0, // right
				0, // down
				0, // left

				Keycode::SPACE,
			}
		},
		{
			Event::MAP_SYSTEM,
			Device::SUBTYPE_MOTO_DROID_KEYBOARD,
			"Droid/Milestone Keyboard",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

				// JS 1
				PP_ZERO_LIST(11)

				// JS 2
				PP_ZERO_LIST(11)

				// KB
				Keycode::LALT,

				PP_ZERO_LIST(8) // F1-F8

				0, // Left Arrow
				PP_ZERO_LIST(10) // '1', '2', ... '0'
				0, // +
				0, // =
				0, // Pound
				0, // Clr Home
				Keycode::BACK_SPACE,

				0, // Ctrl
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
				0, // @
				0, // *
				0, // Up Arrow
				0, // Restore

				Keycode::RALT, // Run/Stop
				0, // Shift/Lock
				Keycode::A,
				Keycode::S,
				Keycode::D,
				Keycode::F,
				Keycode::G,
				Keycode::H,
				Keycode::J,
				Keycode::K,
				0, // l
				0, // :
				0, // ;
				0, // =
				Keycode::ENTER,

				0, // Commodore Logo
				Keycode::LSHIFT,
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
				Keycode::RSHIFT,
				Keycode::UP,
				Keycode::RIGHT,
				Keycode::DOWN,
				Keycode::LEFT,

				Keycode::SPACE,
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
				Keycode::GAME_B,
				Keycode::GAME_Y,

				// JS 2
				PP_ZERO_LIST(11)

				// Keyboard
				Keycode::GAME_START,
				Keycode::GAME_LEFT_THUMB, // F1 ... F8
				0,
				Keycode::GAME_L1,
				0,
				Keycode::GAME_R1,
				0,
				Keycode::GAME_X,
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
			Keycode::Pandora::Y,

			// JS 2
			PP_ZERO_LIST(11)

			// KB
			Keycode::Pandora::SELECT,

			Keycode::_1,
			Keycode::_2,
			Keycode::_3,
			Keycode::_4,
			0, // F5 - F8
			0,
			0,
			0,

			0, // Left Arrow
			0, // 1 - 4
			0,
			0,
			0,
			Keycode::_5,
			Keycode::_6,
			Keycode::_7,
			Keycode::_8,
			Keycode::_9,
			Keycode::_0,
			0, // +
			0, // -
			0, // Pound
			0, // CLR HOME
			Keycode::BACK_SPACE,

			0, // CTRL
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
			0, // @
			0, // *
			0, // Up Arrow
			0, // Restore

			0, // Run/Stop
			0, // Shift/Lock
			Keycode::A,
			Keycode::S,
			Keycode::D,
			Keycode::F,
			Keycode::G,
			Keycode::H,
			Keycode::J,
			Keycode::K,
			Keycode::L,
			0, // :
			0, // ;
			0,
			Keycode::ENTER,

			0, // Commodore
			Keycode::LSHIFT,
			Keycode::Z,
			Keycode::X,
			Keycode::C,
			Keycode::V,
			Keycode::B,
			Keycode::N,
			Keycode::M,
			Keycode::COMMA,
			Keycode::PERIOD,
			0, // slash
			0, // right shift
			0, // up
			0, // right
			0, // down
			0, // left

			Keycode::SPACE,
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
			AppleGC::Y,

			// JS 2
			PP_ZERO_LIST(11)

			// Keyboard
			AppleGC::RSTICK_RIGHT,
			AppleGC::RSTICK_LEFT, // F1 ... F8
			0,
			AppleGC::L1,
			0,
			AppleGC::R1,
			0,
			AppleGC::B,
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

			Wiimote::UP,
			Wiimote::RIGHT,
			Wiimote::DOWN,
			Wiimote::LEFT,
			0, 0, 0, 0,
			Wiimote::_2,
			Wiimote::_1,
			0,

			// JS 2
			PP_ZERO_LIST(11)

			// KB
			Wiimote::PLUS,

			Wiimote::MINUS, // F1
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

			WiiCC::UP,
			WiiCC::RIGHT,
			WiiCC::DOWN,
			WiiCC::LEFT,
			0, 0, 0, 0,
			WiiCC::B,
			WiiCC::Y,
			WiiCC::X,

			// JS 2
			PP_ZERO_LIST(11)

			// KB
			WiiCC::PLUS,
			WiiCC::MINUS, // F1 ... F8
			0,
			WiiCC::L,
			0,
			WiiCC::R,
			0,
			WiiCC::A,
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

			iControlPad::UP,
			iControlPad::RIGHT,
			iControlPad::DOWN,
			iControlPad::LEFT,
			0, 0, 0, 0,
			iControlPad::X,
			iControlPad::A,
			iControlPad::Y,

			// JS 2
			PP_ZERO_LIST(11)

			// Keyboard
			iControlPad::START,
			iControlPad::SELECT, // F1 ... F8
			0,
			iControlPad::L,
			0,
			iControlPad::R,
			0,
			iControlPad::B,
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

			ICade::UP,
			ICade::RIGHT,
			ICade::DOWN,
			ICade::LEFT,
			0, 0, 0, 0,
			ICade::A,
			ICade::X,
			ICade::Y,

			// JS 2
			PP_ZERO_LIST(11)

			// Keyboard
			ICade::START,
			ICade::SELECT, // F1 - F5
			0,
			ICade::C,
			0,
			ICade::Z,
			0,
			ICade::B,
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

			Zeemote::UP,
			Zeemote::RIGHT,
			Zeemote::DOWN,
			Zeemote::LEFT,
			0, 0, 0, 0,
			Zeemote::B,
			Zeemote::A,

			// JS 2
			PP_ZERO_LIST(11)

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
			PS3::TRIANGLE,

			// JS 2
			PP_ZERO_LIST(11)

			// Keyboard
			PS3::START,
			PS3::SELECT, // F1 ... F8
			0,
			PS3::L1,
			0,
			PS3::R1,
			0,
			PS3::CIRCLE,
		}
	},
};

const uint defaultPS3Profiles = sizeofArray(defaultPS3Profile);

};
