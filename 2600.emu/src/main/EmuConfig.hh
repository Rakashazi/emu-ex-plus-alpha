#pragma once

static const char *touchConfigFaceBtnName = "JS Buttons", *touchConfigCenterBtnName = "Select/Reset";
static const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nStella Team\nstella.sourceforge.net";
static const uint systemFaceBtns = 2, systemCenterBtns = 2;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 0;
static const char *systemAspectRatioString = "4:3";

namespace EmuControls
{

using namespace Input;
static const uint categories = 5;
static const uint gamepadKeys = 10;
static const uint switchKeys = 5;
static const uint keyboardKeys = 12;
static const uint systemTotalKeys = gameActionKeys + gamepadKeys + switchKeys + keyboardKeys*2;

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

typedef uint KeyArray[systemTotalKeys];

// Key Input/Keyboard

#ifdef INPUT_SUPPORTS_KEYBOARD
static const KeyArray webOSKB =
{
	EMU_CONTROLS_IN_GAME_ACTIONS_WEBOS_KB_PROFILE_INIT,

	EMU_CONTROLS_WEBOS_KB_8WAY_DIRECTION_PROFILE_INIT,
	asciiKey(','),
	asciiKey('m'),
	asciiKey(' '),
	Input::Key::ENTER,
	asciiKey('k'),
	asciiKey('l'),
};

static const KeyArray genericKB =
{
	0,
	Input::Key::MENU,
	0,
	asciiKey('p'),
	asciiKey('`'),
	0,
	Input::Key::ESCAPE,

	Input::Key::UP,
	Input::Key::RIGHT,
	Input::Key::DOWN,
	Input::Key::LEFT,
	0,
	0,
	0,
	0,
	asciiKey('x'),
	asciiKey('c'),
	asciiKey(' '),
	Input::Key::ENTER,
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
	asciiKey('x'),
	asciiKey('c'),
	asciiKey(' '),
	Input::Key::ENTER,
	asciiKey('s'),
	asciiKey('d'),
};

static const KeyArray xperiaPlay =
{
		0,
		Input::Key::MENU,
		0,
		0,
		Input::Key::SEARCH,
		0,
		0,

		Input::Key::UP,
		Input::Key::RIGHT,
		Input::Key::DOWN,
		Input::Key::LEFT,
		0,
		0,
		0,
		0,
		Input::Key::CENTER,
		Input::Key::GAME_X,
		Input::Key::GAME_SELECT,
		Input::Key::GAME_START,
		Input::Key::GAME_Y,
		Input::Key::ESCAPE,
		Input::Key::GAME_L1,
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
	Input::Wiimote::_1,
	Input::Wiimote::_2,
	Input::Wiimote::MINUS,
	Input::Wiimote::PLUS,
	Input::Wiimote::A,
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
	Input::Wiimote::Y,
	Input::Wiimote::B,
	Input::Wiimote::MINUS,
	Input::Wiimote::PLUS,
	Input::Wiimote::L,
	Input::Wiimote::R,
	Input::Wiimote::X,
};

static const ConstKeyProfile wii[] =
{
		{ "Default Wiimote", wiimote },
		{ "Default Classic Controller", wiiCC }
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
	Input::iControlPad::X,
	Input::iControlPad::A,
	Input::iControlPad::SELECT,
	Input::iControlPad::START,
	Input::iControlPad::Y,
	Input::iControlPad::B,
};

static const ConstKeyProfile iCP[] =
{
		{ "Default iControlPad", iCPDefault }
};

// iCade

static const KeyArray iCadeDefault =
{
	0,
	Input::ICade::D,
	0,
	0,
	0,
	0,
	0,

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
	Input::ICade::A,
	Input::ICade::C,
	Input::ICade::E,
	Input::ICade::G,
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
		Input::Zeemote::A,
		Input::Zeemote::B,
		Input::Zeemote::C,
		Input::Zeemote::POWER,
};

static const ConstKeyProfile zeemote[] =
{
		{ "Default Zeemote JS1", zeemoteDefaults }
};

}
