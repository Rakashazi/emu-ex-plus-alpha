#include <util/preprocessor/repeat.h>
#include <EmuInput.hh>

namespace EmuControls
{

void transposeKeysForPlayer(KeyConfig::KeyArray &key, uint player)
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

static const uint joystickKeyOffset = gameActionKeys;
static const uint joystick2KeyOffset = joystickKeyOffset + joystickKeys;
static const uint switchKeyOffset = joystick2KeyOffset + joystickKeys;
static const uint keyboardKeyOffset = switchKeyOffset + switchKeys;
static const uint keyboard2KeyOffset = keyboardKeyOffset + keyboardKeys;

const KeyCategory category[categories]
{
		EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
		KeyCategory("Joystick", gamepadName, joystickKeyOffset),
		KeyCategory("Joystick 2", gamepadName, joystick2KeyOffset, 1),
		KeyCategory("Console Switches", switchName, switchKeyOffset),
		KeyCategory("Keyboard Controller", keyboardName, keyboardKeyOffset),
		KeyCategory("Keyboard Controller 2", keyboardName, keyboard2KeyOffset)
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

			// JS 1
			EMU_CONTROLS_WEBOS_KB_8WAY_DIRECTION_PROFILE_INIT,
			asciiKey(','),
			asciiKey('m'),

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			asciiKey(' '),
			Input::Keycode::ENTER,
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

			// JS 1
			Input::Keycode::UP,
			Input::Keycode::RIGHT,
			Input::Keycode::DOWN,
			Input::Keycode::LEFT,
			0,
			0,
			0,
			0,
			Input::Keycode::GAME_X,
			Input::Keycode::GAME_Y,

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Input::Keycode::GAME_SELECT,
			Input::Keycode::GAME_START,
			Input::Keycode::GAME_A,
			Input::Keycode::GAME_B,
			Input::Keycode::GAME_L1,
		}
	},
	{
		Input::Event::MAP_KEYBOARD,
		Input::Device::SUBTYPE_OUYA_CONTROLLER,
		"OUYA Controller",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

			// JS 1
			Input::Keycode::UP,
			Input::Keycode::RIGHT,
			Input::Keycode::DOWN,
			Input::Keycode::LEFT,
			0,
			0,
			0,
			0,
			Input::Ouya::A,
			Input::Ouya::O,

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Input::Keycode::GAME_LEFT_THUMB,
			Input::Keycode::GAME_RIGHT_THUMB,
			Input::Ouya::Y,
			Input::Ouya::U,
			Input::Keycode::GAME_L1,
		}
	},
	#ifdef CONFIG_MACHINE_GENERIC_ARMV7
		{
			Input::Event::MAP_KEYBOARD,
			Input::Device::SUBTYPE_XPERIA_PLAY,
			"Xperia Play",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

				// JS 1
				Input::Keycode::UP,
				Input::Keycode::RIGHT,
				Input::Keycode::DOWN,
				Input::Keycode::LEFT,
				0,
				0,
				0,
				0,
				Input::Keycode::CENTER,
				Input::Keycode::GAME_B,

				// JS 2
				PP_ZERO_LIST(10)

				// Switches
				Input::Keycode::GAME_SELECT,
				Input::Keycode::GAME_START,
				Input::Keycode::GAME_X,
				Input::Keycode::GAME_Y,
				Input::Keycode::GAME_L1,
			}
		},
		{
			Input::Event::MAP_KEYBOARD,
			Input::Device::SUBTYPE_MOTO_DROID_KEYBOARD,
			"Droid/Milestone Keyboard",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

				// JS 1
				Input::Keycode::UP,
				Input::Keycode::RIGHT,
				Input::Keycode::DOWN,
				Input::Keycode::LEFT,
				0,
				0,
				0,
				0,
				asciiKey('x'),
				asciiKey('c'),

				// JS 2
				PP_ZERO_LIST(10)

				// Switches
				asciiKey(' '),
				Input::Keycode::ENTER,
				asciiKey('s'),
				asciiKey('d'),
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

			// JS 1
			Input::Keycode::UP,
			Input::Keycode::RIGHT,
			Input::Keycode::DOWN,
			Input::Keycode::LEFT,
			0,
			0,
			0,
			0,
			asciiKey('z'),
			asciiKey('x'),

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			asciiKey(' '),
			Input::Keycode::ENTER,
			asciiKey('a'),
			asciiKey('s'),
			asciiKey('q'),

			// KB 1
			asciiKey('3'),
			asciiKey('4'),
			asciiKey('5'),
			asciiKey('e'),
			asciiKey('r'),
			asciiKey('t'),
			asciiKey('d'),
			asciiKey('f'),
			asciiKey('g'),
			asciiKey('c'),
			asciiKey('v'),
			asciiKey('b'),

			// KB 2
			asciiKey('6'),
			asciiKey('7'),
			asciiKey('8'),
			asciiKey('y'),
			asciiKey('u'),
			asciiKey('i'),
			asciiKey('h'),
			asciiKey('j'),
			asciiKey('k'),
			asciiKey('n'),
			asciiKey('m'),
			asciiKey(','),
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
			"Default Wiimote",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_WIIMOTE_PROFILE_INIT,

				// JS 1
				Input::Wiimote::UP,
				Input::Wiimote::RIGHT,
				Input::Wiimote::DOWN,
				Input::Wiimote::LEFT,
				0,
				0,
				0,
				0,
				Input::Wiimote::_1,
				Input::Wiimote::_2,

				// JS 2
				PP_ZERO_LIST(10)

				// Switches
				Input::Wiimote::MINUS,
				Input::Wiimote::PLUS,
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
		"Default Classic / Wii U Pro Controller",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_WII_CC_PROFILE_INIT,

			// JS 1
			Input::WiiCC::UP,
			Input::WiiCC::RIGHT,
			Input::WiiCC::DOWN,
			Input::WiiCC::LEFT,
			0,
			0,
			0,
			0,
			Input::WiiCC::Y,
			Input::WiiCC::B,

			// JS 2
			PP_ZERO_LIST(10)

			// Switches
			Input::WiiCC::MINUS,
			Input::WiiCC::PLUS,
			Input::WiiCC::L,
			Input::WiiCC::R,
			Input::WiiCC::X,
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
			"Default iControlPad",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_ICP_NUBS_PROFILE_INIT,

				// JS 1
				Input::iControlPad::UP,
				Input::iControlPad::RIGHT,
				Input::iControlPad::DOWN,
				Input::iControlPad::LEFT,
				0,
				0,
				0,
				0,
				Input::iControlPad::X,
				Input::iControlPad::A,

				// JS 2
				PP_ZERO_LIST(10)

				// Switches
				Input::iControlPad::SELECT,
				Input::iControlPad::START,
				Input::iControlPad::Y,
				Input::iControlPad::B,
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
			"Default iCade",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_ICADE_PROFILE_INIT,

				// JS 1
				Input::ICade::UP,
				Input::ICade::RIGHT,
				Input::ICade::DOWN,
				Input::ICade::LEFT,
				0,
				0,
				0,
				0,
				Input::ICade::H,
				Input::ICade::F,

				// JS 2
				PP_ZERO_LIST(10)

				// Switches
				Input::ICade::A,
				Input::ICade::C,
				Input::ICade::E,
				Input::ICade::G,
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
			"Default Zeemote",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_UNBINDED_PROFILE_INIT,

				// JS 1
				Input::Zeemote::UP,
				Input::Zeemote::RIGHT,
				Input::Zeemote::DOWN,
				Input::Zeemote::LEFT,
				0,
				0,
				0,
				0,
				Input::Zeemote::A,
				Input::Zeemote::B,

				// JS 2
				PP_ZERO_LIST(10)

				// Switches
				Input::Zeemote::C,
				Input::Zeemote::POWER,
			}
	},
};

const uint defaultZeemoteProfiles = sizeofArray(defaultZeemoteProfile);

};
