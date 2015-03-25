/*  This file is part of MSX.emu.

	MSX.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MSX.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MSX.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "main"
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/CommonFrameworkIncludes.hh>
#include <imagine/io/ZipIO.hh>

// TODO: remove when namespace code is complete
#ifdef __APPLE__
#define Fixed MacTypes_Fixed
#define Rect MacTypes_Rect
#include <MacTypes.h>
#undef Fixed
#undef Rect
#endif

#if defined CONFIG_BASE_ANDROID || defined CONFIG_ENV_WEBOS || defined CONFIG_BASE_IOS || defined CONFIG_MACHINE_IS_PANDORA
static const bool checkForMachineFolderOnStart = true;
#else
static const bool checkForMachineFolderOnStart = false;
#endif

static bool canInstallCBIOS = true;

extern "C"
{
	#include <blueMSX/IoDevice/Casette.h>
	#include <blueMSX/IoDevice/Disk.h>
	#include <blueMSX/Input/JoystickPort.h>
	#include <blueMSX/Board/Board.h>
	#include <blueMSX/Board/MSX.h>
	#include <blueMSX/Board/Coleco.h>
	#include <blueMSX/Language/Language.h>
	#include <blueMSX/Z80/R800.h>
	#include <blueMSX/Memory/MegaromCartridge.h>
	#include <blueMSX/Input/InputEvent.h>
	#include <blueMSX/Utils/SaveState.h>
}

#include <blueMSX/Utils/ziphelper.h>

const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2014\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nBlueMSX Team\nbluemsx.com";
BoardInfo boardInfo = { 0 };
extern bool fdcActive;
Machine *machine = 0;
Mixer *mixer = 0;
CLINK Int16 *mixerGetBuffer(Mixer* mixer, UInt32 *samplesOut);

static FsSys::PathString machineCustomPath{};
static FsSys::PathString machineBasePath{};

static void setMachineBasePath(FsSys::PathString &outPath, const FsSys::PathString &customPath)
{
	if(!strlen(customPath.data()))
	{
		#if defined CONFIG_ENV_LINUX && !defined CONFIG_MACHINE_PANDORA
		string_printf(outPath, "%s/MSX.emu", Base::assetPath());
		#else
		string_printf(outPath, "%s/MSX.emu", Base::storagePath());
		#endif
	}
	else
	{
		string_printf(outPath, "%s", customPath.data());
	}
	logMsg("set machine file path: %s", outPath.data());
}

const char *machineBasePathStr()
{
	return machineBasePath.data();
}

void chdirToMachineBaseDir(char *prevWDir, size_t prevWDirSize)
{
	string_copy(prevWDir, FsPosix::workDir(), prevWDirSize);
	FsSys::chdir(machineBasePathStr());
	logMsg("changed to machine base path: %s", machineBasePathStr());
}

void chdirToPrevWorkingDir(char *prevWDir)
{
	FsSys::chdir(prevWDir);
	logMsg("changed back to: %s", prevWDir);
}

static char cartName[2][512]{};
extern RomType currentRomType[2];
static char diskName[2][512]{};
static char tapeName[512]{};
char hdName[4][512]{};

static bool insertDisk(const char *path, uint slot = 0);
static bool insertROM(const char *path, uint slot = 0);

static bool isTapeExtension(const char *name)
{
	return string_hasDotExtension(name, "cas");
}

static bool isDiskExtension(const char *name)
{
	return string_hasDotExtension(name, "dsk");
}

static bool isROMExtension(const char *name)
{
	return string_hasDotExtension(name, "rom") || string_hasDotExtension(name, "mx1")
			|| string_hasDotExtension(name, "mx2") || string_hasDotExtension(name, "col");
}

static bool isMSXExtension(const char *name)
{
	return isROMExtension(name) || isDiskExtension(name) || string_hasDotExtension(name, "zip");
}

static int msxFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isMSXExtension(name);
}

static int isMSXROMExtension(const char *name, int type)
{
	return isROMExtension(name) || string_hasDotExtension(name, "zip");
}

static int isMSXDiskExtension(const char *name, int type)
{
	return isDiskExtension(name) || string_hasDotExtension(name, "zip");
}

static int isMSXTapeExtension(const char *name, int type)
{
	return isTapeExtension(name) || string_hasDotExtension(name, "zip");
}

static const uint msxKeyboardKeys = 92;

enum
{
	msxKeyIdxUp = EmuControls::systemKeyMapStart,
	msxKeyIdxRight,
	msxKeyIdxDown,
	msxKeyIdxLeft,
	msxKeyIdxLeftUp,
	msxKeyIdxRightUp,
	msxKeyIdxRightDown,
	msxKeyIdxLeftDown,
	msxKeyIdxJS1Btn,
	msxKeyIdxJS2Btn,
	msxKeyIdxJS1BtnTurbo,
	msxKeyIdxJS2BtnTurbo,

	msxKeyIdxUp2,
	msxKeyIdxRight2,
	msxKeyIdxDown2,
	msxKeyIdxLeft2,
	msxKeyIdxLeftUp2,
	msxKeyIdxRightUp2,
	msxKeyIdxRightDown2,
	msxKeyIdxLeftDown2,
	msxKeyIdxJS1Btn2,
	msxKeyIdxJS2Btn2,
	msxKeyIdxJS1BtnTurbo2,
	msxKeyIdxJS2BtnTurbo2,

	msxKeyIdxColeco0Num,
	msxKeyIdxColeco1Num,
	msxKeyIdxColeco2Num,
	msxKeyIdxColeco3Num,
	msxKeyIdxColeco4Num,
	msxKeyIdxColeco5Num,
	msxKeyIdxColeco6Num,
	msxKeyIdxColeco7Num,
	msxKeyIdxColeco8Num,
	msxKeyIdxColeco9Num,
	msxKeyIdxColecoStar,
	msxKeyIdxColecoHash,

	msxKeyIdxColeco0Num2,
	msxKeyIdxColeco1Num2,
	msxKeyIdxColeco2Num2,
	msxKeyIdxColeco3Num2,
	msxKeyIdxColeco4Num2,
	msxKeyIdxColeco5Num2,
	msxKeyIdxColeco6Num2,
	msxKeyIdxColeco7Num2,
	msxKeyIdxColeco8Num2,
	msxKeyIdxColeco9Num2,
	msxKeyIdxColecoStar2,
	msxKeyIdxColecoHash2,

	msxKeyIdxToggleKb,
	msxKeyIdxKbStart,
	msxKeyIdxKbEnd = msxKeyIdxKbStart + (msxKeyboardKeys - 1)
};

static const uint msxKeyConfigBase = 384;
static const uint msxJSKeys = 13;

enum
{
	CFGKEY_MACHINE_NAME = 256, CFGKEY_SKIP_FDC_ACCESS = 257,
	CFGKEY_MACHINE_FILE_PATH = 258
};

#define optionMachineNameDefault "MSX2"
static char optionMachineNameStr[128] = optionMachineNameDefault;
static PathOption optionMachineName(CFGKEY_MACHINE_NAME, optionMachineNameStr, optionMachineNameDefault);
Byte1Option optionSkipFdcAccess(CFGKEY_SKIP_FDC_ACCESS, 1);
PathOption optionFirmwarePath(CFGKEY_MACHINE_FILE_PATH, machineCustomPath, "");
static uint activeBoardType = BOARD_MSX;
const char *EmuSystem::inputFaceBtnName = "A/B";
const char *EmuSystem::inputCenterBtnName = "Space/KB";
const uint EmuSystem::inputFaceBtns = 2;
const uint EmuSystem::inputCenterBtns = 2;
const bool EmuSystem::inputHasTriggerBtns = false;
const bool EmuSystem::inputHasRevBtnLayout = false;
const bool EmuSystem::inputHasKeyboard = true;
const char *EmuSystem::configFilename = "MsxEmu.config";
const uint EmuSystem::maxPlayers = 2;
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = sizeofArray(EmuSystem::aspectRatioInfo);
#include <emuframework/CommonGui.hh>

const char *EmuSystem::shortSystemName()
{
	return "MSX";
}

const char *EmuSystem::systemName()
{
	return "MSX";
}

bool EmuSystem::readConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_MACHINE_NAME: optionMachineName.readFromIO(io, readSize);
		bcase CFGKEY_SKIP_FDC_ACCESS: optionSkipFdcAccess.readFromIO(io, readSize);
		bcase CFGKEY_MACHINE_FILE_PATH: optionFirmwarePath.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	if(!optionMachineName.isDefault())
	{
		optionMachineName.writeToIO(io);
	}
	optionSkipFdcAccess.writeWithKeyIfNotDefault(io);
	optionFirmwarePath.writeToIO(io);
}

void EmuSystem::initOptions()
{
	optionSoundRate.initDefault(44100);
	optionSoundRate.isConst = 1;
}

void EmuSystem::onOptionsLoaded()
{
	setMachineBasePath(machineBasePath, machineCustomPath);
	fixFilePermissions(machineBasePath);
}

FsDirFilterFunc EmuFilePicker::defaultFsFilter = msxFsFilter;
FsDirFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = msxFsFilter;

static const uint msxMaxResX = (256) * 2, msxResY = 224,
		msxMaxFrameBuffResX = (272) * 2, msxMaxFrameBuffResY = 240;

static uint msxResX = msxMaxResX/2;
static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;
static uint16 screenBuff[msxMaxFrameBuffResX*msxMaxFrameBuffResY] __attribute__ ((aligned (8))) {0};
//static uint16 dummyLine[msxMaxFrameBuffResX] __attribute__ ((aligned (8)));

static SysVController::KbMap kbToEventMap
{
	EC_Q, EC_W, EC_E, EC_R, EC_T, EC_Y, EC_U, EC_I, EC_O, EC_P,
	EC_A, EC_S, EC_D, EC_F, EC_G, EC_H, EC_J, EC_K, EC_L, EC_NONE,
	EC_CAPS, EC_Z, EC_X, EC_C, EC_V, EC_B, EC_N, EC_M, EC_BKSPACE, EC_NONE,
	EC_NONE, EC_NONE, EC_NONE, EC_SPACE, EC_SPACE, EC_SPACE, EC_SPACE, EC_CTRL, EC_CTRL, EC_RETURN
};

static SysVController::KbMap kbToEventMap2
{
	EC_F1, EC_F1, EC_F2, EC_F2, EC_F3, EC_F3, EC_F4, EC_F4, EC_F5, EC_F5, // 0-9
	EC_1, EC_2, EC_3, EC_4, EC_5, EC_6, EC_7, EC_8, EC_9, EC_0, // 10-19
	EC_TAB, EC_8 | (EC_LSHIFT << 8), EC_9 | (EC_LSHIFT << 8), EC_3 | (EC_LSHIFT << 8), EC_4 | (EC_LSHIFT << 8), EC_SEMICOL | (EC_LSHIFT << 8), EC_NEG, EC_SEMICOL, EC_ESC, EC_NONE,
	EC_NONE, EC_NONE, EC_NONE, EC_SPACE, EC_SPACE, EC_SPACE, EC_SPACE, EC_PERIOD, EC_PERIOD, EC_RETURN
};

static void setupVKeyboardMap(uint boardType)
{
	if(boardType == BOARD_COLECO)
	{
		uint playerShift = pointerInputPlayer ? 12 : 0;
		iterateTimes(9, i) // 1 - 9
			kbToEventMap2[10 + i] = EC_COLECO1_1 + i + playerShift;
		kbToEventMap2[19] = EC_COLECO1_0 + playerShift;
		kbToEventMap2[23] = EC_COLECO1_HASH + playerShift;
	}
	else
	{
		iterateTimes(10, i) // 1 - 0
			kbToEventMap2[10 + i] = EC_1 + i;
		kbToEventMap2[23] = EC_3 | (EC_LSHIFT << 8);
	}
	vController.updateKeyboardMapping();
}

void updateVControllerKeyboardMapping(uint mode, SysVController::KbMap &map)
{
	memcpy(map, mode ? kbToEventMap2 : kbToEventMap, sizeof(SysVController::KbMap));
}

void updateVControllerMapping(uint player, SysVController::Map &map)
{
	map[SysVController::F_ELEM] = player ? EC_JOY2_BUTTON2 : EC_JOY1_BUTTON2;
	map[SysVController::F_ELEM+1] = player ? EC_JOY2_BUTTON1 : EC_JOY1_BUTTON1;

	map[SysVController::C_ELEM] = activeBoardType == BOARD_COLECO ? (player ? EC_COLECO2_STAR : EC_COLECO1_STAR)
																	: EC_SPACE;
	map[SysVController::C_ELEM+1] = EC_KEYCOUNT;

	uint up = player ? EC_JOY2_UP : EC_JOY1_UP;
	uint down = player ? EC_JOY2_DOWN : EC_JOY1_DOWN;
	uint left = player ? EC_JOY2_LEFT : EC_JOY1_LEFT;
	uint right = player ? EC_JOY2_RIGHT : EC_JOY1_RIGHT;
	map[SysVController::D_ELEM] = up | (left << 8);
	map[SysVController::D_ELEM+1] = up;
	map[SysVController::D_ELEM+2] = up | (right << 8);
	map[SysVController::D_ELEM+3] = left;
	map[SysVController::D_ELEM+5] = right;
	map[SysVController::D_ELEM+6] = down | (left << 8);
	map[SysVController::D_ELEM+7] = down;
	map[SysVController::D_ELEM+8] = down | (right << 8);
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	switch(input)
	{
		case msxKeyIdxUp: return EC_JOY1_UP;
		case msxKeyIdxRight: return EC_JOY1_RIGHT;
		case msxKeyIdxDown: return EC_JOY1_DOWN;
		case msxKeyIdxLeft: return EC_JOY1_LEFT;
		case msxKeyIdxLeftUp: return EC_JOY1_LEFT | (EC_JOY1_UP << 8);
		case msxKeyIdxRightUp: return EC_JOY1_RIGHT | (EC_JOY1_UP << 8);
		case msxKeyIdxRightDown: return EC_JOY1_RIGHT | (EC_JOY1_DOWN << 8);
		case msxKeyIdxLeftDown: return EC_JOY1_LEFT | (EC_JOY1_DOWN << 8);
		case msxKeyIdxJS1BtnTurbo: turbo = 1;
		case msxKeyIdxJS1Btn: return EC_JOY1_BUTTON1;
		case msxKeyIdxJS2BtnTurbo: turbo = 1;
		case msxKeyIdxJS2Btn: return EC_JOY1_BUTTON2;

		case msxKeyIdxUp2: return EC_JOY2_UP;
		case msxKeyIdxRight2: return EC_JOY2_RIGHT;
		case msxKeyIdxDown2: return EC_JOY2_DOWN;
		case msxKeyIdxLeft2: return EC_JOY2_LEFT;
		case msxKeyIdxLeftUp2: return EC_JOY2_LEFT | (EC_JOY2_UP << 8);
		case msxKeyIdxRightUp2: return EC_JOY2_RIGHT | (EC_JOY2_UP << 8);
		case msxKeyIdxRightDown2: return EC_JOY2_RIGHT | (EC_JOY2_DOWN << 8);
		case msxKeyIdxLeftDown2: return EC_JOY2_LEFT | (EC_JOY2_DOWN << 8);
		case msxKeyIdxJS1BtnTurbo2: turbo = 1;
		case msxKeyIdxJS1Btn2: return EC_JOY2_BUTTON1;
		case msxKeyIdxJS2BtnTurbo2: turbo = 1;
		case msxKeyIdxJS2Btn2: return EC_JOY2_BUTTON2;

		case msxKeyIdxColeco0Num ... msxKeyIdxColecoHash :
			return (input - msxKeyIdxColeco0Num) + EC_COLECO1_0;
		case msxKeyIdxColeco0Num2 ... msxKeyIdxColecoHash2 :
			return (input - msxKeyIdxColeco0Num) + EC_COLECO2_0;

		case msxKeyIdxToggleKb: return EC_KEYCOUNT;
		case msxKeyIdxKbStart ... msxKeyIdxKbEnd :
			return (input - msxKeyIdxKbStart) + 1;
		default: bug_branch("%d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(uint state, uint emuKey)
{
	uint event1 = emuKey & 0xFF;
	if(event1 == EC_KEYCOUNT)
	{
		if(state == Input::PUSHED)
			vController.toggleKeyboard();
	}
	else
	{
		assert(event1 < EC_KEYCOUNT);
		eventMap[event1] = state == Input::PUSHED;
		uint event2 = emuKey >> 8;
		if(event2) // extra event for diagonals
		{
			eventMap[event2] = state == Input::PUSHED;
		}
	}
}

// frameBuffer implementation
Pixel* frameBufferGetLine(FrameBuffer* frameBuffer, int y)
{
	//logMsg("getting line %d", y);
	/*if(y < 8 || y >= (224+8)) return dummyLine;
    return (Pixel*)&screenBuff[(y - 8) * msxResX];*/
	return (Pixel*)&screenBuff[y * msxResX];
}

