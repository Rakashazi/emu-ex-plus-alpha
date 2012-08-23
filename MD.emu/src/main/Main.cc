/*  This file is part of NES.emu.

	NES.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NES.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NES.emu.  If not, see <http://www.gnu.org/licenses/> */

#define thisModuleName "main"
#include <resource2/image/png/ResourceImagePng.h>
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

#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	#define CONFIG_FILE_NAME "MdEmu.config"
#else
	#define CONFIG_FILE_NAME "config"
#endif

static const GfxLGradientStopDesc navViewGrad[] =
{
	{ .0, VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	{ .03, VertexColorPixelFormat.build(0., 0., 1. * .4, 1.) },
	{ .3, VertexColorPixelFormat.build(0., 0., 1. * .4, 1.) },
	{ .97, VertexColorPixelFormat.build(0., 0., .6 * .4, 1.) },
	{ 1., VertexColorPixelFormat.build(.5, .5, .5, 1.) },
};

static const char *touchConfigFaceBtnName = "A/B/C", *touchConfigCenterBtnName = "Mode/Start";
static const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nGenesis Plus Team\ncgfm2.emuviews.com";
const uint EmuSystem::maxPlayers = 4;
static const uint systemFaceBtns = 6, systemCenterBtns = 2;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 1;
uint EmuSystem::aspectRatioX = 4, EmuSystem::aspectRatioY = 3;
#define systemAspectRatioString "4:3"
#include "CommonGui.hh"

namespace EmuControls
{

KeyCategory category[categories] =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
		KeyCategory("Gamepad Controls", gamepadName, gameActionKeys),
};

}

#include "system.h"
#include "loadrom.h"
#include "md_cart.h"
#include "input.h"
#include "io_ctrl.h"
#include "sram.h"
#include "state.h"
#include "sound.h"
#include "vdp_ctrl.h"
#include "genesis.h"
#include "genplus-config.h"
#include <scd/scd.h>

t_config config = { 0 };
uint config_ym2413_enabled = 1;
static int8 mdInputPortDev[2] = { -1, -1 };

// controls

enum
{
	mdKeyIdxUp = EmuControls::systemKeyMapStart,
	mdKeyIdxRight,
	mdKeyIdxDown,
	mdKeyIdxLeft,
	mdKeyIdxLeftUp,
	mdKeyIdxRightUp,
	mdKeyIdxRightDown,
	mdKeyIdxLeftDown,
	mdKeyIdxMode,
	mdKeyIdxStart,
	mdKeyIdxA,
	mdKeyIdxB,
	mdKeyIdxC,
	mdKeyIdxX,
	mdKeyIdxY,
	mdKeyIdxZ,
	mdKeyIdxATurbo,
	mdKeyIdxBTurbo,
	mdKeyIdxCTurbo,
	mdKeyIdxXTurbo,
	mdKeyIdxYTurbo,
	mdKeyIdxZTurbo
};

enum {
	CFGKEY_MDKEY_UP = 256, CFGKEY_MDKEY_RIGHT = 257,
	CFGKEY_MDKEY_DOWN = 258, CFGKEY_MDKEY_LEFT = 259,
	CFGKEY_MDKEY_MODE = 260, CFGKEY_MDKEY_START = 261,
	CFGKEY_MDKEY_A = 262, CFGKEY_MDKEY_B = 263,
	CFGKEY_MDKEY_C = 264, CFGKEY_MDKEY_X = 265,
	CFGKEY_MDKEY_Y = 266, CFGKEY_MDKEY_Z = 267,
	CFGKEY_MDKEY_A_TURBO = 268, CFGKEY_MDKEY_B_TURBO = 269,
	CFGKEY_MDKEY_C_TURBO = 270, CFGKEY_MDKEY_X_TURBO = 271,
	CFGKEY_MDKEY_Y_TURBO = 272, CFGKEY_MDKEY_Z_TURBO = 273,
	CFGKEY_MDKEY_LEFT_UP = 274, CFGKEY_MDKEY_RIGHT_UP = 275,
	CFGKEY_MDKEY_RIGHT_DOWN = 276, CFGKEY_MDKEY_LEFT_DOWN = 277,
	CFGKEY_MDKEY_BIG_ENDIAN_SRAM = 278, CFGKEY_MDKEY_SMS_FM = 279,
	CFGKEY_MDKEY_6_BTN_PAD = 280, CFGKEY_MD_CD_BIOS_USA_PATH = 281,
	CFGKEY_MD_CD_BIOS_JPN_PATH = 282, CFGKEY_MD_CD_BIOS_EUR_PATH = 283
};

