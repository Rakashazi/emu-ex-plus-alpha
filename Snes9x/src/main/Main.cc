#define LOGTAG "main"
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppInlines.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/EmuVideo.hh>
#include "internal.hh"
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>
#include <imagine/util/string.h>

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

namespace EmuEx
{

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2022\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nSnes9x Team\nwww.snes9x.com";
#if PIXEL_FORMAT == RGB565
constexpr auto srcPixFmt = IG::PIXEL_FMT_RGB565;
#else
#error "incompatible PIXEL_FORMAT value"
#endif
static EmuSystemTaskContext emuSysTask{};
static EmuVideo *emuVideo{};
static constexpr auto SNES_HEIGHT_480i = SNES_HEIGHT * 2;
static constexpr auto SNES_HEIGHT_EXTENDED_480i = SNES_HEIGHT_EXTENDED * 2;
bool EmuSystem::hasCheats = true;
bool EmuSystem::hasPALVideoSystem = true;
bool EmuSystem::hasResetModes = true;
bool EmuSystem::canRenderRGBA8888 = false;
bool EmuApp::needsGlobalInstance = true;

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter =
	[](std::string_view name)
	{
		return IG::stringEndsWithAny(name, ".smc", ".sfc", ".fig", ".mgd", ".bs", ".SMC", ".SFC", ".FIG", ".MGD", ".BS");
	};
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = defaultFsFilter;

const BundledGameInfo &EmuSystem::bundledGameInfo(unsigned idx) const
{
	static constexpr BundledGameInfo info[]
	{
		{"Bio Worm", "Bio Worm.7z"}
	};

	return info[0];
}

const char *EmuSystem::shortSystemName() const
{
	return "SFC-SNES";
}

const char *EmuSystem::systemName() const
{
	return "Super Famicom (SNES)";
}

void EmuSystem::renderFramebuffer(EmuVideo &video)
{
	IG::Pixmap srcPix{{video.image().size(), srcPixFmt}, GFX.Screen};
	video.startFrameWithFormat({}, srcPix);
}

void EmuSystem::reset(ResetMode mode)
{
	assert(hasContent());
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

FS::FileString EmuSystem::stateFilename(int slot, std::string_view name) const
{
	return IG::format<FS::FileString>("{}.0{}." FREEZE_EXT, name, saveSlotCharUpper(slot));
}

#undef FREEZE_EXT

static FS::PathString sramFilename(EmuSystem &sys)
{
	return sys.contentSaveFilePath(".srm");
}

void EmuSystem::saveState(IG::CStringView path)
{
	if(!S9xFreezeGame(path))
		return throwFileWriteError();
}

void EmuSystem::loadState(IG::CStringView path)
{
	if(S9xUnfreezeGame(path))
	{
		IPPU.RenderThisFrame = TRUE;
	}
	else
		return throwFileReadError();
}

void EmuSystem::onFlushBackupMemory(BackupMemoryDirtyFlags)
{
	if(!hasContent())
		return;
	if(Memory.SRAMSize)
	{
		logMsg("saving backup memory");
		auto saveStr = sramFilename(*this);
		Memory.SaveSRAM(saveStr.data());
	}
}

bool EmuSystem::vidSysIsPAL() { return Settings.PAL; }
unsigned EmuSystem::multiresVideoBaseX() { return 256; }
unsigned EmuSystem::multiresVideoBaseY() { return 239; }

void EmuSystem::loadContent(IO &io, EmuSystemCreateParams, OnLoadProgressDelegate)
{
	auto size = io.size();
	if(size > CMemory::MAX_ROM_SIZE + 512)
	{
		throw std::runtime_error("ROM is too large");
	}
	#ifndef SNES9X_VERSION_1_4
	IG::fill(Memory.NSRTHeader);
	#endif
	Memory.HeaderCount = 0;
	strncpy(Memory.ROMFilename, contentFileName().data(), sizeof(Memory.ROMFilename));
	auto forceVideoSystemSettings = []() -> std::pair<bool, bool> // ForceNTSC, ForcePAL
	{
		switch(optionVideoSystem.val)
		{
			case 1: return {true, false};
			case 2: return {false, true};
			case 3: return {true, true};
		}
		return {false, false};
	};
	Settings.ForceNTSC = forceVideoSystemSettings().first;
	Settings.ForcePAL = forceVideoSystemSettings().second;
	auto buff = io.buffer();
	if(!buff)
	{
		throwFileReadError();
	}
	if(!Memory.LoadROMMem((const uint8*)buff.data(), buff.size()))
	{
		throw std::runtime_error("Error loading game");
	}
	setupSNESInput(*this, EmuApp::get(appContext()).defaultVController());
	auto saveStr = sramFilename(*this);
	Memory.LoadSRAM(saveStr.data());
	IPPU.RenderThisFrame = TRUE;
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

void EmuSystem::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
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
	emuSysTask = taskCtx;
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

void EmuSystem::onInit()
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
	S9xCheatsEnable();
	#else
	S9xInitSound(Settings.SoundPlaybackRate, Settings.Stereo, 0);
	assert(Settings.H_Max == SNES_CYCLES_PER_SCANLINE);
	assert(Settings.HBlankStart == (256 * Settings.H_Max) / SNES_HCOUNTER_MAX);
	#endif
}

}

#ifndef SNES9X_VERSION_1_4
bool8 S9xDeinitUpdate (int width, int height)
#else
bool8 S9xDeinitUpdate(int width, int height, bool8)
#endif
{
	using namespace EmuEx;
	assumeExpr(emuVideo);
	if((height == SNES_HEIGHT_EXTENDED || height == SNES_HEIGHT_EXTENDED_480i)
		&& !optionAllowExtendedVideoLines)
	{
		bool is480i = height >= SNES_HEIGHT_480i;
		height = is480i ? SNES_HEIGHT_480i : SNES_HEIGHT;
	}
	IG::Pixmap srcPix{{{width, height}, srcPixFmt}, GFX.Screen};
	emuVideo->startFrameWithFormat(emuSysTask, srcPix);
	#ifndef SNES9X_VERSION_1_4
	memset(GFX.ZBuffer, 0, GFX.ScreenSize);
	memset(GFX.SubZBuffer, 0, GFX.ScreenSize);
	#endif
	return true;
}

#ifndef SNES9X_VERSION_1_4
bool8 S9xContinueUpdate(int width, int height)
{
	return S9xDeinitUpdate(width, height);
}
#endif
