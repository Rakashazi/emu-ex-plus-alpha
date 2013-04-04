#include <util/preprocessor/repeat.h>
#include <EmuInput.hh>

namespace EmuControls
{

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
		"Run",
		"I",
		"II",
		"Turbo I",
		"Turbo II",
		"III",
		"IV",
		"V",
		"VI",
};

static const uint gamepadKeyOffset = gameActionKeys;
static const uint gamepad2KeyOffset = gamepadKeyOffset + gamepadKeys;
static const uint gamepad3KeyOffset = gamepad2KeyOffset + gamepadKeys;
static const uint gamepad4KeyOffset = gamepad3KeyOffset + gamepadKeys;
static const uint gamepad5KeyOffset = gamepad4KeyOffset + gamepadKeys;

const KeyCategory category[categories]
{
		EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
		KeyCategory("Gamepad", gamepadName, gamepadKeyOffset),
		KeyCategory("Gamepad 2", gamepadName, gamepad2KeyOffset, 1),
		KeyCategory("Gamepad 3", gamepadName, gamepad3KeyOffset, 1),
		KeyCategory("Gamepad 4", gamepadName, gamepad4KeyOffset, 1),
		KeyCategory("Gamepad 5", gamepadName, gamepad5KeyOffset, 1)
};

#ifdef INPUT_SUPPORTS_KEYBOARD

const KeyConfig defaultKeyProfile[] =
{
#ifdef CONFIG_ENV_WEBOS
	{
		Input::Event::MAP_KEYBOARD,
		0,
		"WebOS Keyboard",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_WEBOS_KB_PROFILE_INIT,

			EMU_CONTROLS_WEBOS_KB_8WAY_DIRECTION_PROFILE_INIT,
			asciiKey(' '),
			Input::Keycode::ENTER,
			asciiKey(','),
			asciiKey('m'),
			asciiKey('i'),
			asciiKey('o'),
			asciiKey('n'),
			asciiKey('j'),
			asciiKey('k'),
			asciiKey('l'),
		}
	},
#endif
#ifdef CONFIG_BASE_ANDROID
	KEY_CONFIG_ANDROID_NAV_KEYS,
	{
		Input::Event::MAP_KEYBOARD,
		Input::Device::SUBTYPE_PS3_CONTROLLER,
		"PS3 Controller",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_PS3_GAMEPAD_PROFILE_INIT,

			Input::Keycode::UP,
			Input::Keycode::RIGHT,
			Input::Keycode::DOWN,
			Input::Keycode::LEFT,
			0,
			0,
			0,
			0,
			Input::Keycode::GAME_SELECT,
			Input::Keycode::GAME_START,
			Input::Keycode::GAME_Y,
			Input::Keycode::GAME_X,
		}
	},
	{
		Input::Event::MAP_KEYBOARD,
		Input::Device::SUBTYPE_OUYA_CONTROLLER,
		"OUYA Controller",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

			Input::Keycode::UP,
			Input::Keycode::RIGHT,
			Input::Keycode::DOWN,
			Input::Keycode::LEFT,
			0,
			0,
			0,
			0,
			Input::Keycode::GAME_LEFT_THUMB,
			Input::Keycode::GAME_RIGHT_THUMB,
			Input::Ouya::A,
			Input::Ouya::O,
		}
	},
	#ifdef CONFIG_MACHINE_GENERIC_ARMV7
		{
			Input::Event::MAP_KEYBOARD,
			Input::Device::SUBTYPE_XPERIA_PLAY,
			"Xperia Play",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

				Input::Keycode::UP,
				Input::Keycode::RIGHT,
				Input::Keycode::DOWN,
				Input::Keycode::LEFT,
				0,
				0,
				0,
				0,
				Input::Keycode::GAME_SELECT,
				Input::Keycode::GAME_START,
				Input::Keycode::GAME_B,
				Input::Keycode::CENTER,
				0,
				0,
				0,
				0,
				0,
				0,
			}
		},
		{
			Input::Event::MAP_KEYBOARD,
			Input::Device::SUBTYPE_MOTO_DROID_KEYBOARD,
			"Droid/Milestone Keyboard",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

				Input::Keycode::UP,
				Input::Keycode::RIGHT,
				Input::Keycode::DOWN,
				Input::Keycode::LEFT,
				0,
				0,
				0,
				0,
				asciiKey(' '),
				Input::Keycode::ENTER,
				asciiKey('c'),
				asciiKey('x'),
				asciiKey('r'),
				asciiKey('e'),
				asciiKey('z'),
				asciiKey('s'),
				asciiKey('d'),
				asciiKey('f'),
			}
		},
	#endif