static bool usingMultiTap = 0;
static BasicByteOption optionBigEndianSram(CFGKEY_MDKEY_BIG_ENDIAN_SRAM, 0);
static BasicByteOption optionSmsFM(CFGKEY_MDKEY_SMS_FM, 1);
static BasicByteOption option6BtnPad(CFGKEY_MDKEY_6_BTN_PAD, 0);
FsSys::cPath cdBiosUSAPath = "", cdBiosJpnPath = "", cdBiosEurPath = "";
static PathOption<CFGKEY_MD_CD_BIOS_USA_PATH> optionCDBiosUsaPath(cdBiosUSAPath, sizeof(cdBiosUSAPath), "");
static PathOption<CFGKEY_MD_CD_BIOS_JPN_PATH> optionCDBiosJpnPath(cdBiosJpnPath, sizeof(cdBiosJpnPath), "");
static PathOption<CFGKEY_MD_CD_BIOS_EUR_PATH> optionCDBiosEurPath(cdBiosEurPath, sizeof(cdBiosEurPath), "");

void EmuSystem::initOptions()
{
	#ifndef CONFIG_BASE_ANDROID
	optionFrameSkip.initDefault(optionFrameSkipAuto);
	#endif
	optionTouchCtrlBtnSpace.initDefault(100);
	optionTouchCtrlBtnStagger.initDefault(3);
}

bool EmuSystem::readConfig(Io *io, uint key, uint readSize)
{
	switch(key)
	{
		bcase CFGKEY_MDKEY_UP: readKeyConfig2(io, mdKeyIdxUp, readSize);
		bcase CFGKEY_MDKEY_RIGHT: readKeyConfig2(io, mdKeyIdxRight, readSize);
		bcase CFGKEY_MDKEY_DOWN: readKeyConfig2(io, mdKeyIdxDown, readSize);
		bcase CFGKEY_MDKEY_LEFT: readKeyConfig2(io, mdKeyIdxLeft, readSize);
		bcase CFGKEY_MDKEY_LEFT_UP: readKeyConfig2(io, mdKeyIdxLeftUp, readSize);
		bcase CFGKEY_MDKEY_RIGHT_UP: readKeyConfig2(io, mdKeyIdxRightUp, readSize);
		bcase CFGKEY_MDKEY_RIGHT_DOWN: readKeyConfig2(io, mdKeyIdxRightDown, readSize);
		bcase CFGKEY_MDKEY_LEFT_DOWN: readKeyConfig2(io, mdKeyIdxLeftDown, readSize);
		bcase CFGKEY_MDKEY_MODE: readKeyConfig2(io, mdKeyIdxMode, readSize);
		bcase CFGKEY_MDKEY_START: readKeyConfig2(io, mdKeyIdxStart, readSize);
		bcase CFGKEY_MDKEY_A: readKeyConfig2(io, mdKeyIdxA, readSize);
		bcase CFGKEY_MDKEY_B: readKeyConfig2(io, mdKeyIdxB, readSize);
		bcase CFGKEY_MDKEY_C: readKeyConfig2(io, mdKeyIdxC, readSize);
		bcase CFGKEY_MDKEY_X: readKeyConfig2(io, mdKeyIdxX, readSize);
		bcase CFGKEY_MDKEY_Y: readKeyConfig2(io, mdKeyIdxY, readSize);
		bcase CFGKEY_MDKEY_Z: readKeyConfig2(io, mdKeyIdxZ, readSize);
		bcase CFGKEY_MDKEY_A_TURBO: readKeyConfig2(io, mdKeyIdxATurbo, readSize);
		bcase CFGKEY_MDKEY_B_TURBO: readKeyConfig2(io, mdKeyIdxBTurbo, readSize);
		bcase CFGKEY_MDKEY_C_TURBO: readKeyConfig2(io, mdKeyIdxCTurbo, readSize);
		bcase CFGKEY_MDKEY_X_TURBO: readKeyConfig2(io, mdKeyIdxXTurbo, readSize);
		bcase CFGKEY_MDKEY_Y_TURBO: readKeyConfig2(io, mdKeyIdxYTurbo, readSize);
		bcase CFGKEY_MDKEY_Z_TURBO: readKeyConfig2(io, mdKeyIdxZTurbo, readSize);
		bcase CFGKEY_MDKEY_BIG_ENDIAN_SRAM: optionBigEndianSram.readFromIO(io, readSize);
		bcase CFGKEY_MDKEY_SMS_FM: optionSmsFM.readFromIO(io, readSize);
		bcase CFGKEY_MDKEY_6_BTN_PAD: option6BtnPad.readFromIO(io, readSize);
		bcase CFGKEY_MD_CD_BIOS_USA_PATH: optionCDBiosUsaPath.readFromIO(io, readSize);
		bcase CFGKEY_MD_CD_BIOS_JPN_PATH: optionCDBiosJpnPath.readFromIO(io, readSize);
		bcase CFGKEY_MD_CD_BIOS_EUR_PATH: optionCDBiosEurPath.readFromIO(io, readSize);
		bdefault: return 0;
	}
	return 1;
}

