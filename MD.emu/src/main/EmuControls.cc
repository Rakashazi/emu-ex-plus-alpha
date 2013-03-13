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
		"Mode",
		"Start",
		"A",
		"B",
		"C",
		"X",
		"Y",
		"Z",
		"Turbo A",
		"Turbo B",
		"Turbo C",
		"Turbo X",
		"Turbo Y",
		"Turbo Z",
};

static const uint gamepadKeyOffset = gameActionKeys;
static const uint gamepad2KeyOffset = gamepadKeyOffset + gamepadKeys;
static const uint gamepad3KeyOffset = gamepad2KeyOffset + gamepadKeys;
static const uint gamepad4KeyOffset = gamepad3KeyOffset + gamepadKeys;

const KeyCategory category[categories]
{
	EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
	KeyCategory("Gamepad", gamepadName, gameActionKeys),
	KeyCategory("Gamepad 2", gamepadName, gamepad2KeyOffset, 1),
	KeyCategory("Gamepad 3", gamepadName, gamepad3KeyOffset, 1),
	KeyCategory("Gamepad 4", gamepadName, gamepad4KeyOffset, 1)
};

#ifdef INPUT_SUPPORTS_KEYBOARD

const KeyConfig defaultKeyProfile[] =
{
#ifdef CONFIG_ENV_WEBOS
	{
			Input::Event::MAP_KEYBOARD,
			"WebOS Keyboard",
			{
					EMU_CONTROLS_IN_GAME_ACTIONS_WEBOS_KB_PROFILE_INIT,

					EMU_CONTROLS_WEBOS_KB_8WAY_DIRECTION_PROFILE_INIT,
					asciiKey(' '),
					Input::Keycode::ENTER,
					asciiKey('n'),
					asciiKey('m'),
					asciiKey(','),
					asciiKey('j'),
					asciiKey('k'),
					asciiKey('l'),
			}
	},
#endif
#ifdef CONFIG_BASE_ANDROID
	{
			Input::Event::MAP_KEYBOARD,
			"Android Nav + Keyboard",
			{
					EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

					Input::Keycode::UP,
					Input::Keycode::RIGHT,
					Input::Keycode::DOWN,
					Input::Keycode::LEFT,
					0, 0, 0, 0,
					asciiKey(' '),
					Input::Keycode::ENTER,
					asciiKey('z'),
					asciiKey('x'),
					asciiKey('c'),
					asciiKey('a'),
					asciiKey('s'),
					asciiKey('d'),
			}
	},
	{
			Input::Event::MAP_KEYBOARD,
			"Xperia Play",
			{
					EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

					Input::Keycode::UP,
					Input::Keycode::RIGHT,
					Input::Keycode::DOWN,
					Input::Keycode::LEFT,
					0, 0, 0, 0,
					Input::Keycode::GAME_SELECT,
					Input::Keycode::GAME_START,
					Input::Keycode::GAME_X,
					Input::Keycode::CENTER,
					Input::Keycode::GAME_B,
					Input::Keycode::GAME_Y,
					Input::Keycode::GAME_L1,
					Input::Keycode::GAME_R1,
			}
	},
	{
			Input::Event::MAP_KEYBOARD,
			"PS3 Controller",
			{
					EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_PS3_GAMEPAD_PROFILE_INIT,

					Input::Keycode::UP,
					Input::Keycode::RIGHT,
					Input::Keycode::DOWN,
					Input::Keycode::LEFT,
					0, 0, 0, 0,
					Input::Keycode::GAME_SELECT,
					Input::Keycode::GAME_START,
					Input::Keycode::GAME_A,
					Input::Keycode::GAME_X,
					Input::Keycode::GAME_Y,
					Input::Keycode::GAME_B,
					Input::Keycode::GAME_L1,
					Input::Keycode::GAME_R1,
			}
	},
#endif
	{
			Input::Event::MAP_KEYBOARD,
			"Default Keyboard",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_PROFILE_INIT,

				Input::Keycode::UP,
				Input::Keycode::RIGHT,
				Input::Keycode::DOWN,
				Input::Keycode::LEFT,
				0, 0, 0, 0,
				asciiKey(' '),
				Input::Keycode::ENTER,
				asciiKey('z'),
				asciiKey('x'),
				asciiKey('c'),
				asciiKey('a'),
				asciiKey('s'),
				asciiKey('d'),
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
					Input::Wiimote::_1,
					Input::Wiimote::_2,
			}
	},
};

const uint defaultWiimoteProfiles = sizeofArray(defaultWiimoteProfile);

const KeyConfig defaultWiiCCProfile[] =
{
	{
		Input::Event::MAP_WII_CC,
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
			Input::WiiCC::Y,
			Input::WiiCC::B,
			Input::WiiCC::A,
			0, 0, 0,
			Input::WiiCC::L,
			Input::WiiCC::R,
			Input::WiiCC::X,
		}
	},
	{
		Input::Event::MAP_WII_CC,
		"Default (6-button)",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_WII_CC_PROFILE_INIT,

			Input::WiiCC::UP,
			Input::WiiCC::RIGHT,
			Input::WiiCC::DOWN,
			Input::WiiCC::LEFT,
			0, 0, 0, 0,
			Input::WiiCC::MINUS,
			Input::WiiCC::PLUS,
			Input::WiiCC::B,
			Input::WiiCC::A,
			Input::WiiCC::R,
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
					Input::iControlPad::A,
					Input::iControlPad::X,
					Input::iControlPad::B,
					Input::iControlPad::Y,
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
					Input::ICade::F,
					Input::ICade::H,
					Input::ICade::G,
					Input::ICade::E,
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

};
