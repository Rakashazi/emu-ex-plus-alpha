#include <imagine/util/preprocessor/repeat.h>
#include <EmuInput.hh>

namespace EmuControls
{

void transposeKeysForPlayer(KeyConfig::KeyArray &key, uint player) {}

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
	"Select",
	"Start",
	"A",
	"B",
	"L",
	"R",
	"Turbo A",
	"Turbo B",
	"A+B",
	"R+B",
};

static const uint gamepadKeyOffset = gameActionKeys;

const KeyCategory category[categories]
{
	EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
	KeyCategory("Set Gamepad Keys", gamepadName, gamepadKeyOffset)
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
			Input::Keycode::GAME_SELECT,
			Input::Keycode::GAME_START,
			Input::Keycode::GAME_B,
			Input::Keycode::GAME_A,
			Input::Keycode::GAME_L1,
			Input::Keycode::GAME_R1,
		}
	},
	{
		Input::Event::MAP_SYSTEM,
		Input::Device::SUBTYPE_PS3_CONTROLLER,
		"PS3 Controller",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_PS3_GAMEPAD_PROFILE_INIT,

			Input::Keycode::PS3::UP,
			Input::Keycode::PS3::RIGHT,
			Input::Keycode::PS3::DOWN,
			Input::Keycode::PS3::LEFT,
			0, 0, 0, 0,
			Input::Keycode::PS3::SELECT,
			Input::Keycode::PS3::START,
			Input::Keycode::PS3::CIRCLE,
			Input::Keycode::PS3::CROSS,
			Input::Keycode::PS3::L1,
			Input::Keycode::PS3::R1,
		}
	},
	{
		Input::Event::MAP_SYSTEM,
		Input::Device::SUBTYPE_OUYA_CONTROLLER,
		"OUYA Controller",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_OUYA_PROFILE_INIT,

			Input::Keycode::Ouya::UP,
			Input::Keycode::Ouya::RIGHT,
			Input::Keycode::Ouya::DOWN,
			Input::Keycode::Ouya::LEFT,
			0, 0, 0, 0,
			Input::Keycode::Ouya::L3,
			Input::Keycode::Ouya::R3,
			Input::Keycode::Ouya::O,
			Input::Keycode::Ouya::U,
			Input::Keycode::Ouya::L1,
			Input::Keycode::Ouya::R1,
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
				Input::Keycode::XperiaPlay::SELECT,
				Input::Keycode::XperiaPlay::START,
				Input::Keycode::XperiaPlay::CIRCLE,
				Input::Keycode::XperiaPlay::CROSS,
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
				Keycode::asciiKey('c'),
				Keycode::asciiKey('x'),
				Keycode::asciiKey('d'),
				Keycode::asciiKey('s'),
				Keycode::asciiKey('b'),
				Keycode::asciiKey('v'),
			}
		},
		{
			Input::Event::MAP_SYSTEM,
			Input::Device::SUBTYPE_NVIDIA_SHIELD,
			"NVidia Shield",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_NVIDIA_SHIELD_PROFILE_INIT,

				Input::Keycode::UP,
				Input::Keycode::RIGHT,
				Input::Keycode::DOWN,
				Input::Keycode::LEFT,
				0, 0, 0, 0,
				Input::Keycode::GAME_LEFT_THUMB,
				Input::Keycode::GAME_START,
				Input::Keycode::GAME_B,
				Input::Keycode::GAME_A,
				Input::Keycode::GAME_L1,
				Input::Keycode::GAME_R1,
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
			Keycode::asciiKey(' '),
			Input::Keycode::ENTER,
			Keycode::asciiKey('x'),
			Keycode::asciiKey('z'),
			Keycode::asciiKey('s'),
			Keycode::asciiKey('a'),
			Keycode::asciiKey('v'),
			Keycode::asciiKey('c'),
		}
	},
	#ifdef CONFIG_MACHINE_PANDORA
	{
		Input::Event::MAP_SYSTEM,
		Input::Device::SUBTYPE_PANDORA_HANDHELD,
		"Default Pandora",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_PANDORA_ALT_PROFILE_INIT,

			Input::Keycode::Pandora::UP,
			Input::Keycode::Pandora::RIGHT,
			Input::Keycode::Pandora::DOWN,
			Input::Keycode::Pandora::LEFT,
			0, 0, 0, 0,
			Input::Keycode::Pandora::SELECT,
			Input::Keycode::Pandora::START,
			Input::Keycode::Pandora::B,
			Input::Keycode::Pandora::X,
			Input::Keycode::Pandora::L,
			Input::Keycode::Pandora::R,
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
			Input::Evdev::GAME_SELECT,
			Input::Evdev::GAME_START,
			Input::Evdev::GAME_B,
			Input::Evdev::GAME_A,
			Input::Evdev::GAME_Y,
			Input::Evdev::GAME_X,
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
			EMU_CONTROLS_IN_GAME_ACTIONS_APPLEGC_PROFILE_INIT,
			Input::AppleGC::UP,
			Input::AppleGC::RIGHT,
			Input::AppleGC::DOWN,
			Input::AppleGC::LEFT,
			0, 0, 0, 0,
			Input::AppleGC::RSTICK_LEFT,
			Input::AppleGC::RSTICK_RIGHT,
			Input::AppleGC::A,
			Input::AppleGC::X,
			Input::AppleGC::B,
			Input::AppleGC::Y,
			Input::AppleGC::L1,
			Input::AppleGC::R1,
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
			Input::Wiimote::_2,
			Input::Wiimote::_1,
			Input::Wiimote::B,
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
			Input::Zeemote::POWER,
			Input::Zeemote::A,
			Input::Zeemote::B,
			Input::Zeemote::C,
		}
	},
};

const uint defaultZeemoteProfiles = sizeofArray(defaultZeemoteProfile);

const KeyConfig defaultPS3Profile[] =
{
	{
		Input::Event::MAP_PS3PAD,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_PS3PAD_PROFILE_INIT,

			Input::PS3::UP,
			Input::PS3::RIGHT,
			Input::PS3::DOWN,
			Input::PS3::LEFT,
			0, 0, 0, 0,
			Input::PS3::SELECT,
			Input::PS3::START,
			Input::PS3::CIRCLE,
			Input::PS3::CROSS,
			Input::PS3::L1,
			Input::PS3::R1,
		}
	},
};

const uint defaultPS3Profiles = sizeofArray(defaultPS3Profile);

};