void EmuSystem::writeConfig(Io *io)
{
	if(!optionBigEndianSram.isDefault())
	{
		io->writeVar((uint16)optionBigEndianSram.ioSize());
		optionBigEndianSram.writeToIO(io);
	}
	if(!optionSmsFM.isDefault())
	{
		io->writeVar((uint16)optionSmsFM.ioSize());
		optionSmsFM.writeToIO(io);
	}
	if(!option6BtnPad.isDefault())
	{
		io->writeVar((uint16)option6BtnPad.ioSize());
		option6BtnPad.writeToIO(io);
	}
	optionCDBiosUsaPath.writeToIO(io);
	optionCDBiosJpnPath.writeToIO(io);
	optionCDBiosEurPath.writeToIO(io);
	writeKeyConfig2(io, mdKeyIdxUp, CFGKEY_MDKEY_UP);
	writeKeyConfig2(io, mdKeyIdxRight, CFGKEY_MDKEY_RIGHT);
	writeKeyConfig2(io, mdKeyIdxDown, CFGKEY_MDKEY_DOWN);
	writeKeyConfig2(io, mdKeyIdxLeft, CFGKEY_MDKEY_LEFT);
	writeKeyConfig2(io, mdKeyIdxLeftUp, CFGKEY_MDKEY_LEFT_UP);
	writeKeyConfig2(io, mdKeyIdxRightUp, CFGKEY_MDKEY_RIGHT_UP);
	writeKeyConfig2(io, mdKeyIdxRightDown, CFGKEY_MDKEY_RIGHT_DOWN);
	writeKeyConfig2(io, mdKeyIdxLeftDown, CFGKEY_MDKEY_LEFT_DOWN);
	writeKeyConfig2(io, mdKeyIdxMode, CFGKEY_MDKEY_MODE);
	writeKeyConfig2(io, mdKeyIdxStart, CFGKEY_MDKEY_START);
	writeKeyConfig2(io, mdKeyIdxA, CFGKEY_MDKEY_A);
	writeKeyConfig2(io, mdKeyIdxB, CFGKEY_MDKEY_B);
	writeKeyConfig2(io, mdKeyIdxC, CFGKEY_MDKEY_C);
	writeKeyConfig2(io, mdKeyIdxX, CFGKEY_MDKEY_X);
	writeKeyConfig2(io, mdKeyIdxY, CFGKEY_MDKEY_Y);
	writeKeyConfig2(io, mdKeyIdxZ, CFGKEY_MDKEY_Z);
	writeKeyConfig2(io, mdKeyIdxATurbo, CFGKEY_MDKEY_A_TURBO);
	writeKeyConfig2(io, mdKeyIdxBTurbo, CFGKEY_MDKEY_B_TURBO);
	writeKeyConfig2(io, mdKeyIdxCTurbo, CFGKEY_MDKEY_C_TURBO);
	writeKeyConfig2(io, mdKeyIdxXTurbo, CFGKEY_MDKEY_X_TURBO);
	writeKeyConfig2(io, mdKeyIdxYTurbo, CFGKEY_MDKEY_Y_TURBO);
	writeKeyConfig2(io, mdKeyIdxZTurbo, CFGKEY_MDKEY_Z_TURBO);
}

