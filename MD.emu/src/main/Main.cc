/*  This file is part of MD.emu.

	MD.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MD.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MD.emu.  If not, see <http://www.gnu.org/licenses/> */

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
#ifndef NO_SCD
#include <scd/scd.h>
#endif
#include <main/Cheats.hh>

const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2013\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nGenesis Plus Team\ncgfm2.emuviews.com";
t_config config = { 0 };
uint config_ym2413_enabled = 1;
static int8 mdInputPortDev[2] = { -1, -1 };

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
	return string_hasDotExtension(name, "cue") || string_hasDotExtension(name, "iso");
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
	CFGKEY_BIG_ENDIAN_SRAM = 278, CFGKEY_SMS_FM = 279,
	CFGKEY_6_BTN_PAD = 280, CFGKEY_MD_CD_BIOS_USA_PATH = 281,
	CFGKEY_MD_CD_BIOS_JPN_PATH = 282, CFGKEY_MD_CD_BIOS_EUR_PATH = 283,
	CFGKEY_MD_REGION = 284, CFGKEY_VIDEO_SYSTEM = 285,
};

static bool usingMultiTap = 0;
static Byte1Option optionBigEndianSram(CFGKEY_BIG_ENDIAN_SRAM, 0);
static Byte1Option optionSmsFM(CFGKEY_SMS_FM, 1);
static Byte1Option option6BtnPad(CFGKEY_6_BTN_PAD, 0);
static Byte1Option optionRegion(CFGKEY_MD_REGION, 0);
#ifndef NO_SCD
FsSys::cPath cdBiosUSAPath = "", cdBiosJpnPath = "", cdBiosEurPath = "";
static PathOption optionCDBiosUsaPath(CFGKEY_MD_CD_BIOS_USA_PATH, cdBiosUSAPath, sizeof(cdBiosUSAPath), "");
static PathOption optionCDBiosJpnPath(CFGKEY_MD_CD_BIOS_JPN_PATH, cdBiosJpnPath, sizeof(cdBiosJpnPath), "");
static PathOption optionCDBiosEurPath(CFGKEY_MD_CD_BIOS_EUR_PATH, cdBiosEurPath, sizeof(cdBiosEurPath), "");
#endif
static Byte1Option optionVideoSystem(CFGKEY_VIDEO_SYSTEM, 0);
static uint autoDetectedVidSysPAL = 0;

const uint EmuSystem::maxPlayers = 4;
uint EmuSystem::aspectRatioX = 4, EmuSystem::aspectRatioY = 3;
#include "CommonGui.hh"

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
		bcase CFGKEY_BIG_ENDIAN_SRAM: optionBigEndianSram.readFromIO(io, readSize);
		bcase CFGKEY_SMS_FM: optionSmsFM.readFromIO(io, readSize);
		bcase CFGKEY_6_BTN_PAD: option6BtnPad.readFromIO(io, readSize);
		#ifndef NO_SCD
		bcase CFGKEY_MD_CD_BIOS_USA_PATH: optionCDBiosUsaPath.readFromIO(io, readSize);
		bcase CFGKEY_MD_CD_BIOS_JPN_PATH: optionCDBiosJpnPath.readFromIO(io, readSize);
		bcase CFGKEY_MD_CD_BIOS_EUR_PATH: optionCDBiosEurPath.readFromIO(io, readSize);
		#endif
		bcase CFGKEY_MD_REGION:
		{
			optionRegion.readFromIO(io, readSize);
			if(optionRegion < 4)
			{
				config.region_detect = optionRegion;
			}
			else
				optionRegion = 0;
		}
		bcase CFGKEY_VIDEO_SYSTEM: optionVideoSystem.readFromIO(io, readSize);
		bdefault: return 0;
	}
	return 1;
}

void EmuSystem::writeConfig(Io *io)
{
	optionBigEndianSram.writeWithKeyIfNotDefault(io);
	optionSmsFM.writeWithKeyIfNotDefault(io);
	option6BtnPad.writeWithKeyIfNotDefault(io);
	optionVideoSystem.writeWithKeyIfNotDefault(io);
	#ifndef NO_SCD
	optionCDBiosUsaPath.writeToIO(io);
	optionCDBiosJpnPath.writeToIO(io);
	optionCDBiosEurPath.writeToIO(io);
	#endif
	optionRegion.writeWithKeyIfNotDefault(io);
}

FsDirFilterFunc EmuFilePicker::defaultFsFilter = mdFsFilter;
FsDirFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = mdROMFsFilter;

