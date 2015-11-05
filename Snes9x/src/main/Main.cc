#define LOGTAG "main"
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/CommonFrameworkIncludes.hh>
#include "EmuConfig.hh"
#include <imagine/mem/mem.h>

#include <snes9x.h>
#ifndef SNES9X_VERSION_1_4
#include <apu/apu.h>
#include <controls.h>
#else
#include <apu.h>
#include <soundux.h>
#endif
#include <memmap.h>
#include <snapshot.h>
#include <cheats.h>

const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2014\nRobert Broglia\nwww.explusalpha.com\n\n(c) 1996-2011 the\nSnes9x Team\nwww.snes9x.com";

#ifndef SNES9X_VERSION_1_4

static const int SNES_AUTO_INPUT = 255;
static const int SNES_JOYPAD = CTL_JOYPAD;
static const int SNES_MOUSE_SWAPPED = CTL_MOUSE;
static const int SNES_SUPERSCOPE = CTL_SUPERSCOPE;
static int snesInputPort = SNES_AUTO_INPUT;
static int snesActiveInputPort = SNES_JOYPAD;

#else

static int snesInputPort = SNES_JOYPAD;
static const int &snesActiveInputPort = snesInputPort;

#endif

// controls

enum
{
	s9xKeyIdxUp = EmuControls::systemKeyMapStart,
	s9xKeyIdxRight,
	s9xKeyIdxDown,
	s9xKeyIdxLeft,
	s9xKeyIdxLeftUp,
	s9xKeyIdxRightUp,
	s9xKeyIdxRightDown,
	s9xKeyIdxLeftDown,
	s9xKeyIdxSelect,
	s9xKeyIdxStart,
	s9xKeyIdxA,
	s9xKeyIdxB,
	s9xKeyIdxX,
	s9xKeyIdxY,
	s9xKeyIdxL,
	s9xKeyIdxR,
	s9xKeyIdxATurbo,
	s9xKeyIdxBTurbo,
	s9xKeyIdxXTurbo,
	s9xKeyIdxYTurbo
};

enum {
	CFGKEY_MULTITAP = 276, CFGKEY_BLOCK_INVALID_VRAM_ACCESS = 277
};

static Byte1Option optionMultitap(CFGKEY_MULTITAP, 0);
#ifndef SNES9X_VERSION_1_4
static Byte1Option optionBlockInvalidVRAMAccess(CFGKEY_BLOCK_INVALID_VRAM_ACCESS, 1);
#endif

#include <emuframework/CommonGui.hh>
#include <emuframework/CommonCheatGui.hh>

const char *EmuSystem::inputFaceBtnName = "A/B/X/Y";
const char *EmuSystem::inputCenterBtnName = "Select/Start";
const uint EmuSystem::inputFaceBtns = 6;
const uint EmuSystem::inputCenterBtns = 2;
const bool EmuSystem::inputHasTriggerBtns = true;
const bool EmuSystem::inputHasRevBtnLayout = false;
#ifdef SNES9X_VERSION_1_4
const char *EmuSystem::configFilename = "Snes9x.config";
#else
bool EmuSystem::hasBundledGames = true;
const char *EmuSystem::configFilename = "Snes9xP.config";
#endif
const uint EmuSystem::maxPlayers = 5;
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		{"8:7", 8, 7},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = sizeofArray(EmuSystem::aspectRatioInfo);
bool EmuSystem::hasResetModes = true;

#if defined __ANDROID__ || defined CONFIG_MACHINE_PANDORA
#define GAME_ASSET_EXT "smc"
#else
#define GAME_ASSET_EXT "zip"
#endif

const BundledGameInfo &EmuSystem::bundledGameInfo(uint idx)
{
	static const BundledGameInfo info[]
	{
		{ "Bio Worm", "Bio Worm." GAME_ASSET_EXT	}
	};

	return info[0];
}

const char *EmuSystem::shortSystemName()
{
	return "SFC-SNES";
}

const char *EmuSystem::systemName()
{
	return "Super Famicom (SNES)";
}

void EmuSystem::initOptions()
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	optionTouchCtrlSize.initDefault(700);
	optionTouchCtrlBtnSpace.initDefault(100);
	optionTouchCtrlBtnStagger.initDefault(5); // original SNES layout
	#endif
}

