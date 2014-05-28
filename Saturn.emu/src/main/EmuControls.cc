#include <imagine/util/preprocessor/repeat.h>
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
	"Start",
	"A",
	"B",
	"C",
	"X",
	"Y",
	"Z",
	"L",
	"R",
	"Turbo A",
	"Turbo B",
	"Turbo C",
	"Turbo X",
	"Turbo Y",
	"Turbo Z"
};

static const uint gamepadKeyOffset = gameActionKeys;
static const uint gamepad2KeyOffset = gamepadKeyOffset + gamepadKeys;

const KeyCategory category[categories]
{
	EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
	KeyCategory("Gamepad", gamepadName, gamepadKeyOffset),
	KeyCategory("Gamepad 2", gamepadName, gamepad2KeyOffset, 1)
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

			EMU_CONTROLS_WEBOS_KB_8WAY_DIRECTION_PROFILE_INIT,
			Keycode::asciiKey(' '),
			Input::Keycode::ENTER,
			Keycode::asciiKey(','),
			Keycode::asciiKey('m'),
			Keycode::asciiKey('l'),
			Keycode::asciiKey('k'),
			Keycode::asciiKey('i'),
			Keycode::asciiKey('o'),
			0,
			0,
			0,
			0,
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

			Input::Keycode::UP,
			Input::Keycode::RIGHT,
			Input::Keycode::DOWN,
			Input::Keycode::LEFT,
			0, 0, 0, 0,
			Input::Keycode::GAME_START,
			Input::Keycode::GAME_A,
			Input::Keycode::GAME_B,
			Input::Keycode::GAME_R1,
			Input::Keycode::GAME_X,
			Input::Keycode::GAME_Y,
			Input::Keycode::GAME_L1,
			Input::Keycode::GAME_LEFT_THUMB,
			Input::Keycode::GAME_RIGHT_THUMB,
		}
	},
	{
		Input::Event::MAP_SYSTEM,
		Input::Device::SUBTYPE_PS3_CONTROLLER,
		"PS3 Controller",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_PS3_GAMEPAD_MINIMAL_PROFILE_INIT,

			Input::Keycode::PS3::UP,
			Input::Keycode::PS3::RIGHT,
			Input::Keycode::PS3::DOWN,
			Input::Keycode::PS3::LEFT,
			0, 0, 0, 0,
			Input::Keycode::PS3::START,
			Input::Keycode::PS3::CROSS,
			Input::Keycode::PS3::CIRCLE,
			Input::Keycode::PS3::R1,
			Input::Keycode::PS3::SQUARE,
			Input::Keycode::PS3::TRIANGLE,
			Input::Keycode::PS3::L1,
			Input::Keycode::PS3::L2,
			Input::Keycode::PS3::R2,
		}
	},
	{
		Input::Event::MAP_SYSTEM,
		Input::Device::SUBTYPE_OUYA_CONTROLLER,
		"OUYA Controller",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_OUYA_MINIMAL_PROFILE_INIT,

			Input::Keycode::Ouya::UP,
			Input::Keycode::Ouya::RIGHT,
			Input::Keycode::Ouya::DOWN,
			Input::Keycode::Ouya::LEFT,
			0, 0, 0, 0,
			Input::Keycode::Ouya::R3,
			Input::Keycode::Ouya::O,
			Input::Keycode::Ouya::A,
			Input::Keycode::Ouya::R1,
			Input::Keycode::Ouya::U,
			Input::Keycode::Ouya::Y,
			Input::Keycode::Ouya::L1,
			Input::Keycode::Ouya::L2,
			Input::Keycode::Ouya::R2,
		}
	},
		#ifdef CONFIG_MACHINE_GENERIC_ARMV7
		{
			Input::Event::MAP_SYSTEM,
			Input::Device::SUBTYPE_XPERIA_PLAY,
			"Xperia Play",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

				Input::Keycode::XperiaPlay::UP,
				Input::Keycode::XperiaPlay::RIGHT,
				Input::Keycode::XperiaPlay::DOWN,
				Input::Keycode::XperiaPlay::LEFT,
				0, 0, 0, 0,
				Input::Keycode::XperiaPlay::START,
				Input::Keycode::XperiaPlay::CROSS,
				Input::Keycode::XperiaPlay::CIRCLE,
				0,
				Input::Keycode::XperiaPlay::SQUARE,
				Input::Keycode::XperiaPlay::TRIANGLE,
				0,
				Input::Keycode::XperiaPlay::L1,
				Input::Keycode::XperiaPlay::R1,
			}
		},
		{
			Input::Event::MAP_SYSTEM,
			Input::Device::SUBTYPE_MOTO_DROID_KEYBOARD,
			"Droid/Milestone Keyboard",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

				Input::Keycode::UP,
				Input::Keycode::RIGHT,
				Input::Keycode::DOWN,
				Input::Keycode::LEFT,
				0, 0, 0, 0,
				Keycode::asciiKey(' '),
				Input::Keycode::ENTER,
				Keycode::asciiKey('x'),
				Keycode::asciiKey('z'),
				Keycode::asciiKey('s'),
				Keycode::asciiKey('a'),
				Keycode::asciiKey('q'),
				Keycode::asciiKey('w'),
				Keycode::asciiKey('v'),
				Keycode::asciiKey('c'),
				Keycode::asciiKey('f'),
				Keycode::asciiKey('d'),
			}
		},
		{
			Input::Event::MAP_SYSTEM,
			Input::Device::SUBTYPE_NVIDIA_SHIELD,
			"NVidia Shield",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_NVIDIA_SHIELD_MINIMAL_PROFILE_INIT,

				Input::Keycode::UP,
				Input::Keycode::RIGHT,
				Input::Keycode::DOWN,
				Input::Keycode::LEFT,
				0, 0, 0, 0,
				Input::Keycode::GAME_START,
				Input::Keycode::GAME_A,
				Input::Keycode::GAME_B,
				Input::Keycode::GAME_R1,
				Input::Keycode::GAME_X,
				Input::Keycode::GAME_Y,
				Input::Keycode::GAME_L1,
				Input::Keycode::GAME_LEFT_THUMB,
				Input::Keycode::GAME_RIGHT_THUMB,
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

			Input::Keycode::UP,
			Input::Keycode::RIGHT,
			Input::Keycode::DOWN,
			Input::Keycode::LEFT,
			0, 0, 0, 0,
			Input::Keycode::ENTER,
			Keycode::asciiKey('z'),
			Keycode::asciiKey('x'),
			Keycode::asciiKey('c'),
			Keycode::asciiKey('a'),
			Keycode::asciiKey('s'),
			Keycode::asciiKey('d'),
			Keycode::asciiKey('q'),
			Keycode::asciiKey('w'),
		}
	},
	#ifdef CONFIG_MACHINE_PANDORA
	{
		Input::Event::MAP_SYSTEM,
		Input::Device::SUBTYPE_PANDORA_HANDHELD,
		"Default Pandora",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_OPEN_PANDORA_PROFILE_INIT,

			Input::Keycode::Pandora::UP,
			Input::Keycode::Pandora::RIGHT,
			Input::Keycode::Pandora::DOWN,
			Input::Keycode::Pandora::LEFT,
			0, 0, 0, 0,
			Input::Keycode::Pandora::START,
			Input::Keycode::Pandora::X,
			Input::Keycode::Pandora::B,
			Keycode::asciiKey('p'),
			Input::Keycode::Pandora::A,
			Input::Keycode::Pandora::Y,
			Keycode::asciiKey('o'),
			Input::Keycode::Pandora::L1,
			Input::Keycode::Pandora::R1,
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
			Input::Evdev::UP,
			Input::Evdev::RIGHT,
			Input::Evdev::DOWN,
			Input::Evdev::LEFT,
			0, 0, 0, 0,
			Input::Evdev::GAME_START,
			Input::Evdev::GAME_A,
			Input::Evdev::GAME_B,
			Input::Evdev::GAME_C,
			Input::Evdev::GAME_X,
			Input::Evdev::GAME_Y,
			Input::Evdev::GAME_Z,
			Input::Evdev::GAME_L1,
			Input::Evdev::GAME_R1,
		}
	},
};

