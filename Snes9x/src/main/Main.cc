#define LOGTAG "main"
#include <emuframework/EmuSystemInlines.hh>
#include <emuframework/EmuAppInlines.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>
#include <imagine/util/string.h>

#include <snes9x.h>
#include <memmap.h>
#include <display.h>
#include <snapshot.h>
#include <cheats.h>
#ifndef SNES9X_VERSION_1_4
#include <apu/bapu/snes/snes.hpp>
#else
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
constexpr auto SNES_HEIGHT_480i = SNES_HEIGHT * 2;
constexpr auto SNES_HEIGHT_EXTENDED_480i = SNES_HEIGHT_EXTENDED * 2;
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

static IG::PixmapView snesPixmapView(IG::WP size)
{
	return {{size, srcPixFmt}, GFX.Screen, {(int)GFX.Pitch, PixmapView::Units::BYTE}};
}

const char *EmuSystem::shortSystemName() const
{
	return "SFC-SNES";
}

const char *EmuSystem::systemName() const
{
	return "Super Famicom (SNES)";
}

void Snes9xSystem::renderFramebuffer(EmuVideo &video)
{
	video.startFrameWithFormat({}, snesPixmapView(video.image().size()));
}

void Snes9xSystem::reset(EmuApp &, ResetMode mode)
{
	assert(hasContent());
	if(mode == ResetMode::HARD)
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

FS::FileString Snes9xSystem::stateFilename(int slot, std::string_view name) const
{
	return IG::format<FS::FileString>("{}.0{}." FREEZE_EXT, name, saveSlotCharUpper(slot));
}

#undef FREEZE_EXT

static FS::PathString sramFilename(EmuSystem &sys)
{
	return sys.contentSaveFilePath(".srm");
}

void Snes9xSystem::saveState(IG::CStringView path)
{
	if(!S9xFreezeGame(path))
		return throwFileWriteError();
}

void Snes9xSystem::loadState(EmuApp &, IG::CStringView path)
{
	if(S9xUnfreezeGame(path))
	{
		IPPU.RenderThisFrame = TRUE;
	}
	else
		return throwFileReadError();
}

void Snes9xSystem::onFlushBackupMemory(BackupMemoryDirtyFlags)
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

VideoSystem Snes9xSystem::videoSystem() const { return Settings.PAL ? VideoSystem::PAL : VideoSystem::NATIVE_NTSC; }
WP Snes9xSystem::multiresVideoBaseSize() const { return {256, 239}; }

void Snes9xSystem::loadContent(IO &io, EmuSystemCreateParams, OnLoadProgressDelegate)
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
	auto forceVideoSystemSettings = [&]() -> std::pair<bool, bool> // ForceNTSC, ForcePAL
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
	setupSNESInput(EmuApp::get(appContext()).defaultVController());
	auto saveStr = sramFilename(*this);
	Memory.LoadSRAM(saveStr.data());
	IPPU.RenderThisFrame = TRUE;
}

void Snes9xSystem::configAudioRate(IG::FloatSeconds frameTime, int rate)
{
	constexpr double ntscFrameRate = 21477272. / 357366.;
	constexpr double palFrameRate = 21281370. / 425568.;
	const double systemFrameRate = videoSystem() == VideoSystem::PAL ? palFrameRate : ntscFrameRate;
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

void Snes9xSystem::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
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
		{ .0, Gfx::VertexColorPixelFormat.build((139./255.) * .4, (149./255.) * .4, (230./255.) * .4, 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build((139./255.) * .4, (149./255.) * .4, (230./255.) * .4, 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build((46./255.) * .4, (50./255.) * .4, (77./255.) * .4, 1.) },
		{ 1., view.separatorColor() },
	};
	view.setBackgroundGradient(navViewGrad);
}

}

#ifndef SNES9X_VERSION_1_4
bool8 S9xDeinitUpdate (int width, int height)
#else
bool8 S9xDeinitUpdate(int width, int height, bool8)
#endif
{
	using namespace EmuEx;
	auto &sys = gSnes9xSystem();
	assumeExpr(emuVideo);
	if((height == SNES_HEIGHT_EXTENDED || height == SNES_HEIGHT_EXTENDED_480i)
		&& !sys.optionAllowExtendedVideoLines)
	{
		bool is480i = height >= SNES_HEIGHT_480i;
		height = is480i ? SNES_HEIGHT_480i : SNES_HEIGHT;
	}
	emuVideo->startFrameWithFormat(emuSysTask, snesPixmapView({width, height}));
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
