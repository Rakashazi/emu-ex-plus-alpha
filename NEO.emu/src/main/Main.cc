#define LOGTAG "main"
#include <imagine/base/Pipe.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/util/ScopeGuard.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/CommonFrameworkIncludes.hh>
#include "EmuConfig.hh"

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
		static CONF_ITEM conf {0};
		if(string_equal(name, "rompath"))
		{
			static CONF_ITEM rompath {0};
			return &rompathConfItem;
		}
		else if(string_equal(name, "dump"))
		{
			static CONF_ITEM dump {0};
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

const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2012-2014\nRobert Broglia\nwww.explusalpha.com\n\n(c) 2011 the\nGngeo Team\ncode.google.com/p/gngeo";
CLINK void main_frame();
static ROM_DEF *activeDrv{};

static Base::Pipe guiPipe;
static FS::PathString datafilePath{};

static constexpr bool backgroundRomLoading = true;
static bool loadThreadIsRunning = false;

enum { MSG_LOAD_FAILED, MSG_LOAD_OK, MSG_START_PROGRESS, MSG_UPDATE_PROGRESS };

struct GUIMessage
{
	constexpr GUIMessage() {}
	constexpr GUIMessage(uint8 type, uint8 shortArg, int intArg): intArg(intArg), shortArg(shortArg), type(type) {}
	int intArg = 0;
	uint8 shortArg = 0;
	uint8 type = 0;
};

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

static bool systemEnumIsValid(uint8 val)
{
	return val < SYS_MAX;
}

static bool countryEnumIsValid(uint8 val)
{
	return val < CTY_MAX;
}

static Byte1Option optionListAllGames(CFGKEY_LIST_ALL_GAMES, 0);
static Byte1Option optionBIOSType(CFGKEY_BIOS_TYPE, SYS_UNIBIOS, 0, systemEnumIsValid);
static Byte1Option optionMVSCountry(CFGKEY_MVS_COUNTRY, CTY_USA, 0, countryEnumIsValid);
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
			auto gameStr = EmuSystem::fullGameName().data();
			if(EmuSystem::gameIsRunning() && (strstr(gameStr, "Sidekicks 2") || strstr(gameStr, "Sidekicks 3")
					|| strstr(gameStr, "Ultimate 11") || strstr(gameStr, "Neo-Geo Cup")
					|| strstr(gameStr, "Spin Master")))
				needsTimer = 1;
			if(needsTimer) logMsg("auto enabled timer interrupt");
			conf.raster = needsTimer;
	}
}

const char *EmuSystem::inputFaceBtnName = "A/B/C/D";
const char *EmuSystem::inputCenterBtnName = "Select/Start";
const uint EmuSystem::inputFaceBtns = 4;
const uint EmuSystem::inputCenterBtns = 2;
const bool EmuSystem::inputHasTriggerBtns = false;
const bool EmuSystem::inputHasRevBtnLayout = false;
const char *EmuSystem::configFilename = "NeoEmu.config";
const uint EmuSystem::maxPlayers = 2;
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = IG::size(EmuSystem::aspectRatioInfo);
bool EmuSystem::handlesGenericIO = false; // TODO: need to re-factor GnGeo file loading code
#include <emuframework/CommonGui.hh>

const char *EmuSystem::shortSystemName()
{
	return "NeoGeo";
}

const char *EmuSystem::systemName()
{
	return "Neo Geo";
}

using namespace IG;

CLINK int gn_strictROMChecking()
{
	return optionStrictROMChecking;
}

void EmuSystem::initOptions()
{
	optionAutoSaveState.initDefault(0);
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	optionTouchCtrlSize.initDefault(700);
	optionTouchCtrlBtnSpace.initDefault(100);
	optionTouchCtrlBtnStagger.initDefault(5);
	#endif
}

void EmuSystem::onOptionsLoaded()
{
	conf.system = (SYSTEM)optionBIOSType.val;
	conf.country = (COUNTRY)optionMVSCountry.val;
	// TODO: remove now that long names are correctly used
	for(auto &e : recentGameList)
	{
		ROM_DEF *drv = dr_check_zip(e.path.data());
		if(!drv)
			continue;
		logMsg("updating recent game name %s to %s", e.name.data(), drv->longname);
		string_copy(e.name, drv->longname);
		free(drv);
	}
}

bool EmuSystem::readConfig(IO &io, uint key, uint readSize)
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

void EmuSystem::writeConfig(IO &io)
{
	optionListAllGames.writeWithKeyIfNotDefault(io);
	optionBIOSType.writeWithKeyIfNotDefault(io);
	optionMVSCountry.writeWithKeyIfNotDefault(io);
	optionTimerInt.writeWithKeyIfNotDefault(io);
	optionCreateAndUseCache.writeWithKeyIfNotDefault(io);
	optionStrictROMChecking.writeWithKeyIfNotDefault(io);
}

static bool hasNeoGeoExtension(const char *name)
{
	return false; // archives handled by EmuFramework
}