static const PixelFormatDesc *pixFmt = &PixelFormatRGB565;

static const uint mdMaxResX = 320, mdMaxResY = 240;
static int mdResX = 256, mdResY = 224;
static uint16 nativePixBuff[mdMaxResX*mdMaxResY] __attribute__ ((aligned (8))) {0};
t_bitmap bitmap = { (uint8*)nativePixBuff, mdResY, mdResX * pixFmt->bytesPerPixel };

void updateVControllerMapping(uint player, SysVController::Map &map)
{
	uint playerMask = player << 30;
	map[SysVController::F_ELEM] = INPUT_A | playerMask;
	map[SysVController::F_ELEM+1] = INPUT_B | playerMask;
	map[SysVController::F_ELEM+2] = INPUT_C | playerMask;
	map[SysVController::F_ELEM+3] = INPUT_X | playerMask;
	map[SysVController::F_ELEM+4] = INPUT_Y | playerMask;
	map[SysVController::F_ELEM+5] = INPUT_Z | playerMask;

	map[SysVController::C_ELEM] = INPUT_MODE | playerMask;
	map[SysVController::C_ELEM+1] = INPUT_START | playerMask;

	map[SysVController::D_ELEM] = INPUT_UP | INPUT_LEFT | playerMask;
	map[SysVController::D_ELEM+1] = INPUT_UP | playerMask;
	map[SysVController::D_ELEM+2] = INPUT_UP | INPUT_RIGHT | playerMask;
	map[SysVController::D_ELEM+3] = INPUT_LEFT | playerMask;
	map[SysVController::D_ELEM+5] = INPUT_RIGHT | playerMask;
	map[SysVController::D_ELEM+6] = INPUT_DOWN | INPUT_LEFT | playerMask;
	map[SysVController::D_ELEM+7] = INPUT_DOWN | playerMask;
	map[SysVController::D_ELEM+8] = INPUT_DOWN | INPUT_RIGHT | playerMask;
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	assert(input >= mdKeyIdxUp);
	uint player = (input - mdKeyIdxUp) / EmuControls::gamepadKeys;
	uint playerMask = player << 30;
	input -= EmuControls::gamepadKeys * player;
	switch(input)
	{
		case mdKeyIdxUp: return INPUT_UP | playerMask;
		case mdKeyIdxRight: return INPUT_RIGHT | playerMask;
		case mdKeyIdxDown: return INPUT_DOWN | playerMask;
		case mdKeyIdxLeft: return INPUT_LEFT | playerMask;
		case mdKeyIdxLeftUp: return INPUT_LEFT | INPUT_UP | playerMask;
		case mdKeyIdxRightUp: return INPUT_RIGHT | INPUT_UP | playerMask;
		case mdKeyIdxRightDown: return INPUT_RIGHT | INPUT_DOWN | playerMask;
		case mdKeyIdxLeftDown: return INPUT_LEFT | INPUT_DOWN | playerMask;
		case mdKeyIdxMode: return INPUT_MODE | playerMask;
		case mdKeyIdxStart: return INPUT_START | playerMask;
		case mdKeyIdxATurbo: turbo = 1;
		case mdKeyIdxA: return INPUT_A | playerMask;
		case mdKeyIdxBTurbo: turbo = 1;
		case mdKeyIdxB: return INPUT_B | playerMask;
		case mdKeyIdxCTurbo: turbo = 1;
		case mdKeyIdxC: return INPUT_C | playerMask;
		case mdKeyIdxXTurbo: turbo = 1;
		case mdKeyIdxX: return INPUT_X | playerMask;
		case mdKeyIdxYTurbo: turbo = 1;
		case mdKeyIdxY: return INPUT_Y | playerMask;
		case mdKeyIdxZTurbo: turbo = 1;
		case mdKeyIdxZ: return INPUT_Z | playerMask;
		default: bug_branch("%d", input);
	}
	return 0;
}

static uint playerIdxMap[4] = { 0 };