static bool doubleWidthFrame = 0;
int frameBufferGetDoubleWidth(FrameBuffer* frameBuffer, int y) { return doubleWidthFrame; }
void frameBufferSetDoubleWidth(FrameBuffer* frameBuffer, int y, int val)
{
	if(doubleWidthFrame != val)
	{
		logMsg("setting double width line %d, %d", y, val);
		doubleWidthFrame = val;
		msxResX = doubleWidthFrame ? msxMaxResX : msxMaxResX/2;
		if(emuVideo.vidPix.w() != msxResX)
		{
			emuVideo.resizeImage(msxResX, msxResY);
		}
	}
}

static bool insertMedia()
{
	iterateTimes(2, i)
	{
		switch(currentRomType[i])
		{
			bcase ROM_SCC: logMsg("loading SCC"); boardChangeCartridge(i, ROM_SCC, "", 0);
			bcase ROM_SCCPLUS: logMsg("loading SCC+"); boardChangeCartridge(i, ROM_SCCPLUS, "", 0);
			bcase ROM_SUNRISEIDE:
				logMsg("loading Sunrise IDE");
				if(!boardChangeCartridge(i, ROM_SUNRISEIDE, "Sunrise IDE", 0))
				{
					popup.postError("Error loading Sunrise IDE device");
				}
			bdefault:
				if(strlen(cartName[i])) logMsg("loading ROM %s", cartName[i]);
				if(strlen(cartName[i]) && !insertROM(cartName[i], i))
				{
					popup.printf(3, 1, "Error loading ROM%d:\n%s", i, cartName[i]);
					return 0;
				}
		}
	}

	iterateTimes(2, i)
	{
		if(strlen(diskName[i])) logMsg("loading Disk %s", diskName[i]);
		if(strlen(diskName[i]) && !insertDisk(diskName[i], i))
		{
			popup.printf(3, 1, "Error loading Disk%d:\n%s", i, diskName[i]);
			return 0;
		}
	}

	iterateTimes(4, i)
	{
		if(strlen(hdName[i])) logMsg("loading HD %s", hdName[i]);
		if(strlen(hdName[i]) && !insertDisk(hdName[i], diskGetHdDriveId(i / 2, i % 2)))
		{
			popup.printf(3, 1, "Error loading Disk%d:\n%s", i, hdName[i]);
			return 0;
		}
	}
	return 1;
}

