/*  This file is part of GBA.emu.

	PCE.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	PCE.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with PCE.emu.  If not, see <http://www.gnu.org/licenses/> */

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

#include <vbam/gba/GBA.h>
#include <vbam/gba/Sound.h>
#include <vbam/common/SoundDriver.h>
#include <vbam/Util.h>
void setGameSpecificSettings();
void CPULoop(bool renderGfx, bool processGfx, bool renderAudio);
void CPUReset();
void CPUCleanUp();
bool CPUReadBatteryFile(const char *);
bool CPUWriteBatteryFile(const char *);
bool CPUReadState(const char *);
bool CPUWriteState(const char *);


#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	#define CONFIG_FILE_NAME "GbaEmu.config"
#else
	#define CONFIG_FILE_NAME "config"
#endif

const uint EmuSystem::maxPlayers = 1;
uint EmuSystem::aspectRatioX = 3, EmuSystem::aspectRatioY = 2;
#include "CommonGui.hh"

// controls

namespace EmuControls
{

KeyCategory category[categories] =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
		KeyCategory("Gamepad Controls", gamepadName, gameActionKeys),
};

}

enum
{
	gbaKeyIdxUp = EmuControls::systemKeyMapStart,
	gbaKeyIdxRight,
	gbaKeyIdxDown,
	gbaKeyIdxLeft,
	gbaKeyIdxLeftUp,
	gbaKeyIdxRightUp,
	gbaKeyIdxRightDown,
	gbaKeyIdxLeftDown,
	gbaKeyIdxSelect,
	gbaKeyIdxStart,
	gbaKeyIdxA,
	gbaKeyIdxB,
	gbaKeyIdxL,
	gbaKeyIdxR,
	gbaKeyIdxATurbo,
	gbaKeyIdxBTurbo,
	gbaKeyIdxAB,
	gbaKeyIdxRB,
};

namespace GbaKeyStatus
{
	static const uint A = BIT(0), B = BIT(1),
			SELECT = BIT(2), START = BIT(3),
			RIGHT = BIT(4), LEFT = BIT(5), UP = BIT(6), DOWN = BIT(7),
			R = BIT(8), L = BIT(9);
}