void EmuSystem::onOptionsLoaded()
{
	#ifndef SNES9X_VERSION_1_4
	Settings.BlockInvalidVRAMAccessMaster = optionBlockInvalidVRAMAccess;
	#endif
}

bool EmuSystem::readConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_MULTITAP: optionMultitap.readFromIO(io, readSize);
		#ifndef SNES9X_VERSION_1_4
		bcase CFGKEY_BLOCK_INVALID_VRAM_ACCESS: optionBlockInvalidVRAMAccess.readFromIO(io, readSize);
		#endif
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionMultitap.writeWithKeyIfNotDefault(io);
	#ifndef SNES9X_VERSION_1_4
	optionBlockInvalidVRAMAccess.writeWithKeyIfNotDefault(io);
	#endif
}

static bool hasSNESExtension(const char *name)
{
	return string_hasDotExtension(name, "smc") ||
			string_hasDotExtension(name, "sfc") ||
			string_hasDotExtension(name, "fig");
}

EmuNameFilterFunc EmuFilePicker::defaultFsFilter = hasSNESExtension;
EmuNameFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = hasSNESExtension;

static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;

static int snesPointerX = 0, snesPointerY = 0, snesPointerBtns = 0, snesMouseClick = 0;

CLINK bool8 S9xReadMousePosition(int which, int &x, int &y, uint32 &buttons)
{
    if (which == 1)
    	return 0;

    //logMsg("reading mouse %d: %d %d %d, prev %d %d", which1_0_to_1, snesPointerX, snesPointerY, snesPointerBtns, IPPU.PrevMouseX[which1_0_to_1], IPPU.PrevMouseY[which1_0_to_1]);
    x = IG::scalePointRange((float)snesPointerX, (float)emuVideoLayer.gameRect().xSize(), (float)256.);
    y = IG::scalePointRange((float)snesPointerY, (float)emuVideoLayer.gameRect().ySize(), (float)224.);
    buttons = snesPointerBtns;

    if(snesMouseClick)
    	snesMouseClick--;
    if(snesMouseClick == 1)
    {
    	//logDMsg("ending click");
    	snesPointerBtns = 0;
    }

    return 1;
}

CLINK bool8 S9xReadSuperScopePosition(int &x, int &y, uint32 &buttons)
{
	//logMsg("reading super scope: %d %d %d", snesPointerX, snesPointerY, snesPointerBtns);
	x = snesPointerX;
	y = snesPointerY;
	buttons = snesPointerBtns;
	return 1;
}

#ifndef SNES9X_VERSION_1_4
uint16 *S9xGetJoypadBits(uint idx);
uint8 *S9xGetMouseBits(uint idx);
uint8 *S9xGetMouseDeltaBits(uint idx);
int16 *S9xGetMousePosBits(uint idx);
int16 *S9xGetSuperscopePosBits();
uint8 *S9xGetSuperscopeBits();
#else
static uint16 joypadData[5];
static uint16 *S9xGetJoypadBits(uint idx)
{
	return &joypadData[idx];
}

CLINK uint32 S9xReadJoypad(int which)
{
	assert(which < 5);
	//logMsg("reading joypad %d", which);
	return 0x80000000 | joypadData[which];
}

bool JustifierOffscreen()
{
	return false;
}

void JustifierButtons(uint32& justifiers) { }

static bool usingMouse() { return IPPU.Controller == SNES_MOUSE_SWAPPED; }
static bool usingGun() { return IPPU.Controller == SNES_SUPERSCOPE; }

#endif

static uint doubleClickFrames, rightClickFrames;
static ContentDrag mouseScroll;

