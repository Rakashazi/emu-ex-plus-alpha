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
#include <logger/interface.h>
#include <gfx/GfxSprite.hh>
#include <audio/Audio.hh>
#include <fs/sys.hh>
#include <io/sys.hh>
#include <gui/View.hh>
#include <util/strings.h>
#include <util/time/sys.hh>
#include <EmuSystem.hh>
#include <CommonFrameworkIncludes.hh>

const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2013\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nFCEUX Team\nfceux.com";
uint fceuCheats = 0;
#ifdef __clang__
PathOption optionFirmwarePath(0, nullptr, 0, nullptr); // unused, make linker happy
#endif

#include <fceu/driver.h>
#include <fceu/state.h>
#include <fceu/fceu.h>
#include <fceu/ppu.h>
#include <fceu/fds.h>
#include <fceu/input.h>
#include <fceu/cheat.h>

static bool isFDSBIOSExtension(const char *name)
{
	return string_hasDotExtension(name, "rom") || string_hasDotExtension(name, "bin");
}

static bool isFDSExtension(const char *name)
{
	return string_hasDotExtension(name, "fds");
}

static bool isROMExtension(const char *name)
{
	return string_hasDotExtension(name, "nes") || string_hasDotExtension(name, "unf");
}

static bool isNESExtension(const char *name)
{
	return isROMExtension(name) || isFDSExtension(name) || string_hasDotExtension(name, "zip");
}

static int biosFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isFDSBIOSExtension(name);
}

static int nesFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isNESExtension(name);
}

// controls

enum
{
	nesKeyIdxUp = EmuControls::systemKeyMapStart,
	nesKeyIdxRight,
	nesKeyIdxDown,
	nesKeyIdxLeft,
	nesKeyIdxLeftUp,
	nesKeyIdxRightUp,
	nesKeyIdxRightDown,
	nesKeyIdxLeftDown,
	nesKeyIdxSelect,
	nesKeyIdxStart,
	nesKeyIdxA,
	nesKeyIdxB,
	nesKeyIdxATurbo,
	nesKeyIdxBTurbo,
	nesKeyIdxAB,
};

static ESI nesInputPortDev[2] = { SI_UNSET, SI_UNSET };

enum {
	CFGKEY_FDS_BIOS_PATH = 270, CFGKEY_FOUR_SCORE = 271,
	CFGKEY_VIDEO_SYSTEM = 272,
};

FsSys::cPath fdsBiosPath = "";
static PathOption optionFdsBiosPath(CFGKEY_FDS_BIOS_PATH, fdsBiosPath, sizeof(fdsBiosPath), "");
static Byte1Option optionFourScore(CFGKEY_FOUR_SCORE, 0);
static Byte1Option optionVideoSystem(CFGKEY_VIDEO_SYSTEM, 0);
static uint autoDetectedVidSysPAL = 0;

const uint EmuSystem::maxPlayers = 4;
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		{"8:7", 8, 7},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = sizeofArray(EmuSystem::aspectRatioInfo);
#include "CommonGui.hh"

#if defined __ANDROID__ || defined CONFIG_MACHINE_PANDORA
#define GAME_ASSET_EXT "nes"
#else
#define GAME_ASSET_EXT "zip"
#endif

const BundledGameInfo &EmuSystem::bundledGameInfo(uint idx)
{
	static const BundledGameInfo info[]
	{
		{ "Test Game", "game." GAME_ASSET_EXT }
	};

	return info[0];
}

const char *EmuSystem::shortSystemName()
{
	return "FC-NES";
}

const char *EmuSystem::systemName()
{
	return "Famicom (Nintendo Entertainment System)";
}

using namespace IG;

static void setDirOverrides()
{
	FCEUI_SetDirOverride(FCEUIOD_NV, EmuSystem::savePath());
	FCEUI_SetDirOverride(FCEUIOD_CHEATS, EmuSystem::savePath());
	FCEUI_SetDirOverride(FCEUIOD_PALETTE, EmuSystem::savePath());
}

