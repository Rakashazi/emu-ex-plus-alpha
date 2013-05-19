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
		"Coin/Select",
		"Start",
		"A",
		"B",
		"C",
		"D",
		"Turbo A",
		"Turbo B",
		"Turbo C",
		"Turbo D",
		"A + B + C",
};

static const char *switchName[switchKeys] =
{
	"Test Switch"
};

static const uint joystickKeyOffset = gameActionKeys;
static const uint joystick2KeyOffset = joystickKeyOffset + joystickKeys;
static const uint switchKeyOffset = joystick2KeyOffset + joystickKeys;

const KeyCategory category[categories]
{
	EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
	KeyCategory("Set Joystick Keys", gamepadName, joystickKeyOffset),
	KeyCategory("Set Joystick 2 Keys", gamepadName, joystick2KeyOffset, 1),
	KeyCategory("Set Switch Keys", switchName, switchKeyOffset)
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
			asciiKey('m'),
			asciiKey(','),
			asciiKey('k'),
			asciiKey('l'),
			0,
			0,
			0,
			0,
			asciiKey('p'),
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

			Input::Keycode::PS3::UP,
			Input::Keycode::PS3::RIGHT,
			Input::Keycode::PS3::DOWN,
			Input::Keycode::PS3::LEFT,
			0, 0, 0, 0,
			Input::Keycode::PS3::SELECT,
			Input::Keycode::PS3::START,
			Input::Keycode::PS3::CROSS,
			Input::Keycode::PS3::CIRCLE,
			Input::Keycode::PS3::SQUARE,
			Input::Keycode::PS3::TRIANGLE,
		}
	},
	{
		Input::Event::MAP_KEYBOARD,
		Input::Device::SUBTYPE_OUYA_CONTROLLER,
		"OUYA Controller",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

			Input::Keycode::Ouya::UP,
			Input::Keycode::Ouya::RIGHT,
			Input::Keycode::Ouya::DOWN,
			Input::Keycode::Ouya::LEFT,
			0, 0, 0, 0,
			Input::Keycode::Ouya::L3,
			Input::Keycode::Ouya::R3,
			Input::Keycode::Ouya::O,
			Input::Keycode::Ouya::A,
			Input::Keycode::Ouya::U,
			Input::Keycode::Ouya::Y,
		}
	},
		#ifdef CONFIG_MACHINE_GENERIC_ARMV7
		{
			Input::Event::MAP_KEYBOARD,
			Input::Device::SUBTYPE_XPERIA_PLAY,
			"Xperia Play",
			{
				EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

				Input::Keycode::XperiaPlay::UP,
				Input::Keycode::XperiaPlay::RIGHT,
				Input::Keycode::XperiaPlay::DOWN,
				Input::Keycode::XperiaPlay::LEFT,
				0, 0, 0, 0,
				Input::Keycode::XperiaPlay::SELECT,
				Input::Keycode::XperiaPlay::START,
				Input::Keycode::XperiaPlay::CROSS,
				Input::Keycode::XperiaPlay::CIRCLE,
				Input::Keycode::XperiaPlay::SQUARE,
				Input::Keycode::XperiaPlay::TRIANGLE,
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
				0, 0, 0, 0,
				asciiKey(' '),
				Input::Keycode::ENTER,
				asciiKey('x'),
				asciiKey('c'),
				asciiKey('s'),
				asciiKey('d'),
				asciiKey('v'),
				asciiKey('b'),
				asciiKey('f'),
				asciiKey('g'),
				asciiKey('q'),
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
			0, 0, 0, 0,
			asciiKey(' '),
			Input::Keycode::ENTER,
			asciiKey('z'),
			asciiKey('x'),
			asciiKey('a'),
			asciiKey('s'),
			asciiKey('c'),
			asciiKey('v'),
			asciiKey('d'),
			asciiKey('f'),
			asciiKey('q'),
			asciiKey('w'),
		}
	},
	#ifdef CONFIG_MACHINE_PANDORA
	{
		Input::Event::MAP_KEYBOARD,
		Input::Device::CONFIG_PANDORA_HANDHELD,
		"Default Pandora",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_OPEN_PANDORA_PROFILE_INIT,

			Input::Keycode::Pandora::UP,
			Input::Keycode::Pandora::RIGHT,
			Input::Keycode::Pandora::DOWN,
			Input::Keycode::Pandora::LEFT,
			0, 0, 0, 0,
			Input::Keycode::Pandora::SELECT,
			Input::Keycode::Pandora::START,
			Input::Keycode::Pandora::X,
			Input::Keycode::Pandora::B,
			Input::Keycode::Pandora::A,
			Input::Keycode::Pandora::Y,
		}
	},
	#endif
};

const uint defaultKeyProfiles = sizeofArray(defaultKeyProfile);

#endif

#ifdef CONFIG_INPUT_EVDEV

const KeyConfig defaultEvdevProfile[] =
{
	{
		Input::Event::MAP_EVDEV,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_UNBINDED_PROFILE_INIT,
			Input::Evdev::UP,
			Input::Evdev::RIGHT,
			Input::Evdev::DOWN,
			Input::Evdev::LEFT,
			0, 0, 0, 0,
			Input::Evdev::GAME_SELECT,
			Input::Evdev::GAME_START,
			Input::Evdev::GAME_A,
			Input::Evdev::GAME_B,
			Input::Evdev::GAME_X,
			Input::Evdev::GAME_Y,
		}
	},
};

const uint defaultEvdevProfiles = sizeofArray(defaultEvdevProfile);

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
			0, 0, 0, 0,
			Input::Wiimote::MINUS,
			Input::Wiimote::PLUS,
			Input::Wiimote::_1,
			Input::Wiimote::_2,
			Input::Wiimote::A,
			Input::Wiimote::B,
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
			0, 0, 0, 0,
			Input::WiiCC::MINUS,
			Input::WiiCC::PLUS,
			Input::WiiCC::B,
			Input::WiiCC::A,
			Input::WiiCC::Y,
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
			Input::iControlPad::X,
			Input::iControlPad::B,
			Input::iControlPad::A,
			Input::iControlPad::Y,
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
			0, 0, 0, 0,
			Input::ICade::A,
			Input::ICade::C,
			Input::ICade::H,
			Input::ICade::G,
			Input::ICade::F,
			Input::ICade::E,
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
			0, 0, 0, 0,
			Input::Zeemote::C,
			Input::Zeemote::POWER,
			Input::Zeemote::A,
			Input::Zeemote::B,
		}
	},
};

const uint defaultZeemoteProfiles = sizeofArray(defaultZeemoteProfile);

// PS3

const KeyConfig defaultPS3Profile[] =
{
	{
		Input::Event::MAP_PS3PAD,
		0,
		"Default",
		{
			EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_PS3PAD_PROFILE_INIT,

			Input::PS3::UP,
			Input::PS3::RIGHT,
			Input::PS3::DOWN,
			Input::PS3::LEFT,
			0, 0, 0, 0,
			Input::PS3::SELECT,
			Input::PS3::START,
			Input::PS3::CROSS,
			Input::PS3::CIRCLE,
			Input::PS3::SQUARE,
			Input::PS3::TRIANGLE,
		}
	},
};

const uint defaultPS3Profiles = sizeofArray(defaultPS3Profile);

};