void updateVControllerMapping(uint player, SysVController::Map &map)
{
	uint playerMask = player << 29;
	map[SysVController::F_ELEM] = SNES_A_MASK | playerMask;
	map[SysVController::F_ELEM+1] = SNES_B_MASK | playerMask;
	map[SysVController::F_ELEM+2] = SNES_X_MASK | playerMask;
	map[SysVController::F_ELEM+3] = SNES_Y_MASK | playerMask;
	map[SysVController::F_ELEM+4] = SNES_TL_MASK | playerMask;
	map[SysVController::F_ELEM+5] = SNES_TR_MASK | playerMask;

	map[SysVController::C_ELEM] = SNES_SELECT_MASK | playerMask;
	map[SysVController::C_ELEM+1] = SNES_START_MASK | playerMask;

	map[SysVController::D_ELEM] = SNES_UP_MASK | SNES_LEFT_MASK | playerMask;
	map[SysVController::D_ELEM+1] = SNES_UP_MASK | playerMask;
	map[SysVController::D_ELEM+2] = SNES_UP_MASK | SNES_RIGHT_MASK | playerMask;
	map[SysVController::D_ELEM+3] = SNES_LEFT_MASK | playerMask;
	map[SysVController::D_ELEM+5] = SNES_RIGHT_MASK | playerMask;
	map[SysVController::D_ELEM+6] = SNES_DOWN_MASK | SNES_LEFT_MASK | playerMask;
	map[SysVController::D_ELEM+7] = SNES_DOWN_MASK | playerMask;
	map[SysVController::D_ELEM+8] = SNES_DOWN_MASK | SNES_RIGHT_MASK | playerMask;
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	assert(input >= s9xKeyIdxUp);
	uint player = (input - s9xKeyIdxUp) / EmuControls::gamepadKeys;
	uint playerMask = player << 29;
	input -= EmuControls::gamepadKeys * player;
	switch(input)
	{
		case s9xKeyIdxUp: return SNES_UP_MASK | playerMask;
		case s9xKeyIdxRight: return SNES_RIGHT_MASK | playerMask;
		case s9xKeyIdxDown: return SNES_DOWN_MASK | playerMask;
		case s9xKeyIdxLeft: return SNES_LEFT_MASK | playerMask;
		case s9xKeyIdxLeftUp: return SNES_LEFT_MASK | SNES_UP_MASK | playerMask;
		case s9xKeyIdxRightUp: return SNES_RIGHT_MASK | SNES_UP_MASK | playerMask;
		case s9xKeyIdxRightDown: return SNES_RIGHT_MASK | SNES_DOWN_MASK | playerMask;
		case s9xKeyIdxLeftDown: return SNES_LEFT_MASK | SNES_DOWN_MASK | playerMask;
		case s9xKeyIdxSelect: return SNES_SELECT_MASK | playerMask;
		case s9xKeyIdxStart: return SNES_START_MASK | playerMask;
		case s9xKeyIdxXTurbo: turbo = 1;
		case s9xKeyIdxX: return SNES_X_MASK | playerMask;
		case s9xKeyIdxYTurbo: turbo = 1;
		case s9xKeyIdxY: return SNES_Y_MASK | playerMask;
		case s9xKeyIdxATurbo: turbo = 1;
		case s9xKeyIdxA: return SNES_A_MASK | playerMask;
		case s9xKeyIdxBTurbo: turbo = 1;
		case s9xKeyIdxB: return SNES_B_MASK | playerMask;
		case s9xKeyIdxL: return SNES_TL_MASK | playerMask;
		case s9xKeyIdxR: return SNES_TR_MASK | playerMask;
		default: bug_branch("%d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(uint state, uint emuKey)
{
	uint player = emuKey >> 29; // player is encoded in upper 3 bits of input code
	assert(player < maxPlayers);
	auto padData = S9xGetJoypadBits(player);
	if(state == Input::PUSHED)
		setBits(*padData, emuKey & 0xFFFF);
	else
		unsetBits(*padData, emuKey & 0xFFFF);
}

static int snesResX = 256, snesResY = 224;

static bool renderToScreen = 0;

#ifndef SNES9X_VERSION_1_4
bool8 S9xDeinitUpdate (int width, int height)
#else
bool8 S9xDeinitUpdate(int width, int height, bool8)
#endif
{
	if(likely(renderToScreen))
	{
		if(unlikely(snesResX != width || snesResY != height))
		{
			if(snesResX == width && snesResY < 240 && height < 240)
			{
				//logMsg("ignoring vertical screen res change for now");
			}
			else
			{
				logMsg("resolution changed to %d,%d", width, height);
				snesResX = width;
				snesResY = height;
				emuVideo.resizeImage(snesResX, snesResY);
			}
		}

		updateAndDrawEmuVideo();
		renderToScreen = 0;
	}
	else
	{
		//logMsg("skipping render");
	}

	return 1;
}

void EmuSystem::reset(ResetMode mode)
{
	assert(gameIsRunning());
	if(mode == RESET_HARD)
	{
		S9xReset();
	}
	else
	{
		S9xSoftReset();
	}
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

#ifndef SNES9X_VERSION_1_4
	#define FREEZE_EXT "frz"
#else
	#define FREEZE_EXT "s96"
#endif

FS::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return FS::makePathStringPrintf("%s/%s.0%c." FREEZE_EXT, statePath, gameName, saveSlotChar(slot));
}

#undef FREEZE_EXT

static FS::PathString sprintSRAMFilename()
{
	return FS::makePathStringPrintf("%s/%s.srm", EmuSystem::savePath(), EmuSystem::gameName().data());
}

static FS::PathString sprintCheatsFilename()
{
	return FS::makePathStringPrintf("%s/%s.cht", EmuSystem::savePath(), EmuSystem::gameName().data());
}

int EmuSystem::saveState()
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	fixFilePermissions(saveStr);
	if(!S9xFreezeGame(saveStr.data()))
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
		if(S9xUnfreezeGame(saveStr.data()))
		{
			IPPU.RenderThisFrame = TRUE;
			return STATE_RESULT_OK;
		}
		else
			return STATE_RESULT_IO_ERROR;
	}
	return STATE_RESULT_NO_FILE;
}

void EmuSystem::saveBackupMem() // for manually saving when not closing game
{
	if(gameIsRunning())
	{
		if(Memory.SRAMSize)
		{
			logMsg("saving backup memory");
			auto saveStr = sprintSRAMFilename();
			fixFilePermissions(saveStr);
			Memory.SaveSRAM(saveStr.data());
		}
		auto cheatsStr = sprintCheatsFilename();
		if(!Cheat.num_cheats)
			logMsg("no cheats present, removing .cht file if present");
		else
			logMsg("saving %d cheat(s)", Cheat.num_cheats);
		S9xSaveCheatFile(cheatsStr.data());
	}
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		auto saveStr = sprintStateFilename(-1);
		fixFilePermissions(saveStr);
		if(!S9xFreezeGame(saveStr.data()))
			logMsg("error saving state %s", saveStr.data());
	}
}