void EmuSystem::initOptions() {}

void EmuSystem::onOptionsLoaded()
{
	setDirOverrides();
}

bool EmuSystem::readConfig(Io &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_FOUR_SCORE: optionFourScore.readFromIO(io, readSize);
		bcase CFGKEY_FDS_BIOS_PATH: optionFdsBiosPath.readFromIO(io, readSize);
		bcase CFGKEY_VIDEO_SYSTEM: optionVideoSystem.readFromIO(io, readSize);
		logMsg("fds bios path %s", fdsBiosPath);
	}
	return 1;
}

void EmuSystem::writeConfig(Io *io)
{
	optionFourScore.writeWithKeyIfNotDefault(io);
	optionVideoSystem.writeWithKeyIfNotDefault(io);
	optionFdsBiosPath.writeToIO(io);
}

FsDirFilterFunc EmuFilePicker::defaultFsFilter = nesFsFilter;
FsDirFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = nesFsFilter;

#ifdef USE_PIX_RGB565
static const PixelFormatDesc *pixFmt = &PixelFormatRGB565;
#else
static const PixelFormatDesc *pixFmt = &PixelFormatRGBA8888;
#endif

const char *fceuReturnedError = 0;

#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
void updateVControllerMapping(uint player, SysVController::Map &map)
{
	uint playerMask = player << 8;
	map[SysVController::F_ELEM] = bit(0) | playerMask;
	map[SysVController::F_ELEM+1] = bit(1) | playerMask;

	map[SysVController::C_ELEM] = bit(2) | playerMask;
	map[SysVController::C_ELEM+1] = bit(3) | playerMask;

	map[SysVController::D_ELEM] = bit(4) | bit(6) | playerMask;
	map[SysVController::D_ELEM+1] = bit(4) | playerMask;
	map[SysVController::D_ELEM+2] = bit(4) | bit(7) | playerMask;
	map[SysVController::D_ELEM+3] = bit(6) | playerMask;
	map[SysVController::D_ELEM+5] = bit(7) | playerMask;
	map[SysVController::D_ELEM+6] = bit(5) | bit(6) | playerMask;
	map[SysVController::D_ELEM+7] = bit(5) | playerMask;
	map[SysVController::D_ELEM+8] = bit(5) | bit(7) | playerMask;
}
#endif

static uint32 padData = 0, zapperData[3];

