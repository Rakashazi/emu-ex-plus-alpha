#define LOGTAG "main"
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppInlines.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/EmuVideo.hh>
#include "internal.hh"

#include <snes9x.h>
#include <memmap.h>
#include <display.h>
#include <snapshot.h>
#include <cheats.h>
#ifndef SNES9X_VERSION_1_4
#include <apu/apu.h>
#include <apu/bapu/snes/snes.hpp>
#include <controls.h>
#else
#include <apu.h>
#include <soundux.h>
#endif

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2021\nRobert Broglia\nwww.explusalpha.com\n\n(c) 1996-2011 the\nSnes9x Team\nwww.snes9x.com";
#if PIXEL_FORMAT == RGB565
static constexpr auto srcPixFmt = IG::PIXEL_FMT_RGB565;
#else
#error "incompatible PIXEL_FORMAT value"
#endif
static EmuSystemTask *emuSysTask{};
static EmuVideo *emuVideo{};
static const unsigned heightChangeFrameDelay = 4;
static unsigned heightChangeFrames = heightChangeFrameDelay;
bool EmuSystem::hasCheats = true;
bool EmuSystem::hasPALVideoSystem = true;
bool EmuSystem::hasResetModes = true;

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter =
	[](const char *name)
	{
		return string_hasDotExtension(name, "smc") ||
				string_hasDotExtension(name, "sfc") ||
				string_hasDotExtension(name, "fig") ||
				string_hasDotExtension(name, "mgd") ||
				string_hasDotExtension(name, "bs");
	};
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = defaultFsFilter;

const BundledGameInfo &EmuSystem::bundledGameInfo(unsigned idx)
{
	static const BundledGameInfo info[]
	{
		{"Bio Worm", "Bio Worm.7z"}
	};

	return info[0];
}

const char *EmuSystem::shortSystemName()
{
	return "SFC-SNES";
}

const char *EmuSystem::systemName()
{
	return "Super Famicom (SNES)";
}

#ifndef SNES9X_VERSION_1_4
bool8 S9xDeinitUpdate (int width, int height)
#else
bool8 S9xDeinitUpdate(int width, int height, bool8)
#endif
{
	assumeExpr(emuVideo);
	if(height == 239 && emuVideo->size().y == 224 && heightChangeFrames) [[unlikely]]
	{
		// ignore rapid 224 -> 239 -> 224 height changes
		//logMsg("skipped height change");
		heightChangeFrames--;
		height = 224;
	}
	else
	{
		heightChangeFrames = heightChangeFrameDelay;
	}
	IG::Pixmap srcPix{{{width, height}, srcPixFmt}, GFX.Screen};
	emuVideo->startFrameWithAltFormat(emuSysTask, srcPix);
	#ifndef SNES9X_VERSION_1_4
	memset(GFX.ZBuffer, 0, GFX.ScreenSize);
	memset(GFX.SubZBuffer, 0, GFX.ScreenSize);
	#endif
	return 1;
}

void EmuSystem::renderFramebuffer(EmuVideo &video)
{
	IG::Pixmap srcPix{{video.image().size(), srcPixFmt}, GFX.Screen};
	video.startFrameWithAltFormat({}, srcPix);
}

void EmuSystem::reset(ResetMode mode)
{
	assert(gameIsRunning());
	if(mode == RESET_HARD)
	{
		S9xReset();
	}
	else
	{
		S9xSoftReset();
	}
}

#ifndef SNES9X_VERSION_1_4
#define FREEZE_EXT "frz"
#else
#define FREEZE_EXT "s96"
#endif

FS::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return FS::makePathStringPrintf("%s/%s.0%c." FREEZE_EXT, statePath, gameName, saveSlotCharUpper(slot));
}

#undef FREEZE_EXT

static FS::PathString sprintSRAMFilename()
{
	return FS::makePathStringPrintf("%s/%s.srm", EmuSystem::savePath(), EmuSystem::gameName().data());
}

static FS::PathString sprintCheatsFilename()
{
	return FS::makePathStringPrintf("%s/%s.cht", EmuSystem::savePath(), EmuSystem::gameName().data());
}

EmuSystem::Error EmuSystem::saveState(const char *path)
{
	if(!S9xFreezeGame(path))
		return EmuSystem::makeFileWriteError();
	else
		return {};
}

EmuSystem::Error EmuSystem::loadState(const char *path)
{
	if(S9xUnfreezeGame(path))
	{
		IPPU.RenderThisFrame = TRUE;
		return {};
	}
	else
		return EmuSystem::makeFileReadError();
}

void EmuSystem::saveBackupMem() // for manually saving when not closing game
{
	if(gameIsRunning())
	{
		if(Memory.SRAMSize)
		{
			logMsg("saving backup memory");
			auto saveStr = sprintSRAMFilename();
			Memory.SaveSRAM(saveStr.data());
		}
		auto cheatsStr = sprintCheatsFilename();
		if(!numCheats())
			logMsg("no cheats present, removing .cht file if present");
		else
			logMsg("saving %u cheat(s)", numCheats());
		S9xSaveCheatFile(cheatsStr.data());
	}
}

void S9xAutoSaveSRAM (void)
{
	EmuSystem::saveBackupMem();
}

void EmuSystem::closeSystem()
{
	saveBackupMem();
}

bool EmuSystem::vidSysIsPAL() { return Settings.PAL; }
unsigned EmuSystem::multiresVideoBaseX() { return 256; }
unsigned EmuSystem::multiresVideoBaseY() { return 239; }