uint isROMExtension(const char *name)
{
	return string_hasDotExtension(name, "bin") || string_hasDotExtension(name, "smd") ||
			string_hasDotExtension(name, "md") || string_hasDotExtension(name, "gen")
		#ifndef NO_SYSTEM_PBC
			|| string_hasDotExtension(name, "sms")
		#endif
			;
}

static bool isMDExtension(const char *name)
{
	return isROMExtension(name) || string_hasDotExtension(name, "zip");
}

static bool isMDCDExtension(const char *name)
{
	return string_hasDotExtension(name, "bin") || string_hasDotExtension(name, "iso");
}

static int mdROMFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isMDExtension(name);
}

static int mdFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isMDExtension(name)
	#ifndef NO_SCD
		|| isMDCDExtension(name)
	#endif
	;
}

FsDirFilterFunc EmuFilePicker::defaultFsFilter = mdFsFilter;
FsDirFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = mdROMFsFilter;

#include "MdOptionView.hh"
static MdOptionView oCategoryMenu;
#include "MdMenuView.hh"
static MdMenuView mMenu;

static const PixelFormatDesc *pixFmt = &PixelFormatRGB565;

static const uint mdMaxResX = 320, mdMaxResY = 240;
static int mdResX = 256, mdResY = 224;
static uint16 nativePixBuff[mdMaxResX*mdMaxResY] __attribute__ ((aligned (8)));
t_bitmap bitmap = { (uint8*)nativePixBuff, mdResY, mdResX * pixFmt->bytesPerPixel };

static uint ptrInputToSysButton(uint input)
{
	switch(input)
	{
		case SysVController::F_ELEM: return INPUT_A;
		case SysVController::F_ELEM+1: return INPUT_B;
		case SysVController::F_ELEM+2: return INPUT_C;
		case SysVController::F_ELEM+3: return INPUT_X;
		case SysVController::F_ELEM+4: return INPUT_Y;
		case SysVController::F_ELEM+5: return INPUT_Z;

		case SysVController::C_ELEM: return INPUT_MODE;
		case SysVController::C_ELEM+1: return INPUT_START;

		case SysVController::D_ELEM: return INPUT_UP | INPUT_LEFT;
		case SysVController::D_ELEM+1: return INPUT_UP;
		case SysVController::D_ELEM+2: return INPUT_UP | INPUT_RIGHT;
		case SysVController::D_ELEM+3: return INPUT_LEFT;
		case SysVController::D_ELEM+5: return INPUT_RIGHT;
		case SysVController::D_ELEM+6: return INPUT_DOWN | INPUT_LEFT;
		case SysVController::D_ELEM+7: return INPUT_DOWN;
		case SysVController::D_ELEM+8: return INPUT_DOWN | INPUT_RIGHT;
		default: bug_branch("%d", input); return 0;
	}
}

