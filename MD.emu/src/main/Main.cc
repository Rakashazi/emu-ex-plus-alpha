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

#define LOGTAG "main"
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/EmuAppInlines.hh>
#include "internal.hh"
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
#include "EmuConfig.hh"
#ifndef NO_SCD
#include <scd/scd.h>
#endif
#include <fileio/fileio.h>
#include "Cheats.hh"

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2014\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nGenesis Plus Team\ncgfm2.emuviews.com";
t_config config{};
uint config_ym2413_enabled = 1;
int8 mdInputPortDev[2]{-1, -1};

bool hasMDExtension(const char *name)
{
	return hasROMExtension(name);
}

static bool hasMDCDExtension(const char *name)
{
	return string_hasDotExtension(name, "cue") || string_hasDotExtension(name, "iso");
}

static bool hasMDWithCDExtension(const char *name)
{
	return hasMDExtension(name)
	#ifndef NO_SCD
		|| hasMDCDExtension(name)
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

bool usingMultiTap = false;
Byte1Option optionBigEndianSram{CFGKEY_BIG_ENDIAN_SRAM, 0};
Byte1Option optionSmsFM{CFGKEY_SMS_FM, 1};
Byte1Option option6BtnPad{CFGKEY_6_BTN_PAD, 0};
Byte1Option optionRegion{CFGKEY_MD_REGION, 0};
#ifndef NO_SCD
FS::PathString cdBiosUSAPath{}, cdBiosJpnPath{}, cdBiosEurPath{};
PathOption optionCDBiosUsaPath{CFGKEY_MD_CD_BIOS_USA_PATH, cdBiosUSAPath, ""};
PathOption optionCDBiosJpnPath{CFGKEY_MD_CD_BIOS_JPN_PATH, cdBiosJpnPath, ""};
PathOption optionCDBiosEurPath{CFGKEY_MD_CD_BIOS_EUR_PATH, cdBiosEurPath, ""};
#endif
Byte1Option optionVideoSystem{CFGKEY_VIDEO_SYSTEM, 0};
static uint autoDetectedVidSysPAL = 0;

const char *EmuSystem::inputFaceBtnName = "A/B/C";
const char *EmuSystem::inputCenterBtnName = "Mode/Start";
const uint EmuSystem::inputFaceBtns = 6;
const uint EmuSystem::inputCenterBtns = 2;
const bool EmuSystem::inputHasTriggerBtns = false;
const bool EmuSystem::inputHasRevBtnLayout = true;
const char *EmuSystem::configFilename = "MdEmu.config";
bool EmuSystem::hasCheats = true;
const uint EmuSystem::maxPlayers = 4;
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = IG::size(EmuSystem::aspectRatioInfo);
bool EmuSystem::hasPALVideoSystem = true;

const char *EmuSystem::shortSystemName()
{
	return "MD-Genesis";
}

const char *EmuSystem::systemName()
{
	return "Mega Drive (Sega Genesis)";
}

void EmuSystem::initOptions()
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	optionTouchCtrlSize.initDefault(750);
	optionTouchCtrlBtnSpace.initDefault(100);
	#endif
}

void EmuSystem::onOptionsLoaded()
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	vController.gp.activeFaceBtns = option6BtnPad ? 6 : 3;
	#endif
	config_ym2413_enabled = optionSmsFM;
}

bool EmuSystem::readConfig(IO &io, uint key, uint readSize)
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

void EmuSystem::writeConfig(IO &io)
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

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter = hasMDWithCDExtension;
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = hasMDExtension;

static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;

static const uint mdMaxResX = 320, mdMaxResY = 240;
static int mdResX = 256, mdResY = 224;
static uint16 nativePixBuff[mdMaxResX*mdMaxResY] __attribute__ ((aligned (8))) {0};
t_bitmap bitmap = { (uint8*)nativePixBuff, mdResY, mdResX * (int)pixFmt.bytesPerPixel() };

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
	padData = IG::setOrClearBits(padData, (uint16)emuKey, state == Input::PUSHED);
}

void commitVideoFrame()
{
	if(unlikely(bitmap.viewport.w != mdResX || bitmap.viewport.h != mdResY))
	{
		mdResX = bitmap.viewport.w;
		mdResY = bitmap.viewport.h;
		bitmap.pitch = mdResX * pixFmt.bytesPerPixel();
		logMsg("mode change: %dx%d", mdResX, mdResY);
		emuVideo.resizeImage(mdResX, mdResY);
	}
	updateAndDrawEmuVideo();
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	//logMsg("frame start");
	RAMCheatUpdate();
	system_frame(!processGfx, renderGfx);

	int16 audioBuff[snd.buffer_size * 2];
	int frames = audio_update(audioBuff);
	if(renderAudio)
	{
		//logMsg("%d frames", frames);
		writeSound(audioBuff, frames);
	}
	//logMsg("frame end");
}

bool EmuSystem::vidSysIsPAL() { return vdp_pal; }
uint EmuSystem::multiresVideoBaseX() { return 0; }
uint EmuSystem::multiresVideoBaseY() { return 0; }
bool touchControlsApplicable() { return 1; }

