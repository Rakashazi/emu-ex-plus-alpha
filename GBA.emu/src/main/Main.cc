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

#define LOGTAG "main"
#include <emuframework/EmuSystem.hh>
#include <emuframework/CommonFrameworkIncludes.hh>
#include <main/Main.hh>
#include <main/Cheats.hh>
#include <vbam/gba/GBA.h>
#include <vbam/gba/Sound.h>
#include <vbam/gba/RTC.h>
#include <vbam/common/SoundDriver.h>
#include <vbam/common/Patch.h>
#include <vbam/Util.h>

void setGameSpecificSettings(GBASys &gba);
void CPULoop(GBASys &gba, bool renderGfx, bool processGfx, bool renderAudio);
void CPUCleanUp();
bool CPUReadBatteryFile(GBASys &gba, const char *);
bool CPUWriteBatteryFile(GBASys &gba, const char *);
bool CPUReadState(GBASys &gba, const char *);
bool CPUWriteState(GBASys &gba, const char *);

const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2012-2014\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nVBA-m Team\nvba-m.com";
const char *EmuSystem::inputFaceBtnName = "A/B";
const char *EmuSystem::inputCenterBtnName = "Select/Start";
const uint EmuSystem::inputFaceBtns = 4;
const uint EmuSystem::inputCenterBtns = 2;
const bool EmuSystem::inputHasTriggerBtns = true;
const bool EmuSystem::inputHasRevBtnLayout = false;
const char *EmuSystem::configFilename = "GbaEmu.config";
const bool EmuSystem::hasBundledGames = true;
const uint EmuSystem::maxPlayers = 1;
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"3:2 (Original)", 3, 2},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = sizeofArray(EmuSystem::aspectRatioInfo);
#include <emuframework/CommonGui.hh>
#include <emuframework/CommonCheatGui.hh>

#if defined __ANDROID__ || defined CONFIG_MACHINE_PANDORA
#define GAME_ASSET_EXT "gba"
#else
#define GAME_ASSET_EXT "zip"
#endif

const BundledGameInfo &EmuSystem::bundledGameInfo(uint idx)
{
	static const BundledGameInfo info[]
	{
		{ "Motocross Challenge", "Motocross Challenge." GAME_ASSET_EXT }
	};

	return info[0];
}

const char *EmuSystem::shortSystemName()
{
	return "GBA";
}

const char *EmuSystem::systemName()
{
	return "Game Boy Advance";
}

using namespace IG;

// controls

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
	static const uint A = bit(0), B = bit(1),
			SELECT = bit(2), START = bit(3),
			RIGHT = bit(4), LEFT = bit(5), UP = bit(6), DOWN = bit(7),
			R = bit(8), L = bit(9);
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

void updateVControllerMapping(uint player, SysVController::Map &map)
{
	using namespace GbaKeyStatus;
	map[SysVController::F_ELEM] = A;
	map[SysVController::F_ELEM+1] = B;
	map[SysVController::F_ELEM+2] = L;
	map[SysVController::F_ELEM+3] = R;

	map[SysVController::C_ELEM] = SELECT;
	map[SysVController::C_ELEM+1] = START;

	map[SysVController::D_ELEM] = UP | LEFT;
	map[SysVController::D_ELEM+1] = UP;
	map[SysVController::D_ELEM+2] = UP | RIGHT;
	map[SysVController::D_ELEM+3] = LEFT;
	map[SysVController::D_ELEM+5] = RIGHT;
	map[SysVController::D_ELEM+6] = DOWN | LEFT;
	map[SysVController::D_ELEM+7] = DOWN;
	map[SysVController::D_ELEM+8] = DOWN | RIGHT;
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

void EmuSystem::handleInputAction(uint state, uint emuKey)
{
	if(state == Input::PUSHED)
		unsetBits(P1, emuKey);
	else
		setBits(P1, emuKey);
}

enum
{
	CFGKEY_RTC_EMULATION = 256
};

Byte1Option optionRtcEmulation(CFGKEY_RTC_EMULATION, RTC_EMU_AUTO, 0, optionIsValidWithMax<2>);
bool detectedRtcGame = 0;

bool EmuSystem::readConfig(Io &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_RTC_EMULATION: optionRtcEmulation.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(Io *io)
{
	optionRtcEmulation.writeWithKeyIfNotDefault(io);
}

static bool isGBAExtension(const char *name)
{
	return string_hasDotExtension(name, "gba")
			|| string_hasDotExtension(name, "zip")
			|| string_hasDotExtension(name, "7z");
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

void EmuSystem::initOptions()
{
	#ifndef CONFIG_BASE_ANDROID
	optionFrameSkip.initDefault(optionFrameSkipAuto); // auto-frameskip default due to highly variable CPU usage
	#endif
}

void EmuSystem::onOptionsLoaded() {}

void EmuSystem::resetGame()
{
	assert(gameIsRunning());
	CPUReset(gGba);
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

FsSys::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return makeFSPathStringPrintf("%s/%s%c.sgm", statePath, gameName, saveSlotChar(slot));
}

int EmuSystem::saveState()
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	fixFilePermissions(saveStr);
	if(CPUWriteState(gGba, saveStr.data()))
		return STATE_RESULT_OK;
	else
		return STATE_RESULT_IO_ERROR;
}

int EmuSystem::loadState(int saveStateSlot)
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	if(CPUReadState(gGba, saveStr.data()))
		return STATE_RESULT_OK;
	else
		return STATE_RESULT_IO_ERROR;
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		auto saveStr = sprintStateFilename(-1);
		fixFilePermissions(saveStr);
		CPUWriteState(gGba, saveStr.data());
	}
}

