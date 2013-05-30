/*  This file is part of GBA.emu.

	PCE.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	PCE.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with PCE.emu.  If not, see <http://www.gnu.org/licenses/> */

#define thisModuleName "main"
#include <logger/interface.h>
#include <util/area2.h>
#include <gfx/GfxSprite.hh>
#include <audio/Audio.hh>
#include <fs/sys.hh>
#include <io/sys.hh>
#include <gui/View.hh>
#include <util/strings.h>
#include <util/time/sys.hh>
#include <EmuSystem.hh>
#include <CommonFrameworkIncludes.hh>
#include <sys/time.h>
#ifdef __APPLE__
	#include <mach/semaphore.h>
	#include <mach/task.h>
	#include <mach/mach.h>
#else
	#include <semaphore.h>
#endif

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

	CLINK void (*vsync_hook)(void);
	CLINK int warp_mode_enabled;
}

const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2013\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nVice Team\nwww.viceteam.org";
#ifdef __APPLE__
	static semaphore_t execSem, execDoneSem;
#else
	static sem_t execSem, execDoneSem;
#endif
static ThreadPThread c64Thread;
static uint16 pix[520*312]  __attribute__ ((aligned (8))) {0};
static bool c64IsInit = 0, isPal = 0,
		runningFrame = 0, doAudio = 0,
		shiftLock = 0, ctrlLock = 0;
static uint c64VidX = 320, c64VidY = 200,
		c64VidActiveX = 0, c64VidActiveY = 0;

#if defined(CONFIG_BASE_IOS) && defined(CONFIG_BASE_IOS_JB)
	const char *firmwareBasePath = "/User/Media/C64.emu";
#else
	FsSys::cPath firmwareBasePath = "";
#endif
static FsSys::cPath sysFilePath[3] { "" };

enum
{
	CFGKEY_DRIVE_TRUE_EMULATION = 256, CFGKEY_AUTOSTART_WARP = 257,
	CFGKEY_AUTOSTART_TDE = 258, CFGKEY_C64_MODEL = 259,
	CFGKEY_BORDER_MODE = 260, CFGKEY_SWAP_JOYSTICK_PORTS = 261,
	CFGKEY_SID_ENGINE = 262, CFGKEY_CROP_NORMAL_BORDERS = 263,
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
	switch(model)
	{
		case C64MODEL_C64_NTSC:
		case C64MODEL_C64C_NTSC:
		case C64MODEL_C64_OLD_NTSC:
			isPal = 0;
		bdefault:
			isPal = 1;
	}
	if(isPal)
	{
		logMsg("C64 model has PAL timings");
	}
	c64model_set(model);
	EmuSystem::configAudioRate();
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
	if(engine < SID_ENGINE_FASTSID || engine > SID_ENGINE_RESID_FP)
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
static Option<OptionMethodFunc<int, c64Model, setC64Model>, uint8>
	optionC64Model(CFGKEY_C64_MODEL, C64MODEL_C64_NTSC);
static Option<OptionMethodFunc<int, borderMode, setBorderMode>, uint8>
	optionBorderMode(CFGKEY_BORDER_MODE, VICII_NORMAL_BORDERS);
static Option<OptionMethodFunc<int, sidEngine, setSidEngine>, uint8>
	optionSidEngine(CFGKEY_SID_ENGINE, SID_ENGINE_FASTSID);
static Byte1Option optionSwapJoystickPorts(CFGKEY_SWAP_JOYSTICK_PORTS, 0);

void EmuSystem::initOptions()
{
}

bool EmuSystem::readConfig(Io *io, uint key, uint readSize)
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
	}
	return 1;
}

void EmuSystem::writeConfig(Io *io)
{
	optionDriveTrueEmulation.writeWithKeyIfNotDefault(io);
	optionAutostartWarp.writeWithKeyIfNotDefault(io);
	optionAutostartTDE.writeWithKeyIfNotDefault(io);
	optionC64Model.writeWithKeyIfNotDefault(io);
	optionBorderMode.writeWithKeyIfNotDefault(io);
	optionCropNormalBorders.writeWithKeyIfNotDefault(io);
	optionSidEngine.writeWithKeyIfNotDefault(io);
	optionSwapJoystickPorts.writeWithKeyIfNotDefault(io);
}