static bool msxIsInit()
{
	return boardInfo.run != 0;
}

static void clearAllMediaNames()
{
	strcpy(cartName[0], "");
	strcpy(cartName[1], "");
	strcpy(diskName[0], "");
	strcpy(diskName[1], "");
	strcpy(hdName[0], "");
	strcpy(hdName[1], "");
	strcpy(hdName[2], "");
	strcpy(hdName[3], "");
	strcpy(tapeName, "");
}

static void ejectMedia()
{
	boardChangeCartridge(0, ROM_UNKNOWN, 0, 0);
	boardChangeCartridge(1, ROM_UNKNOWN, 0, 0);
	diskChange(0, 0, 0);
	diskChange(1, 0, 0);
	diskChange(diskGetHdDriveId(0, 0), 0, 0);
	diskChange(diskGetHdDriveId(0, 1), 0, 0);
	diskChange(diskGetHdDriveId(1, 0), 0, 0);
	diskChange(diskGetHdDriveId(1, 1), 0, 0);
}

static void destroyMSX()
{
	assert(machine);
	logMsg("destroying MSX");
	fdcActive = 0;
	if(msxIsInit())
	{
		ejectMedia();
		boardInfo.destroy();
	}
	mem_zero(boardInfo);
	clearAllMediaNames();
}