void EmuSystem::handleInputAction(uint state, uint emuKey)
{
	uint player = emuKey >> 30; // player is encoded in upper 2 bits of input code
	assert(player <= 4);
	uint16 &padData = input.pad[playerIdxMap[player]];
	if(state == Input::PUSHED)
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
	}
	emuView.updateAndDrawContent();
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	//logMsg("frame start");
	RAMCheatUpdate();
	system_frame(!processGfx, renderGfx);

	int16 audioMemBuff[snd.buffer_size * 2];
	int16 *audioBuff = nullptr;
	#ifdef USE_NEW_AUDIO
	Audio::BufferContext *aBuff = nullptr;
	if(renderAudio)
	{
		if(!(aBuff = Audio::getPlayBuffer(snd.buffer_size)))
		{
			return;
		}
		audioBuff = (int16*)aBuff->data;
		assert(aBuff->frames >= (uint)snd.buffer_size);
	}
	else
	{
		audioBuff = audioMemBuff;
	}
	#else
	audioBuff = audioMemBuff;
	#endif

	int frames = audio_update(audioBuff);
	if(renderAudio)
	{
		//logMsg("%d frames", frames);
		#ifdef USE_NEW_AUDIO
		if(renderAudio)
			Audio::commitPlayBuffer(aBuff, frames);
		#else
		if(renderAudio)
			Audio::writePcm((uchar*)audioBuff, frames);
		#endif
	}
	//logMsg("frame end");
}

bool EmuSystem::vidSysIsPAL() { return vdp_pal; }
bool touchControlsApplicable() { return 1; }

