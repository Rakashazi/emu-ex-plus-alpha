#include <imagine/util/preprocessor/repeat.h>
#include <emuframework/EmuInput.hh>

namespace EmuControls
{

const uint categories = 6;
static constexpr uint joystickKeys = 10;
static constexpr uint switchKeys = 5;
static constexpr uint keyboardKeys = 12;
const uint systemTotalKeys = gameActionKeys + joystickKeys*2 + switchKeys + keyboardKeys*2;

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

const KeyCategory category[MAX_CATEGORIES]
{
	EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
	{"Set Joystick Keys", gamepadName, joystickKeyOffset},
	{"Set Joystick 2 Keys", gamepadName, joystick2KeyOffset, true},
	{"Set Console Switch Keys", switchName, switchKeyOffset},
	{"Set Keyboard Keys", keyboardName, keyboardKeyOffset},
	{"Set Keyboard 2 Keys", keyboardName, keyboard2KeyOffset}
};

const KeyConfig defaultKeyProfile[] =
{
	#ifdef CONFIG_BASE_ANDROID
	KEY_CONFIG_ANDROID_NAV_KEYS,
	#endif
	{
		Event::MAP_SYSTEM,
		0,
		"PC Keyboard",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_PROFILE_INIT,

			// JS 1
			Keycode::UP,
			Keycode::RIGHT,
			Keycode::DOWN,
			Keycode::LEFT,
			0, 0, 0, 0,
			Keycode::Z,
			Keycode::X,

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Keycode::SPACE,
			Keycode::ENTER,
			Keycode::A,
			Keycode::S,
			Keycode::Q,

			// KB 1
			Keycode::_3,
			Keycode::_4,
			Keycode::_5,
			Keycode::E,
			Keycode::R,
			Keycode::T,
			Keycode::D,
			Keycode::F,
			Keycode::G,
			Keycode::C,
			Keycode::V,
			Keycode::B,

			// KB 2
			Keycode::_6,
			Keycode::_7,
			Keycode::_8,
			Keycode::Y,
			Keycode::U,
			Keycode::I,
			Keycode::H,
			Keycode::J,
			Keycode::K,
			Keycode::N,
			Keycode::M,
			Keycode::COMMA,
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

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Keycode::GAME_R1,
			Keycode::GAME_START,
			Keycode::GAME_Y,
			Keycode::GAME_B,
			Keycode::GAME_L1,
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

			// JS 1
			EMU_CONTROLS_WEBOS_KB_8WAY_DIRECTION_PROFILE_INIT,
			Keycode::COMMA,
			Keycode::M,

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Keycode::SPACE,
			Keycode::ENTER,
			Keycode::K,
			Keycode::L,
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

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Keycode::PS3::SELECT,
			Keycode::PS3::START,
			Keycode::PS3::TRIANGLE,
			Keycode::PS3::CIRCLE,
			Keycode::PS3::L1,
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

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Keycode::Ouya::L3,
			Keycode::Ouya::R3,
			Keycode::Ouya::Y,
			Keycode::Ouya::A,
			Keycode::Ouya::L1,
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

				// JS 2
				PP_ZERO_LIST(10)

				// Switches
				Keycode::XperiaPlay::SELECT,
				Keycode::XperiaPlay::START,
				Keycode::XperiaPlay::TRIANGLE,
				Keycode::XperiaPlay::CIRCLE,
				Keycode::XperiaPlay::L1,
			}
		},
		{
			Event::MAP_SYSTEM,
			Device::SUBTYPE_MOTO_DROID_KEYBOARD,
			"Droid/Milestone Keyboard",
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

				// JS 2
				PP_ZERO_LIST(10)

				// Switches
				Keycode::SPACE,
				Keycode::ENTER,
				Keycode::S,
				Keycode::D,
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

				// JS 2
				PP_ZERO_LIST(10)

				// Switches
				Keycode::GAME_R1,
				Keycode::GAME_START,
				Keycode::GAME_Y,
				Keycode::GAME_B,
				Keycode::GAME_L1,
			}
		},
		#endif
	#endif
	#ifdef CONFIG_MACHINE_PANDORA
	{
		Event::MAP_SYSTEM,
		Device::SUBTYPE_PANDORA_HANDHELD,
		"Pandora Keys",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_PANDORA_PROFILE_INIT,

			// JS 1
			Keycode::Pandora::UP,
			Keycode::Pandora::RIGHT,
			Keycode::Pandora::DOWN,
			Keycode::Pandora::LEFT,
			0, 0, 0, 0,
			Keycode::Pandora::X,
			Keycode::Pandora::A,

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Keycode::Pandora::SELECT,
			Keycode::Pandora::START,
			Keycode::Pandora::Y,
			Keycode::Pandora::B,
			Keycode::Q,
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

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			AppleGC::RSTICK_LEFT,
			AppleGC::RSTICK_RIGHT,
			AppleGC::Y,
			AppleGC::B,
			AppleGC::L1,
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
		"Default Wiimote",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_WIIMOTE_PROFILE_INIT,

			// JS 1
			Wiimote::UP,
			Wiimote::RIGHT,
			Wiimote::DOWN,
			Wiimote::LEFT,
			0, 0, 0, 0,
			Wiimote::_1,
			Wiimote::_2,

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Wiimote::MINUS,
			Wiimote::PLUS,
			Wiimote::A,
		}
	},
};

const uint defaultWiimoteProfiles = sizeofArray(defaultWiimoteProfile);

const KeyConfig defaultWiiCCProfile[] =
{
	{
		Event::MAP_WII_CC,
		0,
		"Default Classic / Wii U Pro Controller",
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

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			WiiCC::MINUS,
			WiiCC::PLUS,
			WiiCC::X,
			WiiCC::A,
			WiiCC::L,
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
		"Default iControlPad",
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

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			iControlPad::SELECT,
			iControlPad::START,
			iControlPad::Y,
			iControlPad::B,
			iControlPad::L,
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
		"Default iCade",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ICADE_PROFILE_INIT,

			// JS 1
			ICade::UP,
			ICade::RIGHT,
			ICade::DOWN,
			ICade::LEFT,
			0, 0, 0, 0,
			ICade::A,
			ICade::X,

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			ICade::SELECT,
			ICade::START,
			ICade::Y,
			ICade::B,
			ICade::Z,
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
		"Default Zeemote",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_UNBINDED_PROFILE_INIT,

			// JS 1
			Zeemote::UP,
			Zeemote::RIGHT,
			Zeemote::DOWN,
			Zeemote::LEFT,
			0, 0, 0, 0,
			Zeemote::A,
			Zeemote::B,

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Zeemote::C,
			Zeemote::POWER,
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

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			PS3::SELECT,
			PS3::START,
			PS3::TRIANGLE,
			PS3::CIRCLE,
			PS3::L1,
		}
	},
};

const uint defaultPS3Profiles = sizeofArray(defaultPS3Profile);

};