const uint EmuSystem::maxPlayers = 2;
uint EmuSystem::aspectRatioX = 4, EmuSystem::aspectRatioY = 3;
#include "CommonGui.hh"

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

static const uint SHIFT_BIT = BIT(8);

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
	const uint p2Bit = player ? BIT(5) : 0;
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
	const uint p2Bit = BIT(5);
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
		uint player = (key & BIT(5)) ? 2 : 1;
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

static int c64DiskExtensionFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isC64DiskExtension(name);
}

static int c64TapeExtensionFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isC64TapeExtension(name);
}

static int c64CartExtensionFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isC64CartExtension(name);
}

static bool isC64Extension(const char *name)
{
	return isC64DiskExtension(name) ||
			isC64TapeExtension(name) ||
			isC64CartExtension(name) ||
			string_hasDotExtension(name, "prg");
}

static int c64FsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isC64Extension(name);
}

FsDirFilterFunc EmuFilePicker::defaultFsFilter = c64FsFilter;
FsDirFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = c64FsFilter;

static const PixelFormatDesc *pixFmt = &PixelFormatRGB565;

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

void EmuSystem::sprintStateFilename(char *str, size_t size, int slot, const char *statePath, const char *gameName)
{
	snprintf(str, size, "%s/%s.%c.vsf", statePath, gameName, saveSlotChar(slot));
}

struct SnapshotTrapData
{
	constexpr SnapshotTrapData() { }
	uint result = STATE_RESULT_IO_ERROR;
	FsSys::cPath pathStr {0};
};

static void loadSnapshotTrap(WORD, void *data)
{
	auto snapData = (SnapshotTrapData*)data;
	logMsg("loading state: %s", snapData->pathStr);
	if(machine_read_snapshot(snapData->pathStr, 0) < 0)
		snapData->result = STATE_RESULT_IO_ERROR;
	else
		snapData->result = STATE_RESULT_OK;
}

static void saveSnapshotTrap(WORD, void *data)
{
	auto snapData = (SnapshotTrapData*)data;
	logMsg("saving state: %s", snapData->pathStr);
	if(machine_write_snapshot(snapData->pathStr, 1, 1, 0) < 0)
		snapData->result = STATE_RESULT_IO_ERROR;
	else
		snapData->result = STATE_RESULT_OK;
}

int EmuSystem::saveState()
{
	SnapshotTrapData data;
	sprintStateFilename(data.pathStr, saveStateSlot);
	if(Config::envIsIOSJB)
		fixFilePermissions(data.pathStr);
	interrupt_maincpu_trigger_trap(saveSnapshotTrap, (void*)&data);
	runFrame(0, 0, 0); // execute cpu trap
	return data.result;
}