void EmuSystem::handleOnScreenInputAction(uint state, uint vCtrlKey)
{
	handleInputAction(pointerInputPlayer, state, ptrInputToSysButton(vCtrlKey));
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	switch(input)
	{
		case mdKeyIdxUp: return INPUT_UP;
		case mdKeyIdxRight: return INPUT_RIGHT;
		case mdKeyIdxDown: return INPUT_DOWN;
		case mdKeyIdxLeft: return INPUT_LEFT;
		case mdKeyIdxLeftUp: return INPUT_LEFT | INPUT_UP;
		case mdKeyIdxRightUp: return INPUT_RIGHT | INPUT_UP;
		case mdKeyIdxRightDown: return INPUT_RIGHT | INPUT_DOWN;
		case mdKeyIdxLeftDown: return INPUT_LEFT | INPUT_DOWN;
		case mdKeyIdxMode: return INPUT_MODE;
		case mdKeyIdxStart: return INPUT_START;
		case mdKeyIdxATurbo: turbo = 1;
		case mdKeyIdxA: return INPUT_A;
		case mdKeyIdxBTurbo: turbo = 1;
		case mdKeyIdxB: return INPUT_B;
		case mdKeyIdxCTurbo: turbo = 1;
		case mdKeyIdxC: return INPUT_C;
		case mdKeyIdxXTurbo: turbo = 1;
		case mdKeyIdxX: return INPUT_X;
		case mdKeyIdxYTurbo: turbo = 1;
		case mdKeyIdxY: return INPUT_Y;
		case mdKeyIdxZTurbo: turbo = 1;
		case mdKeyIdxZ: return INPUT_Z;
		default: bug_branch("%d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(uint player, uint state, uint emuKey)
{
	uint16 &padData = input.pad[usingMultiTap ? player : player * 4];
	if(state == INPUT_PUSHED)
		setBits(padData, emuKey);
	else
		unsetBits(padData, emuKey);
}

void commitVideoFrame()
{
	if(unlikely(bitmap.viewport.w != mdResX || bitmap.viewport.h != mdResY))
	{
		mdResX = bitmap.viewport.w;
		mdResY = bitmap.viewport.h;
		bitmap.pitch = mdResX * pixFmt->bytesPerPixel;
		emuView.resizeImage(mdResX, mdResY);
		if(optionImageZoom == optionImageZoomIntegerOnly)
			emuView.placeEmu();
	}
	emuView.updateAndDrawContent();
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	//logMsg("frame start");
	system_frame(!processGfx, renderGfx);

	int frames = audio_update();
	if(renderAudio)
	{
		//int frames = audio_update();
		//logMsg("%d frames", frames);
		if(likely(frames))
			Audio::writePcm((uchar*)snd.buffer, frames);
	}
	//logMsg("frame end");
}

bool EmuSystem::vidSysIsPAL() { return vdp_pal; }
static bool touchControlsApplicable() { return 1; }

void EmuSystem::resetGame()
{
	assert(gameIsRunning());
	if(sCD.isActive)
		system_reset();
	else
		gen_reset(0);
}

static char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'A';
		case 0 ... 9: return '0' + slot;
		default: bug_branch("%d", slot); return 0;
	}
}

void EmuSystem::sprintStateFilename(char *str, size_t size, int slot, const char *gamePath, const char *gameName)
{
	snprintf(str, size, "%s/%s.0%c.gp", gamePath, gameName, saveSlotChar(slot));
}

template <size_t S>
static void sprintSaveFilename(char (&str)[S])
{
	snprintf(str, S, "%s/%s.srm", EmuSystem::gamePath, EmuSystem::gameName);
}

template <size_t S>
static void sprintBRAMSaveFilename(char (&str)[S])
{
	snprintf(str, S, "%s/%s.brm", EmuSystem::gamePath, EmuSystem::gameName);
}

static const uint maxSaveStateSize = STATE_SIZE+4;

static int saveMDState(const char *path)
{
	//static uchar stateData[maxSaveStateSize] ATTRS(aligned(4));
	uchar *stateData = (uchar*)malloc(maxSaveStateSize);
	if(!stateData)
		return STATE_RESULT_IO_ERROR;
	logMsg("saving state data");
	int size = state_save(stateData);
	logMsg("writing to file");
	CallResult ret;
	if((ret = IoSys::writeToNewFile(path, stateData, size)) != OK)
	{
		free(stateData);
		logMsg("error writing state file");
		switch(ret)
		{
			case PERMISSION_DENIED: return STATE_RESULT_NO_FILE_ACCESS;
			default: return STATE_RESULT_IO_ERROR;
		}
	}
	free(stateData);
	logMsg("wrote %d byte state", size);
	return STATE_RESULT_OK;
}

static int loadMDState(const char *path)
{
	CallResult ret;
	Io *f = IoSys::open(path, 0, &ret);
	if(!f)
	{
		switch(ret)
		{
			case PERMISSION_DENIED: return STATE_RESULT_NO_FILE_ACCESS;
			case NOT_FOUND: return STATE_RESULT_NO_FILE;
			default: return STATE_RESULT_IO_ERROR;
		}
	}

	const uchar *stateData = f->mmapConst();
	if(!stateData)
	{
		delete f;
		return STATE_RESULT_IO_ERROR;
	}
	if(state_load(stateData) <= 0)
	{
		delete f;
		logMsg("invalid state file");
		return STATE_RESULT_INVALID_DATA;
	}

	delete f;

	//sound_restore();
	return STATE_RESULT_OK;
}

int EmuSystem::saveState()
{
	FsSys::cPath saveStr;
	sprintStateFilename(saveStr, saveStateSlot);
	#ifdef CONFIG_BASE_IOS_SETUID
		fixFilePermissions(saveStr);
	#endif
	logMsg("saving state %s", saveStr);
	return saveMDState(saveStr);
}

int EmuSystem::loadState()
{
	FsSys::cPath saveStr;
	sprintStateFilename(saveStr, saveStateSlot);
	logMsg("loading state %s", saveStr);
	return loadMDState(saveStr);
}

void EmuSystem::saveBackupMem() // for manually saving when not closing game
{
	if(!gameIsRunning())
		return;
	#ifndef NO_SCD
	if(sCD.isActive)
	{
		logMsg("saving BRAM");
		FsSys::cPath saveStr;
		sprintBRAMSaveFilename(saveStr);
		Io *bramFile = IoSys::create(saveStr);
		if(!bramFile)
			logMsg("error creating bram file");
		else
		{
			bramFile->fwrite(bram, sizeof(bram), 1);
			uchar sramTemp[0x10000];
			memcpy(sramTemp, sram.sram, 0x10000); // make a temp copy to byte-swap
			for(uint i = 0; i < 0x10000; i += 2)
			{
				IG::swap(sramTemp[i], sramTemp[i+1]);
			}
			bramFile->fwrite(sramTemp, 0x10000, 1);
			delete bramFile;
		}
	}
	else
	#endif
	if(sram.on)
	{
		FsSys::cPath saveStr;
		sprintSaveFilename(saveStr);
		#ifdef CONFIG_BASE_IOS_SETUID
			fixFilePermissions(saveStr);
		#endif

		logMsg("saving SRAM%s", optionBigEndianSram ? ", byte-swapped" : "");

		uchar sramTemp[0x10000];
		uchar *sramPtr = sram.sram;
		if(optionBigEndianSram)
		{
			memcpy(sramTemp, sram.sram, 0x10000); // make a temp copy to byte-swap
			for(uint i = 0; i < 0x10000; i += 2)
			{
				IG::swap(sramTemp[i], sramTemp[i+1]);
			}
			sramPtr = sramTemp;
		}
		if(IoSys::writeToNewFile(saveStr, sramPtr, 0x10000) == IO_ERROR)
			logMsg("error creating sram file");
	}
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		FsSys::cPath saveStr;
		sprintStateFilename(saveStr, -1);
		#ifdef CONFIG_BASE_IOS_SETUID
			fixFilePermissions(saveStr);
		#endif
		saveMDState(saveStr);
	}
}

