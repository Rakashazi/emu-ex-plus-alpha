/*  This file is part of NEO.emu.

	NEO.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NEO.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NEO.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "main"
#include <emuframework/EmuSystemInlines.hh>
#include <emuframework/EmuAppInlines.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/fs/FS.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/format.hh>
#include <imagine/util/zlib.hh>
#include <imagine/logger/logger.h>

extern "C"
{
	#include <gngeo/roms.h>
	#include <gngeo/conf.h>
	#include <gngeo/emu.h>
	#include <gngeo/fileio.h>
	#include <gngeo/timer.h>
	#include <gngeo/memory.h>
	#include <gngeo/video.h>
	#include <gngeo/resfile.h>
	#include <gngeo/menu.h>

	CONFIG conf{};
	GN_Rect visible_area;

	GN_Surface *buffer{};
	static CONF_ITEM rompathConfItem{};

	CONF_ITEM* cf_get_item_by_name(const char *nameStr)
	{
		using namespace EmuEx;
		static CONF_ITEM conf{};
		std::string_view name{nameStr};
		if(name == "dump")
		{
			static CONF_ITEM dump{};
			return &dump;
		}
		else if(name == "effect")
		{
			strcpy(conf.data.dt_str.str, "none");
		}
		else if(name == "blitter")
		{
			strcpy(conf.data.dt_str.str, "soft");
		}
		else if(name == "transpack")
		{
			strcpy(conf.data.dt_str.str, "");
		}
		else
		{
			logErr("unknown conf item %s", nameStr);
		}
		return &conf;
	}

	const char *get_gngeo_dir(void *contextPtr)
	{
		auto &sys = EmuEx::EmuApp::get(*(IG::ApplicationContext*)contextPtr).system();
		return sys.contentSaveDirectoryPtr();
	}

	PathArray get_rom_path(void *contextPtr)
	{
		auto &sys = EmuEx::EmuApp::get(*(IG::ApplicationContext*)contextPtr).system();
		PathArray path;
		strncpy(path.data, sys.contentDirectory().data(), sizeof(path));
		return path;
	}
}

CLINK void main_frame(void *emuTaskPtr, void *neoSystemPtr, void *emuVideoPtr);

namespace EmuEx
{

constexpr SystemLogger log{"NEO.emu"};
const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2012-2024\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nGngeo Team\ncode.google.com/p/gngeo";
bool EmuSystem::handlesGenericIO = false; // TODO: need to re-factor GnGeo file loading code
bool EmuSystem::canRenderRGBA8888 = false;
bool EmuSystem::hasRectangularPixels = true;
bool EmuApp::needsGlobalInstance = true;

NeoApp::NeoApp(ApplicationInitParams initParams, ApplicationContext &ctx):
	EmuApp{initParams, ctx}, neoSystem{ctx} {}

NeoSystem::NeoSystem(ApplicationContext ctx):
	EmuSystem{ctx}
{
	visible_area.x = 0;//16;
	visible_area.y = 16;
	visible_area.w = 304;//320;
	visible_area.h = 224;
	sdlSurf.pitch = FBResX*2;
	sdlSurf.w = FBResX;
	sdlSurf.pixels = screenBuff;
	buffer = &sdlSurf;
	conf.sample_rate = 4096; // must be initialized to any valid value for YM2610Init()
	strcpy(rompathConfItem.data.dt_str.str, ".");
	if(!Config::envIsAndroid)
	{
		IG::formatTo(datafilePath, "{}/gngeo_data.zip", appContext().assetPath());
	}
}

const char *EmuSystem::shortSystemName() const
{
	return "NeoGeo";
}

const char *EmuSystem::systemName() const
{
	return "Neo Geo";
}

static bool hasNeoGeoExtension(std::string_view name)
{
	return false; // archives handled by EmuFramework
}

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter = hasNeoGeoExtension;

void NeoSystem::reset(EmuApp &, ResetMode mode)
{
	assert(hasContent());
	neogeo_reset();
	cpu_z80_init();
	YM2610Reset();
}

FS::FileString NeoSystem::stateFilename(int slot, std::string_view name) const
{
	return IG::format<FS::FileString>("{}.0{}.sta", name, saveSlotCharUpper(slot));
}

size_t NeoSystem::stateSize()
{
	return saveStateSize;
}

void NeoSystem::readState(EmuApp &app, std::span<uint8_t> buff)
{
	/* Save pointers */
	Uint8 *ng_lo = memory.ng_lo;
	Uint8 *fix_game_usage=memory.fix_game_usage;
	Uint8 *bksw_unscramble = memory.bksw_unscramble;
	int *bksw_offset=memory.bksw_offset;

	DynArray<uint8_t> uncompArr;
	if(hasGzipHeader(buff))
	{
		uncompArr = uncompressGzipState(buff, saveStateSize);
		buff = uncompArr;
	}
	MapIO buffIO{buff};
	if(!openState(buffIO, STREAD))
		throw std::runtime_error("Invalid state data");
	makeState(buffIO, STREAD);

	/* Restore them */
	memory.ng_lo=ng_lo;
	memory.fix_game_usage=fix_game_usage;
	memory.bksw_unscramble=bksw_unscramble;
	memory.bksw_offset=bksw_offset;

	cpu_68k_bankswitch(bankaddress);
	if (memory.current_vector==0)
		memcpy(memory.rom.cpu_m68k.p, memory.rom.bios_m68k.p, 0x80);
	else
		memcpy(memory.rom.cpu_m68k.p, memory.game_vector, 0x80);
	if (memory.vid.currentpal)
	{
		current_pal = memory.vid.pal_neo[1];
		current_pc_pal = (Uint32 *) memory.vid.pal_host[1];
	}
	else
	{
		current_pal = memory.vid.pal_neo[0];
		current_pc_pal = (Uint32 *) memory.vid.pal_host[0];
	}
	if (memory.vid.currentfix)
	{
		current_fix = memory.rom.game_sfix.p;
		fix_usage = memory.fix_game_usage;
	}
	else
	{
		current_fix = memory.rom.bios_sfix.p;
		fix_usage = memory.fix_board_usage;
	}
}