EmuSystem::Error EmuSystem::loadGame(IO &io, EmuSystemCreateParams, OnLoadProgressDelegate)
{
	auto size = io.size();
	if(size > CMemory::MAX_ROM_SIZE + 512)
	{
		return makeError("ROM is too large");
	}
	#ifndef SNES9X_VERSION_1_4
	IG::fill(Memory.NSRTHeader);
	#endif
	Memory.HeaderCount = 0;
	string_copy(Memory.ROMFilename, fullGamePath());
	Settings.ForceNTSC = Settings.ForcePAL = 0;
	switch(optionVideoSystem.val)
	{
		bcase 1: Settings.ForceNTSC = 1;
		bcase 2: Settings.ForcePAL = 1;
		bcase 3: Settings.ForceNTSC = Settings.ForcePAL = 1;
	}
	auto buffView = io.constBufferView();
	if(!buffView)
	{
		return makeFileReadError();
	}
	if(!Memory.LoadROMMem((const uint8*)buffView.data(), buffView.size()))
	{
		return makeError("Error loading game");
	}
	setupSNESInput();
	auto saveStr = sprintSRAMFilename();
	Memory.LoadSRAM(saveStr.data());
	IPPU.RenderThisFrame = TRUE;
	checkAndEnableGlobalCheats();
	return {};
}

void EmuSystem::configAudioRate(IG::FloatSeconds frameTime, uint32_t rate)
{
	constexpr double ntscFrameRate = 21477272. / 357366.;
	constexpr double palFrameRate = 21281370. / 425568.;
	const double systemFrameRate = vidSysIsPAL() ? palFrameRate : ntscFrameRate;
	Settings.SoundPlaybackRate = std::round(rate * (systemFrameRate * frameTime.count()));
	#ifndef SNES9X_VERSION_1_4
	S9xUpdateDynamicRate(0, 10);
	#else
	S9xSetPlaybackRate(Settings.SoundPlaybackRate);
	#endif
	logMsg("sound rate:%d from system frame rate:%f", Settings.SoundPlaybackRate, systemFrameRate);
}

static void mixSamples(uint32_t samples, EmuAudio *audio)
{
	if(!samples) [[unlikely]]
		return;
	assumeExpr(samples % 2 == 0);
	int16_t audioBuff[samples];
	S9xMixSamples((uint8*)audioBuff, samples);
	if(audio)
	{
		//logMsg("%d frames", frames);
		audio->writeFrames(audioBuff, samples / 2);
	}
}

void EmuSystem::runFrame(EmuSystemTask *task, EmuVideo *video, EmuAudio *audio)
{
	if(snesActiveInputPort != SNES_JOYPAD)
	{
		if(doubleClickFrames)
			doubleClickFrames--;
		if(rightClickFrames)
			rightClickFrames--;

		#ifndef SNES9X_VERSION_1_4
		switch(snesActiveInputPort)
		{
			bcase SNES_MOUSE_SWAPPED:
			{
				int x,y;
				uint32 buttons;
				S9xReadMousePosition(0, x, y, buttons);
				*S9xGetMouseBits(0) &= ~(0x40 | 0x80);
				if(buttons == 1)
					*S9xGetMouseBits(0) |= 0x40;
				else if(buttons == 2)
					*S9xGetMouseBits(0) |= 0x80;
				S9xGetMousePosBits(0)[0] = x;
				S9xGetMousePosBits(0)[1] = y;
			}
		}
		#endif
	}
	emuSysTask = task;
	emuVideo = video;
	IPPU.RenderThisFrame = video ? TRUE : FALSE;
	#ifndef SNES9X_VERSION_1_4
	S9xSetSamplesAvailableCallback([](void *audio)
		{
			int samples = S9xGetSampleCount();
			mixSamples(samples, (EmuAudio*)audio);
		}, (void*)audio);
	#endif
	S9xMainLoop();
	// video rendered in S9xDeinitUpdate
	#ifdef SNES9X_VERSION_1_4
	auto samples = updateAudioFramesPerVideoFrame() * 2;
	mixSamples(samples, audio);
	#endif
}

void EmuApp::onCustomizeNavView(EmuApp::NavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build((139./255.) * .4, (149./255.) * .4, (230./255.) * .4, 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build((139./255.) * .4, (149./255.) * .4, (230./255.) * .4, 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build((46./255.) * .4, (50./255.) * .4, (77./255.) * .4, 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};
	view.setBackgroundGradient(navViewGrad);
}

EmuSystem::Error EmuSystem::onInit(Base::ApplicationContext)
{
	static uint16 screenBuff[512*478] __attribute__ ((aligned (8)));
	#ifndef SNES9X_VERSION_1_4
	GFX.Screen = screenBuff;
	#else
	GFX.Screen = (uint8*)screenBuff;
	#endif
	Memory.Init();
	S9xGraphicsInit();
	S9xInitAPU();
	assert(Settings.Stereo == TRUE);
	#ifndef SNES9X_VERSION_1_4
	S9xInitSound(0);
	S9xUnmapAllControls();
	#else
	S9xInitSound(Settings.SoundPlaybackRate, Settings.Stereo, 0);
	assert(Settings.H_Max == SNES_CYCLES_PER_SCANLINE);
	assert(Settings.HBlankStart == (256 * Settings.H_Max) / SNES_HCOUNTER_MAX);
	#endif
	return {};
}
