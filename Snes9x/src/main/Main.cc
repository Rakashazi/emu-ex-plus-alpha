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
#include <EmuSystem.hh>
#include <CommonFrameworkIncludes.hh>

#include <snes9x.h>
#ifdef USE_SNES9X_15X
	#include <apu/apu.h>
	#include <controls.h>
#else
	#include <apu.h>
	#include <soundux.h>
#endif
#include <memmap.h>
#include <snapshot.h>

static int snesInputPort = SNES_JOYPAD;

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
	CFGKEY_SNESKEY_UP = 256, CFGKEY_SNESKEY_RIGHT = 257,
	CFGKEY_SNESKEY_DOWN = 258, CFGKEY_SNESKEY_LEFT = 259,
	CFGKEY_SNESKEY_SELECT = 260, CFGKEY_SNESKEY_START = 261,
	CFGKEY_SNESKEY_A = 262, CFGKEY_SNESKEY_B = 263,
	CFGKEY_SNESKEY_X = 264, CFGKEY_SNESKEY_Y = 265,
	CFGKEY_SNESKEY_L = 266, CFGKEY_SNESKEY_R = 267,
	CFGKEY_SNESKEY_A_TURBO = 268, CFGKEY_SNESKEY_B_TURBO = 269,
	CFGKEY_SNESKEY_X_TURBO = 270, CFGKEY_SNESKEY_Y_TURBO = 271,
	CFGKEY_SNESKEY_LEFT_UP = 272, CFGKEY_SNESKEY_RIGHT_UP = 273,
	CFGKEY_SNESKEY_RIGHT_DOWN = 274, CFGKEY_SNESKEY_LEFT_DOWN = 275,

	CFGKEY_MULTITAP = 276,
};

static Byte1Option optionMultitap(CFGKEY_MULTITAP, 0);

const uint EmuSystem::maxPlayers = 5;
uint EmuSystem::aspectRatioX = 4, EmuSystem::aspectRatioY = 3;
#include <CommonGui.hh>

namespace EmuControls
{

KeyCategory category[categories] =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
		KeyCategory("Gamepad Controls", gamepadName, gameActionKeys),
};

}

void EmuSystem::initOptions()
{
	#ifndef CONFIG_BASE_ANDROID
	optionFrameSkip.initDefault(optionFrameSkipAuto); // auto-frameskip default due to highly variable CPU usage
	#endif
	#ifdef CONFIG_BASE_IOS
		if(Base::runningDeviceType() != Base::DEV_TYPE_IPAD)
	#endif
	{
			if(!Config::envIsWebOS3)
				optionTouchCtrlSize.initDefault(700);
	}
	optionTouchCtrlBtnSpace.initDefault(100);
	optionTouchCtrlBtnStagger.initDefault(5); // original SNES layout
}

