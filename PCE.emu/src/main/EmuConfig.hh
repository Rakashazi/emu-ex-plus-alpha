#pragma once

#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	#define CONFIG_FILE_NAME "PceEmu.config"
#else
	#define CONFIG_FILE_NAME "config"
#endif

static const char *touchConfigFaceBtnName = "I/II", *touchConfigCenterBtnName = "Select/Run";
static const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2013\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nMednafen Team\nmednafen.sourceforge.net";
static const uint systemFaceBtns = 6, systemCenterBtns = 2;;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 0;
#define systemAspectRatioString "4:3"

namespace EmuControls
{

using namespace Input;
static const uint categories = 2;
static const uint gamepadKeys = 18;
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
	asciiKey('i'),
	asciiKey('o'),
	asciiKey('n'),
	asciiKey('j'),
	asciiKey('k'),
	asciiKey('l'),
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
	asciiKey('c'),
	asciiKey('x'),
	asciiKey('r'),
	asciiKey('e'),
	asciiKey('z'),
	asciiKey('s'),
	asciiKey('d'),
	asciiKey('f'),
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
	asciiKey('c'),
	asciiKey('x'),
	asciiKey('r'),
	asciiKey('e'),
	asciiKey('z'),
	asciiKey('s'),
	asciiKey('d'),
	asciiKey('f'),
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
	0,
	0,
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
	Input::Wiimote::_2,
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
	Input::Wiimote::B,
	Input::Wiimote::Y,
	Input::Wiimote::A,
	Input::Wiimote::X,
};

static const KeyArray wiiCC6Btn =
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
	Input::Wiimote::R,
	Input::Wiimote::A,
	0,
	0,
	Input::Wiimote::B,
	Input::Wiimote::Y,
	Input::Wiimote::X,
	Input::Wiimote::L,
};

static const ConstKeyProfile wii[] =
{
		{ "Default Wiimote", wiimote },
		{ "Default Classic Controller", wiiCC },
		{ "Classic Controller (6-button)", wiiCC6Btn }
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
	Input::iControlPad::A,
	0,
	0,
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
	Input::ICade::F,
	0,
	0
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
	Input::Zeemote::B,
	Input::Zeemote::A,
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

// PS3

static const KeyArray ps3PadDefaults =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_PS3PAD_PROFILE_INIT,

		Input::Ps3::UP,
		Input::Ps3::RIGHT,
		Input::Ps3::DOWN,
		Input::Ps3::LEFT,
		0,
		0,
		0,
		0,
		Input::Ps3::SELECT,
		Input::Ps3::START,
		Input::Ps3::CIRCLE,
		Input::Ps3::CROSS,
		Input::Ps3::TRIANGLE,
		Input::Ps3::SQUARE,
		0,
		0,
		0,
		0,
};

static const ConstKeyProfile ps3Pad[] =
{
		{ "Default PS3 Gamepad", ps3PadDefaults }
};

}