size_t NeoSystem::writeState(std::span<uint8_t> buff, SaveStateFlags flags)
{
	if(flags.uncompressed)
	{
		MapIO buffIO{buff};
		openState(buffIO, STWRITE);
		makeState(buffIO, STWRITE);
		return buffIO.tell();
	}
	else
	{
		assert(saveStateSize);
		auto stateArr = DynArray<uint8_t>{saveStateSize};
		MapIO buffIO{stateArr};
		openState(buffIO, STWRITE);
		makeState(buffIO, STWRITE);
		return compressGzip(buff, stateArr, Z_DEFAULT_COMPRESSION);
	}
}

void NeoSystem::loadBackupMemory(EmuApp &app)
{
	log.info("loading nvram & memcard");
	app.setupStaticBackupMemoryFile(nvramFileIO, ".nv", 0x10000);
	app.setupStaticBackupMemoryFile(memcardFileIO, ".memcard", 0x800);
	nvramFileIO.read(memory.sram, 0x10000, 0);
	memcardFileIO.read(memory.memcard, 0x800, 0);
}

void NeoSystem::onFlushBackupMemory(EmuApp &app, BackupMemoryDirtyFlags flags)
{
	if(flags & SRAM_DIRTY_BIT)
	{
		log.info("saving nvram");
		nvramFileIO.write(memory.sram, 0x10000, 0);
	}
	if(flags & MEMCARD_DIRTY_BIT)
	{
		log.info("saving memcard");
		memcardFileIO.write(memory.memcard, 0x800, 0);
	}
}

WallClockTimePoint NeoSystem::backupMemoryLastWriteTime(const EmuApp &app) const
{
	return appContext().fileUriLastWriteTime(app.contentSavePath("memcard").c_str());
}

void NeoSystem::closeSystem()
{
	close_game();
	nvramFileIO = {};
	memcardFileIO = {};
}

static auto openGngeoDataIO(IG::ApplicationContext ctx, IG::CStringView filename)
{
	#ifdef __ANDROID__
	return ctx.openAsset(filename, {.accessHint = IOAccessHint::All});
	#else
	auto &datafilePath = static_cast<NeoApp&>(ctx.application()).system().datafilePath;
	return FS::findFileInArchive(ArchiveIO{datafilePath}, filename);
	#endif
}