static uint playerInputShift(uint player)
{
	switch(player)
	{
		case 1: return 8;
		case 2: return 16;
		case 3: return 24;
	}
	return 0;
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	assert(input >= nesKeyIdxUp);
	uint player = (input - nesKeyIdxUp) / EmuControls::gamepadKeys;
	uint playerMask = player << 8;
	input -= EmuControls::gamepadKeys * player;
	switch(input)
	{
		case nesKeyIdxUp: return bit(4) | playerMask;
		case nesKeyIdxRight: return bit(7) | playerMask;
		case nesKeyIdxDown: return bit(5) | playerMask;
		case nesKeyIdxLeft: return bit(6) | playerMask;
		case nesKeyIdxLeftUp: return bit(6) | bit(4) | playerMask;
		case nesKeyIdxRightUp: return bit(7) | bit(4) | playerMask;
		case nesKeyIdxRightDown: return bit(7) | bit(5) | playerMask;
		case nesKeyIdxLeftDown: return bit(6) | bit(5) | playerMask;
		case nesKeyIdxSelect: return bit(2) | playerMask;
		case nesKeyIdxStart: return bit(3) | playerMask;
		case nesKeyIdxATurbo: turbo = 1;
		case nesKeyIdxA: return bit(0) | playerMask;
		case nesKeyIdxBTurbo: turbo = 1;
		case nesKeyIdxB: return bit(1) | playerMask;
		case nesKeyIdxAB: return bit(0) | bit(1) | playerMask;
		default: bug_branch("%d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(uint state, uint emuKey)
{
	uint player = emuKey >> 8;
	auto key = emuKey & 0xFF;
	if(unlikely(GameInfo->type==GIT_VSUNI)) // TODO: make coin insert separate key
	{
		if(state == Input::PUSHED && key == bit(3))
			FCEUI_VSUniCoin();
	}
	if(state == Input::PUSHED)
		setBits(padData, key << playerInputShift(player));
	else
		unsetBits(padData, key << playerInputShift(player));
}

void EmuSystem::resetGame()
{
	assert(gameIsRunning());
	FCEUI_ResetNES();
}

static char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 's';
		case 0 ... 9: return 48 + slot;
		default: bug_branch("%d", slot); return 0;
	}
}

void EmuSystem::sprintStateFilename(char *str, size_t size, int slot, const char *statePath, const char *gameName)
{
	snprintf(str, size, "%s/%s.fc%c", statePath, gameName, saveSlotChar(slot));
}

int EmuSystem::saveState()
{
	FsSys::cPath saveStr;
	sprintStateFilename(saveStr, saveStateSlot);
	if(Config::envIsIOSJB)
		fixFilePermissions(saveStr);
	if(!FCEUI_SaveState(saveStr))
		return STATE_RESULT_IO_ERROR;
	else
		return STATE_RESULT_OK;
}

int EmuSystem::loadState(int saveStateSlot)
{
	FsSys::cPath saveStr;
	sprintStateFilename(saveStr, saveStateSlot);
	if(FsSys::fileExists(saveStr))
	{
		logMsg("loading state %s", saveStr);
		if(!FCEUI_LoadState(saveStr))
			return STATE_RESULT_IO_ERROR;
		else
			return STATE_RESULT_OK;
	}
	else
		return STATE_RESULT_NO_FILE;
}

void EmuSystem::saveBackupMem() // for manually saving when not closing game
{
	if(gameIsRunning())
	{
		logMsg("saving backup memory if needed");
		// TODO: fix iOS permissions if needed
		if(isFDS)
			FCEU_FDSWriteModifiedDisk();
		else
			GameInterface(GI_WRITESAVE);
		FCEU_FlushGameCheats(0, 0, false);
	}
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		FsSys::cPath saveStr;
		sprintStateFilename(saveStr, -1);
		if(Config::envIsIOSJB)
			fixFilePermissions(saveStr);
		FCEUI_SaveState(saveStr);
	}
}

void EmuSystem::closeSystem()
{
	FCEUI_CloseGame();
	fceuCheats = 0;
}

void FCEUD_SetPalette(uint8 index, uint8 r, uint8 g, uint8 b)
{
	#ifdef USE_PIX_RGB565
		nativeCol[index] = pixFmt->build(r >> 3, g >> 2, b >> 3, 0);
	#else
		nativeCol[index] = pixFmt->build(r, g, b, 0);
	#endif
	//logMsg("set palette %d %X", index, nativeCol[index]);
}

void FCEUD_GetPalette(uint8 index, uint8 *r, uint8 *g, uint8 *b)
{
	bug_exit("called FCEUD_GetPalette()");
	/**r = palData[index][0];
	*g = palData[index][1];
	*b = palData[index][2];*/
}

static bool usingZapper = 0;
static void cacheUsingZapper()
{
	assert(GameInfo);
	iterateTimes(2, i)
	{
		if(joyports[i].type == SI_ZAPPER)
		{
			usingZapper = 1;
			return;
		}
	}
	usingZapper = 0;
}

static const char* fceuInputToStr(int input)
{
	switch(input)
	{
		case SI_UNSET: return "Unset";
		case SI_GAMEPAD: return "Gamepad";
		case SI_ZAPPER: return "Zapper";
		case SI_NONE: return "None";
		default: bug_branch("%d", input); return 0;
	}
}