void S9xAutoSaveSRAM (void)
{
	EmuSystem::saveBackupMem();
}

void EmuSystem::closeSystem()
{
	saveBackupMem();
}

bool EmuSystem::vidSysIsPAL() { return 0; }
uint EmuSystem::multiresVideoBaseX() { return 256; }
uint EmuSystem::multiresVideoBaseY() { return 239; }
bool touchControlsApplicable() { return snesActiveInputPort == SNES_JOYPAD; }

static void setupSNESInput()
{
	#ifndef SNES9X_VERSION_1_4
	int inputSetup = snesInputPort;
	if(inputSetup == SNES_AUTO_INPUT)
	{
		inputSetup = SNES_JOYPAD;
		if(EmuSystem::gameIsRunning() && !strncmp((const char *) Memory.NSRTHeader + 24, "NSRT", 4))
		{
			switch (Memory.NSRTHeader[29])
			{
				case 0x00:	// Everything goes
				break;

				case 0x10:	// Mouse in Port 0
				inputSetup = SNES_MOUSE_SWAPPED;
				break;

				case 0x01:	// Mouse in Port 1
				inputSetup = SNES_MOUSE_SWAPPED;
				break;

				case 0x03:	// Super Scope in Port 1
				inputSetup = SNES_SUPERSCOPE;
				break;

				case 0x06:	// Multitap in Port 1
				//S9xSetController(1, CTL_MP5,        1, 2, 3, 4);
				break;

				case 0x66:	// Multitap in Ports 0 and 1
				//S9xSetController(0, CTL_MP5,        0, 1, 2, 3);
				//S9xSetController(1, CTL_MP5,        4, 5, 6, 7);
				break;

				case 0x08:	// Multitap in Port 1, Mouse in new Port 1
				inputSetup = SNES_MOUSE_SWAPPED;
				break;

				case 0x04:	// Pad or Super Scope in Port 1
				inputSetup = SNES_SUPERSCOPE;
				break;

				case 0x05:	// Justifier - Must ask user...
				//S9xSetController(1, CTL_JUSTIFIER,  1, 0, 0, 0);
				break;

				case 0x20:	// Pad or Mouse in Port 0
				inputSetup = SNES_MOUSE_SWAPPED;
				break;

				case 0x22:	// Pad or Mouse in Port 0 & 1
				inputSetup = SNES_MOUSE_SWAPPED;
				break;

				case 0x24:	// Pad or Mouse in Port 0, Pad or Super Scope in Port 1
				// There should be a toggles here for what to put in, I'm leaving it at gamepad for now
				break;

				case 0x27:	// Pad or Mouse in Port 0, Pad or Mouse or Super Scope in Port 1
				// There should be a toggles here for what to put in, I'm leaving it at gamepad for now
				break;

				// Not Supported yet
				case 0x99:	// Lasabirdie
				break;

				case 0x0A:	// Barcode Battler
				break;
			}
		}
		if(inputSetup != SNES_JOYPAD)
			logMsg("using automatic input %d", inputSetup);
	}

	if(inputSetup == SNES_MOUSE_SWAPPED)
	{
		S9xSetController(0, CTL_MOUSE, 0, 0, 0, 0);
		S9xSetController(1, CTL_JOYPAD, 1, 0, 0, 0);
		logMsg("setting mouse input");
	}
	else if(inputSetup == SNES_SUPERSCOPE)
	{
		S9xSetController(0, CTL_JOYPAD, 0, 0, 0, 0);
		S9xSetController(1, CTL_SUPERSCOPE, 0, 0, 0, 0);
		logMsg("setting superscope input");
	}
	else // Joypad
	{
		if(optionMultitap)
		{
			S9xSetController(0, CTL_JOYPAD, 0, 0, 0, 0);
			S9xSetController(1, CTL_MP5, 1, 2, 3, 4);
			logMsg("setting 5-player joypad input");
		}
		else
		{
			S9xSetController(0, CTL_JOYPAD, 0, 0, 0, 0);
			S9xSetController(1, CTL_JOYPAD, 1, 0, 0, 0);
		}
	}
	snesActiveInputPort = inputSetup;
	#else
	Settings.MultiPlayer5Master = Settings.MultiPlayer5 = 0;
	Settings.MouseMaster = Settings.Mouse = 0;
	Settings.SuperScopeMaster = Settings.SuperScope = 0;
	Settings.Justifier = Settings.SecondJustifier = 0;
	if(snesInputPort == SNES_JOYPAD && optionMultitap)
	{
		logMsg("connected multitap");
		Settings.MultiPlayer5Master = Settings.MultiPlayer5 = 1;
		Settings.ControllerOption = IPPU.Controller = SNES_MULTIPLAYER5;
	}
	else
	{
		if(snesInputPort == SNES_MOUSE_SWAPPED)
		{
			logMsg("connected mouse");
			Settings.MouseMaster = Settings.Mouse = 1;
			Settings.ControllerOption = IPPU.Controller = SNES_MOUSE_SWAPPED;
		}
		else if(snesInputPort == SNES_SUPERSCOPE)
		{
			logMsg("connected superscope");
			Settings.SuperScopeMaster = Settings.SuperScope = 1;
			Settings.ControllerOption = IPPU.Controller = SNES_SUPERSCOPE;
		}
		else
		{
			logMsg("connected joypads");
			IPPU.Controller = SNES_JOYPAD;
		}
	}
	#endif
}

