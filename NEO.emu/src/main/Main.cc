#define thisModuleName "main"
#include <resource2/image/png/ResourceImagePng.h>
#include <logger/interface.h>
#include <util/area2.h>
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

	char *get_gngeo_dir(void)
	{
		static char path[] = "./";
		return path;
	}
}

CLINK void main_frame();
static ROM_DEF *activeDrv = 0;

#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	#define CONFIG_FILE_NAME "NeoEmu.config"
#else
	#define CONFIG_FILE_NAME "config"
#endif

static const GfxLGradientStopDesc navViewGrad[] =
{
	{ .0, VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	{ .03, VertexColorPixelFormat.build((255./255.) * .4, (215./255.) * .4, (0./255.) * .4, 1.) },
	{ .3, VertexColorPixelFormat.build((255./255.) * .4, (215./255.) * .4, (0./255.) * .4, 1.) },
	{ .97, VertexColorPixelFormat.build((85./255.) * .4, (71./255.) * .4, (0./255.) * .4, 1.) },
	{ 1., VertexColorPixelFormat.build(.5, .5, .5, 1.) },
};

static const char *touchConfigFaceBtnName = "A/B/C/D", *touchConfigCenterBtnName = "Select/Start";
static const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011\nRobert Broglia\nwww.explusalpha.com\n\n(c) 2011 the\nGngeo Team\ncode.google.com/p/gngeo";
const uint EmuSystem::maxPlayers = 2;
static const uint systemFaceBtns = 4, systemCenterBtns = 2;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 0;
uint EmuSystem::aspectRatioX = 4, EmuSystem::aspectRatioY = 3;
#define systemAspectRatioString "4:3"
#include <CommonGui.hh>

namespace EmuControls
{

KeyCategory category[categories] =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
		KeyCategory("Gamepad Controls", gamepadName, gameActionKeys),
};

}

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
	neogeoKeyIdxTestSwitch
};

enum {
	CFGKEY_NEOGEOKEY_UP = 256, CFGKEY_NEOGEOKEY_RIGHT = 257,
	CFGKEY_NEOGEOKEY_DOWN = 258, CFGKEY_NEOGEOKEY_LEFT = 259,
	CFGKEY_NEOGEOKEY_LEFT_UP = 260, CFGKEY_NEOGEOKEY_RIGHT_UP = 261,
	CFGKEY_NEOGEOKEY_RIGHT_DOWN = 262, CFGKEY_NEOGEOKEY_LEFT_DOWN = 263,
	CFGKEY_NEOGEOKEY_SELECT = 264, CFGKEY_NEOGEOKEY_START = 265,
	CFGKEY_NEOGEOKEY_A = 266, CFGKEY_NEOGEOKEY_B = 267,
	CFGKEY_NEOGEOKEY_X = 268, CFGKEY_NEOGEOKEY_Y = 269,
	CFGKEY_NEOGEOKEY_A_TURBO = 270, CFGKEY_NEOGEOKEY_B_TURBO = 271,
	CFGKEY_NEOGEOKEY_X_TURBO = 272, CFGKEY_NEOGEOKEY_Y_TURBO = 273,
	CFGKEY_NEOGEOKEY_ABC = 274,

	CFGKEY_LIST_ALL_GAMES = 275, CFGKEY_BIOS_TYPE = 276,
	CFGKEY_MVS_COUNTRY = 277, CFGKEY_TIMER_INT = 278,
	CFGKEY_CREATE_USE_CACHE = 279,
	CFGKEY_NEOGEOKEY_TEST_SWITCH = 280, CFGKEY_STRICT_ROM_CHECKING = 281
};

