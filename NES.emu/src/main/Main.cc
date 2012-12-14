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

static uint fceuCheats = 0;

#include <fceu/driver.h>
#include <fceu/state.h>
#include <fceu/fceu.h>
#include <fceu/ppu.h>
#include <fceu/fds.h>
#include <fceu/input.h>

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
	CFGKEY_NESKEY_UP = 256, CFGKEY_NESKEY_RIGHT = 257,
	CFGKEY_NESKEY_DOWN = 258, CFGKEY_NESKEY_LEFT = 259,
	CFGKEY_NESKEY_SELECT = 260, CFGKEY_NESKEY_START = 261,
	CFGKEY_NESKEY_A = 262, CFGKEY_NESKEY_B = 263,
	CFGKEY_NESKEY_A_TURBO = 264, CFGKEY_NESKEY_B_TURBO = 265,
	CFGKEY_NESKEY_LEFT_UP = 266, CFGKEY_NESKEY_RIGHT_UP = 267,
	CFGKEY_NESKEY_RIGHT_DOWN = 268, CFGKEY_NESKEY_LEFT_DOWN = 269,

	CFGKEY_FDS_BIOS_PATH = 270, CFGKEY_FOUR_SCORE = 271,

	CFGKEY_NESKEY_A_B = 272,
};

FsSys::cPath fdsBiosPath = "";
static PathOption optionFdsBiosPath(CFGKEY_FDS_BIOS_PATH, fdsBiosPath, sizeof(fdsBiosPath), "");
static Byte1Option optionFourScore(CFGKEY_FOUR_SCORE, 0);

const uint EmuSystem::maxPlayers = 4;
uint EmuSystem::aspectRatioX = 4, EmuSystem::aspectRatioY = 3;
#include "CommonGui.hh"

namespace EmuControls
{

KeyCategory category[categories] =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
		KeyCategory("Gamepad Controls", gamepadName, gameActionKeys),
};

}

void EmuSystem::initOptions() { }

