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

#define LOGTAG "main"
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppInlines.hh>
#include "internal.hh"
#include "Cheats.hh"
#include <vbam/gba/GBA.h>
#include <vbam/gba/GBAGfx.h>
#include <vbam/gba/Sound.h>
#include <vbam/gba/RTC.h>
#include <vbam/common/SoundDriver.h>
#include <vbam/common/Patch.h>
#include <vbam/Util.h>

void setGameSpecificSettings(GBASys &gba);
void CPULoop(GBASys &gba, bool renderGfx, bool processGfx, bool renderAudio);
void CPUCleanUp();
bool CPUReadBatteryFile(GBASys &gba, const char *);
bool CPUWriteBatteryFile(GBASys &gba, const char *);
bool CPUReadState(GBASys &gba, const char *);
bool CPUWriteState(GBASys &gba, const char *);

bool detectedRtcGame = 0;
const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2012-2014\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nVBA-m Team\nvba-m.com";
bool EmuSystem::hasBundledGames = true;
bool EmuSystem::hasCheats = true;

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter =
	[](const char *name)
	{
		return string_hasDotExtension(name, "gba");
	};
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = defaultFsFilter;

const BundledGameInfo &EmuSystem::bundledGameInfo(uint idx)
{
	static const BundledGameInfo info[]
	{
		{"Motocross Challenge", "Motocross Challenge.7z"}
	};

	return info[0];
}

const char *EmuSystem::shortSystemName()
{
	return "GBA";
}

const char *EmuSystem::systemName()
{
	return "Game Boy Advance";
}

#define USE_PIX_RGB565
#ifdef USE_PIX_RGB565
static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;
int systemColorDepth = 16;
int systemRedShift = 11;
int systemGreenShift = 6;
int systemBlueShift = 0;//1;
#else
static const PixelFormatDesc *pixFmt = &PixelFormatBGRA8888;
int systemColorDepth = 32;
int systemRedShift = 19;
int systemGreenShift = 11;
int systemBlueShift = 3;
#endif

void EmuSystem::reset(ResetMode mode)
{
	assert(gameIsRunning());
	CPUReset(gGba);
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
	return FS::makePathStringPrintf("%s/%s%c.sgm", statePath, gameName, saveSlotChar(slot));
}

std::error_code EmuSystem::saveState()
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	fixFilePermissions(saveStr);
	if(CPUWriteState(gGba, saveStr.data()))
		return {};
	else
		return {EIO, std::system_category()};
}

std::system_error EmuSystem::loadState(int saveStateSlot)
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	if(CPUReadState(gGba, saveStr.data()))
		return {{}};
	else
		return {{EIO, std::system_category()}};
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		auto saveStr = sprintStateFilename(-1);
		fixFilePermissions(saveStr);
		CPUWriteState(gGba, saveStr.data());
	}
}

void EmuSystem::saveBackupMem()
{
	if(gameIsRunning())
	{
		logMsg("saving backup memory");
		auto saveStr = FS::makePathStringPrintf("%s/%s.sav", savePath(), gameName().data());
		fixFilePermissions(saveStr);
		CPUWriteBatteryFile(gGba, saveStr.data());
		writeCheatFile();
	}
}

void EmuSystem::closeSystem()
{
	assert(gameIsRunning());
	logMsg("closing game %s", gameName().data());
	saveBackupMem();
	CPUCleanUp();
	detectedRtcGame = 0;
	cheatsNumber = 0; // reset cheat list
}

static bool applyGamePatches(const char *patchDir, const char *romName, u8 *rom, int &romSize)
{
	auto patchStr = FS::makePathStringPrintf("%s/%s.ips", patchDir, romName);
	if(FS::exists(patchStr.data()))
	{
		logMsg("applying IPS patch: %s", patchStr.data());
		if(!patchApplyIPS(patchStr.data(), &rom, &romSize))
		{
			popup.postError("Error applying IPS patch");
			return false;
		}
		return true;
	}
	string_printf(patchStr, "%s/%s.ups", patchDir, romName);
	if(FS::exists(patchStr.data()))
	{
		logMsg("applying UPS patch: %s", patchStr.data());
		if(!patchApplyUPS(patchStr.data(), &rom, &romSize))
		{
			popup.postError("Error applying UPS patch");
			return false;
		}
		return true;
	}
	string_printf(patchStr, "%s/%s.ppf", patchDir, romName);
	if(FS::exists(patchStr.data()))
	{
		logMsg("applying UPS patch: %s", patchStr.data());
		if(!patchApplyPPF(patchStr.data(), &rom, &romSize))
		{
			popup.postError("Error applying PPF patch");
			return false;
		}
		return true;
	}
	return true; // no patch found
}

static int loadGameCommon(int size)
{
	if(!size)
	{
		popup.postError("Error loading ROM");
		return 0;
	}
	emuVideo.initImage(0, 240, 160);
	setGameSpecificSettings(gGba);
	if(!applyGamePatches(EmuSystem::savePath(), EmuSystem::gameName().data(), gGba.mem.rom, size))
	{
		return 0;
	}
	CPUInit(gGba, 0, 0);
	CPUReset(gGba);
	auto saveStr = FS::makePathStringPrintf("%s/%s.sav", EmuSystem::savePath(), EmuSystem::gameName().data());
	CPUReadBatteryFile(gGba, saveStr.data());
	readCheatFile();
	logMsg("started emu");
	return 1;
}

int EmuSystem::loadGame(const char *path)
{
	bug_exit("should only use loadGameFromIO()");
	return 0;
}

int EmuSystem::loadGameFromIO(IO &io, const char *path, const char *)
{
	closeGame();
	setupGamePaths(path);
	int size = CPULoadRomWithIO(gGba, io);
	return loadGameCommon(size);
}

static void commitVideoFrame()
{
	auto img = emuVideo.startFrame();
	IG::Pixmap framePix{{{240, 160}, IG::PIXEL_RGB565}, gGba.lcd.pix};
	if(!directColorLookup)
	{
		img.pixmap().writeTransformed([](uint16 p){ return systemColorMap.map16[p]; }, framePix);
	}
	else
	{
		img.pixmap().write(framePix);
	}
	img.endFrame();
	updateAndDrawEmuVideo();
}

void systemDrawScreen()
{
	commitVideoFrame();
}

void systemOnWriteDataToSoundBuffer(const u16 * finalWave, int length)
{
	//logMsg("%d audio frames", Audio::pPCM.bytesToFrames(length));
	EmuSystem::writeSound(finalWave, EmuSystem::pcmFormat.bytesToFrames(length));
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	CPULoop(gGba, renderGfx, processGfx, renderAudio);
}

void EmuSystem::configAudioRate(double frameTime)
{
	logMsg("set audio rate %d", (int)optionSoundRate);
	pcmFormat.rate = optionSoundRate;
	double rate = std::round(optionSoundRate * (59.73 * frameTime));
	soundSetSampleRate(gGba, rate);
}

void EmuSystem::onCustomizeNavView(EmuNavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build(42./255., 82./255., 190./255., 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build(42./255., 82./255., 190./255., 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build((42./255.) * .6, (82./255.) * .6, (190./255.) * .6, 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};
	view.setBackgroundGradient(navViewGrad);
}

CallResult EmuSystem::onInit()
{
	emuVideo.initFormat(pixFmt);
	utilUpdateSystemColorMaps(0);
	return OK;
}