void EmuSystem::resetGame()
{
	assert(gameIsRunning());
	#ifndef NO_SCD
	if(sCD.isActive)
		system_reset();
	else
	#endif
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

void EmuSystem::sprintStateFilename(char *str, size_t size, int slot, const char *statePath, const char *gameName)
{
	snprintf(str, size, "%s/%s.0%c.gp", statePath, gameName, saveSlotChar(slot));
}

template <size_t S>
static void sprintSaveFilename(char (&str)[S])
{
	snprintf(str, S, "%s/%s.srm", EmuSystem::savePath(), EmuSystem::gameName);
}

template <size_t S>
static void sprintBRAMSaveFilename(char (&str)[S])
{
	snprintf(str, S, "%s/%s.brm", EmuSystem::savePath(), EmuSystem::gameName);
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

int EmuSystem::loadState(int saveStateSlot)
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
	writeCheatFile();
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
	#ifndef NO_SCD
	if(sCD.isActive)
	{
		scd_deinit();
	}
	#endif
	old_system[0] = old_system[1] = -1;
	clearCheatList();
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

	mem_zero(playerIdxMap);
	playerIdxMap[0] = 0;
	playerIdxMap[1] = 4;

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

	if(cart.special & HW_J_CART)
	{
		input.system[0] = input.system[1] = SYSTEM_MD_GAMEPAD;
		playerIdxMap[2] = 5;
		playerIdxMap[3] = 6;
	}
	else if(usingMultiTap)
	{
		input.system[0] = SYSTEM_TEAMPLAYER;
		input.system[1] = 0;

		playerIdxMap[1] = 1;
		playerIdxMap[2] = 2;
		playerIdxMap[3] = 3;
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
	Audio::setHintPcmFramesPerWrite(vdp_pal ? 950 : 800);
	uint fps = vdp_pal ? 50 : 60;
	#if defined(CONFIG_ENV_WEBOS)
	if(optionFrameSkip != EmuSystem::optionFrameSkipAuto)
	{
		if(!vdp_pal) fps = 62;
	}
	#endif
	audio_init(optionSoundRate, fps);
}

static uint detectISORegion(uint8 bootSector[0x800])
{
	auto bootByte = bootSector[0x20b];

	if(bootByte == 0x7a)
		return REGION_USA;
	else if(bootByte == 0x64)
		return REGION_EUROPE;
	else
		return REGION_JAPAN_NTSC;
}

int EmuSystem::loadGame(const char *path)
{
	closeGame();
	emuView.initImage(0, mdResX, mdResY);
	#ifndef NO_SCD
	// check if loading a .bin with matching .cue
	if(string_hasDotExtension(path, "bin"))
	{
		FsSys::cPath possibleCuePath {0};
		auto len = strlen(path);
		strcpy(possibleCuePath, path);
		possibleCuePath[len-3] = 0; // delete extension
		strcat(possibleCuePath, "cue");
		if(FsSys::fileExists(possibleCuePath))
		{
			logMsg("loading %s instead of .bin file", possibleCuePath);
			setupGamePaths(possibleCuePath);
		}
		else
			setupGamePaths(path);
	}
	else
	#endif
		setupGamePaths(path);
	#ifndef NO_SCD
	CDAccess *cd = nullptr;
	if(isMDCDExtension(fullGamePath) ||
		(string_hasDotExtension(path, "bin") && FsSys::fileSize(fullGamePath) > 1024*1024*10)) // CD
	{
		try
		{
			cd = cdaccess_open(fullGamePath);
		}
		catch(std::exception &e)
		{
			popup.printf(4, 1, "%s", e.what());
			return 0;
		}

		uint region = REGION_USA;
		if (config.region_detect == 1) region = REGION_USA;
	  else if (config.region_detect == 2) region = REGION_EUROPE;
	  else if (config.region_detect == 3) region = REGION_JAPAN_NTSC;
	  else if (config.region_detect == 4) region = REGION_JAPAN_PAL;
	  else
	  {
	  	uint8 bootSector[2048];
	  	cd->Read_Sector(bootSector, 0, 2048);
			region = detectISORegion(bootSector);
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
			delete cd;
			return 0;
		}

		FsSys::cPath loadFullGamePath;
		strcpy(loadFullGamePath, biosPath);
		if(!load_rom(loadFullGamePath)) // load_rom can modify the string
		{
			popup.printf(4, 1, "Error loading BIOS: %s", biosPath);
			delete cd;
			return 0;
		}
		if(!sCD.isActive)
		{
			popup.printf(4, 1, "Invalid BIOS: %s", biosPath);
			delete cd;
			return 0;
		}
	}
	else
	#endif
	if(isMDExtension(fullGamePath)) // ROM
	{
		logMsg("loading ROM %s", fullGamePath);
		FsSys::cPath loadFullGamePath;
		strcpy(loadFullGamePath, fullGamePath);
		if(!load_rom(loadFullGamePath)) // load_rom can modify the string
		{
			popup.post("Error loading game", 1);
			return 0;
		}
	}
	else
	{
		popup.post("Invalid game", 1);
		return 0;
	}
	autoDetectedVidSysPAL = vdp_pal;
	if((int)optionVideoSystem == 1)
	{
		vdp_pal = 0;
	}
	else if((int)optionVideoSystem == 2)
	{
		vdp_pal = 1;
	}
	if(vidSysIsPAL())
		logMsg("using PAL timing");

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
			auto sramFormatStart = sram.sram + 0x10000 - sizeof(fmt64kSram);
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

	#ifndef NO_SCD
	if(sCD.isActive)
	{
		if(Insert_CD(cd) != 0)
		{
			popup.post("Error loading CD", 1);
			delete cd;
			closeGame();
			return 0;
		}
	}
	#endif

	readCheatFile();
	applyCheats();

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

void EmuSystem::savePathChanged() { }

namespace Input
{
void onInputEvent(const Input::Event &e)
{
	if(EmuSystem::isActive())
	{
		int gunDevIdx = 4;
		if(unlikely(e.isPointer() && input.dev[gunDevIdx] == DEVICE_LIGHTGUN))
		{
			if(emuView.gameRect.overlaps(e.x, e.y))
			{
				int xRel = e.x - emuView.gameRect.x, yRel = e.y - emuView.gameRect.y;
				input.analog[gunDevIdx][0] = IG::scalePointRange((float)xRel, (float)emuView.gameRect.xSize(), (float)bitmap.viewport.w);
				input.analog[gunDevIdx][1] = IG::scalePointRange((float)yRel, (float)emuView.gameRect.ySize(), (float)bitmap.viewport.h);
			}
			if(e.state == Input::PUSHED)
			{
				input.pad[gunDevIdx] |= INPUT_A;
				logMsg("gun pushed @ %d,%d, on MD %d,%d", e.x, e.y, input.analog[gunDevIdx][0], input.analog[gunDevIdx][1]);
			}
			else if(e.state == Input::RELEASED)
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

CallResult onInit(int argc, char** argv)
{
	mainInitCommon();
	emuView.initPixmap((uchar*)nativePixBuff, pixFmt, mdResX, mdResY);
	vController.gp.activeFaceBtns = option6BtnPad ? 6 : 3;
	config_ym2413_enabled = optionSmsFM;
	return OK;
}

CallResult onWindowInit()
{
	static const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, VertexColorPixelFormat.build(0., 0., 1. * .4, 1.) },
		{ .3, VertexColorPixelFormat.build(0., 0., 1. * .4, 1.) },
		{ .97, VertexColorPixelFormat.build(0., 0., .6 * .4, 1.) },
		{ 1., VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};

	mainInitWindowCommon(navViewGrad);
	return OK;
}

}