static BasicByteOption optionListAllGames(CFGKEY_LIST_ALL_GAMES, 0);
static BasicByteOption optionBIOSType(CFGKEY_BIOS_TYPE, SYS_UNIBIOS);
static BasicByteOption optionMVSCountry(CFGKEY_MVS_COUNTRY, CTY_USA);
static BasicByteOption optionTimerInt(CFGKEY_TIMER_INT, 2);
static BasicByteOption optionCreateAndUseCache(CFGKEY_CREATE_USE_CACHE, 0);
static BasicByteOption optionStrictROMChecking(CFGKEY_STRICT_ROM_CHECKING, 0);

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
		bcase CFGKEY_NEOGEOKEY_UP: readKeyConfig2(io, neogeoKeyIdxUp, readSize);
		bcase CFGKEY_NEOGEOKEY_RIGHT: readKeyConfig2(io, neogeoKeyIdxRight, readSize);
		bcase CFGKEY_NEOGEOKEY_DOWN: readKeyConfig2(io, neogeoKeyIdxDown, readSize);
		bcase CFGKEY_NEOGEOKEY_LEFT: readKeyConfig2(io, neogeoKeyIdxLeft, readSize);
		bcase CFGKEY_NEOGEOKEY_LEFT_UP: readKeyConfig2(io, neogeoKeyIdxLeftUp, readSize);
		bcase CFGKEY_NEOGEOKEY_RIGHT_UP: readKeyConfig2(io, neogeoKeyIdxRightUp, readSize);
		bcase CFGKEY_NEOGEOKEY_RIGHT_DOWN: readKeyConfig2(io, neogeoKeyIdxRightDown, readSize);
		bcase CFGKEY_NEOGEOKEY_LEFT_DOWN: readKeyConfig2(io, neogeoKeyIdxLeftDown, readSize);
		bcase CFGKEY_NEOGEOKEY_SELECT: readKeyConfig2(io, neogeoKeyIdxSelect, readSize);
		bcase CFGKEY_NEOGEOKEY_START: readKeyConfig2(io, neogeoKeyIdxStart, readSize);
		bcase CFGKEY_NEOGEOKEY_A: readKeyConfig2(io, neogeoKeyIdxA, readSize);
		bcase CFGKEY_NEOGEOKEY_B: readKeyConfig2(io, neogeoKeyIdxB, readSize);
		bcase CFGKEY_NEOGEOKEY_X: readKeyConfig2(io, neogeoKeyIdxX, readSize);
		bcase CFGKEY_NEOGEOKEY_Y: readKeyConfig2(io, neogeoKeyIdxY, readSize);
		bcase CFGKEY_NEOGEOKEY_A_TURBO: readKeyConfig2(io, neogeoKeyIdxATurbo, readSize);
		bcase CFGKEY_NEOGEOKEY_B_TURBO: readKeyConfig2(io, neogeoKeyIdxBTurbo, readSize);
		bcase CFGKEY_NEOGEOKEY_X_TURBO: readKeyConfig2(io, neogeoKeyIdxXTurbo, readSize);
		bcase CFGKEY_NEOGEOKEY_Y_TURBO: readKeyConfig2(io, neogeoKeyIdxYTurbo, readSize);
		bcase CFGKEY_NEOGEOKEY_ABC: readKeyConfig2(io, neogeoKeyIdxABC, readSize);
		bcase CFGKEY_NEOGEOKEY_TEST_SWITCH: readKeyConfig2(io, neogeoKeyIdxTestSwitch, readSize);
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
	writeKeyConfig2(io, neogeoKeyIdxUp, CFGKEY_NEOGEOKEY_UP);
	writeKeyConfig2(io, neogeoKeyIdxRight, CFGKEY_NEOGEOKEY_RIGHT);
	writeKeyConfig2(io, neogeoKeyIdxDown, CFGKEY_NEOGEOKEY_DOWN);
	writeKeyConfig2(io, neogeoKeyIdxLeft, CFGKEY_NEOGEOKEY_LEFT);
	writeKeyConfig2(io, neogeoKeyIdxLeftUp, CFGKEY_NEOGEOKEY_LEFT_UP);
	writeKeyConfig2(io, neogeoKeyIdxRightUp, CFGKEY_NEOGEOKEY_RIGHT_UP);
	writeKeyConfig2(io, neogeoKeyIdxRightDown, CFGKEY_NEOGEOKEY_RIGHT_DOWN);
	writeKeyConfig2(io, neogeoKeyIdxLeftDown, CFGKEY_NEOGEOKEY_LEFT_DOWN);
	writeKeyConfig2(io, neogeoKeyIdxSelect, CFGKEY_NEOGEOKEY_SELECT);
	writeKeyConfig2(io, neogeoKeyIdxStart, CFGKEY_NEOGEOKEY_START);
	writeKeyConfig2(io, neogeoKeyIdxA, CFGKEY_NEOGEOKEY_A);
	writeKeyConfig2(io, neogeoKeyIdxB, CFGKEY_NEOGEOKEY_B);
	writeKeyConfig2(io, neogeoKeyIdxX, CFGKEY_NEOGEOKEY_X);
	writeKeyConfig2(io, neogeoKeyIdxY, CFGKEY_NEOGEOKEY_Y);
	writeKeyConfig2(io, neogeoKeyIdxATurbo, CFGKEY_NEOGEOKEY_A_TURBO);
	writeKeyConfig2(io, neogeoKeyIdxBTurbo, CFGKEY_NEOGEOKEY_B_TURBO);
	writeKeyConfig2(io, neogeoKeyIdxXTurbo, CFGKEY_NEOGEOKEY_X_TURBO);
	writeKeyConfig2(io, neogeoKeyIdxYTurbo, CFGKEY_NEOGEOKEY_Y_TURBO);
	writeKeyConfig2(io, neogeoKeyIdxABC, CFGKEY_NEOGEOKEY_ABC);
	writeKeyConfig2(io, neogeoKeyIdxTestSwitch, CFGKEY_NEOGEOKEY_TEST_SWITCH);
	if(!optionListAllGames.isDefault())
	{
		io->writeVar((uint16)optionListAllGames.ioSize());
		optionListAllGames.writeToIO(io);
	}
	if(!optionBIOSType.isDefault())
	{
		io->writeVar((uint16)optionBIOSType.ioSize());
		optionBIOSType.writeToIO(io);
	}
	if(!optionMVSCountry.isDefault())
	{
		io->writeVar((uint16)optionMVSCountry.ioSize());
		optionMVSCountry.writeToIO(io);
	}
	if(!optionTimerInt.isDefault())
	{
		io->writeVar((uint16)optionTimerInt.ioSize());
		optionTimerInt.writeToIO(io);
	}
	if(!optionCreateAndUseCache.isDefault())
	{
		io->writeVar((uint16)optionCreateAndUseCache.ioSize());
		optionCreateAndUseCache.writeToIO(io);
	}
	if(!optionStrictROMChecking.isDefault())
	{
		io->writeVar((uint16)optionStrictROMChecking.ioSize());
		optionStrictROMChecking.writeToIO(io);
	}
}

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
static uint16 screenBuff[352*256] __attribute__ ((aligned (8)));
static GN_Surface sdlSurf;

