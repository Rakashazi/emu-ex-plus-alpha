/*  This file is part of GBA.emu.

	GBA.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	GBA.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with GBA.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "main"
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppInlines.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/EmuVideo.hh>
#include "internal.hh"
#include <vbam/gba/GBA.h>
#include <vbam/gba/GBAGfx.h>
#include <vbam/gba/Sound.h>
#include <vbam/gba/RTC.h>
#include <vbam/common/SoundDriver.h>
#include <vbam/common/Patch.h>
#include <vbam/Util.h>

void setGameSpecificSettings(GBASys &gba);
void CPULoop(GBASys &gba, EmuSystemTask *task, EmuVideo *video, EmuAudio *audio);
void CPUCleanUp();
bool CPUReadBatteryFile(GBASys &gba, const char *);
bool CPUWriteBatteryFile(GBASys &gba, const char *);
bool CPUReadState(GBASys &gba, const char *);
bool CPUWriteState(GBASys &gba, const char *);

bool detectedRtcGame = 0;
const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2012-2021\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nVBA-m Team\nvba-m.com";
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

FS::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return FS::makePathStringPrintf("%s/%s%c.sgm", statePath, gameName, saveSlotChar(slot));
}

EmuSystem::Error EmuSystem::saveState(const char *path)
{
	if(CPUWriteState(gGba, path))
		return {};
	else
		return makeFileWriteError();
}

EmuSystem::Error EmuSystem::loadState(const char *path)
{
	if(CPUReadState(gGba, path))
		return {};
	else
		return makeFileReadError();
}

void EmuSystem::saveBackupMem()
{
	if(gameIsRunning())
	{
		logMsg("saving backup memory");
		auto saveStr = FS::makePathStringPrintf("%s/%s.sav", savePath(), gameName().data());
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

static EmuSystem::Error applyGamePatches(const char *patchDir, const char *romName, u8 *rom, int &romSize)
{
	auto patchStr = FS::makePathStringPrintf("%s/%s.ips", patchDir, romName);
	if(FS::exists(patchStr.data()))
	{
		logMsg("applying IPS patch: %s", patchStr.data());
		if(!patchApplyIPS(patchStr.data(), &rom, &romSize))
		{
			return EmuSystem::makeError("Error applying IPS patch");
		}
		return {};
	}
	string_printf(patchStr, "%s/%s.ups", patchDir, romName);
	if(FS::exists(patchStr.data()))
	{
		logMsg("applying UPS patch: %s", patchStr.data());
		if(!patchApplyUPS(patchStr.data(), &rom, &romSize))
		{
			return EmuSystem::makeError("Error applying UPS patch");
		}
		return {};
	}
	string_printf(patchStr, "%s/%s.ppf", patchDir, romName);
	if(FS::exists(patchStr.data()))
	{
		logMsg("applying UPS patch: %s", patchStr.data());
		if(!patchApplyPPF(patchStr.data(), &rom, &romSize))
		{
			return EmuSystem::makeError("Error applying PPF patch");
		}
		return {};
	}
	return {}; // no patch found
}

EmuSystem::Error EmuSystem::loadGame(IO &io, EmuSystemCreateParams, OnLoadProgressDelegate)
{
	int size = CPULoadRomWithIO(gGba, io);
	if(!size)
	{
		return makeFileReadError();
	}
	setGameSpecificSettings(gGba);
	if(auto err = applyGamePatches(EmuSystem::savePath(), EmuSystem::gameName().data(), gGba.mem.rom, size);
		err)
	{
		return err;
	}
	CPUInit(gGba, 0, 0);
	CPUReset(gGba);
	auto saveStr = FS::makePathStringPrintf("%s/%s.sav", EmuSystem::savePath(), EmuSystem::gameName().data());
	CPUReadBatteryFile(gGba, saveStr.data());
	readCheatFile();
	return {};
}

void EmuSystem::onPrepareVideo(EmuVideo &video)
{
	video.setFormat({{240, 160}, pixFmt});
}

void systemDrawScreen(EmuSystemTask *task, EmuVideo &video)
{
	auto img = video.startFrame(task);
	IG::Pixmap framePix{{{240, 160}, IG::PIXEL_RGB565}, gGba.lcd.pix};
	if(!directColorLookup)
	{
		img.pixmap().writeTransformed([](uint16_t p){ return systemColorMap.map16[p]; }, framePix);
	}
	else
	{
		img.pixmap().write(framePix);
	}
	img.endFrame();
}

void systemOnWriteDataToSoundBuffer(EmuAudio *audio, const u16 * finalWave, int length)
{
	//logMsg("%d audio frames", Audio::pPCM.bytesToFrames(length));
	if(audio)
	{
		audio->writeFrames(finalWave, audio->format().bytesToFrames(length));
	}
}

void EmuSystem::runFrame(EmuSystemTask *task, EmuVideo *video, EmuAudio *audio)
{
	CPULoop(gGba, task, video, audio);
}

void EmuSystem::configAudioRate(IG::FloatSeconds frameTime, uint32_t rate)
{
	double mixRate = std::round(rate * (59.7275 * frameTime.count()));
	logMsg("set audio rate:%d, mix rate:%d", rate, (int)mixRate);
	soundSetSampleRate(gGba, mixRate);
}

void EmuApp::onCustomizeNavView(EmuApp::NavView &view)
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

EmuSystem::Error EmuSystem::onInit()
{
	utilUpdateSystemColorMaps(0);
	return {};
}