void NeoSystem::loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate onLoadProgressFunc)
{
	if(contentDirectory().empty())
	{
		throwMissingContentDirError();
	}
	onLoadProgress = onLoadProgressFunc;
	auto resetOnLoadProgress = IG::scopeGuard([&](){ onLoadProgress = {}; });
	auto ctx = appContext();
	ROM_DEF *drv = res_load_drv(&ctx, contentName().data());
	if(!drv)
	{
		throw std::runtime_error("This game isn't recognized");
	}
	auto freeDrv = IG::scopeGuard([&](){ free(drv); });
	log.info("rom set {}, {}", drv->name, drv->longname);
	auto gnoFilename = EmuSystem::contentSaveFilePath(".gno");
	if(optionCreateAndUseCache && ctx.fileUriExists(gnoFilename))
	{
		log.info("loading .gno file");
		char errorStr[1024];
		if(!init_game(&ctx, gnoFilename.data(), errorStr))
		{
			throw std::runtime_error(errorStr);
		}
	}
	else
	{
		char errorStr[1024];
		if(!init_game(&ctx, drv->name, errorStr))
		{
			throw std::runtime_error(errorStr);
		}

		if(optionCreateAndUseCache && !ctx.fileUriExists(gnoFilename))
		{
			log.info("{} doesn't exist, creating", gnoFilename);
			dr_save_gno(&memory.rom, gnoFilename.data());
		}
	}
	EmuSystem::setContentDisplayName(drv->longname);
	setTimerIntOption();
	neogeo_frame_counter = 0;
	neogeo_frame_counter_speed = 8;
	fc = 0;
	last_line = 0;
	// clear excess bits from universe bios region/system settings
	memory.memcard[2] = memory.memcard[2] & 0x80;
	memory.memcard[3] = memory.memcard[3] & 0x3;
	auto &app = EmuApp::get(ctx);
	if(auto memcardPath = app.contentSaveFilePath(".memcard"), sharedMemcardPath = app.contentSavePath("memcard");
		!ctx.fileUriExists(memcardPath) && ctx.fileUriExists(sharedMemcardPath))
	{
		logMsg("copying shared memcard");
		FileUtils::readFromUri(ctx, sharedMemcardPath, {memory.memcard, 0x800});
		FileUtils::writeToUri(ctx, memcardPath, {memory.memcard, 0x800});
	}
	static constexpr size_t maxStateSize = 0x60000;
	saveStateSize = writeState(std::span{std::make_unique<uint8_t[]>(maxStateSize).get(), maxStateSize}, {.uncompressed = true});
}

void NeoSystem::configAudioRate(FrameTime outputFrameTime, int outputRate)
{
	Uint16 mixRate = std::round(audioMixRate(outputRate, outputFrameTime));
	if(conf.sample_rate == mixRate)
		return;
	conf.sample_rate = mixRate;
	logMsg("set sound mix rate:%d", (int)mixRate);
	YM2610ChangeSamplerate(mixRate);
}

void NeoSystem::renderFramebuffer(EmuVideo &video)
{
	video.startFrameWithFormat({}, videoPixmap());
}

void NeoSystem::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
{
	//logMsg("run frame %d", (int)processGfx);
	if(video)
		std::ranges::fill(screenBuff, (uint16_t)current_pc_pal[4095]);
	main_frame(&taskCtx, this, video);
	auto audioFrames = updateAudioFramesPerVideoFrame();
	Uint16 audioBuff[audioFrames * 2];
	YM2610Update_stream(audioFrames, audioBuff);
	if(audio)
	{
		audio->writeFrames(audioBuff, audioFrames);
	}
}

FS::FileString NeoSystem::contentDisplayNameForPath(IG::CStringView path) const
{
	auto contentName = contentDisplayNameForPathDefaultImpl(path);
	if(contentName.empty())
		return {};
	auto ctx = appContext();
	ROM_DEF *drv = res_load_drv(&ctx, contentName.data());
	if(!drv)
		return contentName;
	auto freeDrv = IG::scopeGuard([&](){ free(drv); });
	return drv->longname;
}

void EmuApp::onCustomizeNavView(EmuApp::NavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::PackedColor::format.build((255./255.) * .4, (215./255.) * .4, (0./255.) * .4, 1.) },
		{ .3, Gfx::PackedColor::format.build((255./255.) * .4, (215./255.) * .4, (0./255.) * .4, 1.) },
		{ .97, Gfx::PackedColor::format.build((85./255.) * .4, (71./255.) * .4, (0./255.) * .4, 1.) },
		{ 1., view.separatorColor() },
	};
	view.setBackgroundGradient(navViewGrad);
}

}