void EmuSystem::saveBackupMem()
{
	if(gameIsRunning())
	{
		logMsg("saving backup memory");
		auto saveStr = makeFSPathStringPrintf("%s/%s.sav", savePath(), gameName());
		fixFilePermissions(saveStr);
		CPUWriteBatteryFile(gGba, saveStr.data());
		writeCheatFile();
	}
}

bool EmuSystem::vidSysIsPAL() { return 0; }
uint EmuSystem::multiresVideoBaseX() { return 0; }
uint EmuSystem::multiresVideoBaseY() { return 0; }
bool touchControlsApplicable() { return 1; }
void EmuSystem::clearInputBuffers() { P1 = 0x03FF; }

void EmuSystem::closeSystem()
{
	assert(gameIsRunning());
	logMsg("closing game %s", gameName());
	saveBackupMem();
	CPUCleanUp();
	detectedRtcGame = 0;
	cheatsNumber = 0; // reset cheat list
}

static bool applyGamePatches(const char *patchDir, const char *romName, u8 *rom, int &romSize)
{
	auto patchStr = makeFSPathStringPrintf("%s/%s.ips", patchDir, romName);
	if(FsSys::fileExists(patchStr.data()))
	{
		logMsg("applying IPS patch: %s", patchStr.data());
		if(!patchApplyIPS(patchStr.data(), &rom, &romSize))
		{
			popup.postError("Error applying IPS patch");
			return false;
		}
		return true;
	}
	string_printf(patchStr, "%s/%s.ups", patchDir, romName);
	if(FsSys::fileExists(patchStr.data()))
	{
		logMsg("applying UPS patch: %s", patchStr.data());
		if(!patchApplyUPS(patchStr.data(), &rom, &romSize))
		{
			popup.postError("Error applying UPS patch");
			return false;
		}
		return true;
	}
	string_printf(patchStr, "%s/%s.ppf", patchDir, romName);
	if(FsSys::fileExists(patchStr.data()))
	{
		logMsg("applying UPS patch: %s", patchStr.data());
		if(!patchApplyPPF(patchStr.data(), &rom, &romSize))
		{
			popup.postError("Error applying PPF patch");
			return false;
		}
		return true;
	}
	return true; // no patch found
}

static int loadGameCommon(int size)
{
	if(!size)
	{
		popup.postError("Error loading ROM");
		return 0;
	}
	emuVideo.initImage(0, 240, 160);
	setGameSpecificSettings(gGba);
	if(!applyGamePatches(EmuSystem::savePath(), EmuSystem::gameName(), gGba.mem.rom, size))
	{
		return 0;
	}
	CPUInit(gGba, 0, 0);
	CPUReset(gGba);
	auto saveStr = makeFSPathStringPrintf("%s/%s.sav", EmuSystem::savePath(), EmuSystem::gameName());
	CPUReadBatteryFile(gGba, saveStr.data());
	readCheatFile();
	logMsg("started emu");
	return 1;
}

int EmuSystem::loadGame(const char *path)
{
	closeGame();
	setupGamePaths(path);
	int size = CPULoadRom(gGba, fullGamePath());
	return loadGameCommon(size);
}

int EmuSystem::loadGameFromIO(Io &io, const char *origFilename)
{
	closeGame();
	setupGameName(origFilename);
	int size = CPULoadRomWithIO(gGba, io);
	return loadGameCommon(size);
}

static void commitVideoFrame()
{
	updateAndDrawEmuVideo();
}

void systemDrawScreen()
{
	commitVideoFrame();
}

void systemOnWriteDataToSoundBuffer(const u16 * finalWave, int length)
{
	//logMsg("%d audio frames", Audio::pPCM.bytesToFrames(length));
	EmuSystem::writeSound(finalWave, EmuSystem::pcmFormat.bytesToFrames(length));
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	CPULoop(gGba, renderGfx, processGfx, renderAudio);
}

void EmuSystem::configAudioRate()
{
	logMsg("set audio rate %d", (int)optionSoundRate);
	pcmFormat.rate = optionSoundRate;
	soundSetSampleRate(gGba, optionSoundRate *.99534);
}

void EmuSystem::savePathChanged() { }

bool EmuSystem::hasInputOptions() { return false; }

namespace Base
{

CallResult onInit(int argc, char** argv)
{
	emuVideo.initPixmap((char*)gGba.lcd.pix, pixFmt, 240, 160);
	utilUpdateSystemColorMaps(0);

	static const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, VertexColorPixelFormat.build(42./255., 82./255., 190./255., 1.) },
		{ .3, VertexColorPixelFormat.build(42./255., 82./255., 190./255., 1.) },
		{ .97, VertexColorPixelFormat.build((42./255.) * .6, (82./255.) * .6, (190./255.) * .6, 1.) },
		{ 1., VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};

	mainInitCommon(argc, argv, navViewGrad);
	return OK;
}

}