int EmuSystem::loadState(int saveStateSlot)
{
	SnapshotTrapData data;
	sprintStateFilename(data.pathStr, saveStateSlot);
	resources_set_int("WarpMode", 0);
	runFrame(0, 0, 0); // run extra frame in case C64 was just started
	interrupt_maincpu_trigger_trap(loadSnapshotTrap, (void*)&data);
	runFrame(0, 0, 0); // execute cpu trap
	return data.result;
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		SnapshotTrapData data;
		sprintStateFilename(data.pathStr, -1);
		if(Config::envIsIOSJB)
			fixFilePermissions(data.pathStr);
		interrupt_maincpu_trigger_trap(saveSnapshotTrap, (void*)&data);
		runFrame(0, 0, 0); // execute cpu trap
		if(data.result != STATE_RESULT_OK)
		{
			logErr("error writing auto-save state %s", data.pathStr);
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
	logMsg("closing game %s", gameName);
	saveBackupMem();
	resources_set_int("WarpMode", 0);
	tape_image_detach(1);
	file_system_detach_disk(8);
	cartridge_detach_image(-1);
	machine_trigger_reset(MACHINE_RESET_MODE_HARD);
}

static void popupC64FirmwareError()
{
	popup.printf(6, 1, "Can't start C64 with firmware path %s,"
			" make sure it contains the C64, DRIVES, PRINTER directories from Vice", firmwareBasePath);
}

static bool initC64()
{
	if(c64IsInit)
		return 1;

	logMsg("initializing C64");
  if(init_main() < 0)
  {
  	logErr("error in init_main()");
  	return 0;
	}

  resources_set_int("Drive8Type", DRIVE_TYPE_1541II);
  if(!optionDriveTrueEmulation) // on by default
  	resources_set_int("DriveTrueEmulation", 0);
  c64IsInit = 1;
  return 1;
}

int EmuSystem::loadGame(const char *path)
{
	if(!initC64())
	{
		popupC64FirmwareError();
		return 0;
	}

	closeGame();
	setupGamePaths(path);

	if(string_hasDotExtension(path, "crt")) // ROM
	{
		logMsg("loading ROM %s", path);
		if(cartridge_attach_image(CARTRIDGE_CRT, path) < 0)
		{
			popup.postError("Error loading ROM");
			return 0;
		}
	}
	else
	{
		logMsg("loading disk/tape/prg %s", path);
		if(autostart_autodetect(path, nullptr, 0, AUTOSTART_MODE_RUN) != 0)
		{
			popup.postError("Error loading disk/tape");
			return 0;
		}
	}

	return 1;
}

static void execC64Frame()
{
	// signal C64 thread to execute one frame and wait for it to finish
	#ifdef __APPLE__
		semaphore_signal(execSem);
		semaphore_wait(execDoneSem);
	#else
		sem_post(&execSem);
		sem_wait(&execDoneSem);
	#endif
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
			emuView.resizeImage(startX, startY, width, height, c64VidX, c64VidY);
		}
		else
		{
			emuView.resizeImage(c64VidX, c64VidY);
		}
	}
	if(renderGfx)
	{
		emuView.updateAndDrawContent();
	}
	runningFrame = 0;
}

namespace Input
{

void onInputEvent(const Input::Event &e)
{
	handleInputEvent(e);
}

}

void EmuSystem::configAudioRate()
{
	Audio::setHintPcmFramesPerWrite(isPal ? 950 : 800);
	logMsg("set audio rate %d", (int)optionSoundRate);
	pcmFormat.rate = optionSoundRate;
	int mixRate = optionSoundRate * (/*isPal ? 1. :*/ .99715);
	int currRate;
	resources_get_int("SoundSampleRate", &currRate);
	if(currRate != mixRate)
	{
		resources_set_int("SoundSampleRate", mixRate);
	}
}

void EmuSystem::savePathChanged() { }

void setupSysFilePaths(const char *firmwareBasePath)
{
	string_printf(sysFilePath[0], "%s/C64", firmwareBasePath); // emu_id
	string_printf(sysFilePath[1], "%s/DRIVES", firmwareBasePath);
	string_printf(sysFilePath[2], "%s/PRINTER", firmwareBasePath);
}

namespace Base
{

void onAppMessage(int type, int shortArg, int intArg, int intArg2) { }

CallResult onInit(int argc, char** argv)
{
  emuView.initPixmap((uchar*)pix, pixFmt, 320, 200);
	#ifdef __APPLE__
  {
  	auto ret = semaphore_create(mach_task_self(), &execSem, SYNC_POLICY_FIFO, 0);
  	assert(ret == KERN_SUCCESS);
  	ret = semaphore_create(mach_task_self(), &execDoneSem, SYNC_POLICY_FIFO, 0);
  	assert(ret == KERN_SUCCESS);
  }
	#else
		sem_init(&execSem, 0, 0);
		sem_init(&execDoneSem, 0, 0);
	#endif
	c64Thread.create(1,
		[](ThreadPThread &thread)
		{
			#ifdef __APPLE__
				semaphore_wait(execSem);
			#else
				sem_wait(&execSem);
			#endif
			logMsg("running C64");
			maincpu_mainloop();
			return 0;
		}
	);

	#if !(defined(CONFIG_BASE_IOS) && defined(CONFIG_BASE_IOS_JB))
		#if defined CONFIG_BASE_X11
			// check for firmware path in app dir
			FsSys::cPath appDirPath = "";
			string_printf(appDirPath, "%s/C64", Base::appPath);
			if(FsSys::fileExists(appDirPath))
			{
				strcpy(firmwareBasePath, Base::appPath);
			}
			else
		#endif
			{
				string_printf(firmwareBasePath, "%s/C64.emu", Base::storagePath());
			}
	#endif
	logMsg("firmware base path: %s", firmwareBasePath);
	setupSysFilePaths(firmwareBasePath);

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

  mainInitCommon();
  EmuSystem::pcmFormat.channels = 1;
  vController.updateKeyboardMapping();

	return OK;
}

CallResult onWindowInit()
{
	static const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, VertexColorPixelFormat.build(48./255., 36./255., 144./255., 1.) },
		{ .3, VertexColorPixelFormat.build(48./255., 36./255., 144./255., 1.) },
		{ .97, VertexColorPixelFormat.build((48./255.) * .4, (36./255.) * .4, (144./255.) * .4, 1.) },
		{ 1., VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};