bool EmuSystem::readConfig(Io *io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_FOUR_SCORE: optionFourScore.readFromIO(io, readSize);
		bcase CFGKEY_FDS_BIOS_PATH: optionFdsBiosPath.readFromIO(io, readSize);
		logMsg("fds bios path %s", fdsBiosPath);
		bcase CFGKEY_NESKEY_UP: readKeyConfig2(io, nesKeyIdxUp, readSize);
		bcase CFGKEY_NESKEY_RIGHT: readKeyConfig2(io, nesKeyIdxRight, readSize);
		bcase CFGKEY_NESKEY_DOWN: readKeyConfig2(io, nesKeyIdxDown, readSize);
		bcase CFGKEY_NESKEY_LEFT: readKeyConfig2(io, nesKeyIdxLeft, readSize);
		bcase CFGKEY_NESKEY_LEFT_UP: readKeyConfig2(io, nesKeyIdxLeftUp, readSize);
		bcase CFGKEY_NESKEY_RIGHT_UP: readKeyConfig2(io, nesKeyIdxRightUp, readSize);
		bcase CFGKEY_NESKEY_RIGHT_DOWN: readKeyConfig2(io, nesKeyIdxRightDown, readSize);
		bcase CFGKEY_NESKEY_LEFT_DOWN: readKeyConfig2(io, nesKeyIdxLeftDown, readSize);
		bcase CFGKEY_NESKEY_SELECT: readKeyConfig2(io, nesKeyIdxSelect, readSize);
		bcase CFGKEY_NESKEY_START: readKeyConfig2(io, nesKeyIdxStart, readSize);
		bcase CFGKEY_NESKEY_A: readKeyConfig2(io, nesKeyIdxA, readSize);
		bcase CFGKEY_NESKEY_B: readKeyConfig2(io, nesKeyIdxB, readSize);
		bcase CFGKEY_NESKEY_A_TURBO: readKeyConfig2(io, nesKeyIdxATurbo, readSize);
		bcase CFGKEY_NESKEY_B_TURBO: readKeyConfig2(io, nesKeyIdxBTurbo, readSize);
		bcase CFGKEY_NESKEY_A_B: readKeyConfig2(io, nesKeyIdxAB, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(Io *io)
{
	optionFourScore.writeWithKeyIfNotDefault(io);
	optionFdsBiosPath.writeToIO(io);
	writeKeyConfig2(io, nesKeyIdxUp, CFGKEY_NESKEY_UP);
	writeKeyConfig2(io, nesKeyIdxRight, CFGKEY_NESKEY_RIGHT);
	writeKeyConfig2(io, nesKeyIdxDown, CFGKEY_NESKEY_DOWN);
	writeKeyConfig2(io, nesKeyIdxLeft, CFGKEY_NESKEY_LEFT);
	writeKeyConfig2(io, nesKeyIdxLeftUp, CFGKEY_NESKEY_LEFT_UP);
	writeKeyConfig2(io, nesKeyIdxRightUp, CFGKEY_NESKEY_RIGHT_UP);
	writeKeyConfig2(io, nesKeyIdxRightDown, CFGKEY_NESKEY_RIGHT_DOWN);
	writeKeyConfig2(io, nesKeyIdxLeftDown, CFGKEY_NESKEY_LEFT_DOWN);
	writeKeyConfig2(io, nesKeyIdxSelect, CFGKEY_NESKEY_SELECT);
	writeKeyConfig2(io, nesKeyIdxStart, CFGKEY_NESKEY_START);
	writeKeyConfig2(io, nesKeyIdxA, CFGKEY_NESKEY_A);
	writeKeyConfig2(io, nesKeyIdxB, CFGKEY_NESKEY_B);
	writeKeyConfig2(io, nesKeyIdxATurbo, CFGKEY_NESKEY_A_TURBO);
	writeKeyConfig2(io, nesKeyIdxBTurbo, CFGKEY_NESKEY_B_TURBO);
	writeKeyConfig2(io, nesKeyIdxAB, CFGKEY_NESKEY_A_B);
}

FsDirFilterFunc EmuFilePicker::defaultFsFilter = nesFsFilter;
FsDirFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = nesFsFilter;

#ifdef USE_PIX_RGB565
static const PixelFormatDesc *pixFmt = &PixelFormatRGB565;
#else
static const PixelFormatDesc *pixFmt = &PixelFormatRGBA8888;
#endif

const char *fceuReturnedError = 0;

static uint ptrInputToSysButton(int input)
{
	switch(input)
	{
		case SysVController::F_ELEM: return BIT(0);
		case SysVController::F_ELEM+1: return BIT(1);

		case SysVController::C_ELEM: return BIT(2);
		case SysVController::C_ELEM+1: return BIT(3);

		case SysVController::D_ELEM: return BIT(4) | BIT(6);
		case SysVController::D_ELEM+1: return BIT(4); // up
		case SysVController::D_ELEM+2: return BIT(4) | BIT(7);
		case SysVController::D_ELEM+3: return BIT(6); // left
		case SysVController::D_ELEM+5: return BIT(7); // right
		case SysVController::D_ELEM+6: return BIT(5) | BIT(6);
		case SysVController::D_ELEM+7: return BIT(5); // down
		case SysVController::D_ELEM+8: return BIT(5) | BIT(7);
		default: bug_branch("%d", input); return 0;
	}
}

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

void EmuSystem::handleOnScreenInputAction(uint state, uint vCtrlKey)
{
	handleInputAction(pointerInputPlayer, state, ptrInputToSysButton(vCtrlKey));
}

static const uint audioMaxFramesPerUpdate = (Audio::maxRate/49)*2;

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
	#ifdef CONFIG_BASE_IOS_SETUID
		fixFilePermissions(saveStr);
	#endif
	sprintStateFilename(saveStr, saveStateSlot);
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
	//logMsg("set palette %d %X", index, palData[index]);
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

int EmuSystem::loadGame(const char *path)
{
	closeGame();
	emuView.initImage(0, nesPixX, nesVisiblePixY);
	setupGamePaths(path);
	fceuReturnedError = 0;
	FCEUI_SetVidSystem(0); // default to NTSC
	if(!FCEUI_LoadGame(fullGamePath, 1))
	{
		if(fceuReturnedError)
		{
			logMsg("%s", fceuReturnedError);
			popup.post(fceuReturnedError, 1);
		}
		else
		{
			logMsg("failed to load game");
			popup.post("Error loading game", 1);
		}
		return 0;
	}
	if(vidSysIsPAL())
		logMsg("using PAL timing");

	FCEUI_ListCheats(cheatCallback, 0);
	if(fceuCheats)
		logMsg("%d total cheats", fceuCheats);

	setupNESInputPorts();
	EmuSystem::configAudioRate();

	logMsg("started emu");
	return 1;
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
	float rate = (float)optionSoundRate * (PAL ? 1. : 1.0016);
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

static bool renderToScreen = 0;
void FCEUD_commitVideoFrame()
{
	if(likely(renderToScreen))
	{
		emuView.updateAndDrawContent();
		renderToScreen = 0; // render at most once per call of FCEUI_Emulate() with renderToScreen true beforehand
	}
	else
	{
		//logMsg("skipping render");
	}
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	uint8 *gfx; int32 ssize;

	#ifdef USE_NEW_AUDIO
	Audio::BufferContext *aBuff = 0;
	int16 *sound = 0;
	if(renderAudio && (aBuff = Audio::getPlayBuffer(audioMaxFramesPerUpdate)))
		sound = (int16*)aBuff->data;
	#else
	int16 sound[audioMaxFramesPerUpdate/2];
	#endif

	if(renderGfx)
		renderToScreen = 1;
	FCEUI_Emulate(&gfx, sound, &ssize, processGfx ? 0 : 1);
	// gfx rendered in FCEUD_commitVideoFrame called by FCEUI_Emulate
	static bool first = 1;
	if(first)
	{
		logMsg("got %d audio size", ssize);
		first = 0;
	}
	#ifdef USE_NEW_AUDIO
	if(renderAudio && aBuff)
	{
		assert(ssize <= (int)aBuff->frames);
		Audio::commitPlayBuffer(aBuff, ssize);
	}
	#else
	if(renderAudio && ssize)
	{
		assert(ssize <= (int)audioMaxFramesPerUpdate);
		Audio::writePcm((uchar*)sound, ssize);
	}
	#endif
}

namespace Input
{
void onInputEvent(const InputEvent &e)
{
	if(EmuSystem::isActive())
	{
		if(unlikely(e.isPointer() && usingZapper))
		{
			if(e.state == INPUT_PUSHED)
			{
				zapperData[2] = 0;
				if(emuView.gameView.overlaps(e.x, e.y))
				{
					int xRel = e.x - emuView.gameView.xIPos(LT2DO), yRel = e.y - emuView.gameView.yIPos(LT2DO);
					int xNes = IG::scalePointRange((float)xRel, (float)emuView.gameView.iXSize, (float)256.);
					int yNes = IG::scalePointRange((float)yRel, (float)emuView.gameView.iYSize, (float)224.) + 8;
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
			else if(e.state == INPUT_RELEASED)
			{
				zapperData[2] = 0;
			}
		}
	}
	handleInputEvent(e);
}
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	switch(input)
	{
		case nesKeyIdxUp: return BIT(4);
		case nesKeyIdxRight: return BIT(7);
		case nesKeyIdxDown: return BIT(5);
		case nesKeyIdxLeft: return BIT(6);
		case nesKeyIdxLeftUp: return BIT(6) | BIT(4);
		case nesKeyIdxRightUp: return BIT(7) | BIT(4);
		case nesKeyIdxRightDown: return BIT(7) | BIT(5);
		case nesKeyIdxLeftDown: return BIT(6) | BIT(5);
		case nesKeyIdxSelect: return BIT(2);
		case nesKeyIdxStart: return BIT(3);
		case nesKeyIdxATurbo: turbo = 1;
		case nesKeyIdxA: return BIT(0);
		case nesKeyIdxBTurbo: turbo = 1;
		case nesKeyIdxB: return BIT(1);
		case nesKeyIdxAB: return BIT(0) | BIT(1);
		default: bug_branch("%d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(uint player, uint state, uint emuKey)
{
	if(unlikely(GameInfo->type==GIT_VSUNI)) // TODO: make coin insert separate key
	{
		if(state == INPUT_PUSHED && emuKey == BIT(3))
			FCEUI_VSUniCoin();
	}
	if(state == INPUT_PUSHED)
		setBits(padData, emuKey << playerInputShift(player));
	else
		unsetBits(padData, emuKey << playerInputShift(player));
}

void EmuSystem::savePathChanged()
{
	FCEUI_SetDirOverride(FCEUIOD_NV, EmuSystem::savePath());
	FCEUI_SetDirOverride(FCEUIOD_CHEATS, EmuSystem::savePath());
}

namespace Base
{

void onAppMessage(int type, int shortArg, int intArg, int intArg2) { }

CallResult onInit()
{
	Audio::setHintPcmFramesPerWrite(950); // for PAL
	mainInitCommon();
	EmuSystem::pcmFormat.channels = 1;
	emuView.initPixmap((uchar*)nativePixBuff, pixFmt, nesPixX, nesVisiblePixY);
	backupSavestates = 0;
	FCEUI_SetDirOverride(FCEUIOD_NV, EmuSystem::savePath());
	FCEUI_SetDirOverride(FCEUIOD_CHEATS, EmuSystem::savePath());
	if(!FCEUI_Initialize())
	{
		logErr("error in FCEUI_Initialize");
		Base::abort();
	}
	//FCEUI_SetSoundQuality(2);
	return OK;
}

CallResult onWindowInit()
{
	static const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, VertexColorPixelFormat.build(1. * .4, 0., 0., 1.) },
		{ .3, VertexColorPixelFormat.build(1. * .4, 0., 0., 1.) },
		{ .97, VertexColorPixelFormat.build(.5 * .4, 0., 0., 1.) },
		{ 1., VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};

	mainInitWindowCommon(navViewGrad);
	return OK;
}

}