#include "NeoOptionView.hh"
static NeoOptionView oCategoryMenu;
#include "NeoMenuView.hh"
static NeoMenuView mMenu;

namespace NGKey
{
	static const uint COIN1 = BIT(0), COIN2 = BIT(1), SERVICE = BIT(2),

	START1 = BIT(0), SELECT1 = BIT(1),
	START2 = BIT(2), SELECT2 = BIT(3),

	UP = BIT(0), DOWN = BIT(1), LEFT = BIT(2), RIGHT = BIT(3),
	A = BIT(4), B = BIT(5), C = BIT(6), D = BIT(7);
}

static uint ptrInputToSysButton(int input)
{
	using namespace NGKey;
	switch(input)
	{
		case SysVController::F_ELEM: return A;
		case SysVController::F_ELEM+1: return B;
		case SysVController::F_ELEM+2: return C;
		case SysVController::F_ELEM+3: return D;

		case SysVController::C_ELEM: return COIN1 << 16;
		case SysVController::C_ELEM+1: return START1 << 24;

		case SysVController::D_ELEM: return UP | LEFT;
		case SysVController::D_ELEM+1: return UP;
		case SysVController::D_ELEM+2: return UP | RIGHT;
		case SysVController::D_ELEM+3: return LEFT;
		case SysVController::D_ELEM+5: return RIGHT;
		case SysVController::D_ELEM+6: return DOWN | LEFT;
		case SysVController::D_ELEM+7: return DOWN;
		case SysVController::D_ELEM+8: return DOWN | RIGHT;
		default: bug_branch("%d", input); return 0;
	}
}

