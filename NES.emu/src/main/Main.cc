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
#include <fceu/video.h>
#include <fceu/sound.h>

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2014\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nFCEUX Team\nfceux.com";
bool EmuSystem::hasCheats = true;
bool EmuSystem::hasPALVideoSystem = true;
bool EmuSystem::hasResetModes = true;
uint fceuCheats = 0;
ESI nesInputPortDev[2]{SI_UNSET, SI_UNSET};
uint autoDetectedRegion = 0;
static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;
static bool usingZapper = 0;
const char *fceuReturnedError = {};
static uint16 nativeCol[256]{};
static const uint nesPixX = 256, nesPixY = 240, nesVisiblePixY = 224;
static uint8 XBufData[256 * 256 + 16]{};
// Separate front & back buffers not needed for our video implementation
uint8 *XBuf = XBufData;
uint8 *XBackBuf = XBufData;
uint8 *XDBuf{};
uint8 *XDBackBuf{};
int dendy = 0;
bool paldeemphswap = false;
bool swapDuty = false;

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
	// RGB565
	nativeCol[index] = pixFmt.desc().build(r >> 3, g >> 2, b >> 3, 0);
	//logMsg("set palette %d %X", index, nativeCol[index]);
}

void FCEUD_GetPalette(uint8 index, uint8 *r, uint8 *g, uint8 *b)
{
	bug_exit("called FCEUD_GetPalette()");
	/**r = palData[index][0];
	*g = palData[index][1];
	*b = palData[index][2];*/
}

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

bool EmuSystem::vidSysIsPAL()
{
	return PAL || dendy;
}

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

const char *regionToStr(int region)
{
	switch(region)
	{
		case 0: return "NTSC";
		case 1: return "PAL";
		case 2: return "Dendy";
	}
	return "Unknown";
}

static int regionFromName(const char *name)
{
	if(strstr(name, "(E)") || strstr(name, "(e)") || strstr(name, "(EU)")
		|| strstr(name, "(Europe)") || strstr(name, "(PAL)")
		|| strstr(name, "(F)") || strstr(name, "(f)")
		|| strstr(name, "(G)") || strstr(name, "(g)")
		|| strstr(name, "(I)") || strstr(name, "(i)"))
	{
		return 1; // PAL
	}
	else if(strstr(name, "(RU)") || strstr(name, "(ru)"))
	{
		return 2; // Dendy
	}
	return 0; // NTSC
}

static int loadGameCommon(int detectedRegion)
{
	emuVideo.initImage(false, nesPixX, nesVisiblePixY);
	if((int)optionVideoSystem)
	{
		logMsg("Forced region:%s", regionToStr(optionVideoSystem - 1));
		FCEUI_SetRegion(optionVideoSystem - 1, false);
	}
	else
	{
		logMsg("Detected region:%s", regionToStr(autoDetectedRegion));
		FCEUI_SetRegion(detectedRegion, false);
	}

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
	if(!FCEUI_LoadGameWithFile(file, path, 0))
	{
		popup.post("Error loading game", 1);
		return 0;
	}
	autoDetectedRegion = regionFromName(FS::basename(path).data());
	return loadGameCommon(autoDetectedRegion);
}

void EmuSystem::configAudioRate(double frameTime)
{
	pcmFormat.rate = optionSoundRate;
	double systemFrameRate = vidSysIsPAL() ? 50.007 : 60.0988;
	double rate = std::round(optionSoundRate * (systemFrameRate * frameTime));
	FCEUI_Sound(rate);
	logMsg("set NES audio rate %d", FSettings.SndRate);
}

void FCEUD_frameReady(uint8 *buf, bool display)
{
	auto img = emuVideo.startFrame();
	auto pix = img.pixmap();
	IG::Pixmap ppuPix{{{256, 256}, IG::PIXEL_FMT_I8}, buf};
	auto ppuPixRegion = ppuPix.subPixmap({0, 8}, {256, 224});
	pix.writeTransformed([](uint8 p){ return nativeCol[p]; }, ppuPixRegion);
	img.endFrame();
	if(display)
		updateAndDrawEmuVideo();
}

void FCEUD_emulateSound(bool renderAudio)
{
	const uint maxAudioFrames = EmuSystem::audioFramesPerVideoFrame+32;
	int32 sound[maxAudioFrames];
	uint frames = FlushEmulateSound(sound);
	//logMsg("%d frames", frames);
	assert(frames <= maxAudioFrames);
	if(renderAudio)
	{
		int16 sound16[maxAudioFrames];
		iterateTimes(maxAudioFrames, i)
		{
			sound16[i] = sound[i];
		}
		EmuSystem::writeSound(sound16, frames);
	}
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

void EmuSystem::onOptionsLoaded()
{
	FCEUI_SetSoundQuality(optionSoundQuality);
	FCEUI_DisableSpriteLimitation(!optionSpriteLimit);
}

CallResult EmuSystem::onInit()
{
	EmuSystem::pcmFormat.channels = 1;
	emuVideo.initFormat(pixFmt);
	backupSavestates = 0;
	if(!FCEUI_Initialize())
	{
		bug_exit("error in FCEUI_Initialize");
	}
	return OK;
}
