/*  This file is part of NEO.emu.

	MD.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MD.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MD.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "main"
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppInlines.hh>
#include <imagine/base/Pipe.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/util/ScopeGuard.hh>
#include "internal.hh"

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

	CONFIG conf{};
	GN_Rect visible_area;

	extern int skip_this_frame;
	Uint16 play_buffer[16384]{};
	GN_Surface *buffer{};
	static CONF_ITEM rompathConfItem{};

	CONF_ITEM* cf_get_item_by_name(const char *name)
	{
		//logMsg("getting conf item %s", name);
		static CONF_ITEM conf{};
		if(string_equal(name, "rompath"))
		{
			string_copy(rompathConfItem.data.dt_str.str, EmuSystem::gamePath());
			return &rompathConfItem;
		}
		else if(string_equal(name, "dump"))
		{
			static CONF_ITEM dump{};
			return &dump;
		}
		else if(string_equal(name, "effect"))
		{
			strcpy(conf.data.dt_str.str, "none");
		}
		else if(string_equal(name, "blitter"))
		{
			strcpy(conf.data.dt_str.str, "soft");
		}
		else if(string_equal(name, "transpack"))
		{
			strcpy(conf.data.dt_str.str, "");
		}
		else
			bug_exit("unknown conf item %s", name);
		return &conf;
	}

	const char *get_gngeo_dir(void)
	{
		return EmuSystem::savePath();
	}
}

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2012-2014\nRobert Broglia\nwww.explusalpha.com\n\n(c) 2011 the\nGngeo Team\ncode.google.com/p/gngeo";
bool EmuSystem::handlesGenericIO = false; // TODO: need to re-factor GnGeo file loading code
static ROM_DEF *activeDrv{};
static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;
static uint16 screenBuff[352*256] __attribute__ ((aligned (8))){};
static GN_Surface sdlSurf;
static Base::Pipe guiPipe;
static FS::PathString datafilePath{};
static constexpr bool backgroundRomLoading = true;
static bool loadThreadIsRunning = false;
static const int FBResX = 352;
static bool renderToScreen = 0;

enum { MSG_LOAD_FAILED, MSG_LOAD_OK, MSG_START_PROGRESS, MSG_UPDATE_PROGRESS };

CLINK void main_frame();

struct GUIMessage
{
	constexpr GUIMessage() {}
	constexpr GUIMessage(uint8 type, uint8 shortArg, int intArg): intArg(intArg), shortArg(shortArg), type(type) {}
	int intArg = 0;
	uint8 shortArg = 0;
	uint8 type = 0;
};

const char *EmuSystem::shortSystemName()
{
	return "NeoGeo";
}

const char *EmuSystem::systemName()
{
	return "Neo Geo";
}

CLINK int gn_strictROMChecking()
{
	return optionStrictROMChecking;
}

static bool hasNeoGeoExtension(const char *name)
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

static char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'A';
		case 0 ... 9: return 48 + slot;
		default: bug_branch("%d", slot); return 0;
	}
}

FS::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return FS::makePathStringPrintf("%s/%s.0%c.sta", statePath, gameName, saveSlotChar(slot));
}

std::error_code EmuSystem::saveState()
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	fixFilePermissions(saveStr);
	if(!save_stateWithName(saveStr.data()))
		return {EIO, std::system_category()};
	else
		return {};
}

std::system_error EmuSystem::loadState(int saveStateSlot)
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	if(FS::exists(saveStr.data()))
	{
		logMsg("loading state %s", saveStr.data());
		if(load_stateWithName(saveStr.data()))
			return {{}};
		else
			return {{EIO, std::system_category()}};
	}
	return {{ENOENT, std::system_category()}};
}

void EmuSystem::saveBackupMem()
{
	if(gameIsRunning())
	{
    save_nvram(conf.game);
    save_memcard(conf.game);
	}
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		auto saveStr = sprintStateFilename(-1);
		fixFilePermissions(saveStr);
		if(!save_stateWithName(saveStr.data()))
			logMsg("error saving state %s", saveStr.data());
	}
}

void EmuSystem::closeSystem()
{
	close_game();
}

#ifdef USE_GENERATOR68K
CLINK void swap_memory(Uint8 * mem, Uint32 length);

static bool swapCPUMemForDump()
{
	bool swappedBIOS = 0;
	swap_memory(memory.rom.cpu_m68k.p, memory.rom.cpu_m68k.size);
	if (memory.rom.bios_m68k.p[0]==0x10)
	{
		logMsg("BIOS BYTE1=%08x\n",memory.rom.bios_m68k.p[0]);
		swap_memory(memory.rom.bios_m68k.p, memory.rom.bios_m68k.size);
		swappedBIOS = 1;
	}
	swap_memory(memory.game_vector, 0x80);
	return swappedBIOS;
}

static void reverseSwapCPUMemForDump(bool swappedBIOS)
{
	swap_memory(memory.game_vector, 0x80);
	if(swappedBIOS)
	{
		swap_memory(memory.rom.bios_m68k.p, memory.rom.bios_m68k.size);
	}
	swap_memory(memory.rom.cpu_m68k.p, memory.rom.cpu_m68k.size);
}

#endif

static void loadGamePhase2()
{
	EmuSystem::setFullGameName(activeDrv->longname);
	logMsg("set long game name: %s", EmuSystem::fullGameName().data());
	free(activeDrv);
	activeDrv = 0;

	setTimerIntOption();

	logMsg("finished loading game");
}

void gn_init_pbar(uint action,int size)
{
	using namespace Base;
	logMsg("init pbar %d, %d", action, size);
	if(loadThreadIsRunning)
	{
		GUIMessage msg {MSG_START_PROGRESS, (uint8)action, size};
		guiPipe.write(&msg, sizeof(msg));
	}
}
void gn_update_pbar(int pos)
{
	using namespace Base;
	logMsg("update pbar %d", pos);
	if(loadThreadIsRunning)
	{
		GUIMessage msg {MSG_UPDATE_PROGRESS, 0, pos};
		guiPipe.write(&msg, sizeof(msg));
	}
}

static auto openGngeoDataIO(const char *filename)
{
	#ifdef __ANDROID__
	return openAppAssetIO(filename);
	#else
	return FS::fileFromArchive(datafilePath.data(), filename);
	#endif
}

CLINK ROM_DEF *res_load_drv(const char *name)
{
	auto drvFilename = string_makePrintf<32>(DATAFILE_PREFIX "rom/%s.drv", name);
	auto io = openGngeoDataIO(drvFilename.data());
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
	drv->year = io.readVal<uint32>(); // TODO: LE byte-swap on uint32 reads
	iterateTimes(10, i)
		drv->romsize[i] = io.readVal<uint32>();
	drv->nb_romfile = io.readVal<uint32>();
	iterateTimes(drv->nb_romfile, i)
	{
		io.read(drv->rom[i].filename, 32);
		drv->rom[i].region = io.readVal<uint8>();
		drv->rom[i].src = io.readVal<uint32>();
		drv->rom[i].dest = io.readVal<uint32>();
		drv->rom[i].size = io.readVal<uint32>();
		drv->rom[i].crc = io.readVal<uint32>();
	}
	return drv;
}

CLINK void *res_load_data(const char *name)
{
	auto io = openGngeoDataIO(name);
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

class LoadGameInBackgroundView : public View
{
public:
	Gfx::Text text{"Loading...", &View::defaultFace};
	IG::WindowRect rect;
	IG::WindowRect &viewRect() override { return rect; }

	uint pos = 0, max = 0;

	LoadGameInBackgroundView(Base::Window &win): View(win) {}

	void setMax(uint val)
	{
		max = val;
	}

	void setPos(uint val)
	{
		pos = val;
	}

	void place() override
	{
		text.compile(projP);
	}

	void inputEvent(Input::Event e) override { }

	void draw() override
	{
		using namespace Gfx;
		projP.resetTransforms();
		setBlendMode(0);
		if(max)
		{
			logMsg("drawing");
			noTexProgram.use();
			setColor(.0, .0, .75);
			Gfx::GC barHeight = text.ySize*1.5;
			auto bar = makeGCRectRel(projP.bounds().pos(LC2DO) - GP{0_gc, barHeight/2_gc},
				{IG::scalePointRange((Gfx::GC)pos, 0_gc, (Gfx::GC)max, 0_gc, projP.w), barHeight});
			GeomRect::draw(bar);
		}
		texAlphaProgram.use();
		setColor(COLOR_WHITE);
		text.draw(0, 0, C2DO, projP);
	}

	void onAddedToController(Input::Event e) override {}
};

static int onGUIMessageHandler(Base::Pipe &pipe, LoadGameInBackgroundView &loadGameInBackgroundView)
{
	while(pipe.hasData())
	{
		GUIMessage msg;
		pipe.read(&msg, sizeof(msg));
		switch(msg.type)
		{
			bcase MSG_LOAD_FAILED:
			{
				modalViewController.pop();
				char errorStr[1024];
				pipe.read(errorStr, sizeof(errorStr));
				popup.printf(4, 1, "%s", errorStr);
				pipe.deinit();
				return 0;
			}
			bcase MSG_LOAD_OK:
			{
				modalViewController.pop();
				loadGamePhase2();
				EmuSystem::onLoadGameComplete()(1, Input::Event{});
				pipe.deinit();
				return 0;
			}
			bcase MSG_START_PROGRESS:
			{
				switch(msg.shortArg)
				{
					bcase PBAR_ACTION_LOADROM:
					{
						// starts with "Loading..."
					}
					bcase PBAR_ACTION_DECRYPT:
					{
						loadGameInBackgroundView.text.setString("Decrypting...");
					}
					bcase PBAR_ACTION_SAVEGNO:
					{
						loadGameInBackgroundView.text.setString("Building Cache...\n(may take a while)");
					}
				}
				loadGameInBackgroundView.setPos(0);
				loadGameInBackgroundView.setMax(msg.intArg);
				loadGameInBackgroundView.place();
				mainWin.win.postDraw();
			}
			bcase MSG_UPDATE_PROGRESS:
			{
				loadGameInBackgroundView.setPos(msg.intArg);
				mainWin.win.postDraw();
			}
		}
	}
	return 1;
};

int EmuSystem::loadGame(const char *path)
{
	closeGame(1);
	emuVideo.initImage(0, 304, 224, FBResX*2);
	setupGamePaths(path);

	{
		ROM_DEF *drv = dr_check_zip(FS::basename(path).data());
		if(!drv)
		{
			popup.postError("This game isn't recognized");
			return 0;
		}
		activeDrv = drv;
	}

	logMsg("rom set %s, %s", activeDrv->name, activeDrv->longname);
	FS::PathString gnoFilename{};
	string_printf(gnoFilename, "%s/%s.gno", EmuSystem::savePath(), activeDrv->name);

	if(optionCreateAndUseCache && FS::exists(gnoFilename))
	{
		logMsg("loading .gno file");
		char errorStr[1024];
		if(!init_game(gnoFilename.data(), errorStr))
		{
			popup.printf(4, 1, "%s", errorStr);
			free(activeDrv); activeDrv = 0;
			return 0;
		}
	}
	else
	{
		if(backgroundRomLoading)
		{
			if(modalViewController.hasView())
				modalViewController.pop();
			auto loadGameInBackgroundView = new LoadGameInBackgroundView{mainWin.win};
			modalViewController.pushAndShow(*loadGameInBackgroundView, {});
			guiPipe.init({},
				[loadGameInBackgroundView](Base::Pipe &pipe)
				{
					return onGUIMessageHandler(pipe, *loadGameInBackgroundView);
				});
			IG::makeDetachedThread(
				[]()
				{
					using namespace Base;
					FS::PathString gnoFilename{};
					string_printf(gnoFilename, "%s/%s.gno", EmuSystem::savePath(), activeDrv->name);
					loadThreadIsRunning = true;
					auto loadThreadDone = IG::scopeGuard([](){ loadThreadIsRunning = false; });
					char errorStr[1024];
					if(!init_game(activeDrv->name, errorStr))
					{
						GUIMessage msg {MSG_LOAD_FAILED, 0, 0};
						guiPipe.write(&msg, sizeof(msg));
						guiPipe.write(errorStr, sizeof(errorStr));
						EmuSystem::clearGamePaths();
						free(activeDrv); activeDrv = 0;
						return;
					}
					if(optionCreateAndUseCache && !FS::exists(gnoFilename))
					{
						logMsg("%s doesn't exist, creating", gnoFilename.data());
						#ifdef USE_GENERATOR68K
						bool swappedBIOS = swapCPUMemForDump();
						#endif
						dr_save_gno(&memory.rom, gnoFilename.data());
						#ifdef USE_GENERATOR68K
						reverseSwapCPUMemForDump(swappedBIOS);
						#endif
					}
					GUIMessage msg {MSG_LOAD_OK, 0, 0};
					guiPipe.write(&msg, sizeof(msg));
				});
			return -1;
		}
		else
		{
			char errorStr[1024];
			if(!init_game(activeDrv->name, errorStr))
			{
				popup.printf(4, 1, "%s", errorStr);
				free(activeDrv); activeDrv = 0;
				return 0;
			}

			if(optionCreateAndUseCache && !FS::exists(gnoFilename))
			{
				logMsg("%s doesn't exist, creating", gnoFilename.data());
				#ifdef USE_GENERATOR68K
				bool swappedBIOS = swapCPUMemForDump();
				#endif
				dr_save_gno(&memory.rom, gnoFilename.data());
				#ifdef USE_GENERATOR68K
				reverseSwapCPUMemForDump(swappedBIOS);
				#endif
			}
		}
	}

	loadGamePhase2();
	return 1;
}

int EmuSystem::loadGameFromIO(IO &io, const char *path, const char *origFilename)
{
	return 0; // TODO
}

void EmuSystem::configAudioRate(double frameTime)
{
	pcmFormat.rate = optionSoundRate;
	conf.sample_rate = std::round(optionSoundRate * ((60./1.001) * frameTime));
	if(gameIsRunning())
	{
		logMsg("setting YM2610 rate to %d", conf.sample_rate);
		YM2610ChangeSamplerate(conf.sample_rate);
	}
}

CLINK void screen_update()
{
	if(likely(renderToScreen))
	{
		//logMsg("screen render");
		updateAndDrawEmuVideo();
		renderToScreen = 0;
	}
	else
	{
		//logMsg("skipping render");
	}
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	//logMsg("run frame %d", (int)processGfx);
	renderToScreen = renderGfx;
	skip_this_frame = !processGfx;
	if(processGfx)
		IG::fillData(screenBuff, (uint16)current_pc_pal[4095]);
	main_frame();
	YM2610Update_stream(audioFramesPerVideoFrame);
	if(renderAudio)
	{
		writeSound(play_buffer, audioFramesPerVideoFrame);
	}
}

void EmuSystem::onCustomizeNavView(EmuNavView &view)
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

CallResult EmuSystem::onInit()
{
	// start image on y 16, x 24, size 304x224, 48 pixel padding on the right
	emuVideo.initPixmap((char*)screenBuff + (16*FBResX*2) + (24*2), pixFmt, 304, 224, FBResX*2);
	visible_area.x = 0;//16;
	visible_area.y = 16;
	visible_area.w = 304;//320;
	visible_area.h = 224;
	sdlSurf.pitch = FBResX*2;
	sdlSurf.w = FBResX;
	sdlSurf.pixels = screenBuff;
	buffer = &sdlSurf;
	conf.sound = 1;
	strcpy(rompathConfItem.data.dt_str.str, ".");
	if(!Config::envIsAndroid)
	{
		string_printf(datafilePath, "%s/gngeo_data.zip", Base::assetPath().data());
	}
	return OK;
}