static int loadGameCommon()
{
	emuVideo.initImage(0, snesResX, snesResY);
	setupSNESInput();

	auto saveStr = sprintSRAMFilename();
	Memory.LoadSRAM(saveStr.data());

	IPPU.RenderThisFrame = TRUE;
	EmuSystem::configAudioPlayback();
	logMsg("finished loading game");
	return 1;
}

int EmuSystem::loadGame(const char *path)
{
	bug_exit("should only use loadGameFromIO()");
	return 0;
}

int EmuSystem::loadGameFromIO(IO &io, const char *path, const char *origFilename)
{
	closeGame();
	setupGamePaths(path);
	auto size = io.size();
	if(size > CMemory::MAX_ROM_SIZE)
	{
		//popup.postError("ROM size is too big");
    return 0;
	}
	#ifndef SNES9X_VERSION_1_4
	IG::fillData(Memory.NSRTHeader);
	#endif
	Memory.HeaderCount = 0;
	string_copy(Memory.ROMFilename, path);
	bool success;
	if(io.mmapConst())
	{
		success = Memory.LoadROMMem((const uint8*)io.mmapConst(), size);
	}
	else
	{
		auto dataPtr = (uint8*)mem_alloc(size);
		if(!io.read(dataPtr, size))
		{
			mem_free(dataPtr);
			popup.postError("IO Error loading game");
			return 0;
		}
		success = Memory.LoadROMMem(dataPtr, size);
		mem_free(dataPtr);
	}
	if(!success)
	{
		popup.postError("Error loading game");
		return 0;
	}
	return loadGameCommon();
}