using namespace EmuEx;

CLINK int gn_strictROMChecking()
{
	return static_cast<NeoSystem&>(gSystem()).optionStrictROMChecking;
}

CLINK ROM_DEF *res_load_drv(void *contextPtr, const char *name)
{
	auto drvFilename = IG::format<FS::PathString>(DATAFILE_PREFIX "rom/{}.drv", name);
	auto io = EmuEx::openGngeoDataIO(*((IG::ApplicationContext*)contextPtr), drvFilename);
	if(!io)
	{
		logErr("Can't open driver %s", name);
		return nullptr;
	}

	// Fill out the driver struct
	auto drv = (ROM_DEF*)calloc(1, sizeof(ROM_DEF));
	io.read(drv->name, 32);
	io.read(drv->parent, 32);
	io.read(drv->longname, 128);
	drv->year = io.get<uint32_t>(); // TODO: LE byte-swap on uint32_t reads
	for(auto i : iotaCount(10))
	{
		drv->romsize[i] = io.get<uint32_t>();
		//EmuEx::log.debug("ROM region:{} size:{:X}", i, drv->romsize[i]);
	}
	drv->nb_romfile = io.get<uint32_t>();
	for(auto i : iotaCount(drv->nb_romfile))
	{
		io.read(drv->rom[i].filename, 32);
		drv->rom[i].region = io.get<uint8_t>();
		drv->rom[i].src = io.get<uint32_t>();
		drv->rom[i].dest = io.get<uint32_t>();
		drv->rom[i].size = io.get<uint32_t>();
		drv->rom[i].crc = io.get<uint32_t>();
		//EmuEx::log.debug("ROM file:{} region:{}, src:{:X} dest:{:X} size:{:X} crc:{:X}", drv->rom[i].filename,
		//	drv->rom[i].region, drv->rom[i].src, drv->rom[i].dest, drv->rom[i].size, drv->rom[i].crc);
	}
	return drv;
}

CLINK void *res_load_data(void *contextPtr, const char *name)
{
	auto io = EmuEx::openGngeoDataIO(*((IG::ApplicationContext*)contextPtr), name);
	if(!io)
	{
		logErr("Can't data file %s", name);
		return nullptr;
	}
	auto size = io.size();
	auto buffer = (char*)malloc(size);
	io.read(buffer, size);
	return buffer;
}

CLINK void screen_update(void *emuTaskCtxPtr, void *neoSystemPtr, void *emuVideoPtr)
{
	auto taskCtxPtr = (EmuSystemTaskContext*)emuTaskCtxPtr;
	auto emuVideo = (EmuVideo*)emuVideoPtr;
	if(emuVideo) [[likely]]
	{
		//logMsg("screen render");
		emuVideo->startFrameWithFormat(*taskCtxPtr, ((NeoSystem*)neoSystemPtr)->videoPixmap());
	}
	else
	{
		//logMsg("skipping render");
	}
}

CLINK int currentZ80Timeslice()
{
	return IG::remap(memory.vid.current_line, 0, 264, 0, 256);
}

void sramWritten()
{
	EmuEx::gSystem().onBackupMemoryWritten(SRAM_DIRTY_BIT);
}

void memcardWritten()
{
	EmuEx::gSystem().onBackupMemoryWritten(MEMCARD_DIRTY_BIT);
}

void gn_init_pbar(unsigned action, int size)
{
	auto &sys = static_cast<NeoSystem&>(gSystem());
	logMsg("init pbar %d, %d", action, size);
	if(sys.onLoadProgress)
	{
		auto actionString = [](unsigned action)
		{
			switch(action)
			{
				default:
				case PBAR_ACTION_LOADROM: { return ""; } // defaults to "Loading..."
				case PBAR_ACTION_DECRYPT: { return "Decrypting..."; }
				case PBAR_ACTION_SAVEGNO: { return "Building Cache...\n(may take a while)"; };
			}
		};
		sys.onLoadProgress(0, size, actionString(action));
	}
}

void gn_update_pbar(int pos)
{
	auto &sys = static_cast<NeoSystem&>(gSystem());
	logMsg("update pbar %d", pos);
	if(sys.onLoadProgress)
	{
		sys.onLoadProgress(pos, 0, nullptr);
	}
}