#endif
	{
		Input::Event::MAP_KEYBOARD,
		0,
		"PC Keyboard",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_PROFILE_INIT,

			Input::Keycode::UP,
			Input::Keycode::RIGHT,
			Input::Keycode::DOWN,
			Input::Keycode::LEFT,
			0,
			0,
			0,
			0,
			asciiKey(' '),
			Input::Keycode::ENTER,
			asciiKey('c'),
			asciiKey('x'),
			asciiKey('r'),
			asciiKey('e'),
			asciiKey('z'),
			asciiKey('s'),
			asciiKey('d'),
			asciiKey('f'),
		}
	},
};

const uint defaultKeyProfiles = sizeofArray(defaultKeyProfile);

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
				0,
				0,
				0,
				0,
				Input::Wiimote::MINUS,
				Input::Wiimote::PLUS,
				Input::Wiimote::_2,
				Input::Wiimote::_1,
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
		Input::Event::MAP_WII_CC,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_WII_CC_PROFILE_INIT,

			Input::WiiCC::UP,
			Input::WiiCC::RIGHT,
			Input::WiiCC::DOWN,
			Input::WiiCC::LEFT,
			0,
			0,
			0,
			0,
			Input::WiiCC::MINUS,
			Input::WiiCC::PLUS,
			Input::WiiCC::B,
			Input::WiiCC::Y,
			Input::WiiCC::A,
			Input::WiiCC::X,
		}
	},
	{
		Input::Event::MAP_WII_CC,
		0,
		"Default (6-button)",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_WII_CC_PROFILE_INIT,

			Input::WiiCC::UP,
			Input::WiiCC::RIGHT,
			Input::WiiCC::DOWN,
			Input::WiiCC::LEFT,
			0,
			0,
			0,
			0,
			Input::WiiCC::MINUS,
			Input::WiiCC::PLUS,
			Input::WiiCC::R,
			Input::WiiCC::A,
			0,
			0,
			Input::WiiCC::B,
			Input::WiiCC::Y,
			Input::WiiCC::X,
			Input::WiiCC::L,
		}
	}
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
				0,
				0,
				0,
				0,
				Input::iControlPad::SELECT,
				Input::iControlPad::START,
				Input::iControlPad::X,
				Input::iControlPad::A,
				0,
				0,
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
			Input::Event::MAP_ICADE,
			0,
			"Default",
			{
					EMU_CONTROLS_IN_GAME_ACTIONS_UNBINDED_PROFILE_INIT,

				Input::ICade::UP,
				Input::ICade::RIGHT,
				Input::ICade::DOWN,
				Input::ICade::LEFT,
				0,
				0,
				0,
				0,
				Input::ICade::A,
				Input::ICade::C,
				Input::ICade::H,
				Input::ICade::F,
				0,
				0
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
				0,
				0,
				0,
				0,
				Input::Zeemote::C,
				Input::Zeemote::POWER,
				Input::Zeemote::B,
				Input::Zeemote::A,
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

// TODO: PS3 port
//EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_PS3PAD_PROFILE_INIT,
//
//Input::Ps3::UP,
//Input::Ps3::RIGHT,
//Input::Ps3::DOWN,
//Input::Ps3::LEFT,
//0,
//0,
//0,
//0,
//Input::Ps3::SELECT,
//Input::Ps3::START,
//Input::Ps3::CIRCLE,
//Input::Ps3::CROSS,
//Input::Ps3::TRIANGLE,
//Input::Ps3::SQUARE,
//0,
//0,
//0,
//0,

};