void EmuSystem::clearInputBuffers()
{
	iterateTimes((uint)maxPlayers, p)
	{
		*S9xGetJoypadBits(p) = 0;
	}
	snesPointerBtns = 0;
	doubleClickFrames = 0;
	mouseScroll = ContentDrag{ContentDrag::XY_AXIS};
	mouseScroll.dragStartY = std::max(1, mainWin.win.heightMMInPixels(1.));
	mouseScroll.dragStartX = std::max(1, mainWin.win.widthMMInPixels(1.));
}

void EmuSystem::configAudioRate(double frameTime)
{
	pcmFormat.rate = optionSoundRate;
	double systemFrameRate = 59.97;
	Settings.SoundPlaybackRate = std::round(optionSoundRate * (systemFrameRate * frameTime));
	#ifndef SNES9X_VERSION_1_4
	S9xUpdatePlaybackRate();
	#else
	S9xSetPlaybackRate(Settings.SoundPlaybackRate);
	#endif
	logMsg("emu sound rate %d", Settings.SoundPlaybackRate);
}

static void mixSamples(int frames, bool renderAudio)
{
	if(likely(frames))
	{
		uint samples = frames * 2;
		int16 audioBuff[samples];
		S9xMixSamples((uint8_t*)audioBuff, samples);
		if(renderAudio)
		{
			//logMsg("%d frames", frames);
			EmuSystem::writeSound(audioBuff, frames);
		}
	}
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	if(unlikely(snesActiveInputPort != SNES_JOYPAD))
	{
		if(doubleClickFrames)
			doubleClickFrames--;
		if(rightClickFrames)
			rightClickFrames--;

		#ifndef SNES9X_VERSION_1_4
		switch(snesActiveInputPort)
		{
			bcase SNES_MOUSE_SWAPPED:
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
		}
		#endif
	}

	IPPU.RenderThisFrame = processGfx ? TRUE : FALSE;
	if(renderGfx)
		renderToScreen = 1;
	#ifndef SNES9X_VERSION_1_4
	S9xSetSamplesAvailableCallback([](void *renderAudio)
		{
			S9xFinalizeSamples();
			int samples = S9xGetSampleCount();
			mixSamples(samples / 2, renderAudio);
		}, (void*)renderAudio);
	#endif
	S9xMainLoop();
	// video rendered in S9xDeinitUpdate
	#ifdef SNES9X_VERSION_1_4
	mixSamples(audioFramesPerVideoFrame, renderAudio);
	#endif
}

void EmuSystem::savePathChanged() { }

bool EmuSystem::hasInputOptions() { return true; }

