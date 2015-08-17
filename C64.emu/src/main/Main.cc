/*  This file is part of C64.emu.

	C64.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	C64.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with C64.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "main"
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuOptions.hh>
#include <emuframework/CommonFrameworkIncludes.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/thread/Semaphore.hh>
#include <sys/time.h>

extern "C"
{
	#include "machine.h"
	#include "maincpu.h"
	#include "drive.h"
	#include "lib.h"
	#include "util.h"
	#include "ioutil.h"
	#include "uiapi.h"
	#include "console.h"
	#include "monitor.h"
	#include "video.h"
	#include "cmdline.h"
	#include "gfxoutput.h"
	#include "videoarch.h"
	#include "init.h"
	#include "resources.h"
	#include "sysfile.h"
	#include "log.h"
	#include "archdep.h"
	#include "palette.h"
	#include "c64model.h"
	#include "keyboard.h"
	#include "autostart.h"
	#include "kbdbuf.h"
	#include "attach.h"
	#include "raster.h"
	#include "viciitypes.h"
	#include "sound.h"
	#include "cartridge.h"
	#include "tape.h"
	#include "vicii.h"
	#include "interrupt.h"
	#include "sid/sid.h"
	#include "c64/cart/c64cartsystem.h"

	CLINK int warp_mode_enabled;
}

const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2013-2014\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nVice Team\nwww.viceteam.org";
IG::Semaphore execSem{0}, execDoneSem{0};
alignas(8) uint16 pix[520*312]{};
bool runningFrame = false, doAudio = false;
static bool c64IsInit = false, c64FailedInit = false, isPal = false,
		shiftLock = false, ctrlLock = false;
uint c64VidX = 320, c64VidY = 200;
static uint c64VidActiveX = 0, c64VidActiveY = 0;
static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;
static FS::PathString firmwareBasePath{};
FS::PathString sysFilePath[Config::envIsLinux ? 4 : 2][3]{};

void setupSysFilePaths(FS::PathString outPath[3], const FS::PathString &firmwareBasePath);

enum
{
	CFGKEY_DRIVE_TRUE_EMULATION = 256, CFGKEY_AUTOSTART_WARP = 257,
	CFGKEY_AUTOSTART_TDE = 258, CFGKEY_C64_MODEL = 259,
	CFGKEY_BORDER_MODE = 260, CFGKEY_SWAP_JOYSTICK_PORTS = 261,
	CFGKEY_SID_ENGINE = 262, CFGKEY_CROP_NORMAL_BORDERS = 263,
	CFGKEY_SYSTEM_FILE_PATH = 264
};

static int intResource(const char *name)
{
	int val;
	auto failed = resources_get_int(name, &val);
	assert(!failed);
	return val;
}

static void setAutostartWarp(bool on)
{
	resources_set_int("AutostartWarp", on);
}

static bool autostartWarp()
{
	return intResource("AutostartWarp");
}

static void setAutostartTDE(bool on)
{
	resources_set_int("AutostartHandleTrueDriveEmulation", on);
}

static bool autostartTDE()
{
	return intResource("AutostartHandleTrueDriveEmulation");
}

static void setC64ModelIsPal(int model)
{
	switch(model)
	{
		case C64MODEL_C64_NTSC:
		case C64MODEL_C64C_NTSC:
		case C64MODEL_C64_OLD_NTSC:
		case C64MODEL_C64SX_NTSC:
		case C64MODEL_C64_JAP:
		case C64MODEL_PET64_NTSC:
		case C64MODEL_ULTIMAX:
			isPal = 0;
		bdefault:
			isPal = 1;
	}
	if(isPal)
	{
		logMsg("C64 model has PAL timings");
	}
}

static int c64Model()
{
	return c64model_get();
}

static void setC64Model(int model)
{
	if(model < 0 || model >= C64MODEL_NUM)
	{
		logWarn("tried to set C64 model id %d out of range", model);
		return;
	}
	c64model_set(model);
	setC64ModelIsPal(model);
	EmuSystem::configAudioPlayback();
}

static void setBorderMode(int mode)
{
	if(mode < VICII_NORMAL_BORDERS || mode > VICII_NO_BORDERS)
	{
		logWarn("tried to set border mode type %d out of range", mode);
		return;
	}
	resources_set_int("VICIIBorderMode", mode);
}

static int borderMode()
{
	return intResource("VICIIBorderMode");
}

static void setSidEngine(int engine)
{
	if(engine < SID_ENGINE_FASTSID || engine > SID_ENGINE_RESID)
	{
		logWarn("tried to set sid engine %d out of range", engine);
		return;
	}
	resources_set_int("SidEngine", engine);
}

static int sidEngine()
{
	return intResource("SidEngine");
}

static Byte1Option optionDriveTrueEmulation(CFGKEY_DRIVE_TRUE_EMULATION, 1);
static Byte1Option optionCropNormalBorders(CFGKEY_CROP_NORMAL_BORDERS, 1);
static Option<OptionMethodFunc<bool, autostartWarp, setAutostartWarp>, uint8>
	optionAutostartWarp(CFGKEY_AUTOSTART_WARP, 1);
static Option<OptionMethodFunc<bool, autostartTDE, setAutostartTDE>, uint8>
	optionAutostartTDE(CFGKEY_AUTOSTART_TDE, 0);
static Byte1Option optionC64Model(CFGKEY_C64_MODEL, C64MODEL_C64_NTSC, false,
	optionIsValidWithMax<C64MODEL_NUM-1, uint8>);
static Option<OptionMethodFunc<int, borderMode, setBorderMode>, uint8>
	optionBorderMode(CFGKEY_BORDER_MODE, VICII_NORMAL_BORDERS);
static Option<OptionMethodFunc<int, sidEngine, setSidEngine>, uint8>
	optionSidEngine(CFGKEY_SID_ENGINE,
		#ifdef HAVE_RESID
		SID_ENGINE_RESID
		#else
		SID_ENGINE_FASTSID
		#endif
	);
static Byte1Option optionSwapJoystickPorts(CFGKEY_SWAP_JOYSTICK_PORTS, 0);
PathOption optionFirmwarePath(CFGKEY_SYSTEM_FILE_PATH, firmwareBasePath, "");

void EmuSystem::initOptions() {}

void EmuSystem::onOptionsLoaded() {}

bool EmuSystem::readConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_DRIVE_TRUE_EMULATION: optionDriveTrueEmulation.readFromIO(io, readSize);
		bcase CFGKEY_AUTOSTART_WARP: optionAutostartWarp.readFromIO(io, readSize);
		bcase CFGKEY_AUTOSTART_TDE: optionAutostartTDE.readFromIO(io, readSize);
		bcase CFGKEY_C64_MODEL: optionC64Model.readFromIO(io, readSize);
		bcase CFGKEY_BORDER_MODE: optionBorderMode.readFromIO(io, readSize);
		bcase CFGKEY_CROP_NORMAL_BORDERS: optionCropNormalBorders.readFromIO(io, readSize);
		bcase CFGKEY_SID_ENGINE: optionSidEngine.readFromIO(io, readSize);
		bcase CFGKEY_SWAP_JOYSTICK_PORTS: optionSwapJoystickPorts.readFromIO(io, readSize);
		bcase CFGKEY_SYSTEM_FILE_PATH: optionFirmwarePath.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionDriveTrueEmulation.writeWithKeyIfNotDefault(io);
	optionAutostartWarp.writeWithKeyIfNotDefault(io);
	optionAutostartTDE.writeWithKeyIfNotDefault(io);
	optionC64Model.writeWithKeyIfNotDefault(io);
	optionBorderMode.writeWithKeyIfNotDefault(io);
	optionCropNormalBorders.writeWithKeyIfNotDefault(io);
	optionSidEngine.writeWithKeyIfNotDefault(io);
	optionSwapJoystickPorts.writeWithKeyIfNotDefault(io);
	optionFirmwarePath.writeToIO(io);
}

const char *EmuSystem::inputFaceBtnName = "JS Buttons";
const char *EmuSystem::inputCenterBtnName = "F1/KB";
const uint EmuSystem::inputFaceBtns = 2;
const uint EmuSystem::inputCenterBtns = 2;
const bool EmuSystem::inputHasTriggerBtns = false;
const bool EmuSystem::inputHasRevBtnLayout = false;
const bool EmuSystem::inputHasKeyboard = true;
const char *EmuSystem::configFilename = "C64Emu.config";
const uint EmuSystem::maxPlayers = 2;
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = sizeofArray(EmuSystem::aspectRatioInfo);
const bool EmuSystem::hasPALVideoSystem = true;
#include <emuframework/CommonGui.hh>

const char *EmuSystem::shortSystemName()
{
	return "C64";
}

const char *EmuSystem::systemName()
{
	return "Commodore 64";
}

// controls

enum
{
	c64KeyIdxUp = EmuControls::systemKeyMapStart,
	c64KeyIdxRight,
	c64KeyIdxDown,
	c64KeyIdxLeft,
	c64KeyIdxLeftUp,
	c64KeyIdxRightUp,
	c64KeyIdxRightDown,
	c64KeyIdxLeftDown,
	c64KeyIdxBtn,
	c64KeyIdxBtnTurbo,
	c64KeyIdxSwapPorts,

	c64KeyIdxUp2,
	c64KeyIdxRight2,
	c64KeyIdxDown2,
	c64KeyIdxLeft2,
	c64KeyIdxLeftUp2,
	c64KeyIdxRightUp2,
	c64KeyIdxRightDown2,
	c64KeyIdxLeftDown2,
	c64KeyIdxBtn2,
	c64KeyIdxBtnTurbo2,
	c64KeyIdxSwapPorts2,

	c64KeyToggleKB,

	c64KeyF1,
	c64KeyF2,
	c64KeyF3,
	c64KeyF4,
	c64KeyF5,
	c64KeyF6,
	c64KeyF7,
	c64KeyF8,

	c64KeyLeftArrow,
	c64Key1,
	c64Key2,
	c64Key3,
	c64Key4,
	c64Key5,
	c64Key6,
	c64Key7,
	c64Key8,
	c64Key9,
	c64Key0,
	c64KeyPlus,
	c64KeyMinus,
	c64KeyPound,
	c64KeyClrHome,
	c64KeyInstDel,

	c64KeyCtrl,
	c64KeyQ,
	c64KeyW,
	c64KeyE,
	c64KeyR,
	c64KeyT,
	c64KeyY,
	c64KeyU,
	c64KeyI,
	c64KeyO,
	c64KeyP,
	c64KeyAt,
	c64KeyAsterisk,
	c64KeyUpArrow,
	c64KeyRestore,

	c64KeyRunStop,
	c64KeyShiftLock,
	c64KeyA,
	c64KeyS,
	c64KeyD,
	c64KeyF,
	c64KeyG,
	c64KeyH,
	c64KeyJ,
	c64KeyK,
	c64KeyL,
	c64KeyColon,
	c64KeySemiColon,
	c64KeyEquals,
	c64KeyReturn,

	c64KeyCommodore,
	c64KeyLeftShift,
	c64KeyZ,
	c64KeyX,
	c64KeyC,
	c64KeyV,
	c64KeyB,
	c64KeyN,
	c64KeyM,
	c64KeyComma,
	c64KeyPeriod,
	c64KeySlash,
	c64KeyRightShift,
	c64KeyKbUp,
	c64KeyKbRight,
	c64KeyKbDown,
	c64KeyKbLeft,

	c64KeySpace,
	c64KeyCtrlLock,
};

static const uint JOYPAD_FIRE = 0x10,
	JOYPAD_E = 0x08,
	JOYPAD_W = 0x04,
	JOYPAD_S = 0x02,
	JOYPAD_N = 0x01,
	JOYPAD_SW = (JOYPAD_S | JOYPAD_W),
	JOYPAD_SE = (JOYPAD_S | JOYPAD_E),
	JOYPAD_NW = (JOYPAD_N | JOYPAD_W),
	JOYPAD_NE = (JOYPAD_N | JOYPAD_E);

static const uint JS_SHIFT = 16;

static const uint SHIFT_BIT = IG::bit(8);

static constexpr uint mkKeyCode(int row, int col, int shift = 0)
{
	return (shift << 8) | (row << 4) | col;
}

static const uint
KB_NONE = 0x7F,
KB_INST_DEL = mkKeyCode(0,0),
KB_RETURN = mkKeyCode(0,1),
KB_CRSR_LR = mkKeyCode(0,2),
KB_F7 = mkKeyCode(0,3),
KB_F1 = mkKeyCode(0,4),
KB_F3 = mkKeyCode(0,5),
KB_F5 = mkKeyCode(0,6),
KB_CRSR_UD = mkKeyCode(0,7),
KB_3 = mkKeyCode(1,0),
KB_W = mkKeyCode(1,1),
KB_A = mkKeyCode(1,2),
KB_4 = mkKeyCode(1,3),
KB_Z = mkKeyCode(1,4),
KB_S = mkKeyCode(1,5),
KB_E = mkKeyCode(1,6),
KB_5 = mkKeyCode(2,0),
KB_R = mkKeyCode(2,1),
KB_D = mkKeyCode(2,2),
KB_6 = mkKeyCode(2,3),
KB_C = mkKeyCode(2,4),
KB_F = mkKeyCode(2,5),
KB_T = mkKeyCode(2,6),
KB_X = mkKeyCode(2,7),
KB_7 = mkKeyCode(3,0),
KB_Y = mkKeyCode(3,1),
KB_G = mkKeyCode(3,2),
KB_8 = mkKeyCode(3,3),
KB_B = mkKeyCode(3,4),
KB_H = mkKeyCode(3,5),
KB_U = mkKeyCode(3,6),
KB_V = mkKeyCode(3,7),
KB_9 = mkKeyCode(4,0),
KB_I = mkKeyCode(4,1),
KB_J = mkKeyCode(4,2),
KB_0 = mkKeyCode(4,3),
KB_M = mkKeyCode(4,4),
KB_K = mkKeyCode(4,5),
KB_O = mkKeyCode(4,6),
KB_N = mkKeyCode(4,7),
KB_PLUS = mkKeyCode(5,0),
KB_P = mkKeyCode(5,1),
KB_L = mkKeyCode(5,2),
KB_MINUS = mkKeyCode(5,3),
KB_AT_SIGN = mkKeyCode(5,6),
KB_POUND = mkKeyCode(6,0),
KB_ASTERISK = mkKeyCode(6,1),
KB_CLR_HOME = mkKeyCode(6,3),
KB_EQUALS = mkKeyCode(6,5),
KB_UP_ARROW = mkKeyCode(6,6),
KB_1 = mkKeyCode(7,0),
KB_LEFT_ARROW = mkKeyCode(7,1),
KB_2 = mkKeyCode(7,3),
KB_SPACE = mkKeyCode(7,4),
KB_Q = mkKeyCode(7,6),
KB_RUN_STOP = mkKeyCode(7,7),
KB_COLON = mkKeyCode(5,5),
KB_SEMICOLON = mkKeyCode(6,2),
KB_PERIOD = mkKeyCode(5,4),
KB_COMMA = mkKeyCode(5,7),
KB_SLASH = mkKeyCode(6,7),
KB_CTRL = mkKeyCode(7,2),
KB_COMMODORE = mkKeyCode(7,5),
KB_LEFT_SHIFT = mkKeyCode(1,7),
KB_RIGHT_SHIFT = mkKeyCode(6,4),
KB_RESTORE = 0xFF,

// shifted key codes
KBS_F2 = KB_F1 | SHIFT_BIT,
KBS_F4 = KB_F3 | SHIFT_BIT,
KBS_F6 = KB_F5 | SHIFT_BIT,
KBS_F8 = KB_F7 | SHIFT_BIT,

// special function codes
KBEX_SWAP_JS_PORTS = 0xFFFC,
KBEX_CTRL_LOCK = 0xFFFD,
KBEX_SHIFT_LOCK = 0xFFFE,
KBEX_TOGGLE_VKEYBOARD = 0xFFFF
;

static const SysVController::KbMap kbToEventMap
{
	KB_Q, KB_W, KB_E, KB_R, KB_T, KB_Y, KB_U, KB_I, KB_O, KB_P,
	KB_A, KB_S, KB_D, KB_F, KB_G, KB_H, KB_J, KB_K, KB_L, KB_NONE,
	KBEX_SHIFT_LOCK, KB_Z, KB_X, KB_C, KB_V, KB_B, KB_N, KB_M, KB_INST_DEL, KB_NONE,
	KB_NONE, KB_NONE, KB_NONE, KB_SPACE, KB_SPACE, KB_SPACE, KB_SPACE, KB_RUN_STOP, KB_RUN_STOP, KB_RETURN
};

static const SysVController::KbMap kbToEventMapShifted
{
	KB_Q | SHIFT_BIT, KB_W | SHIFT_BIT, KB_E | SHIFT_BIT, KB_R | SHIFT_BIT, KB_T | SHIFT_BIT, KB_Y | SHIFT_BIT, KB_U | SHIFT_BIT, KB_I | SHIFT_BIT, KB_O | SHIFT_BIT, KB_P | SHIFT_BIT,
	KB_A | SHIFT_BIT, KB_S | SHIFT_BIT, KB_D | SHIFT_BIT, KB_F | SHIFT_BIT, KB_G | SHIFT_BIT, KB_H | SHIFT_BIT, KB_J | SHIFT_BIT, KB_K | SHIFT_BIT, KB_L | SHIFT_BIT, KB_NONE,
	KBEX_SHIFT_LOCK, KB_Z | SHIFT_BIT, KB_X | SHIFT_BIT, KB_C | SHIFT_BIT, KB_V | SHIFT_BIT, KB_B | SHIFT_BIT, KB_N | SHIFT_BIT, KB_M | SHIFT_BIT, KB_INST_DEL | SHIFT_BIT, KB_NONE,
	KB_NONE, KB_NONE, KB_NONE, KB_SPACE, KB_SPACE, KB_SPACE, KB_SPACE, KB_RUN_STOP | SHIFT_BIT, KB_RUN_STOP | SHIFT_BIT, KB_RETURN
};

static const SysVController::KbMap kbToEventMap2
{
	KB_F1, KB_F3, KB_F5, KB_F7, KB_AT_SIGN, KB_COMMODORE, KB_CRSR_UD, KB_CRSR_LR, KB_PLUS, KB_MINUS,
	KB_1, KB_2, KB_3, KB_4, KB_5, KB_6, KB_7, KB_8, KB_9, KB_0,
	KB_RESTORE, KB_COLON, KB_SEMICOLON, KB_EQUALS, KB_COMMA, KB_PERIOD, KB_SLASH, KB_ASTERISK, KB_CLR_HOME, KB_NONE,
	KB_NONE, KB_NONE, KB_NONE, KB_SPACE, KB_SPACE, KB_SPACE, KB_SPACE, KB_PERIOD, KBEX_CTRL_LOCK, KB_RETURN
};

static const SysVController::KbMap kbToEventMap2Shifted
{
	KBS_F2, KBS_F4, KBS_F6, KBS_F8, KB_AT_SIGN, KB_COMMODORE, KB_CRSR_UD | SHIFT_BIT, KB_CRSR_LR | SHIFT_BIT, KB_PLUS | SHIFT_BIT, KB_MINUS | SHIFT_BIT,
	KB_1 | SHIFT_BIT, KB_2 | SHIFT_BIT, KB_3 | SHIFT_BIT, KB_4 | SHIFT_BIT, KB_5 | SHIFT_BIT, KB_6 | SHIFT_BIT, KB_7 | SHIFT_BIT, KB_8 | SHIFT_BIT, KB_9 | SHIFT_BIT, KB_0 | SHIFT_BIT,
	KB_RESTORE, KB_COLON | SHIFT_BIT, KB_SEMICOLON | SHIFT_BIT, KB_EQUALS | SHIFT_BIT, KB_COMMA | SHIFT_BIT, KB_PERIOD | SHIFT_BIT, KB_SLASH | SHIFT_BIT, KB_ASTERISK | SHIFT_BIT, KB_CLR_HOME | SHIFT_BIT, KB_NONE,
	KB_NONE, KB_NONE, KB_NONE, KB_SPACE, KB_SPACE, KB_SPACE, KB_SPACE, KB_PERIOD | SHIFT_BIT, KBEX_CTRL_LOCK, KB_RETURN
};

void updateVControllerKeyboardMapping(uint mode, SysVController::KbMap &map)
{
	auto &kbMap = mode ? (shiftLock ? kbToEventMap2Shifted : kbToEventMap2) : (shiftLock ? kbToEventMapShifted : kbToEventMap);
	memcpy(map, &kbMap, sizeof(kbMap));
}

void updateVControllerMapping(uint player, SysVController::Map &map)
{
	const uint p2Bit = player ? IG::bit(5) : 0;
	map[SysVController::F_ELEM] = (JOYPAD_FIRE | p2Bit) << JS_SHIFT;
	map[SysVController::F_ELEM+1] = ((JOYPAD_FIRE | p2Bit) << JS_SHIFT) | SysVController::TURBO_BIT;

	map[SysVController::C_ELEM] = KB_F1;
	map[SysVController::C_ELEM+1] = KBEX_TOGGLE_VKEYBOARD;

	map[SysVController::D_ELEM] = (JOYPAD_NW | p2Bit) << JS_SHIFT;
	map[SysVController::D_ELEM+1] = (JOYPAD_N | p2Bit) << JS_SHIFT;
	map[SysVController::D_ELEM+2] = (JOYPAD_NE | p2Bit) << JS_SHIFT;
	map[SysVController::D_ELEM+3] = (JOYPAD_W | p2Bit) << JS_SHIFT;
	map[SysVController::D_ELEM+5] = (JOYPAD_E | p2Bit) << JS_SHIFT;
	map[SysVController::D_ELEM+6] = (JOYPAD_SW | p2Bit) << JS_SHIFT;
	map[SysVController::D_ELEM+7] = (JOYPAD_S | p2Bit) << JS_SHIFT;
	map[SysVController::D_ELEM+8] = (JOYPAD_SE | p2Bit) << JS_SHIFT;
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	const uint p2Bit = IG::bit(5);
	const uint shiftBit = shiftLock ? SHIFT_BIT : 0;
	switch(input)
	{
		case c64KeyIdxUp: return JOYPAD_N << JS_SHIFT;
		case c64KeyIdxRight: return JOYPAD_E << JS_SHIFT;
		case c64KeyIdxDown: return JOYPAD_S << JS_SHIFT;
		case c64KeyIdxLeft: return JOYPAD_W << JS_SHIFT;
		case c64KeyIdxLeftUp: return JOYPAD_NW << JS_SHIFT;
		case c64KeyIdxRightUp: return JOYPAD_NE << JS_SHIFT;
		case c64KeyIdxRightDown: return JOYPAD_SE << JS_SHIFT;
		case c64KeyIdxLeftDown: return JOYPAD_SW << JS_SHIFT;
		case c64KeyIdxBtn: return JOYPAD_FIRE << JS_SHIFT;
		case c64KeyIdxBtnTurbo: turbo = 1; return JOYPAD_FIRE << JS_SHIFT;
		case c64KeyIdxSwapPorts: return KBEX_SWAP_JS_PORTS;

		case c64KeyIdxUp2: return (JOYPAD_N | p2Bit) << JS_SHIFT;
		case c64KeyIdxRight2: return (JOYPAD_E | p2Bit) << JS_SHIFT;
		case c64KeyIdxDown2: return (JOYPAD_S | p2Bit) << JS_SHIFT;
		case c64KeyIdxLeft2: return (JOYPAD_W | p2Bit) << JS_SHIFT;
		case c64KeyIdxLeftUp2: return (JOYPAD_NW | p2Bit) << JS_SHIFT;
		case c64KeyIdxRightUp2: return (JOYPAD_NE | p2Bit) << JS_SHIFT;
		case c64KeyIdxRightDown2: return (JOYPAD_SE | p2Bit) << JS_SHIFT;
		case c64KeyIdxLeftDown2: return (JOYPAD_SW | p2Bit) << JS_SHIFT;
		case c64KeyIdxBtn2: return (JOYPAD_FIRE | p2Bit) << JS_SHIFT;
		case c64KeyIdxBtnTurbo2: turbo = 1; return JOYPAD_FIRE << JS_SHIFT;
		case c64KeyIdxSwapPorts2: return KBEX_SWAP_JS_PORTS;

		case c64KeyToggleKB : return KBEX_TOGGLE_VKEYBOARD;

		case c64KeyF1 : return KB_F1;
		case c64KeyF2 : return KBS_F2;
		case c64KeyF3 : return KB_F3;
		case c64KeyF4 : return KBS_F4;
		case c64KeyF5 : return KB_F5;
		case c64KeyF6 : return KBS_F6;
		case c64KeyF7 : return KB_F7;
		case c64KeyF8 : return KBS_F8;

		case c64KeyLeftArrow : return KB_LEFT_ARROW;
		case c64Key1 : return KB_1;
		case c64Key2 : return KB_2;
		case c64Key3 : return KB_3;
		case c64Key4 : return KB_4;
		case c64Key5 : return KB_5;
		case c64Key6 : return KB_6;
		case c64Key7 : return KB_7;
		case c64Key8 : return KB_8;
		case c64Key9 : return KB_9;
		case c64Key0 : return KB_0;
		case c64KeyPlus : return KB_PLUS;
		case c64KeyMinus : return KB_MINUS;
		case c64KeyPound : return KB_POUND;
		case c64KeyClrHome : return KB_CLR_HOME;
		case c64KeyInstDel : return KB_INST_DEL;

		case c64KeyCtrl : return KB_CTRL;
		case c64KeyQ : return KB_Q | shiftBit;
		case c64KeyW : return KB_W | shiftBit;
		case c64KeyE : return KB_E | shiftBit;
		case c64KeyR : return KB_R | shiftBit;
		case c64KeyT : return KB_T | shiftBit;
		case c64KeyY : return KB_Y | shiftBit;
		case c64KeyU : return KB_U | shiftBit;
		case c64KeyI : return KB_I | shiftBit;
		case c64KeyO : return KB_O | shiftBit;
		case c64KeyP : return KB_P | shiftBit;
		case c64KeyAt : return KB_AT_SIGN;
		case c64KeyAsterisk : return KB_ASTERISK;
		case c64KeyUpArrow : return KB_UP_ARROW;
		case c64KeyRestore : return KB_RESTORE;

		case c64KeyRunStop : return KB_RUN_STOP;
		case c64KeyShiftLock : return KBEX_SHIFT_LOCK;
		case c64KeyA : return KB_A | shiftBit;
		case c64KeyS : return KB_S | shiftBit;
		case c64KeyD : return KB_D | shiftBit;
		case c64KeyF : return KB_F | shiftBit;
		case c64KeyG : return KB_G | shiftBit;
		case c64KeyH : return KB_H | shiftBit;
		case c64KeyJ : return KB_J | shiftBit;
		case c64KeyK : return KB_K | shiftBit;
		case c64KeyL : return KB_L | shiftBit;
		case c64KeyColon : return KB_COLON;
		case c64KeySemiColon : return KB_SEMICOLON;
		case c64KeyEquals : return KB_EQUALS;
		case c64KeyReturn : return KB_RETURN;

		case c64KeyCommodore : return KB_COMMODORE;
		case c64KeyLeftShift : return KB_LEFT_SHIFT;
		case c64KeyZ : return KB_Z | shiftBit;
		case c64KeyX : return KB_X | shiftBit;
		case c64KeyC : return KB_C | shiftBit;
		case c64KeyV : return KB_V | shiftBit;
		case c64KeyB : return KB_B | shiftBit;
		case c64KeyN : return KB_N | shiftBit;
		case c64KeyM : return KB_M | shiftBit;
		case c64KeyComma : return KB_COMMA;
		case c64KeyPeriod : return KB_PERIOD;
		case c64KeySlash : return KB_SLASH;
		case c64KeyRightShift : return KB_RIGHT_SHIFT;
		case c64KeyKbUp : return KB_CRSR_UD | SHIFT_BIT;
		case c64KeyKbRight : return KB_CRSR_LR;
		case c64KeyKbDown : return KB_CRSR_UD;
		case c64KeyKbLeft : return KB_CRSR_LR | SHIFT_BIT;

		case c64KeySpace : return KB_SPACE;
		case c64KeyCtrlLock : return KBEX_CTRL_LOCK;
		default: bug_branch("%d", input);
	}
	return 0;
}

static void setC64KBKey(int key, bool pushed)
{
	int row = (key >> 4) & 0xF;
	int col = key  & 0xF;
	int shift = (key >> 8) & 0xF;
	if(pushed)
	{
		keyarr[row] |= 1 << col;
		rev_keyarr[col] |= 1 << row;
		if(shift)
		{
			keyarr[1] |= 1 << 7;
			rev_keyarr[7] |= 1 << 1;
		}
	}
	else
	{
		keyarr[row] &= ~(1 << col);
		rev_keyarr[col] &= ~(1 << row);
		if(shift)
		{
			keyarr[1] &= ~(1 << 7);
			rev_keyarr[7] &= ~(1 << 1);
		}
	}
}

void EmuSystem::handleInputAction(uint state, uint emuKey)
{
	if(emuKey & 0xFF0000) // Joystick
	{

		uint key = emuKey >> 16;
		uint player = (key & IG::bit(5)) ? 2 : 1;
		if(optionSwapJoystickPorts)
		{
			player = (player == 1) ? 2 : 1;
		}
		//logMsg("js %X p %d", key & 0x1F, player);
		if(state == Input::PUSHED)
			setBits(joystick_value[player], key & 0x1F);
		else
			unsetBits(joystick_value[player], key & 0x1F);
	}
	else // Keyboard
	{
		// Special Keys
		switch(emuKey)
		{
			case KBEX_SWAP_JS_PORTS:
			{
				if(state == Input::PUSHED)
				{
					if(optionSwapJoystickPorts)
						optionSwapJoystickPorts = 0;
					else
						optionSwapJoystickPorts = 1;
					popup.post("Swapped Joystick Ports", 1);
				}
				return;
			}
			case KBEX_TOGGLE_VKEYBOARD:
			{
				if(state == Input::PUSHED)
					vController.toggleKeyboard();
				return;
			}
			case KBEX_SHIFT_LOCK:
			{
				if(state == Input::PUSHED)
				{
					toggle(shiftLock);
					vController.updateKeyboardMapping();
				}
				return;
			}
			case KBEX_CTRL_LOCK:
			{
				if(state == Input::PUSHED)
				{
					toggle(ctrlLock);
					setC64KBKey(KB_CTRL, ctrlLock);
				}
				return;
			}
		}
		if(unlikely((emuKey & 0xFF) == KB_NONE))
		{
			return;
		}
		if(unlikely((emuKey & 0xFF) == 0xFF))
		{
			logMsg("pushed restore key");
			machine_set_restore_key(state == Input::PUSHED);
			return;
		}

		setC64KBKey(emuKey, state == Input::PUSHED);
	}
}

static bool isC64DiskExtension(const char *name)
{
	return string_hasDotExtension(name, "d64") ||
			string_hasDotExtension(name, "g64") ||
			string_hasDotExtension(name, "p64") ||
			string_hasDotExtension(name, "x64");
}

static bool isC64TapeExtension(const char *name)
{
	return string_hasDotExtension(name, "t64") ||
			string_hasDotExtension(name, "tap");
}

static bool isC64CartExtension(const char *name)
{
	return string_hasDotExtension(name, "bin") ||
			string_hasDotExtension(name, "crt");
}

static bool isC64Extension(const char *name)
{
	return isC64DiskExtension(name) ||
			isC64TapeExtension(name) ||
			isC64CartExtension(name) ||
			string_hasDotExtension(name, "prg");
}

EmuNameFilterFunc EmuFilePicker::defaultFsFilter = isC64Extension;
EmuNameFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = isC64Extension;

void EmuSystem::resetGame()
{
	assert(gameIsRunning());
	machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
}

static char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'a';
		case 0 ... 9: return 48 + slot;
		default: bug_branch("%d", slot); return 0;
	}
}

FS::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return FS::makePathStringPrintf("%s/%s.%c.vsf", statePath, gameName, saveSlotChar(slot));
}

struct SnapshotTrapData
{
	constexpr SnapshotTrapData() { }
	uint result = STATE_RESULT_IO_ERROR;
	FS::PathString pathStr{};
};

static void loadSnapshotTrap(WORD, void *data)
{
	auto snapData = (SnapshotTrapData*)data;
	logMsg("loading state: %s", snapData->pathStr.data());
	if(machine_read_snapshot(snapData->pathStr.data(), 0) < 0)
		snapData->result = STATE_RESULT_IO_ERROR;
	else
		snapData->result = STATE_RESULT_OK;
}

static void saveSnapshotTrap(WORD, void *data)
{
	auto snapData = (SnapshotTrapData*)data;
	logMsg("saving state: %s", snapData->pathStr.data());
	if(machine_write_snapshot(snapData->pathStr.data(), 1, 1, 0) < 0)
		snapData->result = STATE_RESULT_IO_ERROR;
	else
		snapData->result = STATE_RESULT_OK;
}

int EmuSystem::saveState()
{
	SnapshotTrapData data;
	data.pathStr = sprintStateFilename(saveStateSlot);
	fixFilePermissions(data.pathStr);
	interrupt_maincpu_trigger_trap(saveSnapshotTrap, (void*)&data);
	runFrame(0, 0, 0); // execute cpu trap
	return data.result;
}

int EmuSystem::loadState(int saveStateSlot)
{
	resources_set_int("WarpMode", 0);
	SnapshotTrapData data;
	data.pathStr = sprintStateFilename(saveStateSlot);
	runFrame(0, 0, 0); // run extra frame in case C64 was just started
	interrupt_maincpu_trigger_trap(loadSnapshotTrap, (void*)&data);
	runFrame(0, 0, 0); // execute cpu trap, snapshot load may cause reboot from a C64 model change
	if(data.result != STATE_RESULT_OK)
		return data.result;
	// reload snapshot in case last load caused a reboot
	interrupt_maincpu_trigger_trap(loadSnapshotTrap, (void*)&data);
	runFrame(0, 0, 0); // execute cpu trap
	int result = data.result;
	setC64ModelIsPal(c64Model());
	EmuSystem::configAudioPlayback();
	return result;
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		SnapshotTrapData data;
		data.pathStr = sprintStateFilename(-1);
		fixFilePermissions(data.pathStr);
		interrupt_maincpu_trigger_trap(saveSnapshotTrap, (void*)&data);
		runFrame(0, 0, 0); // execute cpu trap
		if(data.result != STATE_RESULT_OK)
		{
			logErr("error writing auto-save state %s", data.pathStr.data());
		}
	}
}

void EmuSystem::saveBackupMem()
{
	if(gameIsRunning())
	{
		// nothing to do for now
	}
}

bool EmuSystem::vidSysIsPAL() { return isPal; }
uint EmuSystem::multiresVideoBaseX() { return 0; }
uint EmuSystem::multiresVideoBaseY() { return 0; }
bool touchControlsApplicable() { return 1; }

void EmuSystem::clearInputBuffers()
{
	shiftLock = 0;
	ctrlLock = 0;
	mem_zero(keyarr);
	mem_zero(rev_keyarr);
	mem_zero(joystick_value);
}

void EmuSystem::closeSystem()
{
	assert(gameIsRunning());
	logMsg("closing game %s", gameName());
	saveBackupMem();
	resources_set_int("WarpMode", 0);
	tape_image_detach(1);
	file_system_detach_disk(8);
	file_system_detach_disk(9);
	file_system_detach_disk(10);
	file_system_detach_disk(11);
	cartridge_detach_image(-1);
	setC64Model(optionC64Model.val);
	machine_trigger_reset(MACHINE_RESET_MODE_HARD);
}

static void popupC64FirmwareError()
{
	popup.printf(6, 1, "System files missing, place C64, DRIVES, & PRINTER directories from VICE"
		" in a path below, or set a custom path in options:\n"
		#if defined CONFIG_ENV_LINUX && !defined CONFIG_MACHINE_PANDORA
		"%s\n%s\n%s", Base::assetPath(), "~/.local/share/C64.emu", "/usr/share/games/vice");
		#else
		"%s/C64.emu", Base::storagePath());
		#endif
}

static bool initC64()
{
	if(c64IsInit)
		return true;

	logMsg("initializing C64");
  if(init_main() < 0)
  {
  	logErr("error in init_main()");
  	c64FailedInit = true;
  	return false;
	}

  resources_set_int("Drive8Type", DRIVE_TYPE_1541II);
  resources_set_int("Drive9Type", DRIVE_TYPE_NONE);
  resources_set_int("Drive10Type", DRIVE_TYPE_NONE);
  resources_set_int("Drive11Type", DRIVE_TYPE_NONE);
  if(!optionDriveTrueEmulation) // on by default
  	resources_set_int("DriveTrueEmulation", 0);
  c64IsInit = true;
  return true;
}

int EmuSystem::loadGame(const char *path)
{
	if(!initC64())
	{
		popupC64FirmwareError();
		return 0;
	}
	if(c64FailedInit)
	{
		auto &ynAlertView = *new YesNoAlertView{mainWin.win,
			"A previous system file load failed, you must restart the app to run any C64 software", Input::keyInputIsPresent(), "Exit Now", "Cancel"};
		ynAlertView.onYes() =
			[](const Input::Event &e)
			{
				Base::exit();
			};
		modalViewController.pushAndShow(ynAlertView);
		return 0;
	}

	closeGame();
	setupGamePaths(path);
	logMsg("loading %s", path);
	if(autostart_autodetect(path, nullptr, 0, AUTOSTART_MODE_RUN) != 0)
	{
		popup.postError("Error loading file");
		return 0;
	}

	return 1;
}

int EmuSystem::loadGameFromIO(IO &io, const char *origFilename)
{
	return 0; // TODO
}

static void execC64Frame()
{
	// signal C64 thread to execute one frame and wait for it to finish
	execSem.notify();
	execDoneSem.wait();
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	runningFrame = 1;
	// "Warp" mode frame
	if(unlikely(warp_mode_enabled && renderGfx))
	{
		raster_skip_frame(&vicii.raster, 1);
		iterateTimes(8, i)
		{
			execC64Frame();
		}
	}

	// Normal frame
	doAudio = renderAudio;
	raster_skip_frame(&vicii.raster, processGfx == 0);
	execC64Frame();
	if(unlikely(c64VidActiveX != c64VidX || c64VidActiveY != c64VidY))
	{
		logMsg("resizing pixmap to %d,%d", c64VidX, c64VidY);
		c64VidActiveX = c64VidX;
		c64VidActiveY = c64VidY;
		if(optionCropNormalBorders && (c64VidY == 247 || c64VidY == 272))
		{
			logMsg("cropping borders");
			// Crop all vertical borders on NTSC, leaving leftover side borders
			int xBorderSize = 32, yBorderSize = 23;
			int height = 200;
			int startX = yBorderSize, startY = yBorderSize;
			if(c64VidY == 272) // PAL
			{
				// Crop all horizontal borders on PAL, leaving leftover top/bottom borders
				yBorderSize = 32;
				height = 206;
				startX = xBorderSize; startY = xBorderSize;
			}
			int width = 320+(xBorderSize*2 - startX*2);
			int widthPadding = startX*2;
			emuVideo.resizeImage(startX, startY, width, height, c64VidX, c64VidY);
		}
		else
		{
			emuVideo.resizeImage(c64VidX, c64VidY);
		}
	}
	if(renderGfx)
	{
		updateAndDrawEmuVideo();
	}
	runningFrame = 0;
}

void EmuSystem::configAudioRate(double frameTime)
{
	logMsg("set audio rate %d", (int)optionSoundRate);
	pcmFormat.rate = optionSoundRate;
	double systemFrameRate = isPal ? 50.12 : 59.826;
	int mixRate = std::round(optionSoundRate * (systemFrameRate * frameTime));
	int currRate;
	resources_get_int("SoundSampleRate", &currRate);
	if(currRate != mixRate)
	{
		resources_set_int("SoundSampleRate", mixRate);
	}
}

void EmuSystem::savePathChanged() { }

bool EmuSystem::hasInputOptions() { return false; }

void setupSysFilePaths(FS::PathString outPath[3], const FS::PathString &firmwareBasePath)
{
	if(!strlen(firmwareBasePath.data()))
	{
		mem_zero(outPath);
		return;
	}
	logMsg("setup system file path: %s", firmwareBasePath.data());
	string_printf(outPath[0], "%s/C64", firmwareBasePath.data()); // emu_id
	string_printf(outPath[1], "%s/DRIVES", firmwareBasePath.data());
	string_printf(outPath[2], "%s/PRINTER", firmwareBasePath.data());
}

namespace Base
{

CallResult onInit(int argc, char** argv)
{
	emuVideo.initPixmap((char*)pix, pixFmt, 320, 200);
	IG::runOnThread(
		[]()
		{
			execSem.wait();
			logMsg("running C64");
			maincpu_mainloop();
		});

	#if defined CONFIG_ENV_LINUX && !defined CONFIG_MACHINE_PANDORA
	setupSysFilePaths(sysFilePath[1], Base::assetPath());
	setupSysFilePaths(sysFilePath[2], "~/.local/share/C64.emu");
	setupSysFilePaths(sysFilePath[3], "/usr/share/games/vice");
	#else
	{
		auto path = FS::makePathStringPrintf("%s/C64.emu", Base::storagePath());
		setupSysFilePaths(sysFilePath[1], path);
	}
	#endif

	maincpu_early_init();
	machine_setup_context();
	drive_setup_context();
	machine_early_init();

	// Initialize system file locator
	sysfile_init(machine_name);

	if(init_resources() < 0)
	{
		bug_exit("init_resources()");
	}

	// Set factory defaults
	if(resources_set_defaults() < 0)
	{
		bug_exit("resources_set_defaults()");
	}

	/*if(log_init() < 0)
	{
		bug_exit("log_init()");
	}*/

	EmuSystem::pcmFormat.channels = 1;

	static const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build(48./255., 36./255., 144./255., 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build(48./255., 36./255., 144./255., 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build((48./255.) * .4, (36./255.) * .4, (144./255.) * .4, 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};

	mainInitCommon(argc, argv, navViewGrad);
	setupSysFilePaths(sysFilePath[0], firmwareBasePath);
	vController.updateKeyboardMapping();
	setC64Model(optionC64Model.val);
	return OK;
}

}