static void connectNESInput(int port, ESI type)
{
	assert(GameInfo);
	if(type == SI_GAMEPAD)
	{
		//logMsg("gamepad to port %d", port);
		FCEUI_SetInput(port, SI_GAMEPAD, &padData, 0);
	}
	else if(type == SI_ZAPPER)
	{
		//logMsg("zapper to port %d", port);
		FCEUI_SetInput(port, SI_ZAPPER, &zapperData, 1);
	}
	else
	{
		FCEUI_SetInput(port, SI_NONE, 0, 0);
	}
}

static void setupNESFourScore()
{
	if(!GameInfo)
		return;
	if(!usingZapper)
	{
		if(optionFourScore)
			logMsg("attaching four score");
		FCEUI_SetInputFourscore(optionFourScore);
	}
	else
		FCEUI_SetInputFourscore(0);
}

bool EmuSystem::vidSysIsPAL() { return PAL; }
uint EmuSystem::multiresVideoBaseX() { return 0; }
uint EmuSystem::multiresVideoBaseY() { return 0; }
bool touchControlsApplicable() { return 1; }

static void setupNESInputPorts()
{
	if(!GameInfo)
		return;
	iterateTimes(2, i)
	{
		if(nesInputPortDev[i] == SI_UNSET) // user didn't specify device, go with auto settings
			connectNESInput(i, GameInfo->input[i] == SI_UNSET ? SI_GAMEPAD : GameInfo->input[i]);
		else
			connectNESInput(i, nesInputPortDev[i]);
		logMsg("attached %s to port %d%s", fceuInputToStr(joyports[i].type), i, nesInputPortDev[i] == SI_UNSET ? " (auto)" : "");
	}
	cacheUsingZapper();
	setupNESFourScore();
}

static int cheatCallback(char *name, uint32 a, uint8 v, int compare, int s, int type, void *data)
{
	logMsg("cheat: %s, %d", name, s);
	fceuCheats++;
	return 1;
}

static int loadGameCommon()
{
	emuView.initImage(0, nesPixX, nesVisiblePixY);
	autoDetectedVidSysPAL = PAL;
	if((int)optionVideoSystem == 1)
	{
		FCEUI_SetVidSystem(0);
	}
	else if((int)optionVideoSystem == 2)
	{
		FCEUI_SetVidSystem(1);
	}
	if(EmuSystem::vidSysIsPAL())
		logMsg("using PAL timing");

	FCEUI_ListCheats(cheatCallback, 0);
	if(fceuCheats)
		logMsg("%d total cheats", fceuCheats);

	setupNESInputPorts();
	EmuSystem::configAudioPlayback();

	logMsg("started emu");
	return 1;
}

int EmuSystem::loadGame(const char *path)
{
	closeGame();
//	emuView.initImage(0, nesPixX, nesVisiblePixY);
	setupGamePaths(path);
	FCEUI_SetVidSystem(0); // default to NTSC
	if(!FCEUI_LoadGame(EmuSystem::fullGamePath(), 1))
	{
		popup.post("Error loading game", 1);
		return 0;
	}
	return loadGameCommon();
}

int EmuSystem::loadGameFromIO(Io &io, const char *origFilename)
{
	closeGame();
	setupGameName(origFilename);
	auto ioStream = new EMUFILE_IO(io);
	auto file = new FCEUFILE();
	file->filename = origFilename;
	file->logicalPath = origFilename;
	file->fullFilename = origFilename;
	file->archiveIndex = -1;
	file->stream = ioStream;
	file->size = ioStream->size();
	FCEUI_SetVidSystem(0); // default to NTSC
	if(!FCEUI_LoadGameWithFile(file, origFilename, 1))
	{
		popup.post("Error loading game", 1);
		return 0;
	}
	return loadGameCommon();
}

void EmuSystem::clearInputBuffers()
{
	mem_zero(zapperData);
	mem_zero(padData);
}