static const char *boardTypeToStr(BoardType type)
{
	switch(type)
	{
    case BOARD_MSX:
    case BOARD_MSX_S3527:
    case BOARD_MSX_S1985:
    case BOARD_MSX_T9769B:
    case BOARD_MSX_T9769C:
    case BOARD_MSX_FORTE_II:
    	return "MSX";
    case BOARD_COLECO:
    	return "Coleco";
    default:
    	return "Unknown";
	}
}

static bool createBoard()
{
	// TODO: 50hz mode
	assert(machine);
	switch (machine->board.type)
	{
		case BOARD_MSX:
		case BOARD_MSX_S3527:
		case BOARD_MSX_S1985:
		case BOARD_MSX_T9769B:
		case BOARD_MSX_T9769C:
		case BOARD_MSX_FORTE_II:
			logMsg("creating MSX");
			joystickPortSetType(0, JOYSTICK_PORT_JOYSTICK);
			joystickPortSetType(1, JOYSTICK_PORT_JOYSTICK);
			activeBoardType = BOARD_MSX;
			setupVKeyboardMap(BOARD_MSX);
			return msxCreate(machine, VDP_SYNC_60HZ, &boardInfo);
		case BOARD_COLECO:
			logMsg("creating Coleco");
			joystickPortSetType(0, JOYSTICK_PORT_COLECOJOYSTICK);
			joystickPortSetType(1, JOYSTICK_PORT_COLECOJOYSTICK);
			activeBoardType = BOARD_COLECO;
			setupVKeyboardMap(BOARD_COLECO);
			return colecoCreate(machine, VDP_SYNC_60HZ, &boardInfo);
		default:
			logErr("error: unknown board type 0x%X", machine->board.type);
			return 0;
	}
}

