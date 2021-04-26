/*  This file is part of 2600.emu.

	2600.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	2600.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with 2600.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/util/preprocessor/repeat.h>
#include <emuframework/EmuInput.hh>

namespace EmuControls
{

const unsigned categories = 6;
static constexpr unsigned joystickKeys = 12;
static constexpr unsigned switchKeys = 5;
static constexpr unsigned keyboardKeys = 12;
const unsigned systemTotalKeys = gameActionKeys + joystickKeys*2 + switchKeys + keyboardKeys*2;

void transposeKeysForPlayer(KeyConfig::KeyArray &key, unsigned player)
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
	"Trigger 2",
	"Trigger 2 Turbo",
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

static const unsigned joystickKeyOffset = gameActionKeys;
static const unsigned joystick2KeyOffset = joystickKeyOffset + joystickKeys;
static const unsigned switchKeyOffset = joystick2KeyOffset + joystickKeys;
static const unsigned keyboardKeyOffset = switchKeyOffset + switchKeys;
static const unsigned keyboard2KeyOffset = keyboardKeyOffset + keyboardKeys;

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
		Map::SYSTEM,
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
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

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
		Map::SYSTEM,
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
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Switches
			Keycode::GAME_R1,
			Keycode::GAME_START,
			Keycode::GAME_Y,
			Keycode::GAME_B,
			Keycode::GAME_L1,
		}
	},
	#endif
	#ifdef CONFIG_BASE_ANDROID
	{
		Map::SYSTEM,
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

			// Switches
			Keycode::PS3::SELECT,
			Keycode::PS3::START,
			Keycode::PS3::TRIANGLE,
			Keycode::PS3::CIRCLE,
			Keycode::PS3::L1,
		}
	},
	{
		Map::SYSTEM,
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

			// Switches
			Keycode::Ouya::L3,
			Keycode::Ouya::R3,
			Keycode::Ouya::Y,
			Keycode::Ouya::A,
			Keycode::Ouya::L1,
		}
	},
	{
		Map::SYSTEM,
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

			// Switches
			Keycode::GAME_R1,
			Keycode::GAME_START,
			Keycode::GAME_Y,
			Keycode::GAME_B,
			Keycode::GAME_L1,
		}
	},
	{
		Map::SYSTEM,
		Device::SUBTYPE_8BITDO_SF30_PRO,
		"8Bitdo SF30 Pro",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_8BITDO_SF30_PRO_PROFILE_INIT,

			// JS 1
			Keycode::UP,
			Keycode::RIGHT,
			Keycode::DOWN,
			Keycode::LEFT,
			0, 0, 0, 0,
			Keycode::GAME_B,
			Keycode::GAME_Y,
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Switches
			Keycode::GAME_R1,
			Keycode::GAME_START,
			Keycode::GAME_X,
			Keycode::GAME_A,
			Keycode::GAME_L1,
		}
	},
	{
		Map::SYSTEM,
		Device::SUBTYPE_8BITDO_SN30_PRO_PLUS,
		"8BitDo SN30 Pro+",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_8BITDO_SN30_PRO_PLUS_PROFILE_INIT,

			// JS 1
			Keycode::UP,
			Keycode::RIGHT,
			Keycode::DOWN,
			Keycode::LEFT,
			0, 0, 0, 0,
			Keycode::GAME_B,
			Keycode::GAME_Y,
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Switches
			Keycode::GAME_R1,
			Keycode::GAME_START,
			Keycode::GAME_X,
			Keycode::GAME_A,
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

				// Switches
				Keycode::XperiaPlay::SELECT,
				Keycode::XperiaPlay::START,
				Keycode::XperiaPlay::TRIANGLE,
				Keycode::XperiaPlay::CIRCLE,
				Keycode::XperiaPlay::L1,
			}
		},
		{
			Map::SYSTEM,
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
				0,
				0,

				// JS 2
				PP_ZERO_LIST(12)

				// Switches
				Keycode::SPACE,
				Keycode::ENTER,
				Keycode::S,
				Keycode::D,
			}
		},
		#endif
	#endif
	#ifdef CONFIG_MACHINE_PANDORA
	{
		Map::SYSTEM,
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
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

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

const unsigned defaultKeyProfiles = std::size(defaultKeyProfile);

#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER

const KeyConfig defaultAppleGCProfile[] =
{
	{
		Map::APPLE_GAME_CONTROLLER,
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

			// Switches
			AppleGC::RSTICK_LEFT,
			AppleGC::RSTICK_RIGHT,
			AppleGC::Y,
			AppleGC::B,
			AppleGC::L1,
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
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Switches
			Wiimote::MINUS,
			Wiimote::PLUS,
			Wiimote::A,
		}
	},
};

const unsigned defaultWiimoteProfiles = std::size(defaultWiimoteProfile);

const KeyConfig defaultWiiCCProfile[] =
{
	{
		Map::WII_CC,
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
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Switches
			WiiCC::MINUS,
			WiiCC::PLUS,
			WiiCC::X,
			WiiCC::A,
			WiiCC::L,
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
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Switches
			iControlPad::SELECT,
			iControlPad::START,
			iControlPad::Y,
			iControlPad::B,
			iControlPad::L,
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
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Switches
			ICade::SELECT,
			ICade::START,
			ICade::Y,
			ICade::B,
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
			0,
			0,

			// JS 2
			PP_ZERO_LIST(12)

			// Switches
			Zeemote::C,
			Zeemote::POWER,
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

			// Switches
			PS3::SELECT,
			PS3::START,
			PS3::TRIANGLE,
			PS3::CIRCLE,
			PS3::L1,
		}
	},
};

const unsigned defaultPS3Profiles = std::size(defaultPS3Profile);

};