void EmuSystem::configAudioRate()
{
	pcmFormat.rate = optionSoundRate;
	bool usingTimer = (uint)optionFrameSkip == optionFrameSkipAuto || PAL;
	float rate = std::round((float)optionSoundRate * (PAL ? 1. : 601./600.));
	#if defined(CONFIG_ENV_WEBOS)
	if(optionFrameSkip != optionFrameSkipAuto)
		rate *= 42660./44100.; // better sync with Pre's refresh rate
	#endif
	FCEUI_Sound(rate);
	logMsg("set NES audio rate %d", FSettings.SndRate);
}


#if 0
void FCEUD_RenderPPULine(uint8 *line, uint y)
{
	if(y < 8 || y >= 224 + 8)
		return;
	y -= 8;
	assert(y < nesVisiblePixY);
	var_copy(outLine, &pixBuff[(y*nesPixX)]);
	iterateTimes(vidPix.x/*-16*/, x)
	{
		outLine[x/*+8*/] = palData[line[x/*+8*/]];
	}
}
#endif

void FCEUD_commitVideo()
{
	emuView.updateAndDrawContent();
}

void FCEUD_emulateSound()
{
	const uint maxAudioFrames = EmuSystem::audioFramesPerVideoFrame+2;
	int16 sound[maxAudioFrames];
	uint frames = FlushEmulateSound(sound);
	assert(frames <= maxAudioFrames);
	//logMsg("%d frames", frames);
	EmuSystem::writeSound(sound, frames);
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	FCEUI_Emulate(renderGfx, processGfx ? 0 : 1, renderAudio);
	// FCEUI_Emulate calls FCEUD_commitVideo & FCEUD_emulateSound depending on parameters
}

namespace Base
{
void onInputEvent(Base::Window &win, const Input::Event &e)
{
	if(EmuSystem::isActive())
	{
		if(unlikely(e.isPointer() && usingZapper))
		{
			if(e.state == Input::PUSHED)
			{
				zapperData[2] = 0;
				if(emuView.gameRect().overlaps({e.x, e.y}))
				{
					int xRel = e.x - emuView.gameRect().x, yRel = e.y - emuView.gameRect().y;
					int xNes = IG::scalePointRange((float)xRel, (float)emuView.gameRect().xSize(), (float)256.);
					int yNes = IG::scalePointRange((float)yRel, (float)emuView.gameRect().ySize(), (float)224.) + 8;
					logMsg("zapper pushed @ %d,%d, on NES %d,%d", e.x, e.y, xNes, yNes);
					zapperData[0] = xNes;
					zapperData[1] = yNes;
					zapperData[2] |= 0x1;
				}
				else // off-screen shot
				{
					zapperData[0] = 0;
					zapperData[1] = 0;
					zapperData[2] |= 0x2;
				}
			}
			else if(e.state == Input::RELEASED)
			{
				zapperData[2] = 0;
			}
		}
	}
	handleInputEvent(win, e);
}
}

void EmuSystem::savePathChanged()
{
	setDirOverrides();
}

namespace Base
{

void onAppMessage(int type, int shortArg, int intArg, int intArg2) {}

CallResult onInit(int argc, char** argv)
{
	EmuSystem::pcmFormat.channels = 1;
	emuView.initPixmap((char*)nativePixBuff, pixFmt, nesPixX, nesVisiblePixY);
	backupSavestates = 0;
	if(!FCEUI_Initialize())
	{
		bug_exit("error in FCEUI_Initialize");
	}
	//FCEUI_SetSoundQuality(2);
	mainInitCommon(argc, argv);
	return OK;
}

CallResult onWindowInit(Base::Window &win)
{
	static const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, VertexColorPixelFormat.build(1. * .4, 0., 0., 1.) },
		{ .3, VertexColorPixelFormat.build(1. * .4, 0., 0., 1.) },
		{ .97, VertexColorPixelFormat.build(.5 * .4, 0., 0., 1.) },
		{ 1., VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};

	mainInitWindowCommon(win, navViewGrad);
	return OK;
}

}