static bool createBoardFromLoadGame()
{
	if(msxIsInit())
		destroyMSX();
	if(!createBoard())
	{
		mem_zero(boardInfo);
		popup.printf(2, 1, "Error initializing %s", machine->name);
		return 0;
	}
	//logMsg("z80 freq %d, r800 %d", ((R800*)boardInfo.cpuRef)->frequencyZ80, ((R800*)boardInfo.cpuRef)->frequencyR800);
	logMsg("max carts %d, disks %d, tapes %d", boardInfo.cartridgeCount, boardInfo.diskdriveCount, boardInfo.casetteCount);
	return 1;
}

static bool initMachine(const char *machineName)
{
	if(machine && string_equal(machine->name, machineName))
	{
		return 1;
	}
	logMsg("loading machine %s", machineName);
	if(machine)
		machineDestroy(machine);
	FsSys::PathString wDir;
	strcpy(wDir.data(), FsSys::workDir());
	FsSys::chdir(machineBasePath.data());
	machine = machineCreate(machineName);
	FsSys::chdir(wDir.data());
	if(!machine)
	{
		popup.printf(5, 1, "Error loading machine files for\n\"%s\",\nmake sure they are in:\n%s", machineName, machineBasePath.data());
		return 0;
	}
	boardSetMachine(machine);
	return 1;
}

static bool getFirstROMFilenameInZip(const char *zipPath, char *nameOut, uint outBytes)
{
	assert(nameOut);
	int count;
	char *fileList = zipGetFileList(zipPath, ".rom", &count);
	if (fileList)
	{
		string_copy(nameOut, fileList, outBytes);
		free(fileList);
		return 1;
	}
	fileList = zipGetFileList(zipPath, ".mx1", &count);
	if (fileList)
	{
		string_copy(nameOut, fileList, outBytes);
		free(fileList);
		return 1;
	}
	fileList = zipGetFileList(zipPath, ".mx2", &count);
	if (fileList)
	{
		string_copy(nameOut, fileList, outBytes);
		free(fileList);
		return 1;
	}
	fileList = zipGetFileList(zipPath, ".col", &count);
	if (fileList)
	{
		string_copy(nameOut, fileList, outBytes);
		free(fileList);
		return 1;
	}
	return 0;
}

static bool getFirstDiskFilenameInZip(const char *zipPath, char *nameOut, uint outBytes)
{
	assert(nameOut);
	int count;
	char *fileList = zipGetFileList(zipPath, ".dsk", &count);
	if (fileList)
	{
		string_copy(nameOut, fileList, outBytes);
		free(fileList);
		return 1;
	}
	return 0;
}

static bool getFirstTapeFilenameInZip(const char *zipPath, char *nameOut, uint outBytes)
{
	assert(nameOut);
	int count;
	char *fileList = zipGetFileList(zipPath, ".cas", &count);
	if (fileList)
	{
		string_copy(nameOut, fileList, outBytes);
		free(fileList);
		return 1;
	}
	return 0;
}

static bool insertROM(const char *path, uint slot)
{
	char fileInZipName[256] = "";
	if(string_hasDotExtension(path, "zip"))
	{
		if(!getFirstROMFilenameInZip(path, fileInZipName, sizeof(fileInZipName)))
		{
			popup.postError("No ROM found in zip");
			return 0;
		}
		logMsg("found %s in zip", fileInZipName);
	}

	if(!boardChangeCartridge(slot, ROM_UNKNOWN, path, strlen(fileInZipName) ? fileInZipName : 0))
	{
		popup.postError("Error loading ROM");
		return 0;
	}

	return 1;
}

static bool insertDisk(const char *path, uint slot)
{
	char fileInZipName[256] = "";
	if(string_hasDotExtension(path, "zip"))
	{
		if(!getFirstDiskFilenameInZip(path, fileInZipName, sizeof(fileInZipName)))
		{
			popup.postError("No Disk found in zip");
			return 0;
		}
		logMsg("found %s in zip", fileInZipName);
	}

	if(!diskChange(slot, path, strlen(fileInZipName) ? fileInZipName : 0))
	{
		popup.postError("Error loading Disk");
		return 0;
	}

	return 1;
}

void EmuSystem::resetGame()
{
	assert(gameIsRunning());
	fdcActive = 0;
	//boardInfo.softReset();
	FsSys::chdir(EmuSystem::gamePath());
	boardInfo.destroy();
	if(!createBoard())
	{
		mem_zero(boardInfo);
		popup.postError("Error during MSX reset");
		destroyMSX();
	}
	insertMedia();
}

static char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'A';
		case 0 ... 9: return '0' + slot;
		default: bug_branch("%d", slot); return 0;
	}
}

FsSys::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return makeFSPathStringPrintf("%s/%s.0%c.sta", statePath, gameName, saveSlotChar(slot));
}

static char saveStateVersion[] = "blueMSX - state  v 8";
extern int pendingInt;