void EmuSystem::reset(ResetMode mode)
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

FS::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return FS::makePathStringPrintf("%s/%s.0%c.gp", statePath, gameName, saveSlotChar(slot));
}

static FS::PathString sprintSaveFilename()
{
	return FS::makePathStringPrintf("%s/%s.srm", EmuSystem::savePath(), EmuSystem::gameName().data());
}

static FS::PathString sprintBRAMSaveFilename()
{
	return FS::makePathStringPrintf("%s/%s.brm", EmuSystem::savePath(), EmuSystem::gameName().data());
}

static const uint maxSaveStateSize = STATE_SIZE+4;

static int saveMDState(const char *path)
{
	uchar *stateData = (uchar*)malloc(maxSaveStateSize);
	if(!stateData)
		return STATE_RESULT_IO_ERROR;
	logMsg("saving state data");
	int size = state_save(stateData);
	logMsg("writing to file");
	CallResult ret;
	if((ret = writeToNewFile(path, stateData, size)) != OK)
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
	FileIO f;
	auto ret = f.open(path);
	if(ret != OK)
	{
		switch(ret)
		{
			case PERMISSION_DENIED: return STATE_RESULT_NO_FILE_ACCESS;
			case NOT_FOUND: return STATE_RESULT_NO_FILE;
			default: return STATE_RESULT_IO_ERROR;
		}
	}

	auto stateData = (const uchar *)f.mmapConst();
	if(!stateData)
	{
		return STATE_RESULT_IO_ERROR;
	}
	if(state_load(stateData) <= 0)
	{
		return STATE_RESULT_INVALID_DATA;
	}

	//sound_restore();
	return STATE_RESULT_OK;
}

int EmuSystem::saveState()
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	fixFilePermissions(saveStr);
	logMsg("saving state %s", saveStr.data());
	return saveMDState(saveStr.data());
}

int EmuSystem::loadState(int saveStateSlot)
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	logMsg("loading state %s", saveStr.data());
	return loadMDState(saveStr.data());
}