	mainInitWindowCommon(navViewGrad);
  logMsg("done init");
	return OK;
}

}

CLINK FILE *sysfile_open(const char *name, char **complete_path_return, const char *open_mode)
{
	forEachInArray(sysFilePath, p)
	{
		FsSys::cPath fullPath;
		string_printf(fullPath, "%s/%s", *p, name);
		auto file = fopen(fullPath, open_mode);
		if(file)
		{
			if(complete_path_return)
			{
				*complete_path_return = strdup(fullPath);
				if(!*complete_path_return)
				{
					logErr("out of memory trying to allocate string in sysfile_open");
					fclose(file);
					return nullptr;
				}
			}
			return file;
		}
	}
	logErr("can't open %s in system paths", name);
	return nullptr;
}

CLINK int sysfile_locate(const char *name, char **complete_path_return)
{
	forEachInArray(sysFilePath, p)
	{
		FsSys::cPath fullPath;
		string_printf(fullPath, "%s/%s", *p, name);
		if(FsSys::fileExists(fullPath))
		{
			if(complete_path_return)
			{
				*complete_path_return = strdup(fullPath);
				if(!*complete_path_return)
				{
					logErr("out of memory trying to allocate string in sysfile_locate");
					return -1;
				}
			}
			return 0;
		}
	}
	logErr("%s not found in system paths", name);
	return -1;
}

CLINK int sysfile_load(const char *name, BYTE *dest, int minsize, int maxsize)
{
	forEachInArray(sysFilePath, p)
	{
		FsSys::cPath complete_path;
		string_printf(complete_path, "%s/%s", *p, name);
		auto file = IoSys::open(complete_path);
		if(file)
		{
			//logMsg("loading system file: %s", complete_path);
			size_t rsize = file->size();
			bool load_at_end;
			if(minsize < 0)
			{
				minsize = -minsize;
				load_at_end = 0;
			}
			else
			{
				load_at_end = 1;
			}
			if(rsize < ((size_t)minsize))
			{
				logErr("ROM %s: short file", complete_path);
				goto fail;
			}
			if(rsize == ((size_t)maxsize + 2))
			{
				logWarn("ROM `%s': two bytes too large - removing assumed start address", complete_path);
				if(fread((char*)dest, 1, 2, file) < 2)
				{
					goto fail;
				}
				rsize -= 2;
			}
			if(load_at_end && rsize < ((size_t)maxsize))
			{
				dest += maxsize - rsize;
			}
			else if(rsize > ((size_t)maxsize))
			{
				logWarn("ROM `%s': long file, discarding end.", complete_path);
				rsize = maxsize;
			}
			if((rsize = fread((char *)dest, 1, rsize, file)) < ((size_t)minsize))
				goto fail;

			file->close();
			return (int)rsize;

			fail:
				logErr("failed loading system file: %s", name);
				file->close();
				return -1;
		}
	}
	logErr("can't load %s in system paths", name);
	return -1;
}

// Number of timer units per second, unused
CLINK signed long vsyncarch_frequency(void)
{
  // Microseconds resolution
  return 1000000;
}