static int saveBlueMSXState(const char *filename)
{
	saveStateCreateForWrite(filename);
	int rv = zipSaveFile(filename, "version", 0, saveStateVersion, sizeof(saveStateVersion));
	if (!rv)
	{
		saveStateDestroy();
		return STATE_RESULT_IO_ERROR;
	}

	SaveState* state = saveStateOpenForWrite("board");

	saveStateSet(state, "pendingInt", pendingInt);
	saveStateSet(state, "cartType00", currentRomType[0]);
	if(strlen(cartName[0]))
		saveStateSetBuffer(state, "cartName00",  cartName[0], strlen(cartName[0]) + 1);
	saveStateSet(state, "cartType01", currentRomType[1]);
	if(strlen(cartName[1]))
		saveStateSetBuffer(state, "cartName01",  cartName[1], strlen(cartName[1]) + 1);
	if(strlen(diskName[0]))
		saveStateSetBuffer(state, "diskName00",  diskName[0], strlen(diskName[0]) + 1);
	if(strlen(diskName[1]))
		saveStateSetBuffer(state, "diskName01",  diskName[1], strlen(diskName[1]) + 1);
	if(strlen(hdName[0]))
		saveStateSetBuffer(state, "diskName02",  hdName[0], strlen(hdName[0]) + 1);
	if(strlen(hdName[1]))
		saveStateSetBuffer(state, "diskName03",  hdName[1], strlen(hdName[1]) + 1);
	if(strlen(hdName[2]))
		saveStateSetBuffer(state, "diskName10",  hdName[2], strlen(hdName[2]) + 1);
	if(strlen(hdName[3]))
		saveStateSetBuffer(state, "diskName11",  hdName[3], strlen(hdName[3]) + 1);
	saveStateClose(state);

	machineSaveState(machine);
	boardInfo.saveState();
	saveStateDestroy();
	return STATE_RESULT_OK;
}

int EmuSystem::saveState()
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	return saveBlueMSXState(saveStr.data());
}

static void closeGameByFailedStateLoad()
{
	EmuSystem::closeGame(0);
	if(!menuViewIsActive)
	{
		// failed while in-game, restore the menu
		restoreMenuFromGame();
	}
	// notify main menu game is closed
	mMenu.gameStopped();
	// leave any sub menus that may depending on running game state
	viewStack.popToRoot();
}

static int loadBlueMSXState(const char *filename)
{
	logMsg("loading state %s", filename);

	assert(machine);
	ejectMedia();

	saveStateCreateForRead(filename);
	int size;
	char *version = (char*)zipLoadFile(filename, "version", &size);
	if(!version)
	{
		saveStateDestroy();
		return STATE_RESULT_IO_ERROR;
	}
	if(0 != strncmp(version, saveStateVersion, sizeof(saveStateVersion) - 1))
	{
		free(version);
		saveStateDestroy();
		popup.postError("Incorrect state version");
		return STATE_RESULT_OTHER_ERROR;
	}
	free(version);

	machineLoadState(machine);
	logMsg("machine name %s", machine->name);
	char optionMachineNameStrOld[128];
	string_copy(optionMachineNameStrOld, optionMachineNameStr, sizeof(optionMachineNameStr));
	string_copy(optionMachineNameStr, machine->name, sizeof(optionMachineNameStr));

	// from this point on, errors are fatal and require the existing game to close
	if(!createBoardFromLoadGame())
	{
		saveStateDestroy();
		string_copy(optionMachineNameStr, optionMachineNameStrOld, sizeof(optionMachineNameStr)); // restore old machine name
		closeGameByFailedStateLoad();
		return STATE_RESULT_OTHER_ERROR;
	}

	clearAllMediaNames();
	SaveState* state = saveStateOpenForRead("board");
	currentRomType[0] = saveStateGet(state, "cartType00", 0);
	saveStateGetBuffer(state, "cartName00",  cartName[0], sizeof(cartName[0]));
	currentRomType[1] = saveStateGet(state, "cartType01", 0);
	saveStateGetBuffer(state, "cartName01",  cartName[1], sizeof(cartName[1]));
	saveStateGetBuffer(state, "diskName00",  diskName[0], sizeof(diskName[0]));
	saveStateGetBuffer(state, "diskName01",  diskName[1], sizeof(diskName[1]));
	saveStateGetBuffer(state, "diskName02",  hdName[0], sizeof(hdName[0]));
	saveStateGetBuffer(state, "diskName03",  hdName[1], sizeof(hdName[1]));
	saveStateGetBuffer(state, "diskName10",  hdName[2], sizeof(hdName[2]));
	saveStateGetBuffer(state, "diskName11",  hdName[3], sizeof(hdName[3]));
	saveStateClose(state);

	if(!insertMedia())
	{
		closeGameByFailedStateLoad();
		return STATE_RESULT_OTHER_ERROR;
	}

	boardInfo.loadState();
	saveStateDestroy();
	emuVideo.initImage(0, msxResX, msxResY);
	return STATE_RESULT_OK;
}

int EmuSystem::loadState(int saveStateSlot)
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	if(FsSys::fileExists(saveStr.data()))
	{
		return loadBlueMSXState(saveStr.data());
	}
	return STATE_RESULT_NO_FILE;
}

void EmuSystem::saveBackupMem()
{
	if(gameIsRunning())
	{
		// TODO: add BlueMSX API to flush volatile data
	}
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		auto saveStr = sprintStateFilename(-1);
		saveBlueMSXState(saveStr.data());
	}
}

