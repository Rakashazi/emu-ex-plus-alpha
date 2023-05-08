/*  This file is part of Swan.emu.

	Swan.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Swan.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Swan.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/util/preprocessor/repeat.h>
#include <emuframework/EmuInput.hh>

namespace EmuEx::Controls
{

constexpr int gamepadKeys = 27;
const int systemTotalKeys = gameActionKeys + gamepadKeys;

constexpr std::array<const std::string_view, gamepadKeys> gamepadName
{
	"Up X1 ↷ Y2",
	"Right X2 ↷ Y3",
	"Down X3 ↷ Y4",
	"Left X4 ↷ Y1",
	"Left+Up",
	"Right+Up",
	"Right+Down",
	"Left+Down",
	"Start",
	"A ↷ X4",
	"B ↷ X1",
	"Y1 ↷ B",
	"Y2 ↷ A",
	"Y3 ↷ X3",
	"Y4 ↷ X2",
	"Turbo A",
	"Turbo B",
	"Turbo Y1",
	"Turbo Y2",
	"Turbo Y3",
	"Turbo Y4",
	"A",
	"B",
	"Y1 ↷ X2",
	"Y2 ↷ X3",
	"Y3 ↷ X4",
	"Y4 ↷ X1",
};

constexpr int gamepadKeyOffset = gameActionKeys;

constexpr KeyCategory category[]
{
	EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
	{"Set Gamepad Keys", gamepadName, gamepadKeyOffset}
};

std::span<const KeyCategory> categories() { return category; }

const KeyConfig defaultKeyProfile[] =
{
	{
		Map::SYSTEM,
		{"PC Keyboard"},
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_PROFILE_INIT,
			Keycode::UP,
			Keycode::RIGHT,
			Keycode::DOWN,
			Keycode::LEFT,
			0, 0, 0, 0,
			Keycode::ENTER,
			Keycode::X,
			Keycode::Z,
			Keycode::Q,
			Keycode::W,
			Keycode::S,
			Keycode::A,
			Keycode::V,
			Keycode::C,
			0, 0,
			Keycode::F,
			Keycode::D,
		}
	},
	#ifdef CONFIG_INPUT_GAMEPAD_DEVICES
	{
		Map::SYSTEM,
		DeviceSubtype::GENERIC_GAMEPAD,
		{"Generic Gamepad"},
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_GENERIC_GAMEPAD_PROFILE_INIT,
			Keycode::UP,
			Keycode::RIGHT,
			Keycode::DOWN,
			Keycode::LEFT,
			0, 0, 0, 0,
			Keycode::GAME_START,
			Keycode::GAME_A,
			Keycode::GAME_X,
			Keycode::GAME_L1,
			Keycode::GAME_R1,
			Keycode::GAME_B,
			Keycode::GAME_Y,
		}
	},
	{
		Map::SYSTEM,
		DeviceSubtype::PS3_CONTROLLER,
		{"PS3 Controller"},
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_PS3_GAMEPAD_PROFILE_INIT,

			Keycode::PS3::UP,
			Keycode::PS3::RIGHT,
			Keycode::PS3::DOWN,
			Keycode::PS3::LEFT,
			0, 0, 0, 0,
			Keycode::PS3::START,
			Keycode::PS3::CROSS,
			Keycode::PS3::SQUARE,
			Keycode::PS3::L1,
			Keycode::PS3::R1,
			Keycode::PS3::CIRCLE,
			Keycode::PS3::TRIANGLE,
		}
	},
	{
		Map::SYSTEM,
		DeviceSubtype::OUYA_CONTROLLER,
		{"OUYA Controller"},
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_OUYA_PROFILE_INIT,

			Keycode::Ouya::UP,
			Keycode::Ouya::RIGHT,
			Keycode::Ouya::DOWN,
			Keycode::Ouya::LEFT,
			0, 0, 0, 0,
			Keycode::Ouya::R3,
			Keycode::Ouya::O,
			Keycode::Ouya::U,
			Keycode::Ouya::L1,
			Keycode::Ouya::R1,
			Keycode::Ouya::A,
			Keycode::Ouya::Y,
		}
	},
	{
		Map::SYSTEM,
		DeviceSubtype::NVIDIA_SHIELD,
		{"NVidia Shield"},
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_NVIDIA_SHIELD_PROFILE_INIT,

			Keycode::UP,
			Keycode::RIGHT,
			Keycode::DOWN,
			Keycode::LEFT,
			0, 0, 0, 0,
			Keycode::GAME_START,
			Keycode::GAME_A,
			Keycode::GAME_X,
			Keycode::GAME_L1,
			Keycode::GAME_R1,
			Keycode::GAME_B,
			Keycode::GAME_Y,
		}
	},
	{
		Map::SYSTEM,
		DeviceSubtype::_8BITDO_SF30_PRO,
		{"8Bitdo SF30 Pro"},
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_8BITDO_SF30_PRO_PROFILE_INIT,

			Keycode::UP,
			Keycode::RIGHT,
			Keycode::DOWN,
			Keycode::LEFT,
			0, 0, 0, 0,
			Keycode::GAME_START,
			Keycode::GAME_A,
			Keycode::GAME_X,
			Keycode::GAME_L1,
			Keycode::GAME_R1,
			Keycode::GAME_B,
			Keycode::GAME_Y,
		}
	},
	{
		Map::SYSTEM,
		DeviceSubtype::_8BITDO_SN30_PRO_PLUS,
		{"8BitDo SN30 Pro+"},
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_8BITDO_SN30_PRO_PLUS_PROFILE_INIT,

			Keycode::UP,
			Keycode::RIGHT,
			Keycode::DOWN,
			Keycode::LEFT,
			0, 0, 0, 0,
			Keycode::GAME_START,
			Keycode::GAME_A,
			Keycode::GAME_X,
			Keycode::GAME_L1,
			Keycode::GAME_R1,
			Keycode::GAME_B,
			Keycode::GAME_Y,
		}
	},
	#endif
	#if defined(__ANDROID__) && __ARM_ARCH == 7
	{
		Map::SYSTEM,
		DeviceSubtype::XPERIA_PLAY,
		{"Xperia Play"},
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

			Keycode::XperiaPlay::UP,
			Keycode::XperiaPlay::RIGHT,
			Keycode::XperiaPlay::DOWN,
			Keycode::XperiaPlay::LEFT,
			0, 0, 0, 0,
			Keycode::XperiaPlay::START,
			Keycode::XperiaPlay::CROSS,
			Keycode::XperiaPlay::SQUARE,
			Keycode::XperiaPlay::L1,
			Keycode::XperiaPlay::R1,
			Keycode::XperiaPlay::CIRCLE,
			Keycode::XperiaPlay::TRIANGLE,
		}
	}
	#endif
	#ifdef CONFIG_MACHINE_PANDORA
	{
		Map::SYSTEM,
		DeviceSubtype::PANDORA_HANDHELD,
		{"Default Pandora"},
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_PANDORA_PROFILE_INIT,

			Keycode::Pandora::UP,
			Keycode::Pandora::RIGHT,
			Keycode::Pandora::DOWN,
			Keycode::Pandora::LEFT,
			0, 0, 0, 0,
			Keycode::Pandora::START,
			Keycode::Pandora::X,
			Keycode::Pandora::A,
			Keycode::Pandora::L,
			Keycode::Pandora::R,
			Keycode::Pandora::B,
			Keycode::Pandora::Y,
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
		{"Default"},
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_APPLEGC_PROFILE_INIT,
			AppleGC::UP,
			AppleGC::RIGHT,
			AppleGC::DOWN,
			AppleGC::LEFT,
			0, 0, 0, 0,
			AppleGC::RSTICK_RIGHT,
			AppleGC::X,
			AppleGC::A,
			AppleGC::L,
			AppleGC::R,
			AppleGC::B,
			AppleGC::Y,
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
		{"Default"},
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_WIIMOTE_PROFILE_INIT,
			Wiimote::UP,
			Wiimote::RIGHT,
			Wiimote::DOWN,
			Wiimote::LEFT,
			0, 0, 0, 0,
			Wiimote::PLUS,
			Wiimote::_2,
			Wiimote::_1,
		}
	},
};

const unsigned defaultWiimoteProfiles = std::size(defaultWiimoteProfile);

const KeyConfig defaultWiiCCProfile[] =
{
	{
		Map::WII_CC,
		{"Default"},
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_WII_CC_PROFILE_INIT,
			WiiCC::UP,
			WiiCC::RIGHT,
			WiiCC::DOWN,
			WiiCC::LEFT,
			0, 0, 0, 0,
			WiiCC::PLUS,
			WiiCC::B,
			WiiCC::Y,
			WiiCC::L,
			WiiCC::R,
			WiiCC::A,
			WiiCC::X,
		}
	},
};

const unsigned defaultWiiCCProfiles = std::size(defaultWiiCCProfile);

// iControlPad

const KeyConfig defaultIControlPadProfile[] =
{
	{
		Map::ICONTROLPAD,
		{"Default"},
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ICP_NUBS_PROFILE_INIT,
			iControlPad::UP,
			iControlPad::RIGHT,
			iControlPad::DOWN,
			iControlPad::LEFT,
			0, 0, 0, 0,
			iControlPad::START,
			iControlPad::X,
			iControlPad::A,
			iControlPad::L,
			iControlPad::R,
			iControlPad::B,
			iControlPad::Y,
		}
	},
};

const unsigned defaultIControlPadProfiles = std::size(defaultIControlPadProfile);

// iCade

const KeyConfig defaultICadeProfile[] =
{
	{
		Map::ICADE,
		{"Default"},
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_UNBINDED_PROFILE_INIT,
			ICade::UP,
			ICade::RIGHT,
			ICade::DOWN,
			ICade::LEFT,
			0, 0, 0, 0,
			ICade::START,
			ICade::A,
			ICade::X,
			ICade::C,
			ICade::Z,
			ICade::B,
			ICade::Y,
		}
	},
};

const unsigned defaultICadeProfiles = std::size(defaultICadeProfile);

// Zeemote

const KeyConfig defaultZeemoteProfile[] =
{
	{
		Map::ZEEMOTE,
		{"Default"},
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_UNBINDED_PROFILE_INIT,
			Zeemote::UP,
			Zeemote::RIGHT,
			Zeemote::DOWN,
			Zeemote::LEFT,
			0, 0, 0, 0,
			Zeemote::POWER,
			Zeemote::B,
			Zeemote::A,
		}
	},
};

const unsigned defaultZeemoteProfiles = std::size(defaultZeemoteProfile);

// PS3

const KeyConfig defaultPS3Profile[] =
{
	{
		Map::PS3PAD,
		{"Default"},
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_PS3PAD_PROFILE_INIT,

			PS3::UP,
			PS3::RIGHT,
			PS3::DOWN,
			PS3::LEFT,
			0, 0, 0, 0,
			PS3::START,
			PS3::CROSS,
			PS3::SQUARE,
			PS3::L1,
			PS3::R1,
			PS3::CIRCLE,
			PS3::TRIANGLE,
		}
	},
};

const unsigned defaultPS3Profiles = std::size(defaultPS3Profile);

};
