/*  This file is part of Snes9x EX.

	Please see COPYING file in root directory for license information. */

module;
#include <snes9x.h>
#include <memmap.h>
#include <display.h>
#include <snapshot.h>
#include <cheats.h>
#ifndef SNES9X_VERSION_1_4
#include <apu/bapu/snes/snes.hpp>
#include <apu/apu.h>
#else
#include <soundux.h>
#endif
#include <zlib.h>
#if defined(SNES9X_ANDROID_BRIDGE) && !defined(SNES9X_VERSION_1_4)
#include "../bridge/Bridge.hh"
#endif

module system;

#ifdef SNES9X_VERSION_1_4
extern "C++" bool8 S9xDeinitUpdate(int width, int height);
extern "C" bool8 S9xDeinitUpdate(int width, int height, bool8) { return S9xDeinitUpdate(width, height); }
static bool S9xInterlaceField() { return (Memory.FillRAM[0x213F] & 0x80) >> 7; }
#endif

namespace EmuEx
{

#if PIXEL_FORMAT == RGB565
constexpr auto srcPixFmt = PixelFmtRGB565;
#else
#error "incompatible PIXEL_FORMAT value"
#endif
static EmuSystemTaskContext emuSysTask{};
static EmuVideo *emuVideo{};
constexpr auto SNES_HEIGHT_480i = SNES_HEIGHT * 2;
constexpr auto SNES_HEIGHT_EXTENDED_480i = SNES_HEIGHT_EXTENDED * 2;

extern "C++" std::string_view EmuSystem::shortSystemName() const { return "SFC-SNES"; }
extern "C++" std::string_view EmuSystem::systemName() const { return "Super Famicom (SNES)"; }

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
#if defined(SNES9X_ANDROID_BRIDGE) && !defined(SNES9X_VERSION_1_4)
	SnesBridge::instance().discontinuity();
#endif
	assume(hasContent());
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
	return format<FS::FileString>("{}.0{}." FREEZE_EXT, name, saveSlotCharUpper(slot));
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
#if defined(SNES9X_ANDROID_BRIDGE) && !defined(SNES9X_VERSION_1_4)
	SnesBridge::instance().discontinuity();
#endif
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
	log.info("loading backup memory");
	Memory.LoadSRAM(sramFilename(app).c_str());
}

void Snes9xSystem::onFlushBackupMemory(EmuApp &app, BackupMemoryDirtyFlags)
{
	if(!Memory.SRAMSize)
		return;
	log.info("saving backup memory");
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
	return endsWithAnyCaseless(name, ".bin", ".bios");
}

IOBuffer Snes9xSystem::readSufamiTurboBios() const
{
	if(sufamiBiosPath.empty())
		throw std::runtime_error{"No Sufami Turbo BIOS set"};
	log.info("loading Sufami Turbo BIOS:{}", sufamiBiosPath);
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
#if defined(SNES9X_ANDROID_BRIDGE) && !defined(SNES9X_VERSION_1_4)
	SnesBridge::instance().close();
#endif
	auto size = io.size();
	if(size > CMemory::MAX_ROM_SIZE + 512)
	{
		throw std::runtime_error("ROM is too large");
	}
	#ifndef SNES9X_VERSION_1_4
	fill(Memory.NSRTHeader);
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
		log.info("detected Sufami Turbo cart");
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
#if defined(SNES9X_ANDROID_BRIDGE) && !defined(SNES9X_VERSION_1_4)
	SnesBridge::instance().open(Memory.ROM, Memory.CalculatedSize);
#endif
	saveStateSize = S9xFreezeSize();
	IPPU.RenderThisFrame = TRUE;
}

#if defined(SNES9X_ANDROID_BRIDGE) && !defined(SNES9X_VERSION_1_4)
void Snes9xSystem::closeSystem() { SnesBridge::instance().close(); }
void Snes9xSystem::onStop() { SnesBridge::instance().cancel(); }
#endif

void Snes9xSystem::configAudioRate(FrameRate outputFrameRate, int outputRate)
{
	#ifndef SNES9X_VERSION_1_4
	// input/output frame rate parameters swapped to generate the sound input rate
	auto inputRate = audioMixRate(32040, outputFrameRate, frameRate());
	if(inputRate == Settings.SoundInputRate && outputRate == Settings.SoundPlaybackRate)
		return;
	Settings.SoundPlaybackRate = outputRate;
	Settings.SoundInputRate = inputRate;
	log.info("set sound input rate:{} output rate:{}", inputRate, outputRate);
	S9xUpdateDynamicRate(0, 10);
	#else
	int mixRate = std::round(audioMixRate(outputRate, outputFrameRate));
	if(mixRate == Settings.SoundPlaybackRate)
		return;
	Settings.SoundPlaybackRate = mixRate;
	log.info("set sound mix rate:{}", mixRate);
	S9xSetPlaybackRate(Settings.SoundPlaybackRate);
	#endif
}

static void mixSamples(int samples, EmuAudio *audio)
{
	if(!samples) [[unlikely]]
		return;
	assume(samples % 2 == 0);
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
#if defined(SNES9X_ANDROID_BRIDGE) && !defined(SNES9X_VERSION_1_4)
	auto manualPad = *S9xGetJoypadBits(0);
	auto externalPad = snesActiveInputPort == SNES_JOYPAD ? SnesBridge::instance().next() : 0;
	*S9xGetJoypadBits(0) = manualPad | externalPad;
#endif
	S9xMainLoop();
#if defined(SNES9X_ANDROID_BRIDGE) && !defined(SNES9X_VERSION_1_4)
	*S9xGetJoypadBits(0) = manualPad;
	SnesBridge::instance().publish(Memory.RAM);
#endif
	// video rendered in S9xDeinitUpdate
	#ifdef SNES9X_VERSION_1_4
	auto samples = updateAudioFramesPerVideoFrame() * 2;
	mixSamples(samples, audio);
	#endif
}

}

extern "C++"
{
bool8 S9xDeinitUpdate (int width, int height)
{
	using namespace EmuEx;
	auto &sys = gSnes9xSystem();
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
}
