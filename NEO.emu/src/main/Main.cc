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
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppInlines.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/EmuVideo.hh>
#include "internal.hh"
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/fs/FS.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/format.hh>

extern "C"
{
	#include <gngeo/roms.h>
	#include <gngeo/conf.h>
	#include <gngeo/emu.h>
	#include <gngeo/fileio.h>
	#include <gngeo/timer.h>
	#include <gngeo/memory.h>
	#include <gngeo/video.h>
	#include <gngeo/screen.h>
	#include <gngeo/menu.h>
	#include <gngeo/resfile.h>

	CONFIG conf{};
	GN_Rect visible_area;

	GN_Surface *buffer{};
	static CONF_ITEM rompathConfItem{};

	CONF_ITEM* cf_get_item_by_name(const char *nameStr)
	{
		using namespace EmuEx;
		//logMsg("getting conf item %s", name);
		static CONF_ITEM conf{};
		std::string_view name{nameStr};
		if(name == "rompath")
		{
			strncpy(rompathConfItem.data.dt_str.str, EmuSystem::contentDirectory().data(), sizeof(rompathConfItem.data.dt_str.str));
			return &rompathConfItem;
		}
		else if(name == "dump")
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

	const char *get_gngeo_dir(void)
	{
		return EmuEx::EmuSystem::contentSaveDirectoryPtr();
	}
}

CLINK void main_frame(void *emuTaskPtr, void *emuVideoPtr);

namespace EmuEx
{

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2012-2022\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nGngeo Team\ncode.google.com/p/gngeo";
bool EmuSystem::handlesGenericIO = false; // TODO: need to re-factor GnGeo file loading code
bool EmuSystem::canRenderRGBA8888 = false;
static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;
static uint16_t screenBuff[352*256] __attribute__ ((aligned (8))){};
static GN_Surface sdlSurf;
static FS::PathString datafilePath{};
static const int FBResX = 352;
// start image on y 16, x 24, size 304x224, 48 pixel padding on the right
static constexpr IG::Pixmap srcPix{{{304, 224}, pixFmt}, screenBuff + (16*FBResX) + (24), {FBResX, IG::Pixmap::Units::PIXEL}};
static EmuSystem::OnLoadProgressDelegate onLoadProgress{};

const char *EmuSystem::shortSystemName()
{
	return "NeoGeo";
}

const char *EmuSystem::systemName()
{
	return "Neo Geo";
}

static bool hasNeoGeoExtension(std::string_view name)
{
	return false; // archives handled by EmuFramework
}

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter = hasNeoGeoExtension;
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = hasNeoGeoExtension;

void EmuSystem::reset(ResetMode mode)
{
	assert(gameIsRunning());
	neogeo_reset();
	cpu_z80_init();
	YM2610Reset();
}

FS::FileString EmuSystem::stateFilename(int slot, std::string_view name)
{
	return IG::format<FS::FileString>("{}.0{}.sta", name, saveSlotCharUpper(slot));
}

void EmuSystem::saveState(IG::ApplicationContext ctx, IG::CStringView path)
{
	if(!save_stateWithName(&ctx, path))
		return EmuSystem::throwFileWriteError();
}

void EmuSystem::loadState(EmuApp &app, IG::CStringView path)
{
	auto ctx = app.appContext();
	if(!load_stateWithName(&ctx, path))
		return EmuSystem::throwFileReadError();
}

static auto nvramPath(IG::ApplicationContext ctx)
{
	return EmuSystem::contentSaveFilePath(ctx, ".nv");
}

static auto memcardPath(IG::ApplicationContext ctx)
{
	return EmuSystem::contentSavePath(ctx, "memcard");
}

void EmuSystem::saveBackupMem(IG::ApplicationContext ctx)
{
	if(!gameIsRunning())
		return;
	FileUtils::writeToUri(ctx, nvramPath(ctx), {memory.sram, 0x10000});
	FileUtils::writeToUri(ctx, memcardPath(ctx), {memory.memcard, 0x800});
}

void EmuSystem::closeSystem(IG::ApplicationContext ctx)
{
	saveBackupMem(ctx);
	close_game();
}

static auto openGngeoDataIO(IG::ApplicationContext ctx, IG::CStringView filename)
{
	#ifdef __ANDROID__
	return ctx.openAsset(filename, IO::AccessHint::ALL);
	#else
	return FS::fileFromArchive(datafilePath, filename);
	#endif
}

void EmuSystem::loadGame(IG::ApplicationContext ctx, IO &, EmuSystemCreateParams, OnLoadProgressDelegate onLoadProgressFunc)
{
	if(contentDirectory().empty())
	{
		throwMissingContentDirError();
	}
	onLoadProgress = onLoadProgressFunc;
	auto resetOnLoadProgress = IG::scopeGuard([&](){ onLoadProgress = {}; });
	ROM_DEF *drv = res_load_drv(&ctx, contentName().data());
	if(!drv)
	{
		throw std::runtime_error("This game isn't recognized");
	}
	auto freeDrv = IG::scopeGuard([&](){ free(drv); });
	logMsg("rom set %s, %s", drv->name, drv->longname);
	auto gnoFilename = EmuSystem::contentSaveFilePath(ctx, ".gno");
	if(optionCreateAndUseCache && ctx.fileUriExists(gnoFilename))
	{
		logMsg("loading .gno file");
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
			logMsg("%s doesn't exist, creating", gnoFilename.data());
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
}

void EmuSystem::configAudioRate(IG::FloatSeconds frameTime, uint32_t rate)
{
	conf.sample_rate = std::round(rate * ((60./1.001) * frameTime.count()));
	if(gameIsRunning())
	{
		logMsg("setting YM2610 rate to %d", conf.sample_rate);
		YM2610ChangeSamplerate(conf.sample_rate);
	}
}

void EmuSystem::renderFramebuffer(EmuVideo &video)
{
	video.startFrameWithFormat({}, srcPix);
}

void EmuSystem::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
{
	//logMsg("run frame %d", (int)processGfx);
	if(video)
		IG::fill(screenBuff, (uint16_t)current_pc_pal[4095]);
	main_frame(&taskCtx, video);
	auto audioFrames = updateAudioFramesPerVideoFrame();
	Uint16 audioBuff[audioFrames * 2];
	YM2610Update_stream(audioFrames, audioBuff);
	if(audio)
	{
		audio->writeFrames(audioBuff, audioFrames);
	}
}

FS::FileString EmuSystem::contentDisplayNameForPath(IG::ApplicationContext ctx, IG::CStringView path)
{
	auto contentName = contentDisplayNameForPathDefaultImpl(ctx, path);
	if(contentName.empty())
		return {};
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
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build((255./255.) * .4, (215./255.) * .4, (0./255.) * .4, 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build((255./255.) * .4, (215./255.) * .4, (0./255.) * .4, 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build((85./255.) * .4, (71./255.) * .4, (0./255.) * .4, 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};
	view.setBackgroundGradient(navViewGrad);
}

void EmuSystem::onInit(IG::ApplicationContext ctx)
{
	visible_area.x = 0;//16;
	visible_area.y = 16;
	visible_area.w = 304;//320;
	visible_area.h = 224;
	sdlSurf.pitch = FBResX*2;
	sdlSurf.w = FBResX;
	sdlSurf.pixels = screenBuff;
	buffer = &sdlSurf;
	conf.sound = 1;
	conf.sample_rate = 44100; // must be initialized to any valid value for YM2610Init()
	strcpy(rompathConfItem.data.dt_str.str, ".");
	if(!Config::envIsAndroid)
	{
		IG::formatTo(datafilePath, "{}/gngeo_data.zip", ctx.assetPath());
	}
}

}

using namespace IG;

CLINK int gn_strictROMChecking()
{
	return EmuEx::optionStrictROMChecking;
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
	auto drv = (ROM_DEF*)calloc(sizeof(ROM_DEF), 1);
	io.read(drv->name, 32);
	io.read(drv->parent, 32);
	io.read(drv->longname, 128);
	drv->year = io.get<uint32_t>(); // TODO: LE byte-swap on uint32_t reads
	iterateTimes(10, i)
		drv->romsize[i] = io.get<uint32_t>();
	drv->nb_romfile = io.get<uint32_t>();
	iterateTimes(drv->nb_romfile, i)
	{
		io.read(drv->rom[i].filename, 32);
		drv->rom[i].region = io.get<uint8_t>();
		drv->rom[i].src = io.get<uint32_t>();
		drv->rom[i].dest = io.get<uint32_t>();
		drv->rom[i].size = io.get<uint32_t>();
		drv->rom[i].crc = io.get<uint32_t>();
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

CLINK void screen_update(void *emuTaskCtxPtr, void *emuVideoPtr)
{
	auto taskCtxPtr = (EmuEx::EmuSystemTaskContext*)emuTaskCtxPtr;
	auto emuVideo = (EmuEx::EmuVideo*)emuVideoPtr;
	if(emuVideo) [[likely]]
	{
		//logMsg("screen render");
		emuVideo->startFrameWithFormat(*taskCtxPtr, EmuEx::srcPix);
	}
	else
	{
		//logMsg("skipping render");
	}
}

void open_nvram(void *contextPtr, char *name)
{
	auto &ctx = *((IG::ApplicationContext*)contextPtr);
	IG::FileUtils::readFromUri(ctx, EmuEx::nvramPath(ctx), {memory.sram, 0x10000});
}

void open_memcard(void *contextPtr, char *name)
{
	auto &ctx = *((IG::ApplicationContext*)contextPtr);
	IG::FileUtils::readFromUri(ctx, EmuEx::memcardPath(ctx), {memory.memcard, 0x800});
}

void gn_init_pbar(unsigned action, int size)
{
	logMsg("init pbar %d, %d", action, size);
	if(EmuEx::onLoadProgress)
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
		EmuEx::onLoadProgress(0, size, actionString(action));
	}
}

void gn_update_pbar(int pos)
{
	logMsg("update pbar %d", pos);
	if(EmuEx::onLoadProgress)
	{
		EmuEx::onLoadProgress(pos, 0, nullptr);
	}
}
