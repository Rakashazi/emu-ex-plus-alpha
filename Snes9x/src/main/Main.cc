#define LOGTAG "main"
#include <emuframework/EmuSystemInlines.hh>
#include <emuframework/EmuAppInlines.hh>
#include <imagine/fs/FS.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/util/format.hh>
#include <imagine/util/string.h>
#include <imagine/util/zlib.hh>
#include <imagine/logger/logger.h>

#include <memmap.h>
#include <display.h>
#include <snapshot.h>
#include <cheats.h>
#ifndef SNES9X_VERSION_1_4
#include <apu/bapu/snes/snes.hpp>
#else
#include <soundux.h>
#endif

#ifdef SNES9X_VERSION_1_4
bool8 S9xDeinitUpdate(int width, int height);
bool8 S9xDeinitUpdate(int width, int height, bool8) { return S9xDeinitUpdate(width, height); }
static bool S9xInterlaceField()
{
	return (Memory.FillRAM[0x213F] & 0x80) >> 7;
}
#endif

namespace EmuEx
{

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2024\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nSnes9x Team\nwww.snes9x.com";
#if PIXEL_FORMAT == RGB565
constexpr auto srcPixFmt = IG::PixelFmtRGB565;
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
bool EmuSystem::hasRectangularPixels = true;
bool EmuApp::needsGlobalInstance = true;

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter =
	[](std::string_view name)
	{
		return IG::endsWithAnyCaseless(name, ".smc", ".sfc", ".swc", ".bs", ".st", ".fig", ".mgd");
	};

Snes9xApp::Snes9xApp(ApplicationInitParams initParams, ApplicationContext &ctx):
	EmuApp{initParams, ctx}, snes9xSystem{ctx} {}

const BundledGameInfo &EmuSystem::bundledGameInfo(int) const
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

MutablePixmapView Snes9xSystem::fbPixmapView(WSize size, bool useInterlaceFields)
{
	MutablePixmapView pix{{size, srcPixFmt}, GFX.Screen, {int(GFX.Pitch), PixmapView::Units::BYTE}};
	if(useInterlaceFields)
	{
		return EmuVideo::takeInterlacedFields(pix, S9xInterlaceField());
	}
	return pix;
}

void Snes9xSystem::renderFramebuffer(EmuVideo&)
{
	emuSysTask = {};
	S9xDeinitUpdate(IPPU.RenderedScreenWidth, IPPU.RenderedScreenHeight);
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

std::string_view Snes9xSystem::stateFilenameExt() const { return "." FREEZE_EXT; }

#undef FREEZE_EXT

static FS::PathString sramFilename(EmuApp &app)
{
	return app.contentSaveFilePath(".srm");
}

size_t Snes9xSystem::stateSize()
{
	return saveStateSize;
}

#ifdef SNES9X_VERSION_1_4
static uint32 S9xFreezeSize()
{
	DynArray<uint8_t> arr{0x100000};
	auto stream = MapIO{arr}.toFileStream("wb");
	S9xFreezeToStream(stream);
	return ftell(stream);
}
#endif

static int unfreezeStateFrom(std::span<uint8_t> buff)
{
	#ifndef SNES9X_VERSION_1_4
	return S9xUnfreezeGameMem(buff.data(), buff.size());
	#else
	return S9xUnfreezeFromStream(MapIO{buff}.toFileStream("rb"));
	#endif
}

void Snes9xSystem::readState(EmuApp &, std::span<uint8_t> buff)
{
	DynArray<uint8_t> uncompArr;
	if(hasGzipHeader(buff))
	{
		uncompArr = uncompressGzipState(buff);
		buff = uncompArr;
	}
	if(!unfreezeStateFrom(buff))
		throw std::runtime_error("Invalid state data");
	IPPU.RenderThisFrame = TRUE;
}

static void freezeStateTo(std::span<uint8_t> buff)
{
	#ifndef SNES9X_VERSION_1_4
	S9xFreezeGameMem(buff.data(), buff.size());
	#else
	S9xFreezeToStream(MapIO{buff}.toFileStream("wb"));
	#endif
}

size_t Snes9xSystem::writeState(std::span<uint8_t> buff, SaveStateFlags flags)
{
	if(flags.uncompressed)
	{
		freezeStateTo(buff);
		return saveStateSize;
	}
	else
	{
		auto uncompArr = DynArray<uint8_t>(saveStateSize);
		freezeStateTo(uncompArr);
		return compressGzip(buff, uncompArr, Z_DEFAULT_COMPRESSION);
	}
}

void Snes9xSystem::loadBackupMemory(EmuApp &app)
{
	if(!Memory.SRAMSize)
		return;
	logMsg("loading backup memory");
	Memory.LoadSRAM(sramFilename(app).c_str());
}

void Snes9xSystem::onFlushBackupMemory(EmuApp &app, BackupMemoryDirtyFlags)
{
	if(!Memory.SRAMSize)
		return;
	logMsg("saving backup memory");
	Memory.SaveSRAM(sramFilename(app).c_str());
}

WallClockTimePoint Snes9xSystem::backupMemoryLastWriteTime(const EmuApp &app) const
{
	return appContext().fileUriLastWriteTime(app.contentSaveFilePath(".srm").c_str());
}

VideoSystem Snes9xSystem::videoSystem() const { return Settings.PAL ? VideoSystem::PAL : VideoSystem::NATIVE_NTSC; }
WSize Snes9xSystem::multiresVideoBaseSize() const { return {256, 239}; }

static bool isSufamiTurboCart(const IOBuffer &buff)
{
	return buff.size() >= 0x80000 && buff.size() <= 0x100000 &&
		buff.stringView(0, 14) == "BANDAI SFC-ADX" && buff.stringView(0x10, 14) != "SFC-ADX BACKUP";
}

static bool isSufamiTurboBios(const IOBuffer &buff)
{
	return buff.size() == 0x40000 &&
		buff.stringView(0, 14) == "BANDAI SFC-ADX" && buff.stringView(0x10, 14) == "SFC-ADX BACKUP";
}

bool Snes9xSystem::hasBiosExtension(std::string_view name)
{
	return IG::endsWithAnyCaseless(name, ".bin", ".bios");
}

IOBuffer Snes9xSystem::readSufamiTurboBios() const
{
	if(sufamiBiosPath.empty())
		throw std::runtime_error{"No Sufami Turbo BIOS set"};
	logMsg("loading Sufami Turbo BIOS:%s", sufamiBiosPath.data());
	if(EmuApp::hasArchiveExtension(appCtx.fileUriDisplayName(sufamiBiosPath)))
	{
		for(auto &entry : FS::ArchiveIterator{appCtx.openFileUri(sufamiBiosPath)})
		{
			if(entry.type() == FS::file_type::directory || !hasBiosExtension(entry.name()))
				continue;
			auto buff = entry.buffer(IOBufferMode::Release);
			if(!isSufamiTurboBios(buff))
				throw std::runtime_error{"Incompatible Sufami Turbo BIOS"};
			return buff;
		}
		throw std::runtime_error{"Sufami Turbo BIOS not in archive, must end in .bin or .bios"};
	}
	else
	{
		auto buff = appCtx.openFileUri(sufamiBiosPath, {.accessHint = IOAccessHint::All}).releaseBuffer();
		if(!isSufamiTurboBios(buff))
			throw std::runtime_error{"Incompatible Sufami Turbo BIOS"};
		return buff;
	}
}

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
	auto forceVideoSystemSettings = [&]() -> std::pair<bool, bool> // ForceNTSC, ForcePAL
	{
		switch(optionVideoSystem)
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
	#ifndef SNES9X_VERSION_1_4
	if(isSufamiTurboCart(buff)) // TODO: loading dual carts
	{
		logMsg("detected Sufami Turbo cart");
		Memory.ROMFilename = contentFileName();
		auto biosBuff = readSufamiTurboBios();
		if(!Memory.LoadMultiCartMem((const uint8*)buff.data(), buff.size(),
			nullptr, 0,
			biosBuff.data(), biosBuff.size()))
		{
			throw std::runtime_error("Error loading ROM");
		}
	}
	else
	#endif
	{
		if(!Memory.LoadROMMem((const uint8*)buff.data(), buff.size(), contentFileName().data()))
		{
			throw std::runtime_error("Error loading ROM");
		}
	}
	setupSNESInput(EmuApp::get(appContext()).defaultVController());
	saveStateSize = S9xFreezeSize();
	IPPU.RenderThisFrame = TRUE;
}

void Snes9xSystem::configAudioRate(FrameTime outputFrameTime, int outputRate)
{
	#ifndef SNES9X_VERSION_1_4
	auto inputRate = frameTimeSecs().count() / duration_cast<FloatSeconds>(outputFrameTime).count() * 32040.;
	if(inputRate == Settings.SoundInputRate && outputRate == Settings.SoundPlaybackRate)
		return;
	Settings.SoundPlaybackRate = outputRate;
	Settings.SoundInputRate = inputRate;
	logMsg("set sound input rate:%.2f output rate:%d", inputRate, outputRate);
	S9xUpdateDynamicRate(0, 10);
	#else
	int mixRate = std::round(audioMixRate(outputRate, outputFrameTime));
	if(mixRate == Settings.SoundPlaybackRate)
		return;
	Settings.SoundPlaybackRate = mixRate;
	logMsg("set sound mix rate:%d", mixRate);
	S9xSetPlaybackRate(Settings.SoundPlaybackRate);
	#endif
}

static void mixSamples(int samples, EmuAudio *audio)
{
	if(!samples) [[unlikely]]
		return;
	assumeExpr(samples % 2 == 0);
	int16_t audioBuff[1800];
	S9xMixSamples((uint8*)audioBuff, samples);
	if(audio)
	{
		//logMsg("%d frames", samples / 2);
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
		if(snesActiveInputPort == SNES_MOUSE_SWAPPED)
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
		else // light gun
		{
			if(snesMouseClick)
				DoGunLatch(snesPointerX, snesPointerY);
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
		{ .0, Gfx::PackedColor::format.build((139./255.) * .4, (149./255.) * .4, (230./255.) * .4, 1.) },
		{ .3, Gfx::PackedColor::format.build((139./255.) * .4, (149./255.) * .4, (230./255.) * .4, 1.) },
		{ .97, Gfx::PackedColor::format.build((46./255.) * .4, (50./255.) * .4, (77./255.) * .4, 1.) },
		{ 1., view.separatorColor() },
	};
	view.setBackgroundGradient(navViewGrad);
}

}

bool8 S9xDeinitUpdate (int width, int height)
{
	using namespace EmuEx;
	auto &sys = gSnes9xSystem();
	assumeExpr(emuVideo);
	if((height == SNES_HEIGHT_EXTENDED || height == SNES_HEIGHT_EXTENDED_480i)
		&& !sys.optionAllowExtendedVideoLines)
	{
		height = IPPU.Interlace ? SNES_HEIGHT_480i : SNES_HEIGHT;
	}
	bool useInterlaceFields = IPPU.Interlace && sys.deinterlaceMode == EmuEx::DeinterlaceMode::Bob;
	emuVideo->isOddField = useInterlaceFields ? S9xInterlaceField() : 0;
	emuVideo->startFrameWithFormat(emuSysTask, sys.fbPixmapView({width, height}, useInterlaceFields));
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
