#include <imagine/util/preprocessor/repeat.h>
#include <emuframework/EmuInput.hh>
#include "EmuConfig.hh"

namespace EmuControls
{

const uint categories = 6;
const uint systemTotalKeys = gameActionKeys + gamepadKeys*5;

void transposeKeysForPlayer(KeyConfig::KeyArray &key, uint player)
{
	genericMultiplayerTranspose(key, player, 1);
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
	"Select",
	"Start",
	"A",
	"B",
	"X",
	"Y",
	"L",
	"R",
	"Turbo A",
	"Turbo B",
	"Turbo X",
	"Turbo Y",
};

static const uint gamepadKeyOffset = gameActionKeys;
static const uint gamepad2KeyOffset = gamepadKeyOffset + gamepadKeys;
static const uint gamepad3KeyOffset = gamepad2KeyOffset + gamepadKeys;
static const uint gamepad4KeyOffset = gamepad3KeyOffset + gamepadKeys;
static const uint gamepad5KeyOffset = gamepad4KeyOffset + gamepadKeys;

const KeyCategory category[MAX_CATEGORIES]
{
	EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
	{"Set Gamepad Keys", gamepadName, gamepadKeyOffset},
	{"Set Gamepad 2 Keys", gamepadName, gamepad2KeyOffset, 1},
	{"Set Gamepad 3 Keys", gamepadName, gamepad3KeyOffset, 1},
	{"Set Gamepad 4 Keys", gamepadName, gamepad4KeyOffset, 1},
	{"Set Gamepad 5 Keys", gamepadName, gamepad5KeyOffset, 1}
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

			Keycode::UP,
			Keycode::RIGHT,
			Keycode::DOWN,
			Keycode::LEFT,
			0,
			0,
			0,
			0,
			Keycode::SPACE,
			Keycode::ENTER,
			Keycode::X,
			Keycode::Z,
			Keycode::S,
			Keycode::A,
			Keycode::Q,
			Keycode::W,
			Keycode::V,
			Keycode::C,
			Keycode::F,
			Keycode::D,
		}
	},
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
			Keycode::GAME_SELECT,
			Keycode::GAME_START,
			Keycode::GAME_B,
			Keycode::GAME_A,
			Keycode::GAME_Y,
			Keycode::GAME_X,
			Keycode::GAME_L1,
			Keycode::GAME_R1,
		}
	},
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
			Keycode::L,
			Keycode::K,
			Keycode::I,
			Keycode::O,
			0,
			0,
			0,
			0,
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

			Keycode::PS3::UP,
			Keycode::PS3::RIGHT,
			Keycode::PS3::DOWN,
			Keycode::PS3::LEFT,
			0, 0, 0, 0,
			Keycode::PS3::SELECT,
			Keycode::PS3::START,
			Keycode::PS3::CIRCLE,
			Keycode::PS3::CROSS,
			Keycode::PS3::TRIANGLE,
			Keycode::PS3::SQUARE,
			Keycode::PS3::L1,
			Keycode::PS3::R1,
		}
	},
	{
		Event::MAP_SYSTEM,
		Device::SUBTYPE_OUYA_CONTROLLER,
		"OUYA Controller",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_OUYA_PROFILE_INIT,

			Keycode::Ouya::UP,
			Keycode::Ouya::RIGHT,
			Keycode::Ouya::DOWN,
			Keycode::Ouya::LEFT,
			0, 0, 0, 0,
			Keycode::Ouya::L3,
			Keycode::Ouya::R3,
			Keycode::Ouya::A,
			Keycode::Ouya::O,
			Keycode::Ouya::Y,
			Keycode::Ouya::U,
			Keycode::Ouya::L1,
			Keycode::Ouya::R1,
		}
	},
		#ifdef CONFIG_MACHINE_GENERIC_ARMV7
		{
			Event::MAP_SYSTEM,
			Device::SUBTYPE_XPERIA_PLAY,
			"Xperia Play",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

				Keycode::XperiaPlay::UP,
				Keycode::XperiaPlay::RIGHT,
				Keycode::XperiaPlay::DOWN,
				Keycode::XperiaPlay::LEFT,
				0, 0, 0, 0,
				Keycode::XperiaPlay::SELECT,
				Keycode::XperiaPlay::START,
				Keycode::XperiaPlay::CIRCLE,
				Keycode::XperiaPlay::CROSS,
				Keycode::XperiaPlay::TRIANGLE,
				Keycode::XperiaPlay::SQUARE,
				Keycode::XperiaPlay::L1,
				Keycode::XperiaPlay::R1,
			}
		},
		{
			Event::MAP_SYSTEM,
			Device::SUBTYPE_MOTO_DROID_KEYBOARD,
			"Droid/Milestone Keyboard",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

				Keycode::UP,
				Keycode::RIGHT,
				Keycode::DOWN,
				Keycode::LEFT,
				0, 0, 0, 0,
				Keycode::SPACE,
				Keycode::ENTER,
				Keycode::C,
				Keycode::X,
				Keycode::D,
				Keycode::S,
				Keycode::E,
				Keycode::W,
				Keycode::B,
				Keycode::V,
				Keycode::G,
				Keycode::F,
			}
		},
		{
			Event::MAP_SYSTEM,
			Device::SUBTYPE_NVIDIA_SHIELD,
			"NVidia Shield",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_NVIDIA_SHIELD_PROFILE_INIT,

				Keycode::UP,
				Keycode::RIGHT,
				Keycode::DOWN,
				Keycode::LEFT,
				0, 0, 0, 0,
				Keycode::GAME_LEFT_THUMB,
				Keycode::GAME_START,
				Keycode::GAME_B,
				Keycode::GAME_A,
				Keycode::GAME_Y,
				Keycode::GAME_X,
				Keycode::GAME_L1,
				Keycode::GAME_R1,
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
			EMU_CONTROLS_IN_GAME_ACTIONS_PANDORA_ALT_PROFILE_INIT,

			Keycode::Pandora::UP,
			Keycode::Pandora::RIGHT,
			Keycode::Pandora::DOWN,
			Keycode::Pandora::LEFT,
			0, 0, 0, 0,
			Keycode::Pandora::SELECT,
			Keycode::Pandora::START,
			Keycode::Pandora::B,
			Keycode::Pandora::X,
			Keycode::Pandora::Y,
			Keycode::Pandora::A,
			Keycode::Pandora::L,
			Keycode::Pandora::R,
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
			AppleGC::UP,
			AppleGC::RIGHT,
			AppleGC::DOWN,
			AppleGC::LEFT,
			0, 0, 0, 0,
			AppleGC::RSTICK_LEFT,
			AppleGC::RSTICK_RIGHT,
			AppleGC::B,
			AppleGC::A,
			AppleGC::Y,
			AppleGC::X,
			AppleGC::L1,
			AppleGC::R1,
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
					0,
					0,
					0,
					0,
					Wiimote::MINUS,
					Wiimote::PLUS,
					Wiimote::A,
					Wiimote::_2,
					Wiimote::B,
					Wiimote::_1,
					0,
					0,
					0,
					0,
					0,
					0,
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
			0,
			0,
			0,
			0,
			WiiCC::MINUS,
			WiiCC::PLUS,
			WiiCC::A,
			WiiCC::B,
			WiiCC::X,
			WiiCC::Y,
			WiiCC::L,
			WiiCC::R,
		}
	}
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
					0,
					0,
					0,
					0,
					iControlPad::SELECT,
					iControlPad::START,
					iControlPad::B,
					iControlPad::X,
					iControlPad::Y,
					iControlPad::A,
					iControlPad::L,
					iControlPad::R,
					0,
					0,
					0,
					0,
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
					0,
					0,
					0,
					0,
					ICade::SELECT,
					ICade::START,
					ICade::B,
					ICade::A,
					ICade::Y,
					ICade::X,
					ICade::Z,
					ICade::C,
					0,
					0,
					0,
					0,
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
					0,
					0,
					0,
					0,
					0,
					Zeemote::POWER,
					Zeemote::A,
					Zeemote::B,
					Zeemote::C,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
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

			PS3::UP,
			PS3::RIGHT,
			PS3::DOWN,
			PS3::LEFT,
			0, 0, 0, 0,
			PS3::SELECT,
			PS3::START,
			PS3::CIRCLE,
			PS3::CROSS,
			PS3::TRIANGLE,
			PS3::SQUARE,
			PS3::L1,
			PS3::R1,
		}
	},
};

const uint defaultPS3Profiles = sizeofArray(defaultPS3Profile);

};
