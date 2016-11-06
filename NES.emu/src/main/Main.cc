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

#define LOGTAG "main"
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppInlines.hh>
#include "internal.hh"
#include <fceu/driver.h>
#include <fceu/state.h>
#include <fceu/fceu.h>
#include <fceu/ppu.h>
#include <fceu/fds.h>
#include <fceu/input.h>
#include <fceu/cheat.h>

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2014\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nFCEUX Team\nfceux.com";
bool EmuSystem::hasCheats = true;
bool EmuSystem::hasPALVideoSystem = true;
bool EmuSystem::hasResetModes = true;
uint fceuCheats = 0;
ESI nesInputPortDev[2]{SI_UNSET, SI_UNSET};
uint autoDetectedVidSysPAL = 0;
#ifdef USE_PIX_RGB565
static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;
#else
static constexpr auto pixFmt = IG::PIXEL_FMT_RGBA8888;
#endif
const char *fceuReturnedError = {};

bool hasFDSBIOSExtension(const char *name)
{
	return string_hasDotExtension(name, "rom") || string_hasDotExtension(name, "bin");
}

static bool hasFDSExtension(const char *name)
{
	return string_hasDotExtension(name, "fds");
}

static bool hasROMExtension(const char *name)
{
	return string_hasDotExtension(name, "nes") || string_hasDotExtension(name, "unf");
}

static bool hasNESExtension(const char *name)
{
	return hasROMExtension(name) || hasFDSExtension(name);
}

const BundledGameInfo &EmuSystem::bundledGameInfo(uint idx)
{
	static const BundledGameInfo info[]
	{
		{ "Test Game", "game.7z" }
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

static void setDirOverrides()
{
	FCEUI_SetDirOverride(FCEUIOD_NV, EmuSystem::savePath());
	FCEUI_SetDirOverride(FCEUIOD_CHEATS, EmuSystem::savePath());
	FCEUI_SetDirOverride(FCEUIOD_PALETTE, EmuSystem::savePath());
}

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter = hasNESExtension;
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = hasNESExtension;

void EmuSystem::reset(ResetMode mode)
{
	assert(gameIsRunning());
	if(mode == RESET_HARD)
		FCEUI_PowerNES();
	else
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

FS::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return FS::makePathStringPrintf("%s/%s.fc%c", statePath, gameName, saveSlotChar(slot));
}

std::error_code EmuSystem::saveState()
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	fixFilePermissions(saveStr);
	if(!FCEUI_SaveState(saveStr.data()))
		return {EIO, std::system_category()};
	else
		return {};
}

std::system_error EmuSystem::loadState(int saveStateSlot)
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	if(FS::exists(saveStr))
	{
		logMsg("loading state %s", saveStr.data());
		if(!FCEUI_LoadState(saveStr.data()))
			return {{EIO, std::system_category()}};
		else
			return {{}};
	}
	else
		return {{ENOENT, std::system_category()}};
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
		auto saveStr = sprintStateFilename(-1);
		fixFilePermissions(saveStr);
		FCEUI_SaveState(saveStr.data());
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
	nativeCol[index] = pixFmt.desc().build(r >> 3, g >> 2, b >> 3, 0);
	#else
	nativeCol[index] = pixFmt.desc().build(r, g, b, 0);
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

void setupNESFourScore()
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

void setupNESInputPorts()
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
	emuVideo.initImage(0, nesPixX, nesVisiblePixY);
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
	bug_exit("should only use loadGameFromIO()");
	return 0;
}

int EmuSystem::loadGameFromIO(IO &io, const char *path, const char *origFilename)
{
	closeGame();
	setupGamePaths(path);
	setDirOverrides();
	auto ioStream = new EMUFILE_IO(io);
	auto file = new FCEUFILE();
	file->filename = path;
	file->logicalPath = path;
	file->fullFilename = path;
	file->archiveIndex = -1;
	file->stream = ioStream;
	file->size = ioStream->size();
	FCEUI_SetVidSystem(0); // default to NTSC
	if(!FCEUI_LoadGameWithFile(file, path, 1))
	{
		popup.post("Error loading game", 1);
		return 0;
	}
	return loadGameCommon();
}

void EmuSystem::configAudioRate(double frameTime)
{
	pcmFormat.rate = optionSoundRate;
	double systemFrameRate = PAL ? 50. : 60.;
	double rate = std::round(optionSoundRate * (systemFrameRate * frameTime));
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
	updateAndDrawEmuVideo();
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

void EmuSystem::savePathChanged()
{
	if(gameIsRunning())
		setDirOverrides();
}

void EmuSystem::onCustomizeNavView(EmuNavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build(1. * .4, 0., 0., 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build(1. * .4, 0., 0., 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build(.5 * .4, 0., 0., 1.) },
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
				if(unlikely(e.isPointer() && usingZapper))
				{
					if(e.state == Input::PUSHED)
					{
						zapperData[2] = 0;
						if(emuVideoLayer.gameRect().overlaps({e.x, e.y}))
						{
							int xRel = e.x - emuVideoLayer.gameRect().x, yRel = e.y - emuVideoLayer.gameRect().y;
							int xNes = IG::scalePointRange((float)xRel, (float)emuVideoLayer.gameRect().xSize(), (float)256.);
							int yNes = IG::scalePointRange((float)yRel, (float)emuVideoLayer.gameRect().ySize(), (float)224.) + 8;
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
		});
}

CallResult EmuSystem::onInit()
{
	EmuSystem::pcmFormat.channels = 1;
	emuVideo.initPixmap((char*)nativePixBuff, pixFmt, nesPixX, nesVisiblePixY);
	backupSavestates = 0;
	if(!FCEUI_Initialize())
	{
		bug_exit("error in FCEUI_Initialize");
	}
	//FCEUI_SetSoundQuality(2);
	return OK;
}