static uint ptrInputToSysButton(int input)
{
	using namespace GbaKeyStatus;
	switch(input)
	{
		case SysVController::F_ELEM: return A;
		case SysVController::F_ELEM+1: return B;
		case SysVController::F_ELEM+2: return L;
		case SysVController::F_ELEM+3: return R;

		case SysVController::C_ELEM: return SELECT;
		case SysVController::C_ELEM+1: return START;

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
	using namespace GbaKeyStatus;
	turbo = 0;
	switch(input)
	{
		case gbaKeyIdxUp: return UP;
		case gbaKeyIdxRight: return RIGHT;
		case gbaKeyIdxDown: return DOWN;
		case gbaKeyIdxLeft: return LEFT;
		case gbaKeyIdxLeftUp: return UP | LEFT;
		case gbaKeyIdxRightUp: return UP | RIGHT;
		case gbaKeyIdxRightDown: return DOWN | RIGHT;
		case gbaKeyIdxLeftDown: return DOWN | LEFT;
		case gbaKeyIdxSelect: return SELECT;
		case gbaKeyIdxStart: return START;
		case gbaKeyIdxATurbo: turbo = 1;
		case gbaKeyIdxA: return A;
		case gbaKeyIdxBTurbo: turbo = 1;
		case gbaKeyIdxB: return B;
		case gbaKeyIdxL: return L;
		case gbaKeyIdxR: return R;
		case gbaKeyIdxAB: return A | B;
		case gbaKeyIdxRB: return R | B;
		default: bug_branch("%d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(uint player, uint state, uint emuKey)
{
	if(state == INPUT_PUSHED)
		unsetBits(P1, emuKey);
	else
		setBits(P1, emuKey);
}

enum
{
	CFGKEY_GBAKEY_UP = 256, CFGKEY_GBAKEY_RIGHT = 257,
	CFGKEY_GBAKEY_DOWN = 258, CFGKEY_GBAKEY_LEFT = 259,
	CFGKEY_GBAKEY_LEFT_UP = 260, CFGKEY_GBAKEY_RIGHT_UP = 261,
	CFGKEY_GBAKEY_RIGHT_DOWN = 262, CFGKEY_GBAKEY_LEFT_DOWN = 263,
	CFGKEY_GBAKEY_SELECT = 264, CFGKEY_GBAKEY_START = 265,
	CFGKEY_GBAKEY_A = 266, CFGKEY_GBAKEY_B = 267,
	CFGKEY_GBAKEY_A_TURBO = 268, CFGKEY_GBAKEY_B_TURBO = 269,
	CFGKEY_GBAKEY_L = 270, CFGKEY_GBAKEY_R = 271,
	CFGKEY_GBAKEY_AB = 272, CFGKEY_GBAKEY_RB = 273,
};

bool EmuSystem::readConfig(Io *io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_GBAKEY_UP: readKeyConfig2(io, gbaKeyIdxUp, readSize);
		bcase CFGKEY_GBAKEY_RIGHT: readKeyConfig2(io, gbaKeyIdxRight, readSize);
		bcase CFGKEY_GBAKEY_DOWN: readKeyConfig2(io, gbaKeyIdxDown, readSize);
		bcase CFGKEY_GBAKEY_LEFT: readKeyConfig2(io, gbaKeyIdxLeft, readSize);
		bcase CFGKEY_GBAKEY_LEFT_UP: readKeyConfig2(io, gbaKeyIdxLeftUp, readSize);
		bcase CFGKEY_GBAKEY_RIGHT_UP: readKeyConfig2(io, gbaKeyIdxRightUp, readSize);
		bcase CFGKEY_GBAKEY_RIGHT_DOWN: readKeyConfig2(io, gbaKeyIdxRightDown, readSize);
		bcase CFGKEY_GBAKEY_LEFT_DOWN: readKeyConfig2(io, gbaKeyIdxLeftDown, readSize);
		bcase CFGKEY_GBAKEY_SELECT: readKeyConfig2(io, gbaKeyIdxSelect, readSize);
		bcase CFGKEY_GBAKEY_START: readKeyConfig2(io, gbaKeyIdxStart, readSize);
		bcase CFGKEY_GBAKEY_A: readKeyConfig2(io, gbaKeyIdxA, readSize);
		bcase CFGKEY_GBAKEY_B: readKeyConfig2(io, gbaKeyIdxB, readSize);
		bcase CFGKEY_GBAKEY_A_TURBO: readKeyConfig2(io, gbaKeyIdxATurbo, readSize);
		bcase CFGKEY_GBAKEY_B_TURBO: readKeyConfig2(io, gbaKeyIdxBTurbo, readSize);
		bcase CFGKEY_GBAKEY_L: readKeyConfig2(io, gbaKeyIdxL, readSize);
		bcase CFGKEY_GBAKEY_R: readKeyConfig2(io, gbaKeyIdxR, readSize);
		bcase CFGKEY_GBAKEY_AB: readKeyConfig2(io, gbaKeyIdxAB, readSize);
		bcase CFGKEY_GBAKEY_RB: readKeyConfig2(io, gbaKeyIdxRB, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(Io *io)
{
	writeKeyConfig2(io, gbaKeyIdxUp, CFGKEY_GBAKEY_UP);
	writeKeyConfig2(io, gbaKeyIdxRight, CFGKEY_GBAKEY_RIGHT);
	writeKeyConfig2(io, gbaKeyIdxDown, CFGKEY_GBAKEY_DOWN);
	writeKeyConfig2(io, gbaKeyIdxLeft, CFGKEY_GBAKEY_LEFT);
	writeKeyConfig2(io, gbaKeyIdxLeftUp, CFGKEY_GBAKEY_LEFT_UP);
	writeKeyConfig2(io, gbaKeyIdxRightUp, CFGKEY_GBAKEY_RIGHT_UP);
	writeKeyConfig2(io, gbaKeyIdxRightDown, CFGKEY_GBAKEY_RIGHT_DOWN);
	writeKeyConfig2(io, gbaKeyIdxLeftDown, CFGKEY_GBAKEY_LEFT_DOWN);
	writeKeyConfig2(io, gbaKeyIdxSelect, CFGKEY_GBAKEY_SELECT);
	writeKeyConfig2(io, gbaKeyIdxStart, CFGKEY_GBAKEY_START);
	writeKeyConfig2(io, gbaKeyIdxA, CFGKEY_GBAKEY_A);
	writeKeyConfig2(io, gbaKeyIdxB, CFGKEY_GBAKEY_B);
	writeKeyConfig2(io, gbaKeyIdxATurbo, CFGKEY_GBAKEY_A_TURBO);
	writeKeyConfig2(io, gbaKeyIdxBTurbo, CFGKEY_GBAKEY_B_TURBO);
	writeKeyConfig2(io, gbaKeyIdxL, CFGKEY_GBAKEY_L);
	writeKeyConfig2(io, gbaKeyIdxR, CFGKEY_GBAKEY_R);
	writeKeyConfig2(io, gbaKeyIdxAB, CFGKEY_GBAKEY_AB);
	writeKeyConfig2(io, gbaKeyIdxRB, CFGKEY_GBAKEY_RB);
}

static bool isGBAExtension(const char *name)
{
	return string_hasDotExtension(name, "gba") || string_hasDotExtension(name, "zip");
}

static int gbaFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isGBAExtension(name);
}

FsDirFilterFunc EmuFilePicker::defaultFsFilter = gbaFsFilter;
FsDirFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = gbaFsFilter;

#define USE_PIX_RGB565
#ifdef USE_PIX_RGB565
static const PixelFormatDesc *pixFmt = &PixelFormatRGB565; //PixelFormatARGB1555; //PixelFormatRGB565
int systemColorDepth = 16;
int systemRedShift = 11;
int systemGreenShift = 6;
int systemBlueShift = 0;//1;
#else
static const PixelFormatDesc *pixFmt = &PixelFormatBGRA8888;
int systemColorDepth = 32;
int systemRedShift = 19;
int systemGreenShift = 11;
int systemBlueShift = 3;
#endif

#include <CommonViewControl.hh>

void EmuSystem::initOptions()
{
	#ifndef CONFIG_BASE_ANDROID
	optionFrameSkip.initDefault(optionFrameSkipAuto); // auto-frameskip default due to highly variable CPU usage
	#endif
}

extern GBALCD gLcd;

void EmuSystem::resetGame()
{
	assert(gameIsRunning());
	CPUReset();
}

static char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'a';
		case 0 ... 9: return 48 + slot;
		default: bug_branch("%d", slot); return 0;
	}
}

void EmuSystem::sprintStateFilename(char *str, size_t size, int slot, const char *gamePath, const char *gameName)
{
	sprintf(str, "%s/%s%c.sgm", gamePath, gameName, saveSlotChar(slot));
}

int EmuSystem::saveState()
{
	FsSys::cPath saveStr;
	sprintStateFilename(saveStr, saveStateSlot);
	#ifdef CONFIG_BASE_IOS_SETUID
		fixFilePermissions(saveStr);
	#endif
	if(CPUWriteState(saveStr))
		return STATE_RESULT_OK;
	else
		return STATE_RESULT_IO_ERROR;
}

int EmuSystem::loadState(int saveStateSlot)
{
	FsSys::cPath saveStr;
	sprintStateFilename(saveStr, saveStateSlot);
	if(CPUReadState(saveStr))
		return STATE_RESULT_OK;
	else
		return STATE_RESULT_IO_ERROR;
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
		CPUWriteState(saveStr);
	}
}

void EmuSystem::saveBackupMem()
{
	if(gameIsRunning())
	{
		logMsg("saving backup memory");
		FsSys::cPath saveStr;
		snprintf(saveStr, sizeof(saveStr), "%s%s", gameName, ".sav");
		#ifdef CONFIG_BASE_IOS_SETUID
			fixFilePermissions(saveStr);
		#endif
		CPUWriteBatteryFile(saveStr);
	}
}

bool EmuSystem::vidSysIsPAL() { return 0; }
static bool touchControlsApplicable() { return 1; }
void EmuSystem::clearInputBuffers() { P1 = 0x03FF; }

void EmuSystem::closeSystem()
{
	assert(gameIsRunning());
	logMsg("closing game %s", gameName);
	saveBackupMem();
	CPUCleanUp();
}

int EmuSystem::loadGame(const char *path)
{
	closeGame();

	string_copy(gamePath, FsSys::workDir());
	#ifdef CONFIG_BASE_IOS_SETUID
		fixFilePermissions(gamePath);
	#endif
	snprintf(fullGamePath, sizeof(fullGamePath), "%s/%s", gamePath, path);
	logMsg("full game path: %s", fullGamePath);
	systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;
	soundInit();
	int size = CPULoadRom(fullGamePath);
	if(size == 0)
	{
		popup.postError("Error loading ROM");
		return 0;
	}
	string_copyUpToLastCharInstance(gameName, path, '.');
	logMsg("set game name: %s", gameName);
	setGameSpecificSettings();
	CPUInit(0, 0);
	CPUReset();
	FsSys::cPath saveStr;
	snprintf(saveStr, sizeof(saveStr), "%s%s", gameName, ".sav");
	CPUReadBatteryFile(saveStr);
	emuView.initImage(0, 240, 160);

	logMsg("started emu");
	return 1;
}

static void commitVideoFrame()
{
	emuView.updateAndDrawContent();
}

void systemDrawScreen()
{
	commitVideoFrame();
}

#ifdef USE_NEW_AUDIO
u16 *systemObtainSoundBuffer(uint samples, uint &buffSamples, void *&ctx)
{
	auto aBuff = Audio::getPlayBuffer(samples/2);
	if(unlikely(!aBuff))
	{
		return nullptr;
	}
	buffSamples = aBuff->frames*2;
	ctx = aBuff;
	return (u16*)aBuff->data;
}

void systemCommitSoundBuffer(uint writtenSamples, void *&ctx)
{
	//logMsg("%d audio frames", writtenSamples/2);
	Audio::commitPlayBuffer((Audio::BufferContext*)ctx, writtenSamples/2);
}
#else
void systemOnWriteDataToSoundBuffer(const u16 * finalWave, int length)
{
	//logMsg("%d audio frames", Audio::pPCM.bytesToFrames(length));
	Audio::writePcm((uchar*)finalWave, EmuSystem::pcmFormat.bytesToFrames(length));
}
#endif

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	CPULoop(renderGfx, processGfx, renderAudio);
}

namespace Input
{

void onInputEvent(const InputEvent &e)
{
	handleInputEvent(e);
}

}

void EmuSystem::configAudioRate()
{
	logMsg("set audio rate %d", (int)optionSoundRate);
	pcmFormat.rate = optionSoundRate;
	soundSetSampleRate(optionSoundRate *.9954);
}

namespace Base
{

void onAppMessage(int type, int shortArg, int intArg, int intArg2) { }

CallResult onInit()
{
	static const GfxLGradientStopDesc navViewGrad[] =
	{
		{ .0, VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, VertexColorPixelFormat.build(42./255., 82./255., 190./255., 1.) },
		{ .3, VertexColorPixelFormat.build(42./255., 82./255., 190./255., 1.) },
		{ .97, VertexColorPixelFormat.build((42./255.) * .6, (82./255.) * .6, (190./255.) * .6, 1.) },
		{ 1., VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};

	mainInitCommon(navViewGrad);
	emuView.initPixmap((uchar*)gLcd.pix, pixFmt, 240, 160);
	utilUpdateSystemColorMaps(0);

	mMenu.init(Config::envIsPS3);
	viewStack.push(&mMenu);
	Gfx::onViewChange();
	mMenu.show();

	Base::displayNeedsUpdate();
	return OK;
}

}