void EmuSystem::saveBackupMem() // for manually saving when not closing game
{
	if(!gameIsRunning())
		return;
	#ifndef NO_SCD
	if(sCD.isActive)
	{
		logMsg("saving BRAM");
		auto saveStr = sprintBRAMSaveFilename();
		FileIO bramFile;
		bramFile.create(saveStr.data());
		if(!bramFile)
			logMsg("error creating bram file");
		else
		{
			bramFile.write(bram, sizeof(bram));
			char sramTemp[0x10000];
			memcpy(sramTemp, sram.sram, 0x10000); // make a temp copy to byte-swap
			for(uint i = 0; i < 0x10000; i += 2)
			{
				std::swap(sramTemp[i], sramTemp[i+1]);
			}
			bramFile.write(sramTemp, 0x10000);
		}
	}
	else
	#endif
	if(sram.on)
	{
		auto saveStr = sprintSaveFilename();
		fixFilePermissions(saveStr);

		logMsg("saving SRAM%s", optionBigEndianSram ? ", byte-swapped" : "");

		uchar sramTemp[0x10000];
		uchar *sramPtr = sram.sram;
		if(optionBigEndianSram)
		{
			memcpy(sramTemp, sram.sram, 0x10000); // make a temp copy to byte-swap
			for(uint i = 0; i < 0x10000; i += 2)
			{
				std::swap(sramTemp[i], sramTemp[i+1]);
			}
			sramPtr = sramTemp;
		}
		if(writeToNewFile(saveStr.data(), sramPtr, 0x10000) == IO_ERROR)
			logMsg("error creating sram file");
	}
	writeCheatFile();
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		auto saveStr = sprintStateFilename(-1);
		fixFilePermissions(saveStr);
		saveMDState(saveStr.data());
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

void setupMDInput()
{
	if(!EmuSystem::gameIsRunning())
	{
		#ifdef CONFIG_VCONTROLS_GAMEPAD
		vController.gp.activeFaceBtns = option6BtnPad ? 6 : 3;
		#endif
		return;
	}

	IG::fillData(playerIdxMap);
	playerIdxMap[0] = 0;
	playerIdxMap[1] = 4;

	uint mdPad = option6BtnPad ? DEVICE_PAD6B : DEVICE_PAD3B;
	iterateTimes(4, i)
		config.input[i].padtype = mdPad;

	if(system_hw == SYSTEM_PBC)
	{
		setupSMSInput();
		io_init();
		#ifdef CONFIG_VCONTROLS_GAMEPAD
		vController.gp.activeFaceBtns = 3;
		#endif
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
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	vController.gp.activeFaceBtns = option6BtnPad ? 6 : 3;
	#endif
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

FS::PathString EmuSystem::willLoadGameFromPath(FS::PathString path)
{
	#ifndef NO_SCD
	// check if loading a .bin with matching .cue
	if(string_hasDotExtension(path.data(), "bin"))
	{
		auto len = strlen(path.data());
		auto possibleCuePath = path;
		possibleCuePath[len-3] = 0; // delete extension
		string_cat(possibleCuePath, "cue");
		if(FS::exists(possibleCuePath))
		{
			logMsg("loading %s instead of .bin file", possibleCuePath.data());
			return possibleCuePath;
		}
	}
	#endif
	return path;
}

int EmuSystem::loadGame(const char *path)
{
	bug_exit("should only use loadGameFromIO()");
	return 0;
}

int EmuSystem::loadGameFromIO(IO &io, const char *path, const char *origFilename)
{
	closeGame();
	emuVideo.initImage(0, mdResX, mdResY);
	setupGamePaths(path);
	#ifndef NO_SCD
	CDAccess *cd{};
	if(hasMDCDExtension(fullGamePath()) ||
		(string_hasDotExtension(path, "bin") && FS::file_size(fullGamePath()) > 1024*1024*10)) // CD
	{
		try
		{
			cd = cdaccess_open_image(fullGamePath(), false);
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
		FileIO io;
		if(!load_rom(io, biosPath, nullptr))
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
	if(hasMDExtension(origFilename)) // ROM
	{
		logMsg("loading ROM %s", path);
		if(!load_rom(io, path, origFilename))
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

	configAudioPlayback();
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
		auto saveStr = sprintBRAMSaveFilename();
		FileIO bramFile;
		bramFile.open(saveStr.data());

		if(!bramFile)
		{
			logMsg("no BRAM on disk, formatting");
			IG::fillData(bram);
			memcpy(bram + sizeof(bram) - sizeof(fmtBram), fmtBram, sizeof(fmtBram));
			auto sramFormatStart = sram.sram + 0x10000 - sizeof(fmt64kSram);
			memcpy(sramFormatStart, fmt64kSram, sizeof(fmt64kSram));
			for(uint i = 0; i < 0x40; i += 2) // byte-swap sram cart format region
			{
				std::swap(sramFormatStart[i], sramFormatStart[i+1]);
			}
		}
		else
		{
			bramFile.read(bram, sizeof(bram));
			bramFile.read(sram.sram, 0x10000);
			for(uint i = 0; i < 0x10000; i += 2) // byte-swap
			{
				std::swap(sram.sram[i], sram.sram[i+1]);
			}
			logMsg("loaded BRAM from disk");
		}
	}
	else
	#endif
	if(sram.on)
	{
		auto saveStr = sprintSaveFilename();

		if(readFromFile(saveStr.data(), sram.sram, 0x10000) <= 0)
			logMsg("no SRAM on disk");
		else
			logMsg("loaded SRAM from disk%s", optionBigEndianSram ? ", will byte-swap" : "");

		if(optionBigEndianSram)
		{
			for(uint i = 0; i < 0x10000; i += 2)
			{
				std::swap(sram.sram[i], sram.sram[i+1]);
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
	IG::fillData(input.pad);
	for(auto &analog : input.analog)
	{
		IG::fillData(analog);
	}
}

void EmuSystem::configAudioRate(double frameTime)
{
	pcmFormat.rate = optionSoundRate;
	audio_init(optionSoundRate, 1. / frameTime);
	if(gameIsRunning())
		sound_restore();
	logMsg("md sound buffer size %d", snd.buffer_size);
}

void EmuSystem::savePathChanged() { }

bool EmuSystem::hasInputOptions() { return true; }

void EmuSystem::onCustomizeNavView(EmuNavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build(0., 0., 1. * .4, 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build(0., 0., 1. * .4, 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build(0., 0., .6 * .4, 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};
	view.setBackgroundGradient(navViewGrad);
}

void EmuSystem::onMainWindowCreated(Base::Window &win)
{
	win.setOnInputEvent(
		[](Base::Window &win, Input::Event e)
		{
			if(EmuSystem::isActive())
			{
				int gunDevIdx = 4;
				if(unlikely(e.isPointer() && input.dev[gunDevIdx] == DEVICE_LIGHTGUN))
				{
					if(emuVideoLayer.gameRect().overlaps({e.x, e.y}))
					{
						int xRel = e.x - emuVideoLayer.gameRect().x, yRel = e.y - emuVideoLayer.gameRect().y;
						input.analog[gunDevIdx][0] = IG::scalePointRange((float)xRel, (float)emuVideoLayer.gameRect().xSize(), (float)bitmap.viewport.w);
						input.analog[gunDevIdx][1] = IG::scalePointRange((float)yRel, (float)emuVideoLayer.gameRect().ySize(), (float)bitmap.viewport.h);
					}
					if(e.state == Input::PUSHED)
					{
						input.pad[gunDevIdx] |= INPUT_A;
						logMsg("gun pushed @ %d,%d, on MD %d,%d", e.x, e.y, input.analog[gunDevIdx][0], input.analog[gunDevIdx][1]);
					}
					else if(e.state == Input::RELEASED)
					{
						input.pad[gunDevIdx] = IG::clearBits(input.pad[gunDevIdx], (uint16)INPUT_A);
					}
				}
			}
			handleInputEvent(win, e);
		});
}

CallResult EmuSystem::onInit()
{
	emuVideo.initPixmap((char*)nativePixBuff, pixFmt, mdResX, mdResY);
	return OK;
}