void EmuSystem::closeSystem()
{
	saveBackupMem();
	if(sCD.isActive)
	{
		scd_deinit();
	}
	old_system[0] = old_system[1] = -1;
}

const char *mdInputSystemToStr(uint8 system)
{
	switch(system)
	{
		case NO_SYSTEM: return "unconnected";
		case SYSTEM_MD_GAMEPAD: return "gamepad";
		case SYSTEM_MS_GAMEPAD: return "sms gamepad";
		case SYSTEM_MOUSE: return "mouse";
		case SYSTEM_MENACER: return "menacer";
		case SYSTEM_JUSTIFIER: return "justifier";
		case SYSTEM_TEAMPLAYER: return "team-player";
		default : return "unknown";
	}
}

static bool inputPortWasAutoSetByGame(uint port)
{
	return old_system[port] != -1;
}

static void setupSMSInput()
{
	input.system[0] = input.system[1] =  SYSTEM_MS_GAMEPAD;
}

static void setupMDInput()
{
	if(!EmuSystem::gameIsRunning())
		return;

	uint mdPad = option6BtnPad ? DEVICE_PAD6B : DEVICE_PAD3B;
	iterateTimes(4, i)
		config.input[i].padtype = mdPad;

	if(system_hw == SYSTEM_PBC)
	{
		setupSMSInput();
		io_init();
		vController.gp.activeFaceBtns = 3;
		return;
	}

	if(usingMultiTap)
	{
		input.system[0] = SYSTEM_TEAMPLAYER;
		input.system[1] = 0;
	}
	else
	{
		iterateTimes(2, i)
		{
			if(mdInputPortDev[i] == -1) // user didn't specify device, go with auto settings
			{
				if(!inputPortWasAutoSetByGame(i))
					input.system[i] = SYSTEM_MD_GAMEPAD;
				else
				{
					logMsg("input port %d set by game detection", i);
					input.system[i] = old_system[i];
				}
			}
			else
				input.system[i] = mdInputPortDev[i];
			logMsg("attached %s to port %d%s", mdInputSystemToStr(input.system[i]), i, mdInputPortDev[i] == -1 ? " (auto)" : "");
		}
	}

	io_init();
	vController.gp.activeFaceBtns = option6BtnPad ? 6 : 3;
}

