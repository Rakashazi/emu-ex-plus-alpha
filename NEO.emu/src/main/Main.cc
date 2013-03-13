#define thisModuleName "main"
#include <resource2/image/png/ResourceImagePng.h>
#include <logger/interface.h>
#include <gfx/GfxSprite.hh>
#include <audio/Audio.hh>
#include <fs/sys.hh>
#include <io/sys.hh>
#include <gui/View.hh>
#include <util/strings.h>
#include <util/time/sys.hh>
#include <util/thread/pthread.hh>
#include <EmuSystem.hh>
#include <CommonFrameworkIncludes.hh>

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

	CONFIG conf = { 0 };
	GN_Rect visible_area;

	extern int skip_this_frame;
	Uint16 play_buffer[16384] = { 0 };
	GN_Surface *buffer = 0;
	static CONF_ITEM datafileConfItem = { 0 };
	static CONF_ITEM rompathConfItem = { 0 };

	CONF_ITEM* cf_get_item_by_name(const char *name)
	{
		//logMsg("getting conf item %s", name);
		static CONF_ITEM conf = { 0 };
		if(string_equal(name, "datafile"))
		{
			static CONF_ITEM datafile = { 0 };
			return &datafileConfItem;
		}
		else if(string_equal(name, "rompath"))
		{
			static CONF_ITEM rompath = { 0 };
			return &rompathConfItem;
		}
		else if(string_equal(name, "dump"))
		{
			static CONF_ITEM dump = { 0 };
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

const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2012-2013\nRobert Broglia\nwww.explusalpha.com\n\n(c) 2011 the\nGngeo Team\ncode.google.com/p/gngeo";
CLINK void main_frame();
static ROM_DEF *activeDrv = 0;

// controls

enum
{
	neogeoKeyIdxUp = EmuControls::systemKeyMapStart,
	neogeoKeyIdxRight,
	neogeoKeyIdxDown,
	neogeoKeyIdxLeft,
	neogeoKeyIdxLeftUp,
	neogeoKeyIdxRightUp,
	neogeoKeyIdxRightDown,
	neogeoKeyIdxLeftDown,
	neogeoKeyIdxSelect,
	neogeoKeyIdxStart,
	neogeoKeyIdxA,
	neogeoKeyIdxB,
	neogeoKeyIdxX,
	neogeoKeyIdxY,
	neogeoKeyIdxATurbo,
	neogeoKeyIdxBTurbo,
	neogeoKeyIdxXTurbo,
	neogeoKeyIdxYTurbo,
	neogeoKeyIdxABC,
	neogeoKeyIdxTestSwitch = EmuControls::systemKeyMapStart + EmuControls::joystickKeys*2
};

enum {
	CFGKEY_LIST_ALL_GAMES = 275, CFGKEY_BIOS_TYPE = 276,
	CFGKEY_MVS_COUNTRY = 277, CFGKEY_TIMER_INT = 278,
	CFGKEY_CREATE_USE_CACHE = 279,
	CFGKEY_NEOGEOKEY_TEST_SWITCH = 280, CFGKEY_STRICT_ROM_CHECKING = 281
};

static Byte1Option optionListAllGames(CFGKEY_LIST_ALL_GAMES, 0);
static Byte1Option optionBIOSType(CFGKEY_BIOS_TYPE, SYS_UNIBIOS);
static Byte1Option optionMVSCountry(CFGKEY_MVS_COUNTRY, CTY_USA);
static Byte1Option optionTimerInt(CFGKEY_TIMER_INT, 2);
static Byte1Option optionCreateAndUseCache(CFGKEY_CREATE_USE_CACHE, 0);
static Byte1Option optionStrictROMChecking(CFGKEY_STRICT_ROM_CHECKING, 0);

static void setTimerIntOption()
{
	switch(optionTimerInt)
	{
		bcase 0: conf.raster = 0;
		bcase 1: conf.raster = 1;
		bcase 2:
			bool needsTimer = 0;
			if(EmuSystem::gameIsRunning() && (strstr(EmuSystem::fullGameName, "Sidekicks 2") || strstr(EmuSystem::fullGameName, "Sidekicks 3")
					|| strstr(EmuSystem::fullGameName, "Ultimate 11") || strstr(EmuSystem::fullGameName, "Neo-Geo Cup")
					|| strstr(EmuSystem::fullGameName, "Spin Master")))
				needsTimer = 1;
			if(needsTimer) logMsg("auto enabled timer interrupt");
			conf.raster = needsTimer;
	}
}

const uint EmuSystem::maxPlayers = 2;
uint EmuSystem::aspectRatioX = 4, EmuSystem::aspectRatioY = 3;
#include <CommonGui.hh>

CLINK int gn_strictROMChecking()
{
	return optionStrictROMChecking;
}

void EmuSystem::initOptions()
{
	optionAutoSaveState.initDefault(0);
	#ifdef CONFIG_BASE_IOS
		if(Base::runningDeviceType() != Base::DEV_TYPE_IPAD)
	#endif
	{
			if(!Config::envIsWebOS3)
				optionTouchCtrlSize.initDefault(700);
	}
	optionTouchCtrlBtnSpace.initDefault(100);
	optionTouchCtrlBtnStagger.initDefault(5);
}

bool EmuSystem::readConfig(Io *io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_LIST_ALL_GAMES: optionListAllGames.readFromIO(io, readSize);
		bcase CFGKEY_BIOS_TYPE: optionBIOSType.readFromIO(io, readSize);
		bcase CFGKEY_MVS_COUNTRY: optionMVSCountry.readFromIO(io, readSize);
		bcase CFGKEY_TIMER_INT: optionTimerInt.readFromIO(io, readSize);
		bcase CFGKEY_CREATE_USE_CACHE: optionCreateAndUseCache.readFromIO(io, readSize);
		bcase CFGKEY_STRICT_ROM_CHECKING: optionStrictROMChecking.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(Io *io)
{
	optionListAllGames.writeWithKeyIfNotDefault(io);
	optionBIOSType.writeWithKeyIfNotDefault(io);
	optionMVSCountry.writeWithKeyIfNotDefault(io);
	optionTimerInt.writeWithKeyIfNotDefault(io);
	optionCreateAndUseCache.writeWithKeyIfNotDefault(io);
	optionStrictROMChecking.writeWithKeyIfNotDefault(io);
}

static bool isNeoGeoExtension(const char *name)
{
	return string_hasDotExtension(name, "zip");
}

static int neogeoFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isNeoGeoExtension(name);
}

FsDirFilterFunc EmuFilePicker::defaultFsFilter = neogeoFsFilter;
FsDirFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = neogeoFsFilter;

static const PixelFormatDesc *pixFmt = &PixelFormatRGB565;
static uint16 screenBuff[352*256] __attribute__ ((aligned (8))) {0};
static GN_Surface sdlSurf;

namespace NGKey
{
	static const uint COIN1 = BIT(0), COIN2 = BIT(1), SERVICE = BIT(2),

	START1 = BIT(0), SELECT1 = BIT(1),
	START2 = BIT(2), SELECT2 = BIT(3),

	UP = BIT(0), DOWN = BIT(1), LEFT = BIT(2), RIGHT = BIT(3),
	A = BIT(4), B = BIT(5), C = BIT(6), D = BIT(7),

	START_EMU_INPUT = BIT(8),
	SELECT_COIN_EMU_INPUT = BIT(9),
	SERVICE_EMU_INPUT = BIT(10);
}

void updateVControllerMapping(uint player, SysVController::Map &map)
{
	using namespace NGKey;
	uint playerMask = player << 11;
	map[SysVController::F_ELEM] = A | playerMask;
	map[SysVController::F_ELEM+1] = B | playerMask;
	map[SysVController::F_ELEM+2] = C | playerMask;
	map[SysVController::F_ELEM+3] = D | playerMask;

	map[SysVController::C_ELEM] = SELECT_COIN_EMU_INPUT | playerMask;
	map[SysVController::C_ELEM+1] = START_EMU_INPUT | playerMask;

	map[SysVController::D_ELEM] = UP | LEFT | playerMask;
	map[SysVController::D_ELEM+1] = UP | playerMask;
	map[SysVController::D_ELEM+2] = UP | RIGHT | playerMask;
	map[SysVController::D_ELEM+3] = LEFT | playerMask;
	map[SysVController::D_ELEM+5] = RIGHT | playerMask;
	map[SysVController::D_ELEM+6] = DOWN | LEFT | playerMask;
	map[SysVController::D_ELEM+7] = DOWN | playerMask;
	map[SysVController::D_ELEM+8] = DOWN | RIGHT | playerMask;
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	using namespace NGKey;
	if(unlikely(input == neogeoKeyIdxTestSwitch))
	{
		return SERVICE_EMU_INPUT;
	}
	assert(input >= neogeoKeyIdxUp);
	uint player = (input - neogeoKeyIdxUp) / EmuControls::joystickKeys;
	uint playerMask = player << 11;
	input -= EmuControls::joystickKeys * player;
	switch(input)
	{
		case neogeoKeyIdxUp: return UP | playerMask;
		case neogeoKeyIdxRight: return RIGHT | playerMask;
		case neogeoKeyIdxDown: return DOWN | playerMask;
		case neogeoKeyIdxLeft: return LEFT | playerMask;
		case neogeoKeyIdxLeftUp: return LEFT | UP | playerMask;
		case neogeoKeyIdxRightUp: return RIGHT | UP | playerMask;
		case neogeoKeyIdxRightDown: return RIGHT | DOWN | playerMask;
		case neogeoKeyIdxLeftDown: return LEFT | DOWN | playerMask;
		case neogeoKeyIdxSelect: return SELECT_COIN_EMU_INPUT | playerMask;
		case neogeoKeyIdxStart: return START_EMU_INPUT | playerMask;
		case neogeoKeyIdxXTurbo: turbo = 1;
		case neogeoKeyIdxX: return C | playerMask;
		case neogeoKeyIdxYTurbo: turbo = 1;
		case neogeoKeyIdxY: return D | playerMask;
		case neogeoKeyIdxATurbo: turbo = 1;
		case neogeoKeyIdxA: return A | playerMask;
		case neogeoKeyIdxBTurbo: turbo = 1;
		case neogeoKeyIdxB: return B | playerMask;
		case neogeoKeyIdxABC: return A | B | C | playerMask;
		default: bug_branch("%d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(uint state, uint emuKey)
{
	uint player = emuKey >> 11;

	if(emuKey & 0xFF) // joystick
	{
		if(state == Input::PUSHED)
			unsetBits(player ? memory.intern_p2 : memory.intern_p1, emuKey & 0xFF);
		else
			setBits(player ? memory.intern_p2 : memory.intern_p1, emuKey & 0xFF);
		return;
	}

	if(emuKey & NGKey::SELECT_COIN_EMU_INPUT)
	{
		if(conf.system == SYS_ARCADE)
		{
			uint bits = player ? NGKey::COIN2 : NGKey::COIN1;
			if(state == Input::PUSHED)
				unsetBits(memory.intern_coin, bits);
			else
				setBits(memory.intern_coin, bits);
		}
		else
		{
			// convert COIN to SELECT
			uint bits = player ? NGKey::SELECT2 : NGKey::SELECT1;
			if(state == Input::PUSHED)
				unsetBits(memory.intern_start, bits);
			else
				setBits(memory.intern_start, bits);
		}
		return;
	}

	if(emuKey & NGKey::START_EMU_INPUT)
	{
		uint bits = player ? NGKey::START2 : NGKey::START1;
		if(state == Input::PUSHED)
			unsetBits(memory.intern_start, bits);
		else
			setBits(memory.intern_start, bits);
		return;
	}

	if(emuKey & NGKey::SERVICE_EMU_INPUT)
	{
		if(state == Input::PUSHED)
			conf.test_switch = 1; // Test Switch is reset to 0 after every frame
		return;
	}
}

static const int FBResX = 352;
static bool renderToScreen = 0;

void EmuSystem::resetGame()
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

void EmuSystem::sprintStateFilename(char *str, size_t size, int slot, const char *statePath, const char *gameName)
{
	snprintf(str, size, "%s/%s.0%c.sta", statePath, gameName, saveSlotChar(slot));
}

int EmuSystem::saveState()
{
	FsSys::cPath saveStr;
	sprintStateFilename(saveStr, saveStateSlot);
	#ifdef CONFIG_BASE_IOS_SETUID
		fixFilePermissions(saveStr);
	#endif
	if(!save_stateWithName(saveStr))
		return STATE_RESULT_IO_ERROR;
	else
		return STATE_RESULT_OK;
}

int EmuSystem::loadState(int saveStateSlot)
{
	FsSys::cPath saveStr;
	sprintStateFilename(saveStr, saveStateSlot);
	if(FsSys::fileExists(saveStr))
	{
		logMsg("loading state %s", saveStr);
		if(load_stateWithName(saveStr))
			return STATE_RESULT_OK;
		else
			return STATE_RESULT_IO_ERROR;
	}
	return STATE_RESULT_NO_FILE;
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
		FsSys::cPath saveStr;
		sprintStateFilename(saveStr, -1);
		#ifdef CONFIG_BASE_IOS_SETUID
			fixFilePermissions(saveStr);
		#endif
		if(!save_stateWithName(saveStr))
			logMsg("error saving state %s", saveStr);
	}
}

void EmuSystem::closeSystem()
{
	close_game();
}

bool EmuSystem::vidSysIsPAL() { return 0; }
bool touchControlsApplicable() { return 1; }

CLINK char romerror[1024];

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
	string_copy(EmuSystem::fullGameName, activeDrv->longname, sizeof(EmuSystem::fullGameName));
	logMsg("set long game name: %s", EmuSystem::fullGameName);
	free(activeDrv);
	activeDrv = 0;

	setTimerIntOption();

	logMsg("finished loading game");
}

static const bool backgroundRomLoading = 1;
static ThreadPThread backgroundThread;

enum { MSG_LOAD_FAILED = Base::MSG_USER, MSG_LOAD_OK, MSG_START_PROGRESS, MSG_UPDATE_PROGRESS };

ptrsize loadGameThread(ThreadPThread &thread)
{
	using namespace Base;

	char gnoFilename[8+4+1];
	snprintf(gnoFilename, sizeof(gnoFilename), "%s.gno", activeDrv->name);

	if(!init_game(activeDrv->name))
	{
		sendMessageToMain(thread, MSG_LOAD_FAILED, 0, 0, 0);
		EmuSystem::clearGamePaths();
		free(activeDrv); activeDrv = 0;
		return 0;
	}

	if(optionCreateAndUseCache && !FsSys::fileExists(gnoFilename))
	{
		logMsg("%s doesn't exist, creating", gnoFilename);
		#ifdef USE_GENERATOR68K
		bool swappedBIOS = swapCPUMemForDump();
		#endif
		dr_save_gno(&memory.rom, gnoFilename);
		#ifdef USE_GENERATOR68K
		reverseSwapCPUMemForDump(swappedBIOS);
		#endif
	}
	sendMessageToMain(thread, MSG_LOAD_OK, 0, 0, 0);
	return 0;
}

void gn_init_pbar(uint action,int size)
{
	using namespace Base;
	logMsg("init pbar %d, %d", action, size);
	if(backgroundThread.running)
	{
		sendMessageToMain(backgroundThread, MSG_START_PROGRESS, action, size, 0);
	}
}
void gn_update_pbar(int pos)
{
	using namespace Base;
	logMsg("update pbar %d", pos);
	if(backgroundThread.running)
	{
		sendMessageToMain(backgroundThread, MSG_UPDATE_PROGRESS, 0, pos, 0);
	}
}

class LoadGameInBackgroundView : public View
{
public:
	Gfx::Text text;
	Rect2<int> rect;
	Rect2<int> &viewRect() { return rect; }

	uint pos, max;
	void setMax(uint val)
	{
		max = val;
	}

	void setPos(uint val)
	{
		pos = val;
	}

	void init()
	{
		text.init("Loading...", View::defaultFace);
		pos = max = 0;
	}

	void deinit()
	{
		text.deinit();
	}

	void place(Rect2<int> rect)
	{
		View::placeRect(rect);
	}

	void place()
	{
		text.compile();
	}

	void inputEvent(const Input::Event &e) { }

	void draw(Gfx::FrameTimeBase frameTime)
	{
		using namespace Gfx;
		if(max)
		{
			resetTransforms();
			setBlendMode(0);
			setColor(.0, .0, .75);
			auto bar = proj.relRect(0, 0,
					IG::scalePointRange((GC)pos, (GC)0, (GC)max, (GC)0, proj.w), text.ySize*1.5,
					LC2DO, LC2DO);
			GeomRect::draw(bar);
		}
		setColor(COLOR_WHITE);
		text.draw(0, 0, C2DO, C2DO);
	}
};

static LoadGameInBackgroundView loadGameInBackgroundView;

int EmuSystem::loadGame(const char *path)
{
	closeGame(1);
	emuView.initImage(0, 304, 224, (FBResX-304)*2);
	setupGamePaths(path);

	{
		ROM_DEF *drv = dr_check_zip(path);
		if(!drv)
		{
			popup.postError("This game isn't recognized");
			return 0;
		}
		activeDrv = drv;
	}

	logMsg("rom set %s, %s", activeDrv->name, activeDrv->longname);
	char gnoFilename[8+4+1];
	snprintf(gnoFilename, sizeof(gnoFilename), "%s.gno", activeDrv->name);

	if(optionCreateAndUseCache && FsSys::fileExists(gnoFilename))
	{
		logMsg("loading .gno file");
		if(!init_game(gnoFilename))
		{
			popup.printf(4, 1, "%s", romerror);
			free(activeDrv); activeDrv = 0;
			return 0;
		}
	}
	else
	{
		if(backgroundRomLoading)
		{
			if(View::modalView) View::modalView->deinit();
			loadGameInBackgroundView.init();
			loadGameInBackgroundView.place(Gfx::viewportRect());
			View::modalView = &loadGameInBackgroundView;
			Base::displayNeedsUpdate();
			backgroundThread.create(1, ThreadPThread::EntryDelegate::create<&loadGameThread>());
			return -1;
		}
		else
		{
			if(!init_game(activeDrv->name))
			{
				popup.printf(4, 1, "%s", romerror);
				free(activeDrv); activeDrv = 0;
				return 0;
			}

			if(optionCreateAndUseCache && !FsSys::fileExists(gnoFilename))
			{
				logMsg("%s doesn't exist, creating", gnoFilename);
				#ifdef USE_GENERATOR68K
				bool swappedBIOS = swapCPUMemForDump();
				#endif
				dr_save_gno(&memory.rom, gnoFilename);
				#ifdef USE_GENERATOR68K
				reverseSwapCPUMemForDump(swappedBIOS);
				#endif
			}
		}
	}

	loadGamePhase2();
	return 1;
}

void EmuSystem::clearInputBuffers()
{
	memory.intern_coin = 0x7;
	memory.intern_start = 0x8F;
	memory.intern_p1 = 0xFF;
	memory.intern_p2 = 0xFF;
}

static uint audioFramesPerUpdate;

void EmuSystem::configAudioRate()
{
	pcmFormat.rate = optionSoundRate;
	conf.sample_rate = optionSoundRate;
	#ifdef CONFIG_ENV_WEBOS
	if(optionFrameSkip != optionFrameSkipAuto)
		conf.sample_rate *= 42660./44100.; // better sync with Pre's refresh rate
	#endif
	if(gameIsRunning())
	{
		logMsg("setting YM2610 rate to %d", conf.sample_rate);
		YM2610ChangeSamplerate(conf.sample_rate);
	}
	audioFramesPerUpdate = conf.sample_rate/60.;
}

CLINK void screen_update()
{
	if(likely(renderToScreen))
	{
		//logMsg("screen render");
		emuView.updateAndDrawContent();
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
		mem_setElem(screenBuff, (uint16)current_pc_pal[4095]);
	main_frame();
	YM2610Update_stream(audioFramesPerUpdate);
	if(renderAudio)
	{
		Audio::writePcm((uchar*)play_buffer, audioFramesPerUpdate);
	}
}

void EmuSystem::savePathChanged() { }

namespace Input
{

void onInputEvent(const Input::Event &e)
{
	handleInputEvent(e);
}

}

namespace Base
{

void onAppMessage(int type, int shortArg, int intArg, int intArg2)
{
	switch(type)
	{
		bcase MSG_LOAD_FAILED:
		{
			View::removeModalView();
			popup.printf(4, 1, "%s", romerror);
		}
		bcase MSG_LOAD_OK:
		{
			View::removeModalView();
			loadGamePhase2();
			EmuSystem::loadGameCompleteDelegate().invoke(1, Input::Event{});
		}
		bcase MSG_START_PROGRESS:
		{
			switch(shortArg)
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
			loadGameInBackgroundView.setMax(intArg);
			loadGameInBackgroundView.place();
			Base::displayNeedsUpdate();
		}
		bcase MSG_UPDATE_PROGRESS:
		{
			loadGameInBackgroundView.setPos(intArg);
			Base::displayNeedsUpdate();
		}
	}
}

CallResult onInit(int argc, char** argv)
{
	mainInitCommon();
	// start image on y 16, x 24, size 304x224, 48 pixel padding on the right
	emuView.initPixmap((uchar*)screenBuff + (16*FBResX*2) + (24*2), pixFmt, 304, 224, (FBResX-304)*2);
	visible_area.x = 0;//16;
	visible_area.y = 16;
	visible_area.w = 304;//320;
	visible_area.h = 224;
	sdlSurf.pitch = FBResX*2;
	sdlSurf.w = FBResX;
	sdlSurf.pixels = screenBuff;
	buffer = &sdlSurf;
	conf.sound = 1;
	conf.system = (SYSTEM)optionBIOSType.val;
	conf.country = (COUNTRY)optionMVSCountry.val;
	strcpy(rompathConfItem.data.dt_str.str, ".");
	sprintf(datafileConfItem.data.dt_str.str, Config::envIsAndroid ? "%s" : "%s/gngeo_data.zip", Base::appPath);
	// TODO: remove now that long names are correctly used
	forEachInDLList(&recentGameList, e)
	{
		ROM_DEF *drv = dr_check_zip(e.path);
		if(!drv)
			continue;
		logMsg("updating recent game name %s to %s", e.name, drv->longname);
		string_copy(e.name, drv->longname, sizeof(e.name));
		free(drv);
	}
	return OK;
}

CallResult onWindowInit()
{
	static const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, VertexColorPixelFormat.build((255./255.) * .4, (215./255.) * .4, (0./255.) * .4, 1.) },
		{ .3, VertexColorPixelFormat.build((255./255.) * .4, (215./255.) * .4, (0./255.) * .4, 1.) },
		{ .97, VertexColorPixelFormat.build((85./255.) * .4, (71./255.) * .4, (0./255.) * .4, 1.) },
		{ 1., VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};

	mainInitWindowCommon(navViewGrad);
	return OK;
}

}
