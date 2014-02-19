#include <util/preprocessor/repeat.h>
#include <EmuInput.hh>

namespace EmuControls
{

void transposeKeysForPlayer(KeyConfig::KeyArray &key, uint player)
{
	generic2PlayerTranspose(key, player, 1);
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
	"Trigger",
	"Trigger Turbo",
};

static const char *switchName[switchKeys] =
{
	"Select",
	"Reset",
	"Left (P1) Difficulty",
	"Right (P2) Difficulty",
	"Color/B&W"
};

static const char *keyboardName[keyboardKeys] =
{
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
	"0",
	"#"
};

static const uint joystickKeyOffset = gameActionKeys;
static const uint joystick2KeyOffset = joystickKeyOffset + joystickKeys;
static const uint switchKeyOffset = joystick2KeyOffset + joystickKeys;
static const uint keyboardKeyOffset = switchKeyOffset + switchKeys;
static const uint keyboard2KeyOffset = keyboardKeyOffset + keyboardKeys;

const KeyCategory category[categories]
{
	EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
	KeyCategory("Set Joystick Keys", gamepadName, joystickKeyOffset),
	KeyCategory("Set Joystick 2 Keys", gamepadName, joystick2KeyOffset, 1),
	KeyCategory("Set Console Switch Keys", switchName, switchKeyOffset),
	KeyCategory("Set Keyboard Keys", keyboardName, keyboardKeyOffset),
	KeyCategory("Set Keyboard 2 Keys", keyboardName, keyboard2KeyOffset)
};

#ifdef INPUT_SUPPORTS_KEYBOARD

const KeyConfig defaultKeyProfile[] =
{
	#ifdef CONFIG_ENV_WEBOS
	{
		Input::Event::MAP_SYSTEM,
		0,
		"WebOS Keyboard",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_WEBOS_KB_PROFILE_INIT,

			// JS 1
			EMU_CONTROLS_WEBOS_KB_8WAY_DIRECTION_PROFILE_INIT,
			Keycode::asciiKey(','),
			Keycode::asciiKey('m'),

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Keycode::asciiKey(' '),
			Input::Keycode::ENTER,
			Keycode::asciiKey('k'),
			Keycode::asciiKey('l'),
		}
	},
	#endif
	#ifdef CONFIG_BASE_ANDROID
	KEY_CONFIG_ANDROID_NAV_KEYS,
	{
		Input::Event::MAP_SYSTEM,
		Input::Device::SUBTYPE_GENERIC_GAMEPAD,
		"Generic Gamepad",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_GENERIC_GAMEPAD_PROFILE_INIT,

			// JS 1
			Input::Keycode::UP,
			Input::Keycode::RIGHT,
			Input::Keycode::DOWN,
			Input::Keycode::LEFT,
			0, 0, 0, 0,
			Input::Keycode::GAME_A,
			Input::Keycode::GAME_B,

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Input::Keycode::GAME_R1,
			Input::Keycode::GAME_START,
			Input::Keycode::GAME_X,
			Input::Keycode::GAME_Y,
			Input::Keycode::GAME_L1,
		}
	},
	{
		Input::Event::MAP_SYSTEM,
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

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Input::Keycode::PS3::SELECT,
			Input::Keycode::PS3::START,
			Input::Keycode::PS3::SQUARE,
			Input::Keycode::PS3::TRIANGLE,
			Input::Keycode::PS3::L1,
		}
	},
	{
		Input::Event::MAP_SYSTEM,
		Input::Device::SUBTYPE_OUYA_CONTROLLER,
		"OUYA Controller",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_OUYA_PROFILE_INIT,

			// JS 1
			Input::Keycode::Ouya::UP,
			Input::Keycode::Ouya::RIGHT,
			Input::Keycode::Ouya::DOWN,
			Input::Keycode::Ouya::LEFT,
			0, 0, 0, 0,
			Input::Keycode::Ouya::O,
			Input::Keycode::Ouya::U,

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Input::Keycode::Ouya::L3,
			Input::Keycode::Ouya::R3,
			Input::Keycode::Ouya::Y,
			Input::Keycode::Ouya::A,
			Input::Keycode::Ouya::L1,
		}
	},
		#ifdef CONFIG_MACHINE_GENERIC_ARMV7
		{
			Input::Event::MAP_SYSTEM,
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

				// JS 2
				PP_ZERO_LIST(10)

				// Switches
				Input::Keycode::XperiaPlay::SELECT,
				Input::Keycode::XperiaPlay::START,
				Input::Keycode::XperiaPlay::SQUARE,
				Input::Keycode::XperiaPlay::TRIANGLE,
				Input::Keycode::XperiaPlay::L1,
			}
		},
		{
			Input::Event::MAP_SYSTEM,
			Input::Device::SUBTYPE_MOTO_DROID_KEYBOARD,
			"Droid/Milestone Keyboard",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

				// JS 1
				Input::Keycode::UP,
				Input::Keycode::RIGHT,
				Input::Keycode::DOWN,
				Input::Keycode::LEFT,
				0, 0, 0, 0,
				Keycode::asciiKey('x'),
				Keycode::asciiKey('c'),

				// JS 2
				PP_ZERO_LIST(10)

				// Switches
				Keycode::asciiKey(' '),
				Input::Keycode::ENTER,
				Keycode::asciiKey('s'),
				Keycode::asciiKey('d'),
			}
		},
		{
			Input::Event::MAP_SYSTEM,
			Input::Device::SUBTYPE_NVIDIA_SHIELD,
			"NVidia Shield",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_NVIDIA_SHIELD_PROFILE_INIT,

				// JS 1
				Input::Keycode::UP,
				Input::Keycode::RIGHT,
				Input::Keycode::DOWN,
				Input::Keycode::LEFT,
				0, 0, 0, 0,
				Input::Keycode::GAME_A,
				Input::Keycode::GAME_B,

				// JS 2
				PP_ZERO_LIST(10)

				// Switches
				Input::Keycode::GAME_R1,
				Input::Keycode::GAME_START,
				Input::Keycode::GAME_X,
				Input::Keycode::GAME_Y,
				Input::Keycode::GAME_L1,
			}
		},
		#endif
	#endif
	{
		Input::Event::MAP_SYSTEM,
		0,
		"PC Keyboard",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_PROFILE_INIT,

			// JS 1
			Input::Keycode::UP,
			Input::Keycode::RIGHT,
			Input::Keycode::DOWN,
			Input::Keycode::LEFT,
			0, 0, 0, 0,
			Keycode::asciiKey('z'),
			Keycode::asciiKey('x'),

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Keycode::asciiKey(' '),
			Input::Keycode::ENTER,
			Keycode::asciiKey('a'),
			Keycode::asciiKey('s'),
			Keycode::asciiKey('q'),

			// KB 1
			Keycode::asciiKey('3'),
			Keycode::asciiKey('4'),
			Keycode::asciiKey('5'),
			Keycode::asciiKey('e'),
			Keycode::asciiKey('r'),
			Keycode::asciiKey('t'),
			Keycode::asciiKey('d'),
			Keycode::asciiKey('f'),
			Keycode::asciiKey('g'),
			Keycode::asciiKey('c'),
			Keycode::asciiKey('v'),
			Keycode::asciiKey('b'),

			// KB 2
			Keycode::asciiKey('6'),
			Keycode::asciiKey('7'),
			Keycode::asciiKey('8'),
			Keycode::asciiKey('y'),
			Keycode::asciiKey('u'),
			Keycode::asciiKey('i'),
			Keycode::asciiKey('h'),
			Keycode::asciiKey('j'),
			Keycode::asciiKey('k'),
			Keycode::asciiKey('n'),
			Keycode::asciiKey('m'),
			Keycode::asciiKey(','),
		}
	},
	#ifdef CONFIG_MACHINE_PANDORA
	{
		Input::Event::MAP_SYSTEM,
		Input::Device::SUBTYPE_PANDORA_HANDHELD,
		"Default Pandora",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_PANDORA_PROFILE_INIT,

			// JS 1
			Input::Keycode::Pandora::UP,
			Input::Keycode::Pandora::RIGHT,
			Input::Keycode::Pandora::DOWN,
			Input::Keycode::Pandora::LEFT,
			0, 0, 0, 0,
			Input::Keycode::Pandora::X,
			Input::Keycode::Pandora::B,

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Input::Keycode::Pandora::SELECT,
			Input::Keycode::Pandora::START,
			Keycode::asciiKey('a'),
			Keycode::asciiKey('s'),
			Keycode::asciiKey('q'),
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

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Input::Evdev::GAME_SELECT,
			Input::Evdev::GAME_START,
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
		"Default Wiimote",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_WIIMOTE_PROFILE_INIT,

			// JS 1
			Input::Wiimote::UP,
			Input::Wiimote::RIGHT,
			Input::Wiimote::DOWN,
			Input::Wiimote::LEFT,
			0, 0, 0, 0,
			Input::Wiimote::_1,
			Input::Wiimote::_2,

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Input::Wiimote::MINUS,
			Input::Wiimote::PLUS,
			Input::Wiimote::A,
		}
	},
};