static void doAudioInit()
{
	uint fps = vdp_pal ? 50 : 60;
	#if defined(CONFIG_ENV_WEBOS)
	if(optionFrameSkip != EmuSystem::optionFrameSkipAuto)
	{
		if(!vdp_pal) fps = 62;
	}
	#endif
	audio_init(optionSoundRate, fps);
}

static uint detectISORegion(Io *iso)
{
	iso->seekA(0x20b);
	auto bootByte = iso->fgetc();

	if(bootByte == 0x20) // use the region code
	{
		iso->seekA(0x200);
		bootByte = iso->fgetc();
		if(bootByte == 0x4a) return REGION_JAPAN_NTSC;
		else if(bootByte == 0x55) return REGION_USA;
		return REGION_EUROPE;
	}

	if(bootByte == 0xa1) return REGION_JAPAN_NTSC;
	else if(bootByte == 0x64) return REGION_EUROPE;

	if(bootByte != 0x7a)
		bug_exit("handle region detection byte %X", bootByte);
	return REGION_USA;
}

int EmuSystem::loadGame(const char *path, bool allowAutosaveState)
{
	closeGame(allowAutosaveState);

	string_copy(gamePath, FsSys::workDir(), sizeof(gamePath));
	#ifdef CONFIG_BASE_IOS_SETUID
		fixFilePermissions(gamePath);
	#endif
	snprintf(fullGamePath, sizeof(fullGamePath), "%s/%s", gamePath, path);
	logMsg("full game path: %s", fullGamePath);
	if(string_hasDotExtension(path, "iso")
		|| (string_hasDotExtension(path, "bin") && FsSys::fileSize(fullGamePath) > 1024*1024*10)) // CD
	{
		uint region;
		if (config.region_detect == 1) region = REGION_USA;
	  else if (config.region_detect == 2) region = REGION_EUROPE;
	  else if (config.region_detect == 3) region = REGION_JAPAN_NTSC;
	  else if (config.region_detect == 4) region = REGION_JAPAN_PAL;
	  else
	  {
			Io *iso = IoSys::open(fullGamePath);
			if(!iso)
			{
				popup.post("Error loading CD", 1);
				return 0;
			}
			region = detectISORegion(iso);
			delete iso;
	  }

		const char *biosPath = optionCDBiosJpnPath;
		const char *biosName = "Japan";
		switch(region)
		{
			bcase REGION_USA: biosPath = optionCDBiosUsaPath; biosName = "USA";
			bcase REGION_EUROPE: biosPath = optionCDBiosEurPath; biosName = "Europe";
		}
		if(!strlen(biosPath))
		{
			popup.printf(4, 1, "Set a %s BIOS in the Options", biosName);
			return 0;
		}

		FsSys::cPath loadFullGamePath;
		strcpy(loadFullGamePath, biosPath);
		if(!load_rom(loadFullGamePath)) // load_rom can modify the string
		{
			popup.printf(4, 1, "Error loading BIOS: %s", biosPath);
			return 0;
		}
		if(!sCD.isActive)
		{
			popup.printf(4, 1, "Invalid BIOS: %s", biosPath);
			return 0;
		}
	}
	else // ROM
	{
		FsSys::cPath loadFullGamePath;
		strcpy(loadFullGamePath, fullGamePath);
		if(!load_rom(loadFullGamePath)) // load_rom can modify the string
		{
			popup.post("Error loading game", 1);
			return 0;
		}
	}

	string_copyUpToLastCharInstance(gameName, path, '.');
	logMsg("set game name: %s", gameName);

	doAudioInit();
	system_init();
	iterateTimes(2, i)
	{
		if(old_system[i] != -1)
			old_system[i] = input.system[i]; // store input ports set by game
	}
	setupMDInput();

	#ifndef NO_SCD
	if(sCD.isActive)
	{
		FsSys::cPath saveStr;
		sprintBRAMSaveFilename(saveStr);
		Io *bramFile = IoSys::open(saveStr);

		if(!bramFile)
		{
			logMsg("no BRAM on disk, formatting");
			mem_zero(bram);
			memcpy(bram + sizeof(bram) - sizeof(fmtBram), fmtBram, sizeof(fmtBram));
			var_copy(sramFormatStart, sram.sram + 0x10000 - sizeof(fmt64kSram));
			memcpy(sramFormatStart, fmt64kSram, sizeof(fmt64kSram));
			for(uint i = 0; i < 0x40; i += 2) // byte-swap sram cart format region
			{
				IG::swap(sramFormatStart[i], sramFormatStart[i+1]);
			}
		}
		else
		{
			bramFile->read(bram, sizeof(bram));
			bramFile->read(sram.sram, 0x10000);
			for(uint i = 0; i < 0x10000; i += 2) // byte-swap
			{
				IG::swap(sram.sram[i], sram.sram[i+1]);
			}
			logMsg("loaded BRAM from disk");
			delete bramFile;
		}
	}
	else
	#endif
	if(sram.on)
	{
		FsSys::cPath saveStr;
		sprintSaveFilename(saveStr);

		if(IoSys::readFromFile(saveStr, sram.sram, 0x10000) == 0)
			logMsg("no SRAM on disk");
		else
			logMsg("loaded SRAM from disk%s", optionBigEndianSram ? ", will byte-swap" : "");

		if(optionBigEndianSram)
		{
			for(uint i = 0; i < 0x10000; i += 2)
			{
				IG::swap(sram.sram[i], sram.sram[i+1]);
			}
		}
	}

	system_reset();

	if(sCD.isActive)
	{
		if(Insert_CD(fullGamePath, string_hasDotExtension(path, "bin")) != 0)
		{
			popup.post("Error loading CD", 1);
			return 0;
		}
	}

	if(allowAutosaveState && optionAutoSaveState)
	{
		FsSys::cPath saveStr;
		sprintStateFilename(saveStr, -1);
		loadMDState(saveStr);
	}

	emuView.initImage(0, mdResX, mdResY);

	logMsg("started emu");
	return 1;
}

