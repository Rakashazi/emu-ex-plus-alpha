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
	{
			Input::Event::MAP_KEYBOARD,
			"Android Nav + Keyboard",
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
	{
			Input::Event::MAP_KEYBOARD,
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
#endif
	{
			Input::Event::MAP_KEYBOARD,
			"Default Keyboard",
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
				asciiKey('x'),
				asciiKey('c'),

				// JS 2
				PP_ZERO_LIST(10)

				// Switches
				asciiKey(' '),
				Input::Keycode::ENTER,
				asciiKey('s'),
				asciiKey('d'),
				asciiKey('w'),

				// KB 1
				asciiKey('4'),
				asciiKey('5'),
				asciiKey('6'),
				asciiKey('r'),
				asciiKey('t'),
				asciiKey('y'),
				asciiKey('f'),
				asciiKey('g'),
				asciiKey('h'),
				asciiKey('v'),
				asciiKey('b'),
				asciiKey('n'),

				// KB 2
				asciiKey('7'),
				asciiKey('8'),
				asciiKey('9'),
				asciiKey('u'),
				asciiKey('i'),
				asciiKey('o'),
				asciiKey('j'),
				asciiKey('k'),
				asciiKey('l'),
				asciiKey('m'),
				asciiKey(','),
				asciiKey('.'),
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
	{
			Input::Event::MAP_WIIMOTE,
			"Default Classic Controller",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_WII_CC_PROFILE_INIT,

				// JS 1
				Input::Wiimote::UP,
				Input::Wiimote::RIGHT,
				Input::Wiimote::DOWN,
				Input::Wiimote::LEFT,
				0,
				0,
				0,
				0,
				Input::Wiimote::Y,
				Input::Wiimote::B,

				// JS 2
				PP_ZERO_LIST(10)

				// Switches
				Input::Wiimote::MINUS,
				Input::Wiimote::PLUS,
				Input::Wiimote::L,
				Input::Wiimote::R,
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
