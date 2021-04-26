/*  This file is part of Saturn.emu.

	Saturn.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Saturn.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Saturn.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/util/preprocessor/repeat.h>
#include <emuframework/EmuInput.hh>
#include "internal.hh"

namespace EmuControls
{

const unsigned categories = 3;
const unsigned systemTotalKeys = gameActionKeys + gamepadKeys*2;

void transposeKeysForPlayer(KeyConfig::KeyArray &key, unsigned player)
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

static const unsigned gamepadKeyOffset = gameActionKeys;
static const unsigned gamepad2KeyOffset = gamepadKeyOffset + gamepadKeys;

const KeyCategory category[MAX_CATEGORIES]
{
	EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
	{"Gamepad", gamepadName, gamepadKeyOffset},
	{"Gamepad 2", gamepadName, gamepad2KeyOffset, true}
};

const KeyConfig defaultKeyProfile[] =
{
	#ifdef CONFIG_BASE_ANDROID
	KEY_CONFIG_ANDROID_NAV_KEYS,
	#endif
	{
		Map::SYSTEM,
		0,
		"PC Keyboard",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_PROFILE_INIT,

			Keycode::UP,
			Keycode::RIGHT,
			Keycode::DOWN,
			Keycode::LEFT,
			0, 0, 0, 0,
			Keycode::ENTER,
			Keycode::Z,
			Keycode::X,
			Keycode::C,
			Keycode::A,
			Keycode::S,
			Keycode::D,
			Keycode::Q,
			Keycode::W,
		}
	},
	#ifdef CONFIG_INPUT_GAMEPAD_DEVICES
	{
		Map::SYSTEM,
		Device::SUBTYPE_GENERIC_GAMEPAD,
		"Generic Gamepad",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_GENERIC_GAMEPAD_PROFILE_INIT,

			Keycode::UP,
			Keycode::RIGHT,
			Keycode::DOWN,
			Keycode::LEFT,
			0, 0, 0, 0,
			Keycode::GAME_START,
			Keycode::GAME_A,
			Keycode::GAME_B,
			Keycode::GAME_R1,
			Keycode::GAME_X,
			Keycode::GAME_Y,
			Keycode::GAME_L1,
			Keycode::GAME_LEFT_THUMB,
			Keycode::GAME_RIGHT_THUMB,
		}
	},
	#endif
	#ifdef CONFIG_ENV_WEBOS
	{
		Map::SYSTEM,
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
		Map::SYSTEM,
		Device::SUBTYPE_PS3_CONTROLLER,
		"PS3 Controller",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_PS3_GAMEPAD_MINIMAL_PROFILE_INIT,

			Keycode::PS3::UP,
			Keycode::PS3::RIGHT,
			Keycode::PS3::DOWN,
			Keycode::PS3::LEFT,
			0, 0, 0, 0,
			Keycode::PS3::START,
			Keycode::PS3::CROSS,
			Keycode::PS3::CIRCLE,
			Keycode::PS3::R1,
			Keycode::PS3::SQUARE,
			Keycode::PS3::TRIANGLE,
			Keycode::PS3::L1,
			Keycode::PS3::L2,
			Keycode::PS3::R2,
		}
	},
	{
		Map::SYSTEM,
		Device::SUBTYPE_OUYA_CONTROLLER,
		"OUYA Controller",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_OUYA_MINIMAL_PROFILE_INIT,

			Keycode::Ouya::UP,
			Keycode::Ouya::RIGHT,
			Keycode::Ouya::DOWN,
			Keycode::Ouya::LEFT,
			0, 0, 0, 0,
			Keycode::Ouya::R3,
			Keycode::Ouya::O,
			Keycode::Ouya::A,
			Keycode::Ouya::R1,
			Keycode::Ouya::U,
			Keycode::Ouya::Y,
			Keycode::Ouya::L1,
			Keycode::Ouya::L2,
			Keycode::Ouya::R2,
		}
	},
	{
		Map::SYSTEM,
		Device::SUBTYPE_NVIDIA_SHIELD,
		"NVidia Shield",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_NVIDIA_SHIELD_MINIMAL_PROFILE_INIT,

			Keycode::UP,
			Keycode::RIGHT,
			Keycode::DOWN,
			Keycode::LEFT,
			0, 0, 0, 0,
			Keycode::GAME_START,
			Keycode::GAME_A,
			Keycode::GAME_B,
			Keycode::GAME_R1,
			Keycode::GAME_X,
			Keycode::GAME_Y,
			Keycode::GAME_L1,
			Keycode::GAME_LEFT_THUMB,
			Keycode::GAME_RIGHT_THUMB,
		}
	},
	{
		Map::SYSTEM,
		Device::SUBTYPE_8BITDO_SF30_PRO,
		"8Bitdo SF30 Pro",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_8BITDO_SF30_PRO_MINIMAL_PROFILE_INIT,

			Keycode::UP,
			Keycode::RIGHT,
			Keycode::DOWN,
			Keycode::LEFT,
			0, 0, 0, 0,
			Keycode::GAME_START,
			Keycode::GAME_B,
			Keycode::GAME_A,
			Keycode::GAME_R1,
			Keycode::GAME_Y,
			Keycode::GAME_X,
			Keycode::GAME_L1,
		}
	},
	{
		Map::SYSTEM,
		Device::SUBTYPE_8BITDO_SN30_PRO_PLUS,
		"8BitDo SN30 Pro+",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_8BITDO_SN30_PRO_PLUS_MINIMAL_PROFILE_INIT,

			Keycode::UP,
			Keycode::RIGHT,
			Keycode::DOWN,
			Keycode::LEFT,
			0, 0, 0, 0,
			Keycode::GAME_START,
			Keycode::GAME_B,
			Keycode::GAME_A,
			Keycode::GAME_R1,
			Keycode::GAME_Y,
			Keycode::GAME_X,
			Keycode::GAME_L1,
		}
	},
		#if __ARM_ARCH == 7
		{
			Map::SYSTEM,
			Device::SUBTYPE_XPERIA_PLAY,
			"Xperia Play",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

				Keycode::XperiaPlay::UP,
				Keycode::XperiaPlay::RIGHT,
				Keycode::XperiaPlay::DOWN,
				Keycode::XperiaPlay::LEFT,
				0, 0, 0, 0,
				Keycode::XperiaPlay::START,
				Keycode::XperiaPlay::CROSS,
				Keycode::XperiaPlay::CIRCLE,
				0,
				Keycode::XperiaPlay::SQUARE,
				Keycode::XperiaPlay::TRIANGLE,
				0,
				Keycode::XperiaPlay::L1,
				Keycode::XperiaPlay::R1,
			}
		},
		{
			Map::SYSTEM,
			Device::SUBTYPE_MOTO_DROID_KEYBOARD,
			"Droid/Milestone Keyboard",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

				Keycode::UP,
				Keycode::RIGHT,
				Keycode::DOWN,
				Keycode::LEFT,
				0, 0, 0, 0,
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
		#endif
	#endif
	#ifdef CONFIG_MACHINE_PANDORA
	{
		Map::SYSTEM,
		Device::SUBTYPE_PANDORA_HANDHELD,
		"Default Pandora",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_OPEN_PANDORA_PROFILE_INIT,

			Keycode::Pandora::UP,
			Keycode::Pandora::RIGHT,
			Keycode::Pandora::DOWN,
			Keycode::Pandora::LEFT,
			0, 0, 0, 0,
			Keycode::Pandora::START,
			Keycode::Pandora::X,
			Keycode::Pandora::B,
			Keycode::P,
			Keycode::Pandora::A,
			Keycode::Pandora::Y,
			Keycode::O,
			Keycode::Pandora::L1,
			Keycode::Pandora::R1,
		}
	},
	#endif
};

const unsigned defaultKeyProfiles = std::size(defaultKeyProfile);

#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER

const KeyConfig defaultAppleGCProfile[] =
{
	{
		Map::APPLE_GAME_CONTROLLER,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_APPLEGC_MINIMAL_PROFILE_INIT,
			AppleGC::UP,
			AppleGC::RIGHT,
			AppleGC::DOWN,
			AppleGC::LEFT,
			0, 0, 0, 0,
			AppleGC::RSTICK_RIGHT,
			AppleGC::A,
			AppleGC::B,
			AppleGC::R1,
			AppleGC::Y,
			AppleGC::X,
			AppleGC::L1,
			AppleGC::L2,
			AppleGC::R2,
		}
	},
};

const unsigned defaultAppleGCProfiles = std::size(defaultAppleGCProfile);

#endif

// Wiimote

const KeyConfig defaultWiimoteProfile[] =
{
	{
		Map::WIIMOTE,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_WIIMOTE_PROFILE_INIT,

			Wiimote::UP,
			Wiimote::RIGHT,
			Wiimote::DOWN,
			Wiimote::LEFT,
			0, 0, 0, 0,
			Wiimote::PLUS,
			Wiimote::A,
			Wiimote::_2,
			Wiimote::B,
			Wiimote::_1,
		}
	},
};

const unsigned defaultWiimoteProfiles = std::size(defaultWiimoteProfile);

const KeyConfig defaultWiiCCProfile[] =
{
	{
		Map::WII_CC,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_WII_CC_PROFILE_INIT,

			WiiCC::UP,
			WiiCC::RIGHT,
			WiiCC::DOWN,
			WiiCC::LEFT,
			0, 0, 0, 0,
			WiiCC::PLUS,
			WiiCC::A,
			WiiCC::B,
			WiiCC::X,
			WiiCC::Y,
			WiiCC::L,
			WiiCC::R,
		}
	},
};

const unsigned defaultWiiCCProfiles = std::size(defaultWiiCCProfile);

// iControlPad

const KeyConfig defaultIControlPadProfile[] =
{
	{
		Map::ICONTROLPAD,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ICP_NUBS_PROFILE_INIT,

			iControlPad::UP,
			iControlPad::RIGHT,
			iControlPad::DOWN,
			iControlPad::LEFT,
			0, 0, 0, 0,
			iControlPad::START,
			iControlPad::B,
			iControlPad::X,
			iControlPad::Y,
			iControlPad::A,
			iControlPad::L,
			iControlPad::R,
		}
	},
};

const unsigned defaultIControlPadProfiles = std::size(defaultIControlPadProfile);

// iCade

const KeyConfig defaultICadeProfile[] =
{
	{
		Map::ICADE,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_UNBINDED_PROFILE_INIT,

			ICade::UP,
			ICade::RIGHT,
			ICade::DOWN,
			ICade::LEFT,
			0, 0, 0, 0,
			ICade::START,
			ICade::A,
			ICade::B,
			ICade::C,
			ICade::X,
			ICade::Y,
			ICade::Z,
		}
	},
};

const unsigned defaultICadeProfiles = std::size(defaultICadeProfile);

// Zeemote

const KeyConfig defaultZeemoteProfile[] =
{
	{
		Map::ZEEMOTE,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_UNBINDED_PROFILE_INIT,

			Zeemote::UP,
			Zeemote::RIGHT,
			Zeemote::DOWN,
			Zeemote::LEFT,
			0, 0, 0, 0,
			0,
			Zeemote::POWER,
			Zeemote::A,
			Zeemote::B,
			Zeemote::C,
		}
	},
};

const unsigned defaultZeemoteProfiles = std::size(defaultZeemoteProfile);

// PS3

const KeyConfig defaultPS3Profile[] =
{
	{
		Map::PS3PAD,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_PS3PAD_ALT_MINIMAL_PROFILE_INIT,

			PS3::UP,
			PS3::RIGHT,
			PS3::DOWN,
			PS3::LEFT,
			0, 0, 0, 0,
			PS3::START,
			PS3::CROSS,
			PS3::CIRCLE,
			PS3::R1,
			PS3::SQUARE,
			PS3::TRIANGLE,
			PS3::L1,
			PS3::L2,
			PS3::R2,
		}
	},
};

const unsigned defaultPS3Profiles = std::size(defaultPS3Profile);

};