const uint defaultWiimoteProfiles = sizeofArray(defaultWiimoteProfile);

const KeyConfig defaultWiiCCProfile[] =
{
	{
		Input::Event::MAP_WII_CC,
		0,
		"Default Classic / Wii U Pro Controller",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_WII_CC_PROFILE_INIT,

			// JS 1
			Input::WiiCC::UP,
			Input::WiiCC::RIGHT,
			Input::WiiCC::DOWN,
			Input::WiiCC::LEFT,
			0, 0, 0, 0,
			Input::WiiCC::Y,
			Input::WiiCC::B,

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Input::WiiCC::MINUS,
			Input::WiiCC::PLUS,
			Input::WiiCC::L,
			Input::WiiCC::R,
			Input::WiiCC::X,
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
		"Default iControlPad",
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

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Input::iControlPad::SELECT,
			Input::iControlPad::START,
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
		"Default iCade",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ICADE_PROFILE_INIT,

			// JS 1
			Input::ICade::UP,
			Input::ICade::RIGHT,
			Input::ICade::DOWN,
			Input::ICade::LEFT,
			0, 0, 0, 0,
			Input::ICade::H,
			Input::ICade::F,

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Input::ICade::A,
			Input::ICade::C,
			Input::ICade::E,
			Input::ICade::G,
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
		"Default Zeemote",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_UNBINDED_PROFILE_INIT,

			// JS 1
			Input::Zeemote::UP,
			Input::Zeemote::RIGHT,
			Input::Zeemote::DOWN,
			Input::Zeemote::LEFT,
			0, 0, 0, 0,
			Input::Zeemote::A,
			Input::Zeemote::B,

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Input::Zeemote::C,
			Input::Zeemote::POWER,
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

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Input::PS3::SELECT,
			Input::PS3::START,
			Input::PS3::SQUARE,
			Input::PS3::TRIANGLE,
			Input::PS3::L1,
		}
	},
};

const uint defaultPS3Profiles = sizeofArray(defaultPS3Profile);

};
