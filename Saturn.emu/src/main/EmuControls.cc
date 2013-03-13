#include <util/preprocessor/repeat.h>
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
			Input::Event::MAP_KEYBOARD,
			"WebOS Keyboard",
			{
					EMU_CONTROLS_IN_GAME_ACTIONS_WEBOS_KB_PROFILE_INIT,

					EMU_CONTROLS_WEBOS_KB_8WAY_DIRECTION_PROFILE_INIT,
					asciiKey(' '),
					Input::Keycode::ENTER,
					asciiKey(','),
					asciiKey('m'),
					asciiKey('l'),
					asciiKey('k'),
					asciiKey('i'),
					asciiKey('o'),
					0,
					0,
					0,
					0,
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
					0,
					0,
					0,
					0,
					asciiKey(' '),
					Input::Keycode::ENTER,
					asciiKey('x'),
					asciiKey('z'),
					asciiKey('s'),
					asciiKey('a'),
					asciiKey('q'),
					asciiKey('w'),
					asciiKey('v'),
					asciiKey('c'),
					asciiKey('f'),
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
					0,
					0,
					0,
					0,
					Input::Keycode::GAME_START,
					Input::Keycode::GAME_B,
					Input::Keycode::CENTER,
					Input::Keycode::GAME_Y,
					Input::Keycode::GAME_X,
					Input::Keycode::GAME_L1,
					Input::Keycode::GAME_R1,
					0,
					0,
					0,
					0,
			}
	},
	{
			Input::Event::MAP_KEYBOARD,
			"PS3 Controller",
			{
					EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_PS3_GAMEPAD_MINIMAL_PROFILE_INIT,

					// JS 1
					Input::Keycode::UP,
					Input::Keycode::RIGHT,
					Input::Keycode::DOWN,
					Input::Keycode::LEFT,
					0,
					0,
					0,
					0,
					Input::Keycode::GAME_START,
					Input::Keycode::GAME_X,
					Input::Keycode::GAME_Y,
					Input::Keycode::GAME_R1,
					Input::Keycode::GAME_A,
					Input::Keycode::GAME_B,
					Input::Keycode::GAME_L1,
					Input::Keycode::GAME_L2,
					Input::Keycode::GAME_R2,
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
					0,
					0,
					0,
					0,
					Input::Keycode::ENTER,
					asciiKey('z'),
					asciiKey('x'),
					asciiKey('c'),
					asciiKey('a'),
					asciiKey('s'),
					asciiKey('d'),
					asciiKey('q'),
					asciiKey('w'),
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
					0,
					0,
					0,
					0,
					Input::Wiimote::MINUS,
					Input::Wiimote::PLUS,
					Input::Wiimote::A,
					Input::Wiimote::_2,
					Input::Wiimote::B,
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
					Input::iControlPad::B,
					Input::iControlPad::X,
					Input::iControlPad::Y,
					Input::iControlPad::A,
					Input::iControlPad::L,
					Input::iControlPad::R,
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
					Input::ICade::G,
					Input::ICade::H,
					Input::ICade::E,
					Input::ICade::F,
					Input::ICade::B,
					Input::ICade::D,
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
			Input::Event::MAP_ZEEMOTE,
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
					0,
					Input::Zeemote::POWER,
					Input::Zeemote::A,
					Input::Zeemote::B,
					Input::Zeemote::C,
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

};