void EmuSystem::onCustomizeNavView(EmuNavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build((139./255.) * .4, (149./255.) * .4, (230./255.) * .4, 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build((139./255.) * .4, (149./255.) * .4, (230./255.) * .4, 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build((46./255.) * .4, (50./255.) * .4, (77./255.) * .4, 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};
	view.setBackgroundGradient(navViewGrad);
}

void EmuSystem::onMainWindowCreated(Base::Window &win)
{
	win.setOnInputEvent(
		[](Base::Window &win, Input::Event e)
		{
			using namespace Input;
			if(unlikely(EmuSystem::isActive() && e.isPointer()))
			{
				switch(snesActiveInputPort)
				{
					bcase SNES_SUPERSCOPE:
					{
						if(e.state == RELEASED)
						{
							snesPointerBtns = 0;
							#ifndef SNES9X_VERSION_1_4
							*S9xGetSuperscopeBits() = 0;
							#endif
						}
						if(emuVideoLayer.gameRect().overlaps({e.x, e.y}))
						{
							int xRel = e.x - emuVideoLayer.gameRect().x, yRel = e.y - emuVideoLayer.gameRect().y;
							snesPointerX = IG::scalePointRange((float)xRel, (float)emuVideoLayer.gameRect().xSize(), (float)256.);
							snesPointerY = IG::scalePointRange((float)yRel, (float)emuVideoLayer.gameRect().ySize(), (float)224.);
							//logMsg("mouse moved to @ %d,%d, on SNES %d,%d", e.x, e.y, snesPointerX, snesPointerY);
							if(e.state == PUSHED)
							{
								snesPointerBtns = 1;
								#ifndef SNES9X_VERSION_1_4
								*S9xGetSuperscopeBits() = 0x80;
								#endif
							}
						}
						else if(e.state == PUSHED)
						{
							snesPointerBtns = 2;
							#ifndef SNES9X_VERSION_1_4
							*S9xGetSuperscopeBits() = 0x40;
							#endif
						}

						#ifndef SNES9X_VERSION_1_4
						S9xGetSuperscopePosBits()[0] = snesPointerX;
						S9xGetSuperscopePosBits()[1] = snesPointerY;
						#endif
					}

					bcase SNES_MOUSE_SWAPPED:
					{
						auto dragState = Input::dragState(e.devId);
						static bool dragWithButton = 0; // true to start next mouse drag with a button held
						switch(mouseScroll.inputEvent(win.contentBounds(), e))
						{
							bcase ContentDrag::PUSHED:
							{
								rightClickFrames = 15;
								if(doubleClickFrames) // check if in double-click time window
								{
									dragWithButton = 1;
								}
								else
								{
									dragWithButton = 0;
									doubleClickFrames = 15;
								}
							}

							bcase ContentDrag::ENTERED_ACTIVE:
							{
								if(dragWithButton)
								{
									snesMouseClick = 0;
									if(!rightClickFrames)
									{
										// in right-click time window
										snesPointerBtns = 2;
										logMsg("started drag with right-button");
									}
									else
									{
										snesPointerBtns = 1;
										logMsg("started drag with left-button");
									}
								}
								else
								{
									logMsg("started drag");
								}
							}

							bcase ContentDrag::LEFT_ACTIVE:
							{
								logMsg("stopped drag");
								snesPointerBtns = 0;
							}

							bcase ContentDrag::ACTIVE:
							{
								snesPointerX += dragState->relX();
								snesPointerY += dragState->relY();
							}

							bcase ContentDrag::RELEASED:
							{
								if(!rightClickFrames)
								{
									logMsg("right clicking mouse");
									snesPointerBtns = 2;
									doubleClickFrames = 15; // allow extra time for a right-click & drag
								}
								else
								{
									logMsg("left clicking mouse");
									snesPointerBtns = 1;
								}
								snesMouseClick = 3;
							}

							bdefault: break;
						}
					}
				}
			}
			handleInputEvent(win, e);
		});
}

CallResult EmuSystem::onInit()
{
	static uint16 screenBuff[512*478] __attribute__ ((aligned (8)));
	#ifndef SNES9X_VERSION_1_4
	GFX.Screen = screenBuff;
	#else
	GFX.Screen = (uint8*)screenBuff;
	#endif

	Memory.Init();
	S9xGraphicsInit();
	S9xInitAPU();
	assert(Settings.Stereo == TRUE);
	#ifndef SNES9X_VERSION_1_4
	S9xInitSound(20, 0);
	S9xUnmapAllControls();
	#else
	S9xInitSound(Settings.SoundPlaybackRate, Settings.Stereo, 0);
	assert(Settings.FrameTime == Settings.FrameTimeNTSC);
	assert(Settings.H_Max == SNES_CYCLES_PER_SCANLINE);
	assert(Settings.HBlankStart == (256 * Settings.H_Max) / SNES_HCOUNTER_MAX);
	#endif

	emuVideo.initPixmap((char*)GFX.Screen, pixFmt, snesResX, snesResY);
	return OK;
}