void EmuSystem::clearInputBuffers()
{
	mem_zero(input.pad);
	mem_zero(input.analog);
}

void EmuSystem::configAudioRate()
{
	pcmFormat.rate = optionSoundRate;
	doAudioInit();
	if(gameIsRunning())
		sound_restore();
	logMsg("md sound buffer size %d", snd.buffer_size);
}

namespace Input
{
void onInputEvent(const InputEvent &e)
{
	if(EmuSystem::active)
	{
		int gunDevIdx = 4;
		if(unlikely(e.isPointer() && input.dev[gunDevIdx] == DEVICE_LIGHTGUN))
		{
			if(emuView.gameView.overlaps(e.x, e.y))
			{
				int xRel = e.x - emuView.gameView.xIPos(LT2DO), yRel = e.y - emuView.gameView.yIPos(LT2DO);
				input.analog[gunDevIdx][0] = IG::scalePointRange((float)xRel, (float)emuView.gameView.iXSize, (float)bitmap.viewport.w);
				input.analog[gunDevIdx][1] = IG::scalePointRange((float)yRel, (float)emuView.gameView.iYSize, (float)bitmap.viewport.h);
			}
			if(e.state == INPUT_PUSHED)
			{
				input.pad[gunDevIdx] |= INPUT_A;
				logMsg("gun pushed @ %d,%d, on MD %d,%d", e.x, e.y, input.analog[gunDevIdx][0], input.analog[gunDevIdx][1]);
			}
			else if(e.state == INPUT_RELEASED)
			{
				unsetBits(input.pad[gunDevIdx], INPUT_A);
			}
		}
	}
	handleInputEvent(e);
}
}

namespace Base
{

void onAppMessage(int type, int shortArg, int intArg, int intArg2) { }

CallResult onInit()
{
	Audio::setHintPcmFramesPerWrite(950); // for PAL
	mainInitCommon();
	emuView.initPixmap((uchar*)nativePixBuff, pixFmt, mdResX, mdResY);
	vController.gp.activeFaceBtns = option6BtnPad ? 6 : 3;
	config_ym2413_enabled = optionSmsFM;
	static uint8 cartMem[MAXROMSIZE] __attribute__ ((aligned (8)));
	cart.rom = cartMem;

	mMenu.init(Config::envIsPS3);
	viewStack.push(&mMenu);
	Gfx::onViewChange();
	mMenu.show();

	//Input::eventHandler(onInputEvent);
	Base::displayNeedsUpdate();
	return(OK);
}

}

#undef thisModuleName
