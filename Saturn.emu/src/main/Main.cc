/*  This file is part of Saturn.emu.

	Saturn.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Saturn.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Saturn.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "main"
#include <emuframework/EmuSystemInlines.hh>
#include <emuframework/EmuAppInlines.hh>
#include <imagine/fs/FS.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/io/IOStream.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/format.hh>
#include <imagine/util/string.h>
#include <mednafen/cdrom/CDInterface.h>
#include <mednafen/state-driver.h>
#include <mednafen/hash/md5.h>
#include <ss/cdb.h>
#include <ss/cart.h>
#include <ss/stvio.h>
#include <ss/smpc.h>
#include <ss/vdp2.h>
#include <mednafen-emuex/MDFNUtils.hh>
#include <mednafen-emuex/ArchiveVFS.hh>
#include <imagine/logger/logger.h>

namespace MDFN_IEN_SS
{

extern bool ResetPending;

void SaveBackupRAM(IG::FileIO&);
void LoadBackupRAM(IG::FileIO&);

}

namespace EmuEx
{

constexpr SystemLogger log{"Saturnemu"};
const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2024\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nMednafen Team\nmednafen.github.io";
bool EmuSystem::handlesArchiveFiles = true;
bool EmuSystem::hasResetModes = true;
bool EmuSystem::hasRectangularPixels = true;
bool EmuSystem::hasPALVideoSystem = true;
bool EmuSystem::canRenderRGB565 = false;
bool EmuSystem::stateSizeChangesAtRuntime = true;
bool EmuApp::needsGlobalInstance = true;

constexpr EmuSystem::BackupMemoryDirtyFlags sramDirtyBit = bit(0);
constexpr EmuSystem::BackupMemoryDirtyFlags nvramDirtyBit = bit(1);
constexpr EmuSystem::BackupMemoryDirtyFlags rtcDirtyBit = bit(2);
constexpr EmuSystem::BackupMemoryDirtyFlags eepromDirtyBit = bit(3);

SaturnApp::SaturnApp(ApplicationInitParams initParams, ApplicationContext &ctx):
	EmuApp{initParams, ctx}, saturnSystem{ctx} {}

static bool hasCDExtension(std::string_view name)
{
	return endsWithAnyCaseless(name, ".toc", ".cue", ".ccd", ".chd", ".m3u");
}

const char *EmuSystem::shortSystemName() const
{
	return "Saturn";
}

const char *EmuSystem::systemName() const
{
	return "Sega Saturn";
}

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter = hasCDExtension;

void SaturnSystem::loadCartNV(EmuApp &app, FileIO &io)
{
	using namespace MDFN_IEN_SS;
	const char* ext = nullptr;
	void* nv_ptr = nullptr;
	bool nv16 = false;
	uint64 nv_size = 0;
	CART_GetNVInfo(&ext, &nv_ptr, &nv16, &nv_size);
	if(!ext)
		return;
	auto fullExt = saveExtMDFN(ext, noMD5InFilenames);
	if(!io)
		io = app.appContext().openFileUri(app.contentSaveFilePath(fullExt), OpenFlags::testCreateFile());
	if(!io)
		throw std::runtime_error(std::format("Error opening {}, please verify save path has write access", contentNameExt(fullExt)));
	auto buff = io.buffer();
	if(!hasGzipHeader(buff))
	{
		log.warn("skipped loading cart memory due to missing gzip header");
		return;
	}
	auto outputSize = uncompressGzip({static_cast<uint8*>(nv_ptr), size_t(nv_size)}, buff);
	if(!outputSize)
		throw std::runtime_error("Error uncompressing cart memory");
	if(nv16)
	{
		for(uint64 i = 0; i < nv_size; i += 2)
		{
			void* p = (uint8*)nv_ptr + i;
			MDFN_ennsb<uint16>(p, MDFN_de16msb(p));
		}
	}
}

void SaturnSystem::saveCartNV(FileIO &io)
{
	using namespace MDFN_IEN_SS;
	if(!io)
		return;
	const char* ext = nullptr;
	void* nv_ptr = nullptr;
	bool nv16 = false;
	uint64 nv_size = 0;
	CART_GetNVInfo(&ext, &nv_ptr, &nv16, &nv_size);
	if(!ext)
		return;
	DynArray<uint8_t> compBuff{size_t(nv_size)};
	DynArray<uint8_t> byteSwapBuff;
	if(nv16)
	{
		log.info("byte swapping cart RAM for file");
		byteSwapBuff.reset(nv_size);
		for(uint64 i = 0; i < nv_size; i += 2)
		{
			void* p = (uint8*)nv_ptr + i;
			void* byteSwapPtr = (uint8*)byteSwapBuff.data() + i;
			MDFN_enmsb<uint16>(byteSwapPtr, MDFN_densb<uint16>(p));
		}
		nv_ptr = byteSwapBuff.data();
	}
	compBuff.trim(compressGzip(compBuff, {static_cast<uint8*>(nv_ptr), size_t(nv_size)}, MDFN_GetSettingI("filesys.state_comp_level")));
	io.truncate(0);
	io.write(compBuff.span(), 0);
}

void SaturnSystem::loadBackupMemory(EmuApp &app)
{
	using namespace MDFN_IEN_SS;
	logMsg("loading backup memory");
	if(ActiveCartType == CART_STV)
	{
		app.setupStaticBackupMemoryFile(stvEepromFileIO, saveExtMDFN("seep", noMD5InFilenames), 0x80);
		STVIO_LoadNV(stvEepromFileIO);
	}
	app.setupStaticBackupMemoryFile(rtcFileIO, saveExtMDFN("smpc", noMD5InFilenames), 12);
	SMPC_LoadNV(rtcFileIO);
	app.setupStaticBackupMemoryFile(backupRamFileIO, saveExtMDFN("bkr", noMD5InFilenames), 32768);
	LoadBackupRAM(backupRamFileIO);
	loadCartNV(app, cartRamFileIO);
}

void SaturnSystem::onFlushBackupMemory(EmuApp &, BackupMemoryDirtyFlags flags)
{
	using namespace MDFN_IEN_SS;
	if(!hasContent())
		return;
	logMsg("saving backup memory");
	if(flags & sramDirtyBit && backupRamFileIO)
		SaveBackupRAM(backupRamFileIO);
	if(flags & nvramDirtyBit && cartRamFileIO)
		saveCartNV(cartRamFileIO);
	if(ActiveCartType == CART_STV && flags & eepromDirtyBit && stvEepromFileIO)
		STVIO_SaveNV(stvEepromFileIO);
	if(flags & rtcDirtyBit && rtcFileIO)
		SMPC_SaveNV(rtcFileIO);
}

WallClockTimePoint SaturnSystem::backupMemoryLastWriteTime(const EmuApp &app) const
{
	return appContext().fileUriLastWriteTime(savePathMDFN(app, 0, "bkr", noMD5InFilenames).c_str());
}

FS::FileString SaturnSystem::stateFilename(int slot, std::string_view name) const
{
	return stateFilenameMDFN(*MDFNGameInfo, slot, name, 'q', noMD5InFilenames);
}

void SaturnSystem::closeSystem()
{
	mdfnGameInfo.CloseGame();
	clearCDInterfaces(CDInterfaces);
	backupRamFileIO = {};
	cartRamFileIO = {};
	stvEepromFileIO = {};
	rtcFileIO = {};
}

WSize SaturnSystem::multiresVideoBaseSize() const { return {704, 0}; }

static FrameTime makeFrameTime(uint8 InterlaceMode)
{
	using namespace MDFN_IEN_SS;
	const double masterClock = VDP2::PAL ? 1734687500. : 1746818181.8181818181;
	const double vdpClock = masterClock / 61. / 4.;
	if(VDP2::PAL)
	{
		const double lines = InterlaceMode ? 312.5 : 313.;
		return std::chrono::duration_cast<FrameTime>(FloatSeconds{454.99 * lines / vdpClock});
	}
	else
	{
		const double lines = InterlaceMode ? 262.5 : 263.;
		return std::chrono::duration_cast<FrameTime>(FloatSeconds{454.99 * lines / vdpClock});
	}
}

static std::vector<std::string> m3uFilenames(auto &io)
{
	std::vector<std::string> filenames;
	auto in = IStream<MapIO>{MapIO{io}};
	for(std::string line; std::getline(in, line);)
	{
		if(line.back() == '\r') // ignore CR on Windows text files
			line.pop_back();
		filenames.emplace_back(std::move(line));
		if(filenames.size() > 15)
			break;
	}
	return filenames;
}

static bool isM3U(std::string_view s)
{
	return endsWithAnyCaseless(s, ".m3u");
}

static ArchiveIO scanCDImages(ArchiveIO arch)
{
	// prioritize .m3u in archives
	bool hasM3U = FS::seekFileInArchive(arch, [](auto &entry) { return isM3U(entry.name()); });
	if(hasM3U)
	{
		log.info("found M3U:{}", arch.name());
		return arch;
	}
	else
	{
		arch.rewind();
		return FS::findFileInArchive(std::move(arch), [](auto &entry) { return EmuSystem::defaultFsFilter(entry.name()); });
	}
}

void SaturnSystem::loadContent(IO &io, EmuSystemCreateParams, OnLoadProgressDelegate)
{
	bool isArchive = EmuApp::hasArchiveExtension(contentFileName());
	auto unloadCD = scopeGuard([&]() { clearCDInterfaces(CDInterfaces); });
	if(isArchive)
	{
		auto cdImgFile = scanCDImages(ArchiveIO{std::move(io)});
		if(!cdImgFile)
			throw std::runtime_error("No recognized file extensions in archive");
		contentFileName_ = cdImgFile.name();
		std::vector<std::string> filenames;
		if(endsWithAnyCaseless(cdImgFile.name(), ".m3u"))
		{
			filenames = m3uFilenames(cdImgFile);
		}
		else
		{
			filenames.emplace_back(cdImgFile.name());
		}
		ArchiveVFS archVFS{std::move(cdImgFile)};
		for(auto &fn : filenames)
		{
			CDInterfaces.emplace_back(CDInterface::Open(&archVFS, std::move(fn), true, 0));
		}
	}
	else
	{
		bool isCHD = endsWithAnyCaseless(contentFileName(), ".chd");
		if(contentDirectory().empty() && !isCHD)
		{
			throwMissingContentDirError();
		}
		std::vector<std::string> filenames;
		if(isM3U(contentFileName()))
		{
			filenames = m3uFilenames(io);
			for(auto &fn : filenames)
			{
				fn = contentDirectory(fn);
			}
		}
		else
		{
			filenames.emplace_back(contentLocation());
		}
		for(auto &fn : filenames)
		{
			CDInterfaces.emplace_back(CDInterface::Open(&NVFS, std::move(fn), false, 0));
		}
	}
	if(!CDInterfaces.size())
		throw std::runtime_error("No disc images found");
	writeCDMD5(mdfnGameInfo, CDInterfaces);
	mdfnGameInfo.LoadCD(&CDInterfaces);
	MDFN_IEN_SS::CDB_SetDisc(false, CDInterfaces[0]);
	unloadCD.cancel();
	mdfnGameInfo.SetInput(12, "builtin", reinterpret_cast<uint8*>(&inputBuff[12]));
	applyInputConfig(EmuApp::get(appContext()));
	if(!videoLines)
	{
		videoLines = MDFN_IEN_SS::VDP2::PAL ? defaultPalLines : defaultNtscLines;
		updateVideoSettings();
	}
	updatePixmap(mSurfacePix.format());
	mSurfacePix.clear();
	lastInterlaceMode = MDFN_IEN_SS::VDP2::InterlaceMode;
	frameTime_ = makeFrameTime(MDFN_IEN_SS::VDP2::InterlaceMode);
}

bool SaturnSystem::onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat fmt)
{
	updatePixmap(fmt);
	return false;
}

void SaturnSystem::updatePixmap(IG::PixelFormat fmt)
{
	mSurfacePix = {{{mdfnGameInfo.fb_width, mdfnGameInfo.fb_height}, fmt}, pixBuff};
}

FrameTime SaturnSystem::frameTime() const { return frameTime_; }

void SaturnSystem::configAudioRate(FrameTime outputFrameTime, int outputRate)
{
	espec.SoundRate = audioMixRate(outputRate, outputFrameTime);
}

void SaturnSystem::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
{
	static constexpr size_t maxAudioFrames = 48000 / minFrameRate;
	static constexpr size_t maxLineWidths = maxFrameBuffHeight;
	using namespace Mednafen;
	int16 audioBuff[maxAudioFrames * 2];
	espec.audio = audio;
	espec.SoundBuf = audio ? audioBuff : nullptr;
	espec.SoundBufMaxSize = audio ? maxAudioFrames : 0;
	espec.SoundBufSize = 0;
	espec.taskCtx = taskCtx;
	espec.video = video;
	espec.skip = !video;
	auto mSurface = toMDFNSurface(mSurfacePix);
	espec.surface = &mSurface;
	int32 lineWidths[maxLineWidths];
	lineWidths[0] = lineWidths[1] = 0; // first line widths are not always set by emulation code
	espec.LineWidths = lineWidths;
	mdfnGameInfo.Emulate(&espec);
	if(lastInterlaceMode != MDFN_IEN_SS::VDP2::InterlaceMode) [[unlikely]]
	{
		lastInterlaceMode = MDFN_IEN_SS::VDP2::InterlaceMode;
		frameTime_ = makeFrameTime(MDFN_IEN_SS::VDP2::InterlaceMode);
		onFrameTimeChanged();
	}
}

void SaturnSystem::reset(EmuApp &, ResetMode mode)
{
	assert(hasContent());
	if(mode == ResetMode::SOFT)
	{
		MDFN_IEN_SS::ResetPending = true;
	}
	else
	{
		MDFN_IEN_SS::SS_Reset(true);
	}
}

int SaturnSystem::currentDiscId() const { return findIndex(CDInterfaces, MDFN_IEN_SS::Cur_CDIF); }

void SaturnSystem::setDisc(int id)
{
	auto currId = currentDiscId();
	if(currId == id)
		return;
	log.info("ejecting disc");
	MDFN_IEN_SS::CDB_SetDisc(true, nullptr);
	if(id != -1)
	{
		log.info("inserting disc:{}", id);
		MDFN_IEN_SS::CDB_SetDisc(false, CDInterfaces[id]);
	}
}

void SaturnSystem::updateVideoSettings()
{
	MDFN_IEN_SS::VDP2::SetGetVideoParams(&mdfnGameInfo, true, videoLines.first, videoLines.last, showHOverscan, false);
}

void SaturnSystem::renderFramebuffer(EmuVideo &video)
{
	espec.taskCtx = {};
	espec.video = &video;
	int32 lineWidths[2]{video.size().x};
	espec.LineWidths = lineWidths;
	MDFND_commitVideoFrame(&espec);
}

double SaturnSystem::videoAspectRatioScale() const
{
	const double horizontalScaler = 352. / 341.; // full pixel width / cropped width
	const double widescreenScaler = 4. / 3.; // scale 4:3 to 16:9
	const bool isWidescreen = widescreenMode == WidescreenMode::On;
	const double baseLines = 224.;
	assumeExpr(videoLines.size() != 0);
	const double lineAspectScaler = baseLines / videoLines.size();
	return (correctLineAspect ? lineAspectScaler : 1.)
		* (showHOverscan ? horizontalScaler : 1.)
		* (isWidescreen ? widescreenScaler : 1.);
}

size_t SaturnSystem::stateSize() { return currStateSize; }
void SaturnSystem::readState(EmuApp&, std::span<uint8_t> buff) { readStateMDFN(buff); }
size_t SaturnSystem::writeState(std::span<uint8_t> buff, SaveStateFlags flags) { return writeStateMDFN(buff, flags); }

void EmuApp::onCustomizeNavView(EmuApp::NavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::PackedColor::format.build((103./255.) * .7, (176./255.) * .7, (255./255.) * .7, 1.) },
		{ .3, Gfx::PackedColor::format.build((103./255.) * .7, (176./255.) * .7, (255./255.) * .7, 1.) },
		{ .97, Gfx::PackedColor::format.build((103./255.) * .4, (176./255.) * .4, (255./255.) * .4, 1.) },
		{ 1., view.separatorColor() },
	};
	view.setBackgroundGradient(navViewGrad);
}

}

namespace Mednafen
{

void MDFN_MidSync(EmulateSpecStruct *espec, const unsigned flags)
{
	if(!espec->audio)
		return;
	//log.debug("{} audio frames", espec.SoundBufSize);
	espec->audio->writeFrames(espec->SoundBuf, std::exchange(espec->SoundBufSize, 0));
}

void MDFND_commitVideoFrame(EmulateSpecStruct *espec)
{
	auto &sys = static_cast<const EmuEx::SaturnSystem&>(*espec->sys);
	int width = std::max(espec->LineWidths[0], espec->LineWidths[1]);
	auto &video = *espec->video;
	video.isOddField = false;
	auto srcPix = sys.mSurfacePix.subView({espec->DisplayRect.x, espec->DisplayRect.y}, {width, espec->DisplayRect.h});
	if(MDFN_IEN_SS::VDP2::InterlaceMode && sys.deinterlaceMode == EmuEx::DeinterlaceMode::Bob)
	{
		bool isOddField = espec->LineWidths[1];
		video.isOddField = isOddField;
		srcPix = video.takeInterlacedFields(srcPix, isOddField);
	}
	video.startFrameWithFormat(espec->taskCtx, srcPix);
}

void MDFN_MediaStateAction(StateMem *sm, const unsigned load, const bool data_only)
{
	auto &sys = static_cast<EmuEx::SaturnSystem&>(EmuEx::gSystem());
	auto discId = sys.currentDiscId();
	bool discPresent = discId != -1;
	std::array<DriveMediaStatus, 1> dms
	{
		{{discPresent ? 2u : 0, discPresent ? uint32(discId) : 0, 0}}
	};
	SFORMAT StateRegs[]
	{
		SFVARN(dms.data()->state_idx, dms.size(), sizeof(*dms.data()), dms.data(), "state_idx"),
		SFVARN(dms.data()->media_idx, dms.size(), sizeof(*dms.data()), dms.data(), "media_idx"),
		SFVARN(dms.data()->orientation_idx, dms.size(), sizeof(*dms.data()), dms.data(), "orientation_idx"),
		SFEND
	};
	if(MDFNSS_StateAction(sm, load, data_only, StateRegs, "MDFNDRIVE_00000000", true) && load)
	{
		// Be sure to set media before loading the emulation module state, as setting media
		// may affect what state is saved in the emulation module code, and setting media
		// can also have side effects(that will be undone by the state load).
		if(!dms[0].state_idx)
		{
			sys.setDisc(-1);
		}
		else if(dms[0].media_idx < sys.CDInterfaces.size())
		{
			sys.setDisc(dms[0].media_idx);
		}
	}
}

}
