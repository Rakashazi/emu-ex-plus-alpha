#pragma once

#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	#define CONFIG_FILE_NAME "SaturnEmu.config"
#else
	#define CONFIG_FILE_NAME "config"
#endif

static const char *touchConfigFaceBtnName = "A-Z", *touchConfigCenterBtnName = "Start";
static const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2012\nRobert Broglia\nwww.explusalpha.com\n\n(c) 2012 the\nYabause Team\nyabause.org";
static const uint systemFaceBtns = 8, systemCenterBtns = 1;
static const bool systemHasTriggerBtns = 1, systemHasRevBtnLayout = 0;
#define systemAspectRatioString "4:3"

namespace EmuControls
{

using namespace Input;
static const uint categories = 2;
static const uint gamepadKeys = 23;
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

typedef uint KeyArray[systemTotalKeys];

// Key Input/Keyboard

#ifdef INPUT_SUPPORTS_KEYBOARD
static const KeyArray webOSKB =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_WEBOS_KB_PROFILE_INIT,

		EMU_CONTROLS_WEBOS_KB_8WAY_DIRECTION_PROFILE_INIT,
		asciiKey(' '),
		Input::Key::ENTER,
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
		Input::Key::ENTER,
		asciiKey('z'),
		asciiKey('x'),
		asciiKey('c'),
		asciiKey('a'),
		asciiKey('s'),
		asciiKey('d'),
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
		Input::Key::ESCAPE,
		Input::Key::CENTER,
		Input::Key::GAME_Y,
		Input::Key::GAME_X,
		Input::Key::GAME_L1,
		Input::Key::GAME_R1,
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
	Input::Wiimote::A,
	Input::Wiimote::B,
	Input::Wiimote::X,
	Input::Wiimote::Y,
	Input::Wiimote::L,
	Input::Wiimote::R,
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
};

static const ConstKeyProfile zeemote[] =
{
		{ "Default Zeemote JS1", zeemoteDefaults }
};



}