void EmuSystem::handleOnScreenInputAction(uint state, uint vCtrlKey)
{
	handleInputAction(pointerInputPlayer, state, ptrInputToSysButton(vCtrlKey));
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	using namespace NGKey;
	switch(input)
	{
		case neogeoKeyIdxUp: return UP;
		case neogeoKeyIdxRight: return RIGHT;
		case neogeoKeyIdxDown: return DOWN;
		case neogeoKeyIdxLeft: return LEFT;
		case neogeoKeyIdxLeftUp: return LEFT | UP;
		case neogeoKeyIdxRightUp: return RIGHT | UP;
		case neogeoKeyIdxRightDown: return RIGHT | DOWN;
		case neogeoKeyIdxLeftDown: return LEFT | DOWN;
		case neogeoKeyIdxSelect: return COIN1 << 16;
		case neogeoKeyIdxStart: return START1 << 24;
		case neogeoKeyIdxTestSwitch: return SERVICE << 16;
		case neogeoKeyIdxXTurbo: turbo = 1;
		case neogeoKeyIdxX: return C;
		case neogeoKeyIdxYTurbo: turbo = 1;
		case neogeoKeyIdxY: return D;
		case neogeoKeyIdxATurbo: turbo = 1;
		case neogeoKeyIdxA: return A;
		case neogeoKeyIdxBTurbo: turbo = 1;
		case neogeoKeyIdxB: return B;
		case neogeoKeyIdxABC: return A | B | C;
		default: bug_branch("%d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(uint player, uint state, uint emuKey)
{
	if(emuKey & 0xFF) // joystick
	{
		if(state == INPUT_PUSHED)
			unsetBits(player ? memory.intern_p2 : memory.intern_p1, emuKey);
		else
			setBits(player ? memory.intern_p2 : memory.intern_p1, emuKey);
		return;
	}

	emuKey >>= 16;

	if(emuKey & 0xFF) // coin/select
	{
		if(emuKey == NGKey::SERVICE)
		{
			if(state == INPUT_PUSHED)
				conf.test_switch = 1; // Test Switch is reset to 0 after every frame
		}
		else if(conf.system == SYS_ARCADE)
		{
			if(player)
				emuKey = NGKey::COIN2;
			if(state == INPUT_PUSHED)
				unsetBits(memory.intern_coin, emuKey);
			else
				setBits(memory.intern_coin, emuKey);
		}
		else
		{
			// convert COIN to SELECT
			emuKey = player ? NGKey::SELECT2 : NGKey::SELECT1;
			if(state == INPUT_PUSHED)
				unsetBits(memory.intern_start, emuKey);
			else
				setBits(memory.intern_start, emuKey);
		}
		return;
	}

	emuKey >>= 8;
	if(emuKey & 0xFF) // start
	{
		if(player)
			emuKey = NGKey::START2;
		if(state == INPUT_PUSHED)
			unsetBits(memory.intern_start, emuKey);
		else
			setBits(memory.intern_start, emuKey);
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

void EmuSystem::sprintStateFilename(char *str, size_t size, int slot, const char *gamePath, const char *gameName)
{
	snprintf(str, size, "%s/%s.0%c.sta", gamePath, gameName, saveSlotChar(slot));
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

int EmuSystem::loadState()
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
static bool touchControlsApplicable() { return 1; }

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

static void loadGamePhase2(bool allowAutosaveState)
{
	string_copy(EmuSystem::gameName, activeDrv->name, sizeof(EmuSystem::gameName));
	string_copy(EmuSystem::fullGameName, activeDrv->longname, sizeof(EmuSystem::fullGameName));
	logMsg("set game name: %s, long: %s", EmuSystem::gameName, EmuSystem::fullGameName);
	free(activeDrv);
	activeDrv = 0;

	emuView.initImage(0, 304, 224, (FBResX-304)*2);
	setTimerIntOption();

	if(allowAutosaveState && optionAutoSaveState)
	{
		FsSys::cPath saveStr;
		EmuSystem::sprintStateFilename(saveStr, -1);
		if(FsSys::fileExists(saveStr))
		{
			logMsg("loading auto-save state");
			load_stateWithName(saveStr);
		}
	}

	logMsg("finished loading game");
}

static const bool backgroundRomLoading = 1;
static ThreadPThread backgroundThread;

enum { MSG_LOAD_FAILED = Base::MSG_USER, MSG_LOAD_OK, MSG_START_PROGRESS, MSG_UPDATE_PROGRESS };

static int loadGameThread(ThreadPThread &thread)
{
	using namespace Base;

	char gnoFilename[8+4+1];
	snprintf(gnoFilename, sizeof(gnoFilename), "%s.gno", activeDrv->name);

	if(!init_game(activeDrv->name))
	{
		sendMessageToMain(thread, MSG_LOAD_FAILED, 0, 0, 0);
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

static bool globalAllowAutosaveState = 0;

class LoadGameInBackgroundView : public View
{
public:
	GfxText text;
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
		View::place(rect);
	}

	void place()
	{
		text.compile();
	}

	void inputEvent(const InputEvent &e) { }

	void draw()
	{
		using namespace Gfx;
		if(max)
		{
			resetTransforms();
			setBlendMode(0);
			setColor(.0, .0, .75);
			Area bar;
			bar.setXSize(IG::scalePointRange((GC)pos, (GC)0, (GC)max, (GC)0, proj.w));
			bar.setYSize(text.ySize*1.5);
			bar.setPos(0, 0, LC2DO, LC2DO);
			GeomRect::draw(&bar);
		}
		setColor(COLOR_WHITE);
		text.draw(0, 0, C2DO, C2DO);
	}
};

static LoadGameInBackgroundView loadGameInBackgroundView;

int EmuSystem::loadGame(const char *path, bool allowAutosaveState)
{
	closeGame(allowAutosaveState);

	string_copy(gamePath, FsSys::workDir(), sizeof(gamePath));
	#ifdef CONFIG_BASE_IOS_SETUID
		fixFilePermissions(gamePath);
	#endif
	snprintf(fullGamePath, sizeof(fullGamePath), "%s/%s", gamePath, path);
	logMsg("full game path: %s", fullGamePath);

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
			if(modalView) modalView->deinit();
			loadGameInBackgroundView.init();
			loadGameInBackgroundView.place(Gfx::viewportRect());
			modalView = &loadGameInBackgroundView;
			Base::displayNeedsUpdate();
			globalAllowAutosaveState = allowAutosaveState;
			backgroundThread.create(1, loadGameThread, 0);
			return 0;
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

	loadGamePhase2(allowAutosaveState);
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

namespace Input
{

void onInputEvent(const InputEvent &e)
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
			removeModalView();
			popup.printf(4, 1, "%s", romerror);
		}
		bcase MSG_LOAD_OK:
		{
			removeModalView();
			loadGamePhase2(globalAllowAutosaveState);
			EmuSystem::loadGameCompleteDelegate().invoke(1);
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

CallResult onInit()
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

	mMenu.init(Config::envIsPS3);
	viewStack.push(&mMenu);
	Gfx::onViewChange();
	mMenu.show();

	Base::displayNeedsUpdate();
	return(OK);
}

}

#undef thisModuleName