EmuNameFilterFunc EmuFilePicker::defaultFsFilter = hasNeoGeoExtension;
EmuNameFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = hasNeoGeoExtension;

static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;
static uint16 screenBuff[352*256] __attribute__ ((aligned (8))) {0};
static GN_Surface sdlSurf;

namespace NGKey
{
	static const uint COIN1 = bit(0), COIN2 = bit(1), SERVICE = bit(2),

	START1 = bit(0), SELECT1 = bit(1),
	START2 = bit(2), SELECT2 = bit(3),

	UP = bit(0), DOWN = bit(1), LEFT = bit(2), RIGHT = bit(3),
	A = bit(4), B = bit(5), C = bit(6), D = bit(7),

	START_EMU_INPUT = bit(8),
	SELECT_COIN_EMU_INPUT = bit(9),
	SERVICE_EMU_INPUT = bit(10);
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
		auto &p = player ? memory.intern_p2 : memory.intern_p1;
		p = IG::setOrClearBits(p, (Uint8)(emuKey & 0xFF), state != Input::PUSHED);
		return;
	}

	if(emuKey & NGKey::SELECT_COIN_EMU_INPUT)
	{
		if(conf.system == SYS_ARCADE)
		{
			uint bits = player ? NGKey::COIN2 : NGKey::COIN1;
			memory.intern_coin = IG::setOrClearBits(memory.intern_coin, (Uint8)bits, state != Input::PUSHED);
		}
		else
		{
			// convert COIN to SELECT
			uint bits = player ? NGKey::SELECT2 : NGKey::SELECT1;
			memory.intern_start = IG::setOrClearBits(memory.intern_start, (Uint8)bits, state != Input::PUSHED);
		}
		return;
	}

	if(emuKey & NGKey::START_EMU_INPUT)
	{
		uint bits = player ? NGKey::START2 : NGKey::START1;
		memory.intern_start = IG::setOrClearBits(memory.intern_start, (Uint8)bits, state != Input::PUSHED);
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

int EmuSystem::saveState()
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	fixFilePermissions(saveStr);
	if(!save_stateWithName(saveStr.data()))
		return STATE_RESULT_IO_ERROR;
	else
		return STATE_RESULT_OK;
}

int EmuSystem::loadState(int saveStateSlot)
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	if(FS::exists(saveStr.data()))
	{
		logMsg("loading state %s", saveStr.data());
		if(load_stateWithName(saveStr.data()))
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

bool EmuSystem::vidSysIsPAL() { return 0; }
uint EmuSystem::multiresVideoBaseX() { return 0; }
uint EmuSystem::multiresVideoBaseY() { return 0; }
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

CLINK ROM_DEF *res_load_drv(char *name)
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

CLINK void *res_load_data(char *name)
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
	Gfx::Text text;
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

	void init()
	{
		text.init("Loading...", View::defaultFace);
		pos = max = 0;
	}

	void deinit() override
	{
		text.deinit();
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
				popup.printf(4, 1, "%s", romerror);
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

	if(optionCreateAndUseCache && FS::exists(gnoFilename))
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
			if(modalViewController.hasView())
				modalViewController.pop();
			auto loadGameInBackgroundView = new LoadGameInBackgroundView{mainWin.win};
			loadGameInBackgroundView->init();
			modalViewController.pushAndShow(*loadGameInBackgroundView, {});
			guiPipe.init(
				[loadGameInBackgroundView](Base::Pipe &pipe)
				{
					return onGUIMessageHandler(pipe, *loadGameInBackgroundView);
				});
			IG::runOnThread(
				[]()
				{
					using namespace Base;
					char gnoFilename[8+4+1];
					snprintf(gnoFilename, sizeof(gnoFilename), "%s.gno", activeDrv->name);
					loadThreadIsRunning = true;
					auto loadThreadDone = IG::scopeGuard([](){ loadThreadIsRunning = false; });
					if(!init_game(activeDrv->name))
					{
						GUIMessage msg {MSG_LOAD_FAILED, 0, 0};
						guiPipe.write(&msg, sizeof(msg));
						EmuSystem::clearGamePaths();
						free(activeDrv); activeDrv = 0;
						return;
					}
					if(optionCreateAndUseCache && !FS::exists(gnoFilename))
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
					GUIMessage msg {MSG_LOAD_OK, 0, 0};
					guiPipe.write(&msg, sizeof(msg));
				});
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

			if(optionCreateAndUseCache && !FS::exists(gnoFilename))
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

int EmuSystem::loadGameFromIO(IO &io, const char *path, const char *origFilename)
{
	return 0; // TODO
}

void EmuSystem::clearInputBuffers()
{
	memory.intern_coin = 0x7;
	memory.intern_start = 0x8F;
	memory.intern_p1 = 0xFF;
	memory.intern_p2 = 0xFF;
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

void EmuSystem::savePathChanged() { }

bool EmuSystem::hasInputOptions() { return false; }

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