bool EmuSystem::vidSysIsPAL() { return 0; }
uint EmuSystem::multiresVideoBaseX() { return 0; }
uint EmuSystem::multiresVideoBaseY() { return 0; }
bool touchControlsApplicable() { return 1; }

void EmuSystem::closeSystem()
{
	destroyMSX();
}

int EmuSystem::loadGame(const char *path)
{
	closeGame(1);
	emuVideo.initImage(0, msxResX, msxResY);
	setupGamePaths(path);

	if(!machine && !initMachine(optionMachineName)) // make sure machine is allocated
	{
		return 0;
	}

	// First, try to load auto-save which will initialize the MSX with stored parameters
	/*if(allowAutosaveState && optionAutoSaveState)
	{
		string_copyUpToLastCharInstance(gameName, path, '.');
		FsSys::PathString saveStr;
		sprintStateFilename(saveStr, -1);
		if(FsSys::fileExists(saveStr) && loadBlueMSXState(saveStr) == STATE_RESULT_OK)
		{
			logMsg("set game name: %s", gameName);
			logMsg("started emu from auto-save state");
			return 1;
		}
		strcpy(gameName, ""); // clear game name from save state failure
	}*/

	// No auto-save, initialize and boot the MSX with the selected media
	if(!initMachine(optionMachineName)) // make sure machine is set to user's selection
	{
		return 0;
	}

	if(!createBoardFromLoadGame())
	{
		return 0;
	}

	char fileInZipName[256] = "";
	if(string_hasDotExtension(path, "zip"))
	{
		if(getFirstROMFilenameInZip(path, fileInZipName, sizeof(fileInZipName)))
		{
			logMsg("found %s in zip", fileInZipName);
			strcpy(cartName[0], path);
			if(!boardChangeCartridge(0, ROM_UNKNOWN, path, fileInZipName))
			{
				destroyMSX();
				popup.postError("Error loading ROM");
				return 0;
			}
		}
		else if(getFirstDiskFilenameInZip(path, fileInZipName, sizeof(fileInZipName)))
		{
			logMsg("found %s in zip", fileInZipName);
			ZipIO fileInZip;
			fileInZip.open(path, fileInZipName);
			bool loadAsHD = fileInZip.size() >= 1024 * 1024;
			fileInZip.close();
			if(loadAsHD)
			{
				logMsg("load disk as HD");
				int hdId = diskGetHdDriveId(0, 0);
				strcpy(cartName[0], "Sunrise IDE");
				if(!boardChangeCartridge(0, ROM_SUNRISEIDE, "Sunrise IDE", 0))
				{
					destroyMSX();
					popup.postError("Error loading Sunrise IDE device");
					return 0;
				}
				strcpy(hdName[0], path);
				if(!diskChange(hdId, path, fileInZipName))
				{
					destroyMSX();
					popup.postError("Error loading HD");
					return 0;
				}
			}
			else
			{
				strcpy(diskName[0], path);
				if(!diskChange(0, path, fileInZipName))
				{
					destroyMSX();
					popup.postError("Error loading Disk");
					return 0;
				}
			}
		}
		else
		{
			destroyMSX();
			popup.postError("No Media in ZIP");
			return 0;
		}
	}
	else if(isROMExtension(path))
	{
		strcpy(cartName[0], path);// cartType[0] = 0;
		if(!boardChangeCartridge(0, ROM_UNKNOWN, path, 0))
		{
			destroyMSX();
			popup.postError("Error loading ROM");
			return 0;
		}
	}
	else if(isDiskExtension(path))
	{
		bool loadAsHD = FsSys::fileSize(path) >= 1024 * 1024;
		if(loadAsHD)
		{
			logMsg("load disk as HD");
			int hdId = diskGetHdDriveId(0, 0);
			strcpy(cartName[0], "Sunrise IDE");
			if(!boardChangeCartridge(0, ROM_SUNRISEIDE, "Sunrise IDE", 0))
			{
				destroyMSX();
				popup.postError("Error loading Sunrise IDE device");
				return 0;
			}
			strcpy(hdName[0], path);
			if(!diskChange(hdId, path, 0))
			{
				destroyMSX();
				popup.postError("Error loading HD");
				return 0;
			}
		}
		else
		{
			strcpy(diskName[0], path);
			if(!diskChange(0, path, 0))
			{
				destroyMSX();
				popup.postError("Error loading Disk");
				return 0;
			}
		}
	}
	else
	{
		bug_exit("unknown file extension used");
	}

	logMsg("started emu");
	return 1;
}

int EmuSystem::loadGameFromIO(IO &io, const char *origFilename)
{
	return 0; // TODO
}

void EmuSystem::clearInputBuffers()
{
	mem_zero(eventMap);
}

void EmuSystem::configAudioRate()
{
	pcmFormat.rate = 44100; // TODO: not all sound chips handle non-44100Hz sample rate
	uint rate = std::round(pcmFormat.rate * .998715);
	#if defined(CONFIG_ENV_WEBOS)
	if(optionFrameSkip != optionFrameSkipAuto)
		rate *= 42660./44100.; // better sync with Pre's refresh rate
	#endif
	mixerSetSampleRate(mixer, rate);
	logMsg("set mixer rate %d", (int)mixerGetSampleRate(mixer));
}

static Int32 soundWrite(void* dummy, Int16 *buffer, UInt32 count)
{
	//logMsg("called audio callback %d samples", count);
	bug_exit("should never be called");
	return 0;
}