const uint defaultEvdevProfiles = sizeofArray(defaultEvdevProfile);

#endif

#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER

const KeyConfig defaultAppleGCProfile[] =
{
	{
		Input::Event::MAP_APPLE_GAME_CONTROLLER,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_APPLEGC_MINIMAL_PROFILE_INIT,
			Input::AppleGC::UP,
			Input::AppleGC::RIGHT,
			Input::AppleGC::DOWN,
			Input::AppleGC::LEFT,
			0, 0, 0, 0,
			Input::AppleGC::RSTICK_RIGHT,
			Input::AppleGC::A,
			Input::AppleGC::B,
			Input::AppleGC::R1,
			Input::AppleGC::Y,
			Input::AppleGC::X,
			Input::AppleGC::L1,
			Input::AppleGC::L2,
			Input::AppleGC::R2,
		}
	},
};

const uint defaultAppleGCProfiles = sizeofArray(defaultAppleGCProfile);

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
			Input::Wiimote::MINUS,
			Input::Wiimote::PLUS,
			Input::Wiimote::A,
			Input::Wiimote::_2,
			Input::Wiimote::B,
			Input::Wiimote::_1,
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
			Input::WiiCC::MINUS,
			Input::WiiCC::PLUS,
			Input::WiiCC::A,
			Input::WiiCC::B,
			Input::WiiCC::X,
			Input::WiiCC::Y,
			Input::WiiCC::L,
			Input::WiiCC::R,
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
			Input::iControlPad::SELECT,
			Input::iControlPad::START,
			Input::iControlPad::B,
			Input::iControlPad::X,
			Input::iControlPad::Y,
			Input::iControlPad::A,
			Input::iControlPad::L,
			Input::iControlPad::R,
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
			Input::ICade::A,
			Input::ICade::C,
			Input::ICade::G,
			Input::ICade::H,
			Input::ICade::E,
			Input::ICade::F,
			Input::ICade::B,
			Input::ICade::D,
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
			0,
			Input::Zeemote::POWER,
			Input::Zeemote::A,
			Input::Zeemote::B,
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
			EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_PS3PAD_ALT_MINIMAL_PROFILE_INIT,

			Input::PS3::UP,
			Input::PS3::RIGHT,
			Input::PS3::DOWN,
			Input::PS3::LEFT,
			0, 0, 0, 0,
			Input::PS3::START,
			Input::PS3::CROSS,
			Input::PS3::CIRCLE,
			Input::PS3::R1,
			Input::PS3::SQUARE,
			Input::PS3::TRIANGLE,
			Input::PS3::L1,
			Input::PS3::L2,
			Input::PS3::R2,
		}
	},
};

const uint defaultPS3Profiles = sizeofArray(defaultPS3Profile);

};