bool EmuSystem::readConfig(Io *io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_MULTITAP: optionMultitap.readFromIO(io, readSize);
		bcase CFGKEY_SNESKEY_UP: readKeyConfig2(io, s9xKeyIdxUp, readSize);
		bcase CFGKEY_SNESKEY_RIGHT: readKeyConfig2(io, s9xKeyIdxRight, readSize);
		bcase CFGKEY_SNESKEY_DOWN: readKeyConfig2(io, s9xKeyIdxDown, readSize);
		bcase CFGKEY_SNESKEY_LEFT: readKeyConfig2(io, s9xKeyIdxLeft, readSize);
		bcase CFGKEY_SNESKEY_LEFT_UP: readKeyConfig2(io, s9xKeyIdxLeftUp, readSize);
		bcase CFGKEY_SNESKEY_RIGHT_UP: readKeyConfig2(io, s9xKeyIdxRightUp, readSize);
		bcase CFGKEY_SNESKEY_RIGHT_DOWN: readKeyConfig2(io, s9xKeyIdxRightDown, readSize);
		bcase CFGKEY_SNESKEY_LEFT_DOWN: readKeyConfig2(io, s9xKeyIdxLeftDown, readSize);
		bcase CFGKEY_SNESKEY_SELECT: readKeyConfig2(io, s9xKeyIdxSelect, readSize);
		bcase CFGKEY_SNESKEY_START: readKeyConfig2(io, s9xKeyIdxStart, readSize);
		bcase CFGKEY_SNESKEY_A: readKeyConfig2(io, s9xKeyIdxA, readSize);
		bcase CFGKEY_SNESKEY_B: readKeyConfig2(io, s9xKeyIdxB, readSize);
		bcase CFGKEY_SNESKEY_X: readKeyConfig2(io, s9xKeyIdxX, readSize);
		bcase CFGKEY_SNESKEY_Y: readKeyConfig2(io, s9xKeyIdxY, readSize);
		bcase CFGKEY_SNESKEY_L: readKeyConfig2(io, s9xKeyIdxL, readSize);
		bcase CFGKEY_SNESKEY_R: readKeyConfig2(io, s9xKeyIdxR, readSize);
		bcase CFGKEY_SNESKEY_A_TURBO: readKeyConfig2(io, s9xKeyIdxATurbo, readSize);
		bcase CFGKEY_SNESKEY_B_TURBO: readKeyConfig2(io, s9xKeyIdxBTurbo, readSize);
		bcase CFGKEY_SNESKEY_X_TURBO: readKeyConfig2(io, s9xKeyIdxXTurbo, readSize);
		bcase CFGKEY_SNESKEY_Y_TURBO: readKeyConfig2(io, s9xKeyIdxYTurbo, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(Io *io)
{
	optionMultitap.writeWithKeyIfNotDefault(io);
	writeKeyConfig2(io, s9xKeyIdxUp, CFGKEY_SNESKEY_UP);
	writeKeyConfig2(io, s9xKeyIdxRight, CFGKEY_SNESKEY_RIGHT);
	writeKeyConfig2(io, s9xKeyIdxDown, CFGKEY_SNESKEY_DOWN);
	writeKeyConfig2(io, s9xKeyIdxLeft, CFGKEY_SNESKEY_LEFT);
	writeKeyConfig2(io, s9xKeyIdxLeftUp, CFGKEY_SNESKEY_LEFT_UP);
	writeKeyConfig2(io, s9xKeyIdxRightUp, CFGKEY_SNESKEY_RIGHT_UP);
	writeKeyConfig2(io, s9xKeyIdxRightDown, CFGKEY_SNESKEY_RIGHT_DOWN);
	writeKeyConfig2(io, s9xKeyIdxLeftDown, CFGKEY_SNESKEY_LEFT_DOWN);
	writeKeyConfig2(io, s9xKeyIdxSelect, CFGKEY_SNESKEY_SELECT);
	writeKeyConfig2(io, s9xKeyIdxStart, CFGKEY_SNESKEY_START);
	writeKeyConfig2(io, s9xKeyIdxA, CFGKEY_SNESKEY_A);
	writeKeyConfig2(io, s9xKeyIdxB, CFGKEY_SNESKEY_B);
	writeKeyConfig2(io, s9xKeyIdxX, CFGKEY_SNESKEY_X);
	writeKeyConfig2(io, s9xKeyIdxY, CFGKEY_SNESKEY_Y);
	writeKeyConfig2(io, s9xKeyIdxL, CFGKEY_SNESKEY_L);
	writeKeyConfig2(io, s9xKeyIdxR, CFGKEY_SNESKEY_R);
	writeKeyConfig2(io, s9xKeyIdxATurbo, CFGKEY_SNESKEY_A_TURBO);
	writeKeyConfig2(io, s9xKeyIdxBTurbo, CFGKEY_SNESKEY_B_TURBO);
	writeKeyConfig2(io, s9xKeyIdxXTurbo, CFGKEY_SNESKEY_X_TURBO);
	writeKeyConfig2(io, s9xKeyIdxYTurbo, CFGKEY_SNESKEY_Y_TURBO);
}

static bool isROMExtension(const char *name)
{
	return string_hasDotExtension(name, "smc") ||
			string_hasDotExtension(name, "sfc") ||
			string_hasDotExtension(name, "fig") ||
			string_hasDotExtension(name, "1");
}

static bool isSNESExtension(const char *name)
{
	return isROMExtension(name) || string_hasDotExtension(name, "zip");
}

static int snesFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isSNESExtension(name);
}

FsDirFilterFunc EmuFilePicker::defaultFsFilter = snesFsFilter;
FsDirFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = snesFsFilter;

static const PixelFormatDesc *pixFmt = &PixelFormatRGB565;
//static uint16 screenBuff[512*478] __attribute__ ((aligned (8))); // moved to globals.cpp

#ifdef USE_SNES9X_15X
uint16 *S9xGetJoypadBits(uint idx);
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

static int snesPointerX = 0, snesPointerY = 0, snesPointerBtns = 0, snesMouseClick = 0;

CLINK bool8 S9xReadMousePosition(int which, int &x, int &y, uint32 &buttons)
{
    if (which == 1)
    	return 0;

    //logMsg("reading mouse %d: %d %d %d, prev %d %d", which1_0_to_1, snesPointerX, snesPointerY, snesPointerBtns, IPPU.PrevMouseX[which1_0_to_1], IPPU.PrevMouseY[which1_0_to_1]);
    x = IG::scalePointRange((float)snesPointerX, (float)emuView.gameView.iXSize, (float)256.);
    y = IG::scalePointRange((float)snesPointerY, (float)emuView.gameView.iYSize, (float)224.);
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

static uint ptrInputToSysButton(int input)
{
	switch(input)
	{
		case SysVController::F_ELEM: return SNES_A_MASK;
		case SysVController::F_ELEM+1: return SNES_B_MASK;
		case SysVController::F_ELEM+2: return SNES_X_MASK;
		case SysVController::F_ELEM+3: return SNES_Y_MASK;
		case SysVController::F_ELEM+4: return SNES_TL_MASK;
		case SysVController::F_ELEM+5: return SNES_TR_MASK;

		case SysVController::C_ELEM: return SNES_SELECT_MASK;
		case SysVController::C_ELEM+1: return SNES_START_MASK;

		case SysVController::D_ELEM: return SNES_UP_MASK | SNES_LEFT_MASK;
		case SysVController::D_ELEM+1: return SNES_UP_MASK;
		case SysVController::D_ELEM+2: return SNES_UP_MASK | SNES_RIGHT_MASK;
		case SysVController::D_ELEM+3: return SNES_LEFT_MASK;
		case SysVController::D_ELEM+5: return SNES_RIGHT_MASK;
		case SysVController::D_ELEM+6: return SNES_DOWN_MASK | SNES_LEFT_MASK;
		case SysVController::D_ELEM+7: return SNES_DOWN_MASK;
		case SysVController::D_ELEM+8: return SNES_DOWN_MASK | SNES_RIGHT_MASK;
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
	switch(input)
	{
		case s9xKeyIdxUp: return SNES_UP_MASK;
		case s9xKeyIdxRight: return SNES_RIGHT_MASK;
		case s9xKeyIdxDown: return SNES_DOWN_MASK;
		case s9xKeyIdxLeft: return SNES_LEFT_MASK;
		case s9xKeyIdxLeftUp: return SNES_LEFT_MASK | SNES_UP_MASK;
		case s9xKeyIdxRightUp: return SNES_RIGHT_MASK | SNES_UP_MASK;
		case s9xKeyIdxRightDown: return SNES_RIGHT_MASK | SNES_DOWN_MASK;
		case s9xKeyIdxLeftDown: return SNES_LEFT_MASK | SNES_DOWN_MASK;
		case s9xKeyIdxSelect: return SNES_SELECT_MASK;
		case s9xKeyIdxStart: return SNES_START_MASK;
		case s9xKeyIdxXTurbo: turbo = 1;
		case s9xKeyIdxX: return SNES_X_MASK;
		case s9xKeyIdxYTurbo: turbo = 1;
		case s9xKeyIdxY: return SNES_Y_MASK;
		case s9xKeyIdxATurbo: turbo = 1;
		case s9xKeyIdxA: return SNES_A_MASK;
		case s9xKeyIdxBTurbo: turbo = 1;
		case s9xKeyIdxB: return SNES_B_MASK;
		case s9xKeyIdxL: return SNES_TL_MASK;
		case s9xKeyIdxR: return SNES_TR_MASK;
		default: bug_branch("%d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(uint player, uint state, uint emuKey)
{
	auto padData = S9xGetJoypadBits(player);
	if(state == INPUT_PUSHED)
		setBits(*padData, emuKey);
	else
		unsetBits(*padData, emuKey);
}

static int snesResX = 256, snesResY = 224;

static bool renderToScreen = 0;

#ifdef USE_SNES9X_15X
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
				emuView.resizeImage(snesResX, snesResY);
			}
		}

		emuView.updateAndDrawContent();
		renderToScreen = 0;
	}
	else
	{
		//logMsg("skipping render");
	}

	return 1;
}

void EmuSystem::resetGame()
{
	assert(gameIsRunning());
	S9xSoftReset();
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

#ifdef USE_SNES9X_15X
	#define FREEZE_EXT "frz"
#else
	#define FREEZE_EXT "s96"
#endif

void EmuSystem::sprintStateFilename(char *str, size_t size, int slot, const char *statePath, const char *gameName)
{
	snprintf(str, size, "%s/%s.0%c." FREEZE_EXT, statePath, gameName, saveSlotChar(slot));
}

#undef FREEZE_EXT

template <size_t S>
static void sprintSRAMFilename(char (&str)[S])
{
	snprintf(str, S, "%s/%s.srm", EmuSystem::savePath(), EmuSystem::gameName);
}

int EmuSystem::saveState()
{
	FsSys::cPath saveStr;
	sprintStateFilename(saveStr, saveStateSlot);
	#ifdef CONFIG_BASE_IOS_SETUID
		fixFilePermissions(saveStr);
	#endif
	if(!S9xFreezeGame(saveStr))
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
		if(S9xUnfreezeGame(saveStr))
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
	if(gameIsRunning() && CPU.SRAMModified)
	{
		logMsg("saving backup memory");
		FsSys::cPath saveStr;
		sprintSRAMFilename(saveStr);
		#ifdef CONFIG_BASE_IOS_SETUID
			fixFilePermissions(saveStr);
		#endif
		Memory.SaveSRAM(saveStr);
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
		if(!S9xFreezeGame(saveStr))
			logMsg("error saving state %s", saveStr);
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
bool touchControlsApplicable() { return snesInputPort == SNES_JOYPAD; }

static void setupSNESInput()
{
	#ifdef USE_SNES9X_15X
	S9xSetController(0, CTL_JOYPAD, 0, 0, 0, 0);
	S9xSetController(1, CTL_JOYPAD, 1, 0, 0, 0);
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

int EmuSystem::loadGame(const char *path)
{
	closeGame();
	emuView.initImage(0, snesResX, snesResY);
	setupGamePaths(path);

	if(snesInputPort == SNES_MOUSE_SWAPPED)
	{
		Settings.Mouse = 1;
		Settings.ControllerOption = SNES_MOUSE_SWAPPED;
	}
	if(!Memory.LoadROM(fullGamePath))
	{
		logMsg("failed to load game");
		popup.postError("Error loading game");
		return 0;
	}
	setupSNESInput();

	FsSys::cPath saveStr;
	sprintSRAMFilename(saveStr);
	Memory.LoadSRAM(saveStr);

	IPPU.RenderThisFrame = TRUE;
	EmuSystem::configAudioRate();
	logMsg("finished loading game");
	return 1;
}

void EmuSystem::clearInputBuffers()
{
	iterateTimes((uint)maxPlayers, p)
	{
		*S9xGetJoypadBits(p) = 0;
	}
	snesPointerBtns = 0;
	doubleClickFrames = 0;
	mouseScroll.init(ContentDrag::XY_AXIS);
	mouseScroll.dragStartY = IG::max(1, Gfx::yMMSizeToPixel(1.));
	mouseScroll.dragStartX = IG::max(1, Gfx::xMMSizeToPixel(1.));
}

void EmuSystem::configAudioRate()
{
	pcmFormat.rate = optionSoundRate;
	Settings.SoundPlaybackRate = optionSoundRate;
	#if defined(CONFIG_ENV_WEBOS)
	if(optionFrameSkip != optionFrameSkipAuto)
		Settings.SoundPlaybackRate = (float)optionSoundRate * (42660./44100.); // better sync with Pre's refresh rate
	#endif
	S9xSetPlaybackRate(Settings.SoundPlaybackRate);
	logMsg("emu sound rate %d", Settings.SoundPlaybackRate);
}

static void doS9xAudio(bool renderAudio)
{
	#ifdef USE_SNES9X_15X
	const uint samples = S9xGetSampleCount();
	#else
	const uint samples = Settings.SoundPlaybackRate*2 / 60;
	#endif
	uint frames = samples/2;

	int16 audioMemBuff[samples];
	int16 *audioBuff = nullptr;
	#ifdef USE_NEW_AUDIO
	Audio::BufferContext *aBuff = nullptr;
	if(renderAudio)
	{
		if(!(aBuff = Audio::getPlayBuffer(frames)))
		{
			return;
		}
		audioBuff = (int16*)aBuff->data;
		assert(aBuff->frames >= frames);
	}
	else
	{
		audioBuff = audioMemBuff;
	}
	#else
	audioBuff = audioMemBuff;
	#endif


	#ifdef USE_SNES9X_15X
	if(!S9xMixSamples((uint8_t*)audioBuff, samples))
	{
		logMsg("not enough samples ready from SNES");
	}
	else
	#else
	S9xMixSamples((uint8_t*)audioBuff, samples);
	#endif

	#ifdef USE_NEW_AUDIO
	if(renderAudio)
		Audio::commitPlayBuffer(aBuff, frames);
	#else
	if(renderAudio)
		Audio::writePcm((uchar*)audioBuff, frames);
	#endif
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	if(unlikely(snesInputPort != SNES_JOYPAD))
	{
		if(doubleClickFrames)
			doubleClickFrames--;
		if(rightClickFrames)
			rightClickFrames--;
	}

	IPPU.RenderThisFrame = processGfx ? TRUE : FALSE;
	if(renderGfx)
		renderToScreen = 1;
	S9xMainLoop();
	// video rendered in S9xDeinitUpdate
	doS9xAudio(renderAudio);
}

void EmuSystem::savePathChanged() { }

namespace Input
{
void onInputEvent(const InputEvent &e)
{
	if(unlikely(EmuSystem::isActive() && e.isPointer()))
	{
		switch(snesInputPort)
		{
			bcase SNES_SUPERSCOPE:
			{
				if(e.state == INPUT_RELEASED)
				{
					snesPointerBtns = 0;
				}
				if(emuView.gameView.overlaps(e.x, e.y))
				{
					int xRel = e.x - emuView.gameView.xIPos(LT2DO), yRel = e.y - emuView.gameView.yIPos(LT2DO);
					snesPointerX = IG::scalePointRange((float)xRel, (float)emuView.gameView.iXSize, (float)256.);
					snesPointerY = IG::scalePointRange((float)yRel, (float)emuView.gameView.iYSize, (float)224.);
					//logMsg("mouse moved to @ %d,%d, on SNES %d,%d", e.x, e.y, snesPointerX, snesPointerY);
					if(e.state == INPUT_PUSHED)
					{
						snesPointerBtns = 1;
					}
				}
				else if(e.state == INPUT_PUSHED)
				{
					snesPointerBtns = 2;
				}
			}

			bcase SNES_MOUSE_SWAPPED:
			{
				auto dragState = Input::dragState(e.devId);
				static bool dragWithButton = 0; // true to start next mouse drag with a button held
				switch(mouseScroll.inputEvent(Gfx::viewportRect(), e))
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
	handleInputEvent(e);
}
}

namespace Base
{

void onAppMessage(int type, int shortArg, int intArg, int intArg2) { }

CallResult onInit()
{
	Audio::setHintPcmFramesPerWrite(950); // for PAL when supported
	//Settings.FrameTimePAL = 20000;
	//Settings.FrameTimeNTSC = 16667;
	//Settings.ForceNTSC = 1;
	//Settings.SixteenBitSound = TRUE; made const
	//Settings.Transparency = TRUE;
	//Settings.AutoSaveDelay = 30;

	#ifdef USE_SNES9X_15X
	Settings.CartAName[0] = 0;
	Settings.CartBName[0] = 0;
	Settings.WrongMovieStateProtection = TRUE;
	Settings.DumpStreamsMaxFrames = -1;
	Settings.HDMATimingHack = 100;
	Settings.BlockInvalidVRAMAccessMaster = TRUE;
	Settings.SoundInputRate = 31977;//32000;
	#else
	//Settings.ShutdownMaster = TRUE;
	//Settings.CyclesPercentage = 100; made const
	//Settings.NextAPUEnabled = 1; made const
	assert(Settings.FrameTime == Settings.FrameTimeNTSC);
	//Settings.OpenGLEnable = 1; made const
	//Settings.SixteenBit = TRUE; made const
	assert(Settings.H_Max == SNES_CYCLES_PER_SCANLINE);
	assert(Settings.HBlankStart == (256 * Settings.H_Max) / SNES_HCOUNTER_MAX);
	#endif

	#if 0
	//CPU.Flags = 0;
	GFX.Pitch = 1024;
	#ifdef USE_SNES9X_15X
	GFX.Screen = screenBuff;
	#else
	GFX.Screen = (uint8*)screenBuff;
	// made static
	/*static uint8 SubScreen[512*478*2] __attribute__ ((aligned (4))),
		ZBuffer[512*478] __attribute__ ((aligned (4))),
		SubZBuffer[512*478] __attribute__ ((aligned (4)));
	GFX.SubScreen = SubScreen;
	GFX.ZBuffer = ZBuffer;
	GFX.SubZBuffer = SubZBuffer;*/
	#endif
	#endif

	Memory.Init();
	S9xGraphicsInit();
	S9xInitAPU();
	assert(Settings.Stereo == TRUE);
	//Settings.SoundPlaybackRate = 44100; // dummy rate, really set in mainInitCommon()
	#ifdef USE_SNES9X_15X
	S9xInitSound(20, 0);
	#else
	S9xInitSound(Settings.SoundPlaybackRate, Settings.Stereo, 0);
	#endif

	mainInitCommon();
	emuView.initPixmap((uchar*)GFX.Screen, pixFmt, snesResX, snesResY);
	return OK;
}

CallResult onWindowInit()
{
	static const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, VertexColorPixelFormat.build((139./255.) * .4, (149./255.) * .4, (230./255.) * .4, 1.) },
		{ .3, VertexColorPixelFormat.build((139./255.) * .4, (149./255.) * .4, (230./255.) * .4, 1.) },
		{ .97, VertexColorPixelFormat.build((46./255.) * .4, (50./255.) * .4, (77./255.) * .4, 1.) },
		{ 1., VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};

	mainInitWindowCommon(navViewGrad);
	return OK;
}

}
