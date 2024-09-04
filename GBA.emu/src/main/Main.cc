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
#include <emuframework/EmuAppInlines.hh>
#include <emuframework/EmuSystemInlines.hh>
#include <imagine/fs/FS.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/format.hh>
#include <imagine/util/string.h>
#include <imagine/util/zlib.hh>
#include <imagine/logger/logger.h>
#include <core/gba/gba.h>
#include <core/gba/gbaGfx.h>
#include <core/gba/gbaSound.h>
#include <core/gba/gbaRtc.h>
#include <core/gba/gbaEeprom.h>
#include <core/gba/gbaFlash.h>
#include <core/gba/gbaCheats.h>
#include <core/base/sound_driver.h>
#include <core/base/patch.h>
#include <core/base/file_util.h>
#include <sys/mman.h>

bool patchApplyIPS(FILE* f, uint8_t** rom, int *size);
bool patchApplyUPS(FILE* f, uint8_t** rom, int *size);
bool patchApplyPPF(FILE* f, uint8_t** rom, int *size);

namespace EmuEx
{

constexpr SystemLogger log{"GBA.emu"};
const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2012-2024\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nVBA-m Team\nvba-m.com";
bool EmuSystem::hasBundledGames = true;
bool EmuSystem::hasCheats = true;
bool EmuApp::needsGlobalInstance = true;
constexpr WSize lcdSize{240, 160};

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter =
	[](std::string_view name)
	{
		return IG::endsWithAnyCaseless(name, ".gba");
	};

GbaApp::GbaApp(ApplicationInitParams initParams, ApplicationContext &ctx):
	EmuApp{initParams, ctx}, gbaSystem{ctx} {}

const BundledGameInfo &EmuSystem::bundledGameInfo(int idx) const
{
	static const BundledGameInfo info[]
	{
		{"Motocross Challenge", "Motocross Challenge.7z"}
	};

	return info[0];
}

const char *EmuSystem::shortSystemName() const
{
	return "GBA";
}

const char *EmuSystem::systemName() const
{
	return "Game Boy Advance";
}

void GbaSystem::reset(EmuApp &, ResetMode mode)
{
	assert(hasContent());
	CPUReset(gGba);
}

FS::FileString GbaSystem::stateFilename(int slot, std::string_view name) const
{
	return IG::format<FS::FileString>("{}{}.sgm", name, saveSlotChar(slot));
}

void GbaSystem::readState(EmuApp &app, std::span<uint8_t> buff)
{
	DynArray<uint8_t> uncompArr;
	if(hasGzipHeader(buff))
	{
		uncompArr = uncompressGzipState(buff, saveStateSize);
		buff = uncompArr;
	}
	if(!CPUReadState(gGba, buff.data()))
		throw std::runtime_error("Invalid state data");
}

size_t GbaSystem::writeState(std::span<uint8_t> buff, SaveStateFlags flags)
{
	assert(buff.size() >= saveStateSize);
	if(flags.uncompressed)
	{
		return CPUWriteState(gGba, buff.data());
	}
	else
	{
		assert(saveStateSize);
		auto stateArr = DynArray<uint8_t>(saveStateSize);
		CPUWriteState(gGba, stateArr.data());
		return compressGzip(buff, stateArr, Z_DEFAULT_COMPRESSION);
	}
}

void GbaSystem::loadBackupMemory(EmuApp &app)
{
	if(coreOptions.saveType == GBA_SAVE_NONE)
		return;
	app.setupStaticBackupMemoryFile(saveFileIO, ".sav", saveMemorySize(), 0xFF);
	auto buff = saveFileIO.buffer(IOBufferMode::Release);
	if(buff.isMappedFile())
		saveFileIO = {};
	saveMemoryIsMappedFile = buff.isMappedFile();
	setSaveMemory(std::move(buff));
}

void GbaSystem::onFlushBackupMemory(EmuApp &app, BackupMemoryDirtyFlags)
{
	if(coreOptions.saveType == GBA_SAVE_NONE)
		return;
	const ByteBuffer &saveData = eepromInUse ? eepromData : flashSaveMemory;
	if(saveMemoryIsMappedFile)
	{
		log.info("flushing backup memory");
		msync(saveData.data(), saveData.size(), MS_SYNC);
	}
	else
	{
		log.info("saving backup memory");
		saveFileIO.write(saveData.span(), 0);
	}
}

WallClockTimePoint GbaSystem::backupMemoryLastWriteTime(const EmuApp &app) const
{
	return appContext().fileUriLastWriteTime(app.contentSaveFilePath(".sav").c_str());
}

void GbaSystem::closeSystem()
{
	assert(hasContent());
	CPUCleanUp();
	saveFileIO = {};
	coreOptions.saveType = GBA_SAVE_NONE;
	detectedRtcGame = 0;
	detectedSensorType = {};
	sensorListener = {};
	darknessLevel = darknessLevelDefault;
	cheatsList.clear();
}

void GbaSystem::applyGamePatches(uint8_t *rom, int &romSize)
{
	auto ctx = appContext();
	// The patchApply* functions are responsible for closing the FILE
	if(auto f = IG::FileUtils::fopenUri(ctx, userFilePath(patchesDir, ".ips"), "rb");
		f)
	{
		log.info("applying IPS patch:{}", userFilePath(patchesDir, ".ips"));
		if(!patchApplyIPS(f, &rom, &romSize))
		{
			throw std::runtime_error(std::format("Error applying IPS patch in:\n{}", patchesDir));
		}
	}
	else if(auto f = IG::FileUtils::fopenUri(ctx, userFilePath(patchesDir, ".ups"), "rb");
		f)
	{
		log.info("applying UPS patch:{}", userFilePath(patchesDir, ".ups"));
		if(!patchApplyUPS(f, &rom, &romSize))
		{
			throw std::runtime_error(std::format("Error applying UPS patch in:\n{}", patchesDir));
		}
	}
	else if(auto f = IG::FileUtils::fopenUri(ctx, userFilePath(patchesDir, ".ppf"), "rb");
		f)
	{
		log.info("applying UPS patch:{}", userFilePath(patchesDir, ".ppf"));
		if(!patchApplyPPF(f, &rom, &romSize))
		{
			throw std::runtime_error(std::format("Error applying PPF patch in:\n{}", patchesDir));
		}
	}
}

void GbaSystem::loadContent(IO &io, EmuSystemCreateParams, OnLoadProgressDelegate)
{
	int size = CPULoadRomWithIO(gGba, io);
	if(!size)
	{
		throwFileReadError();
	}
	setGameSpecificSettings(gGba, size);
	applyGamePatches(gGba.mem.rom, size);
	ByteBuffer biosRom;
	if(shouldUseBios())
	{
		biosRom = appContext().openFileUri(biosPath, {.accessHint = IOAccessHint::All}).buffer(IOBufferMode::Release);
		if(biosRom.size() != 0x4000)
			throw std::runtime_error("BIOS size should be 16KB");
	}
	CPUInit(gGba, biosRom);
	CPUReset(gGba);
	saveStateSize = CPUWriteState(gGba, DynArray<uint8_t>{maxStateSize}.data());
	readCheatFile();
}

static void updateColorMap(auto &map, const PixelDesc &pxDesc)
{
	for(auto i : iotaCount(0x10000))
	{
		auto r = remap(i & 0x1f, 0, 0x1f, 0.f, 1.f);
		auto g = remap((i & 0x3e0) >> 5, 0, 0x1f, 0.f, 1.f);
		auto b = remap((i & 0x7c00) >> 10, 0, 0x1f, 0.f, 1.f);
		map[i] = pxDesc.build(r, g, b, 1.f);
	}
}

bool GbaSystem::onVideoRenderFormatChange(EmuVideo &video, IG::PixelFormat fmt)
{
	log.info("updating system color maps");
	video.setFormat({lcdSize, fmt});
	if(fmt == PixelFmtRGB565)
		updateColorMap(systemColorMap.map16, PixelDescRGB565);
	else
		updateColorMap(systemColorMap.map32, fmt.desc().nativeOrder());
	return true;
}

void GbaSystem::renderFramebuffer(EmuVideo &video)
{
	systemDrawScreen({}, video);
}

void GbaSystem::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
{
	CPULoop(gGba, taskCtx, video, audio);
}

void GbaSystem::configAudioRate(FrameTime outputFrameTime, int outputRate)
{
	long mixRate = std::round(audioMixRate(outputRate, outputFrameTime));
	log.info("set sound mix rate:{}", mixRate);
	soundSetSampleRate(gGba, mixRate);
}

void GbaSystem::onStart()
{
	setSensorActive(true);
}

void GbaSystem::onStop()
{
	setSensorActive(false);
}

void EmuApp::onCustomizeNavView(EmuApp::NavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::PackedColor::format.build(42./255., 82./255., 190./255., 1.) },
		{ .3, Gfx::PackedColor::format.build(42./255., 82./255., 190./255., 1.) },
		{ .97, Gfx::PackedColor::format.build((42./255.) * .6, (82./255.) * .6, (190./255.) * .6, 1.) },
		{ 1., view.separatorColor() },
	};
	view.setBackgroundGradient(navViewGrad);
}

}

void systemDrawScreen(EmuEx::EmuSystemTaskContext taskCtx, EmuEx::EmuVideo &video)
{
	using namespace EmuEx;
	auto img = video.startFrame(taskCtx);
	IG::PixmapView framePix{{lcdSize, IG::PixelFmtRGB565}, gGba.lcd.pix};
	assumeExpr(img.pixmap().size() == framePix.size());
	if(img.pixmap().format() == IG::PixelFmtRGB565)
	{
		img.pixmap().writeTransformed([](uint16_t p){ return systemColorMap.map16[p]; }, framePix);
	}
	else
	{
		assumeExpr(img.pixmap().format().bytesPerPixel() == 4);
		img.pixmap().writeTransformed([](uint16_t p){ return systemColorMap.map32[p]; }, framePix);
	}
	img.endFrame();
}

void systemOnWriteDataToSoundBuffer(EmuEx::EmuAudio *audio, const uint16_t *finalWave, int length)
{
	if(audio)
	{
		int frames = length >> 1; // stereo samples
		//log.debug("{} audio frames", frames);
		audio->writeFrames(finalWave, frames);
	}
}