CLINK unsigned long vsyncarch_gettime(void)
{
//  struct timeval now;
//  gettimeofday(&now, NULL);
//  return 1000000UL * now.tv_sec + now.tv_usec;
	bug_exit("shouldn't be called");
	return 0;
}

CLINK void vsyncarch_init(void) { }

CLINK void vsyncarch_display_speed(double speed, double frame_rate, int warp_enabled) { }

CLINK void vsyncarch_sleep(signed long delay)
{
	logMsg("called vsyncarch_sleep() with %ld", delay);
	bug_exit("shouldn't be called");
}

CLINK void vsyncarch_presync(void) { }

CLINK void vsyncarch_postsync(void) { }

CLINK int vsync_do_vsync(struct video_canvas_s *c, int been_skipped)
{
	sound_flush();
	kbdbuf_flush();
	vsync_hook();
	assert(EmuSystem::gameIsRunning());
	if(likely(runningFrame))
	{
		#ifdef __APPLE__
			semaphore_signal(execDoneSem);
			semaphore_wait(execSem);
		#else
			sem_post(&execDoneSem);
			sem_wait(&execSem);
		#endif
	}
	else
	{
		logMsg("spurious vsync_do_vsync()");
	}
	return 0;
}

CLINK void video_arch_canvas_init(struct video_canvas_s *canvas)
{
	logMsg("created canvas with size %d,%d", canvas->draw_buffer->canvas_width, canvas->draw_buffer->canvas_height);
	canvas->video_draw_buffer_callback = nullptr;
}

CLINK int video_canvas_set_palette(video_canvas_t *c, struct palette_s *palette)
{
	if(!palette)
	{
		return 0; // no palette, nothing to do
	}

	c->palette = palette;

	iterateTimes(palette->num_entries, i)
	{
		auto col = pixFmt->build(palette->entries[i].red/255., palette->entries[i].green/255., palette->entries[i].blue/255., 0.);
		logMsg("set color %d to %X", i, col);
		video_render_setphysicalcolor(c->videoconfig, i, col, pixFmt->bitsPerPixel);
	}

	iterateTimes(256, i)
	{
		video_render_setrawrgb(i, pixFmt->build(i/255., 0., 0., 0.), pixFmt->build(0., i/255., 0., 0.), pixFmt->build(0., 0., i/255., 0.));
	}
	video_render_initraw(c->videoconfig);

	return 0;
}

CLINK void video_canvas_refresh(struct video_canvas_s *canvas, unsigned int xs, unsigned int ys, unsigned int xi, unsigned int yi, unsigned int w, unsigned int h)
{
	if(!EmuSystem::gameIsRunning())
	{
		logMsg("refreshing canvas while game isn't running");
	}
	video_canvas_render(canvas, (BYTE*)pix, w, h, xs, ys, xi, yi, emuView.vidPix.pitch, pixFmt->bitsPerPixel);
}

CLINK void video_canvas_resize(struct video_canvas_s *canvas, char resize_canvas)
{
	c64VidX = canvas->draw_buffer->canvas_width;
	c64VidY = canvas->draw_buffer->canvas_height;
	logMsg("resized canvas to %d,%d", c64VidX, c64VidY);
}

CLINK video_canvas_t *video_canvas_create(video_canvas_t *canvas, unsigned int *width, unsigned int *height, int mapped)
{
	logMsg("renderer %d", canvas->videoconfig->rendermode);
	canvas->videoconfig->filter = VIDEO_FILTER_NONE;
	return canvas;
}

static int soundInit(const char *param, int *speed,
		   int *fragsize, int *fragnr, int *channels)
{
	logMsg("sound init %dHz, %d fragsize, %d fragnr, %d channels", *speed, *fragsize, *fragnr, *channels);
	assert(*channels == 1);
	return 0;
}

static int soundWrite(SWORD *pbuf, size_t nr)
{
	//logMsg("sound write %zd", nr);
	if(likely(doAudio))
		Audio::writePcm((uchar*)pbuf, nr);
	return 0;
}

static sound_device_t soundDevice =
{
    "sound",
    soundInit,
    soundWrite,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    0,
    2
};

CLINK int sound_init_dummy_device()
{
	return sound_register_device(&soundDevice);
}