static bool renderToScreen = 0;
static void commitVideoFrame()
{
	if(likely(renderToScreen))
	{
		updateAndDrawEmuVideo();
		renderToScreen = 0;
	}
}

void RefreshScreen(int screenMode)
{
	//logMsg("called RefreshScreen");
	commitVideoFrame();
	boardInfo.stop(boardInfo.cpuRef);
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	// fast-forward during floppy access, but stop if access ends
	if(unlikely(fdcActive && renderGfx))
	{
		iterateTimes(4, i)
		{
			boardInfo.run(boardInfo.cpuRef);
			((R800*)boardInfo.cpuRef)->terminate = 0;
			bool useFrame = !fdcActive;
			if(useFrame)
			{
				logMsg("FDC activity ended while fast-forwarding");
				renderToScreen = 1;
				commitVideoFrame();
			}
			mixerSync(mixer);
			UInt32 samples;
			uchar *audio = (uchar*)mixerGetBuffer(mixer, &samples);
			if(useFrame)
			{
				writeSound(audio, samples/2);
				return; // done with frame for this update
			}
		}
	}

	// regular frame update
	if(renderGfx)
		renderToScreen = 1;
	boardInfo.run(boardInfo.cpuRef);
	((R800*)boardInfo.cpuRef)->terminate = 0;
	mixerSync(mixer);
	UInt32 samples;
	uchar *audio = (uchar*)mixerGetBuffer(mixer, &samples);
	//logMsg("%d samples", samples/2);
	if(renderAudio && samples)
	{
		writeSound(audio, samples/2);
	}
}

void EmuSystem::savePathChanged() { }

bool EmuSystem::hasInputOptions() { return false; }

namespace Base
{

CallResult onInit(int argc, char** argv)
{
	/*mediaDbCreateRomdb();
	mediaDbAddFromXmlFile("msxromdb.xml");
	mediaDbAddFromXmlFile("msxsysromdb.xml");*/

	#ifdef CONFIG_BASE_IOS
	if(!Base::isSystemApp())
	{
		canInstallCBIOS = false;
	}
	#endif

	// must create the mixer first since mainInitCommon() will access it
	mixer = mixerCreate();
	assert(mixer);

	emuVideo.initPixmap((char*)&screenBuff[8 * msxResX], pixFmt, msxResX, msxResY);

	// Init general emu
	langInit();
	videoManagerReset();
	tapeSetReadOnly(1);
	mediaDbSetDefaultRomType(ROM_UNKNOWN);

	// Init Mixer
	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_PSG, 100);
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_PSG, 50/*40*/);
	mixerEnableChannelType(mixer, MIXER_CHANNEL_PSG, 1);

	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_SCC, 100);
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_SCC, 50/*60*/);
	mixerEnableChannelType(mixer, MIXER_CHANNEL_SCC, 1);

	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_MSXMUSIC, 95);
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_MSXMUSIC, 50/*60*/);
	mixerEnableChannelType(mixer, MIXER_CHANNEL_MSXMUSIC, 1);

	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_MSXAUDIO, 95);
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_MSXAUDIO, 50);
	mixerEnableChannelType(mixer, MIXER_CHANNEL_MSXAUDIO, 1);

	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_MOONSOUND, 95);
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_MOONSOUND, 50);
	mixerEnableChannelType(mixer, MIXER_CHANNEL_MOONSOUND, 1);

	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_YAMAHA_SFG, 95);
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_YAMAHA_SFG, 50);
	mixerEnableChannelType(mixer, MIXER_CHANNEL_YAMAHA_SFG, 1);

	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_PCM, 95);
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_PCM, 50);
	mixerEnableChannelType(mixer, MIXER_CHANNEL_PCM, 1);

	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_IO, 50);
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_IO, 50/*70*/);
	mixerEnableChannelType(mixer, MIXER_CHANNEL_IO, 0);

	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_MIDI, 90);
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_MIDI, 50);
	mixerEnableChannelType(mixer, MIXER_CHANNEL_MIDI, 1);

	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_KEYBOARD, 65);
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_KEYBOARD, 50/*55*/);
	mixerEnableChannelType(mixer, MIXER_CHANNEL_KEYBOARD, 1);

	//mixerSetMasterVolume(mixer, 100);
	mixerSetStereo(mixer, 1);
	mixerEnableMaster(mixer, 1);
	int logFrequency = 50;
	int frequency = (int)(3579545 * ::pow(2.0, (logFrequency - 50) / 15.0515));
	mixerSetBoardFrequencyFixed(frequency);
	mixerSetWriteCallback(mixer, 0, 0, 10000);

	static const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build((127./255.) * .4, (255./255.) * .4, (212./255.) * .4, 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build((127./255.) * .4, (255./255.) * .4, (212./255.) * .4, 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build((42./255.) * .4, (85./255.) * .4, (85./255.) * .4, 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};

	static MenuShownDelegate menuShown =
		[](Base::Window &win)
		{
			if(canInstallCBIOS && checkForMachineFolderOnStart &&
				!strlen(machineCustomPath.data()) && !FsSys::fileExists(machineBasePath.data())) // prompt to install if using default machine path & it doesn't exist
			{
				auto &ynAlertView = *new YesNoAlertView{win};
				ynAlertView.init(installFirmwareFilesMessage, Input::keyInputIsPresent());
				ynAlertView.onYes() =
					[](const Input::Event &e)
					{
						installFirmwareFiles();
					};
				modalViewController.pushAndShow(ynAlertView);
			}
		};

	mainInitCommon(argc, argv, navViewGrad, menuShown);
	return OK;
}

}
