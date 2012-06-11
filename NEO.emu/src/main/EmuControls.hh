#pragma once

namespace EmuControls
{

using namespace Input;
static const uint categories = 2;
static const uint gamepadKeys = 20;
static const uint systemTotalKeys = gameActionKeys + gamepadKeys;

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
		"Test Switch"
};

typedef uint KeyArray[systemTotalKeys];

// Key Input/Keyboard

#ifdef INPUT_SUPPORTS_KEYBOARD
static const KeyArray webOSKB =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_WEBOS_KB_PROFILE_INIT,

		EMU_CONTROLS_WEBOS_KB_8WAY_DIRECTION_PROFILE_INIT,
		asciiKey(' '),
		Input::Key::ENTER,
		asciiKey('m'),
		asciiKey(','),
		asciiKey('k'),
		asciiKey('l'),
		0,
		0,
		0,
		0,
		asciiKey('p'),
};

static const KeyArray genericKB =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_PROFILE_INIT,

		Input::Key::UP,
		Input::Key::RIGHT,
		Input::Key::DOWN,
		Input::Key::LEFT,
		0,
		0,
		0,
		0,
		asciiKey(' '),
		Input::Key::ENTER,
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
};

#ifdef CONFIG_BASE_ANDROID
static const KeyArray androidNav =
{
	EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

	Input::Key::UP,
	Input::Key::RIGHT,
	Input::Key::DOWN,
	Input::Key::LEFT,
	0,
	0,
	0,
	0,
	asciiKey(' '),
	Input::Key::ENTER,
	asciiKey('z'),
	asciiKey('x'),
	asciiKey('a'),
	asciiKey('s'),
	asciiKey('c'),
	asciiKey('v'),
	asciiKey('d'),
	asciiKey('f'),
	asciiKey('q'),
};


static const KeyArray xperiaPlay =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_NO_BACK_PROFILE_INIT,

		Input::Key::UP,
		Input::Key::RIGHT,
		Input::Key::DOWN,
		Input::Key::LEFT,
		0,
		0,
		0,
		0,
		Input::Key::GAME_SELECT,
		Input::Key::GAME_START,
		Input::Key::CENTER,
		Input::Key::ESCAPE,
		Input::Key::GAME_X,
		Input::Key::GAME_Y,
		0,
		0,
		0,
		0,
};
#endif

static const ConstKeyProfile kb[] =
{
#ifdef CONFIG_ENV_WEBOS
	{ "WebOS Keyboard", webOSKB },
#endif
#ifdef CONFIG_BASE_ANDROID
	{ "Android Nav + Keyboard", androidNav },
	{ "Xperia Play", xperiaPlay },
#endif
	{ "Standard Keyboard", genericKB },
};

#endif

// Wiimote

static const KeyArray wiimote =
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
	Input::Wiimote::_1,
	Input::Wiimote::_2,
	Input::Wiimote::A,
	Input::Wiimote::B,
	0,
	0,
	0,
	0,
};

static const KeyArray wiiCC =
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
	Input::Wiimote::MINUS,
	Input::Wiimote::PLUS,
	Input::Wiimote::B,
	Input::Wiimote::A,
	Input::Wiimote::Y,
	Input::Wiimote::X,
};

static const ConstKeyProfile wii[] =
{
		{ "Default Wiimote", wiimote },
		{ "Default Classic Controller", wiiCC },
};

// iControlPad

static const KeyArray iCPDefault =
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
		Input::iControlPad::B,
		Input::iControlPad::A,
		Input::iControlPad::Y,
		0,
		0,
		0,
		0,
};

static const ConstKeyProfile iCP[] =
{
		{ "Default iControlPad", iCPDefault }
};

// iCade

static const KeyArray iCadeDefault =
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
		Input::ICade::G,
		Input::ICade::F,
		Input::ICade::E,
		0,
		0,
		0,
		0,
};

static const ConstKeyProfile iCade[] =
{
		{ "Default iCade", iCadeDefault }
};

// Zeemote

static const KeyArray zeemoteDefaults =
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
		Input::Zeemote::A,
		Input::Zeemote::B,
		0,
		0,
		0,
		0,
		0,
		0,
};

static const ConstKeyProfile zeemote[] =
{
		{ "Default Zeemote JS1", zeemoteDefaults }
};



}
