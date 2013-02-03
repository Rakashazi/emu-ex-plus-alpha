#include <util/preprocessor/repeat.h>
#include <EmuInput.hh>

namespace EmuControls
{

void transposeKeysForPlayer(KeyConfig::KeyArray &key, uint player)
{

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
		"Option",
		"A",
		"B",
		"Turbo A",
		"Turbo B",
};

static const uint gamepadKeyOffset = gameActionKeys;

const KeyCategory category[categories]
{
		EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
		KeyCategory("Gamepad", gamepadName, gamepadKeyOffset)
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
					Input::Keycode::ENTER,
					asciiKey('m'),
					asciiKey(','),
					asciiKey('i'),
					asciiKey('o'),
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
					Input::Keycode::ENTER,
					asciiKey('x'),
					asciiKey('c'),
					asciiKey('d'),
					asciiKey('f')
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
				Input::Keycode::CENTER,
				Input::Keycode::GAME_B,
				Input::Keycode::GAME_X,
				Input::Keycode::GAME_Y,
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
					0,
					0,
					0,
					0,
					Input::Keycode::GAME_START,
					Input::Keycode::GAME_X,
					Input::Keycode::GAME_Y,
					Input::Keycode::GAME_A,
					Input::Keycode::GAME_B,
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
				asciiKey('x'),
				asciiKey('c'),
				asciiKey('d'),
				asciiKey('f')
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
			"Default Wiimote",
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
				Input::Wiimote::PLUS,
				Input::Wiimote::_2,
				Input::Wiimote::_1,
				0,
				0
			}
	},
	{
			Input::Event::MAP_WIIMOTE,
			"Default Classic Controller",
			{
					EMU_CONTROLS_IN_GAME_ACTIONS_WII_CC_PROFILE_INIT,

				Input::Wiimote::UP,
				Input::Wiimote::RIGHT,
				Input::Wiimote::DOWN,
				Input::Wiimote::LEFT,
				0,
				0,
				0,
				0,
				Input::Wiimote::PLUS,
				Input::Wiimote::B,
				Input::Wiimote::Y,
				Input::Wiimote::A,
				Input::Wiimote::X,
			}
	},
};

const uint defaultWiimoteProfiles = sizeofArray(defaultWiimoteProfile);

// iControlPad

const KeyConfig defaultIControlPadProfile[] =
{
	{
			Input::Event::MAP_ICONTROLPAD,
			"Default iControlPad",
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
				Input::iControlPad::START,
				Input::iControlPad::X,
				Input::iControlPad::A,
				0,
				0
			}
	},
};

const uint defaultIControlPadProfiles = sizeofArray(defaultIControlPadProfile);

// iCade

const KeyConfig defaultICadeProfile[] =
{
	{
			Input::Event::MAP_ICADE,
			"Default iCade",
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
			"Default Zeemote",
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
				Input::Zeemote::POWER,
				Input::Zeemote::B,
				Input::Zeemote::A,
				0,
				0
			}
	},
};

const uint defaultZeemoteProfiles = sizeofArray(defaultZeemoteProfile);

};
