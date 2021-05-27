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
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppInlines.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/EmuVideo.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/util/ScopeGuard.hh>
#include "internal.hh"

// TODO: remove when namespace code is complete
#ifdef __APPLE__
#define Fixed MacTypes_Fixed
#define Rect MacTypes_Rect
#include <MacTypes.h>
#undef Fixed
#undef Rect
#endif

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

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2021\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nBlueMSX Team\nbluemsx.com";
bool EmuSystem::handlesGenericIO = false; // TODO: need to re-factor BlueMSX file loading code
bool EmuSystem::hasResetModes = true;
BoardInfo boardInfo{};
Machine *machine{};
Mixer *mixer{};
bool canInstallCBIOS = true;
FS::PathString machineCustomPath{};
FS::PathString machineBasePath{};
FS::FileString cartName[2]{};
extern RomType currentRomType[2];
FS::FileString diskName[2]{};
static FS::FileString tapeName{};
FS::FileString hdName[4]{};
static EmuSystemTask *emuSysTask{};
static EmuVideo *emuVideo{};
static const char saveStateVersion[] = "blueMSX - state  v 8";
extern int pendingInt;

#if defined CONFIG_BASE_ANDROID || defined CONFIG_ENV_WEBOS || defined CONFIG_BASE_IOS || defined CONFIG_MACHINE_IS_PANDORA
static const bool checkForMachineFolderOnStart = true;
#else
static const bool checkForMachineFolderOnStart = false;
#endif

CLINK Int16 *mixerGetBuffer(Mixer* mixer, UInt32 *samplesOut);

FS::PathString makeMachineBasePath(Base::ApplicationContext app, FS::PathString customPath)
{
	FS::PathString outPath;
	if(!strlen(customPath.data()))
	{
		#if defined CONFIG_ENV_LINUX && !defined CONFIG_MACHINE_PANDORA
		string_printf(outPath, "%s/MSX.emu", EmuApp::assetPath(app).data());
		#else
		string_printf(outPath, "%s/MSX.emu", app.sharedStoragePath().data());
		#endif
	}
	else
	{
		string_printf(outPath, "%s", customPath.data());
	}
	logMsg("set machine file path: %s", outPath.data());
	return outPath;
}

const char *machineBasePathStr()
{
	return machineBasePath.data();
}

bool hasMSXTapeExtension(const char *name)
{
	return string_hasDotExtension(name, "cas");
}

bool hasMSXDiskExtension(const char *name)
{
	return string_hasDotExtension(name, "dsk");
}

bool hasMSXROMExtension(const char *name)
{
	return string_hasDotExtension(name, "rom") || string_hasDotExtension(name, "mx1")
			|| string_hasDotExtension(name, "mx2") || string_hasDotExtension(name, "col");
}

static bool hasMSXExtension(const char *name)
{
	return hasMSXROMExtension(name) || hasMSXDiskExtension(name);
}

const char *EmuSystem::shortSystemName()
{
	return "MSX";
}

const char *EmuSystem::systemName()
{
	return "MSX";
}

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter = hasMSXExtension;
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = hasMSXExtension;

static EmuSystem::Error insertMedia(EmuApp &app)
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
					return EmuSystem::makeError("Error loading Sunrise IDE device");
				}
			bdefault:
			{
				if(!strlen(cartName[i].data()))
					continue;
				logMsg("loading ROM %s", cartName[i].data());
				if(!insertROM(app, cartName[i].data(), i))
				{
					return EmuSystem::makeError("Error loading ROM%d:\n%s", i, cartName[i].data());
				}
			}
		}
	}

	iterateTimes(2, i)
	{
		if(!strlen(diskName[i].data()))
			continue;
		logMsg("loading Disk %s", diskName[i].data());
		if(!insertDisk(app, diskName[i].data(), i))
		{
			return EmuSystem::makeError("Error loading Disk%d:\n%s", i, diskName[i].data());
		}
	}

	iterateTimes(4, i)
	{
		if(!strlen(hdName[i].data()))
			continue;
		logMsg("loading HD %s", hdName[i].data());
		if(!insertDisk(app, hdName[i].data(), diskGetHdDriveId(i / 2, i % 2)))
		{
			return EmuSystem::makeError("Error loading Disk%d:\n%s", i, hdName[i].data());
		}
	}
	return {};
}

static bool msxIsInit()
{
	return boardInfo.run != 0;
}

static void clearAllMediaNames()
{
	cartName[0] = {};
	cartName[1] = {};
	diskName[0] = {};
	diskName[1] = {};
	hdName[0] = {};
	hdName[1] = {};
	hdName[2] = {};
	hdName[3] = {};
	tapeName = {};
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

static void destroyBoard(bool clearMediaNames = true)
{
	if(!machine)
		return;
	logMsg("destroying board");
	fdcActive = 0;
	if(msxIsInit())
	{
		ejectMedia();
		boardInfo.destroy();
	}
	boardInfo = {};
	if(clearMediaNames)
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

static bool createBoard(EmuApp &app)
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
			setupVKeyboardMap(app, BOARD_MSX);
			return msxCreate(machine, VDP_SYNC_60HZ, &boardInfo);
		case BOARD_COLECO:
			logMsg("creating Coleco");
			joystickPortSetType(0, JOYSTICK_PORT_COLECOJOYSTICK);
			joystickPortSetType(1, JOYSTICK_PORT_COLECOJOYSTICK);
			activeBoardType = BOARD_COLECO;
			setupVKeyboardMap(app, BOARD_COLECO);
			return colecoCreate(machine, VDP_SYNC_60HZ, &boardInfo);
		default:
			logErr("error: unknown board type 0x%X", machine->board.type);
			return 0;
	}
}

static bool createBoardFromLoadGame(EmuApp &app)
{
	destroyBoard(false);
	if(!createBoard(app))
	{
		boardInfo = {};
		return false;
	}
	//logMsg("z80 freq %d, r800 %d", ((R800*)boardInfo.cpuRef)->frequencyZ80, ((R800*)boardInfo.cpuRef)->frequencyR800);
	logMsg("max carts %d, disks %d, tapes %d", boardInfo.cartridgeCount, boardInfo.diskdriveCount, boardInfo.casetteCount);
	return true;
}

static EmuSystem::Error makeMachineInitError(const char *machineName)
{
	return EmuSystem::makeError("Error loading machine files for\n\"%s\",\nmake sure they are in:\n%s", machineName, machineBasePath.data());
}

static bool initMachine(const char *machineName)
{
	if(machine && string_equal(machine->name, machineName))
	{
		return true;
	}
	logMsg("loading machine %s", machineName);
	if(machine)
		machineDestroy(machine);
	FS::current_path(machineBasePathStr());
	machine = machineCreate(machineName);
	FS::current_path(EmuSystem::savePath());
	if(!machine)
	{
		return false;
	}
	boardSetMachine(machine);
	return true;
}

static void destroyMachine(bool clearMediaNames = true)
{
	if(!machine)
		return;
	destroyBoard(clearMediaNames);
	machineDestroy(machine);
	machine = nullptr;
}

const char *currentMachineName()
{
	if(!machine)
		return "";
	return machine->name;
}

EmuSystem::Error setCurrentMachineName(EmuApp &app, const char *machineName, bool insertMediaFiles)
{
	if(machine && string_equal(machine->name, machineName))
	{
		logMsg("keeping current machine:%s", machine->name);
		return {};
	}
	if(!initMachine(machineName))
	{
		return makeMachineInitError(machineName);
	}
	if(!createBoardFromLoadGame(app))
	{
		return EmuSystem::makeError("Error initializing %s", machine->name);
	}
	if(insertMediaFiles)
		return insertMedia(app);
	else
		return {};
}

template<class MATCH_FUNC>
static FS::FileString getFirstFilenameInArchive(const char *zipPath, MATCH_FUNC nameMatch)
{
	std::error_code ec{};
	for(auto &entry : FS::ArchiveIterator{zipPath, ec})
	{
		if(entry.type() == FS::file_type::directory)
		{
			continue;
		}
		auto name = entry.name();
		logMsg("archive file entry:%s", entry.name());
		if(nameMatch(name))
		{
			return FS::makeFileString(name);
		}
	}
	if(ec)
	{
		logErr("error opening archive:%s", zipPath);
	}
	return {};
}

static FS::FileString getFirstROMFilenameInArchive(const char *zipPath)
{
	return getFirstFilenameInArchive(zipPath, hasMSXROMExtension);
}

static FS::FileString getFirstDiskFilenameInArchive(const char *zipPath)
{
	return getFirstFilenameInArchive(zipPath, hasMSXDiskExtension);
}

static FS::FileString getFirstTapeFilenameInArchive(const char *zipPath)
{
	return getFirstFilenameInArchive(zipPath, hasMSXTapeExtension);
}

static FS::FileString getFirstMediaFilenameInArchive(const char *zipPath)
{
	return getFirstFilenameInArchive(zipPath, hasMSXExtension);
}

bool insertROM(EmuApp &app, const char *name, unsigned slot)
{
	assert(strlen(EmuSystem::gamePath()));
	auto path = FS::makePathString(EmuSystem::gamePath(), name);
	FS::FileString fileInZipName{};
	if(EmuApp::hasArchiveExtension(path.data()))
	{
		fileInZipName = getFirstROMFilenameInArchive(path.data());
		if(!strlen(fileInZipName.data()))
		{
			app.postMessage(true, "No ROM found in archive:%s", path.data());
			return false;
		}
		logMsg("found:%s in archive:%s", fileInZipName.data(), path.data());
	}
	if(!boardChangeCartridge(slot, ROM_UNKNOWN, path.data(), strlen(fileInZipName.data()) ? fileInZipName.data() : nullptr))
	{
		app.postMessage(true, "Error loading ROM");
		return false;
	}
	return true;
}

bool insertDisk(EmuApp &app, const char *name, unsigned slot)
{
	assert(strlen(EmuSystem::gamePath()));
	auto path = FS::makePathString(EmuSystem::gamePath(), name);
	FS::FileString fileInZipName{};
	if(EmuApp::hasArchiveExtension(path.data()))
	{
		fileInZipName = getFirstDiskFilenameInArchive(path.data());
		if(!strlen(fileInZipName.data()))
		{
			app.postMessage(true, "No disk found in archive:%s", path.data());
			return false;
		}
		logMsg("found:%s in archive:%s", fileInZipName.data(), path.data());
	}
	if(!diskChange(slot, path.data(), strlen(fileInZipName.data()) ? fileInZipName.data() : nullptr))
	{
		app.postMessage(true, "Error loading Disk");
		return false;
	}
	return true;
}

void EmuSystem::reset(EmuApp &app, ResetMode mode)
{
	assert(gameIsRunning());
	fdcActive = 0;
	if(mode == RESET_HARD)
	{
		boardInfo.destroy();
		if(!createBoard(app))
		{
			app.postMessage(true, "Error during MSX reset");
			app.exitGame(false);
		}
		if(auto err = insertMedia(app);
			err)
		{
			app.printfMessage(3, true, "%s", err->what());
		}
	}
	else
	{
		boardInfo.softReset();
	}
}

FS::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return FS::makePathStringPrintf("%s/%s.0%c.sta", statePath, gameName, saveSlotCharUpper(slot));
}

static EmuSystem::Error saveBlueMSXState(const char *filename)
{
	if(!zipStartWrite(filename))
	{
		logErr("error creating zip:%s", filename);
		return EmuSystem::makeFileWriteError();
	}
	saveStateCreateForWrite(filename);
	int rv = zipSaveFile(filename, "version", 0, saveStateVersion, sizeof(saveStateVersion));
	if (!rv)
	{
		saveStateDestroy();
		zipEndWrite();
		logErr("error writing to zip:%s", filename);
		return EmuSystem::makeFileWriteError();
	}

	SaveState* state = saveStateOpenForWrite("board");

	saveStateSet(state, "pendingInt", pendingInt);
	saveStateSet(state, "cartType00", currentRomType[0]);
	if(strlen(cartName[0].data()))
		saveStateSetBuffer(state, "cartName00",  cartName[0].data(), strlen(cartName[0].data()) + 1);
	saveStateSet(state, "cartType01", currentRomType[1]);
	if(strlen(cartName[1].data()))
		saveStateSetBuffer(state, "cartName01",  cartName[1].data(), strlen(cartName[1].data()) + 1);
	if(strlen(diskName[0].data()))
		saveStateSetBuffer(state, "diskName00",  diskName[0].data(), strlen(diskName[0].data()) + 1);
	if(strlen(diskName[1].data()))
		saveStateSetBuffer(state, "diskName01",  diskName[1].data(), strlen(diskName[1].data()) + 1);
	if(strlen(hdName[0].data()))
		saveStateSetBuffer(state, "diskName02",  hdName[0].data(), strlen(hdName[0].data()) + 1);
	if(strlen(hdName[1].data()))
		saveStateSetBuffer(state, "diskName03",  hdName[1].data(), strlen(hdName[1].data()) + 1);
	if(strlen(hdName[2].data()))
		saveStateSetBuffer(state, "diskName10",  hdName[2].data(), strlen(hdName[2].data()) + 1);
	if(strlen(hdName[3].data()))
		saveStateSetBuffer(state, "diskName11",  hdName[3].data(), strlen(hdName[3].data()) + 1);
	saveStateClose(state);

	machineSaveState(machine);
	boardInfo.saveState();
	saveStateDestroy();
	zipEndWrite();
	return {};
}

EmuSystem::Error EmuSystem::saveState(const char *path)
{
	return saveBlueMSXState(path);
}

static FS::FileString saveStateGetFileString(SaveState* state, const char* tagName)
{
	FS::FileString name{};
	saveStateGetBuffer(state, tagName,  name.data(), name.size());
	if(strlen(name.data()))
	{
		// strip any file path
		return FS::basename(name);
	}
	return name;
}

static EmuSystem::Error loadBlueMSXState(EmuApp &app, const char *filename)
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
		return EmuSystem::makeFileReadError();
	}
	if(0 != strncmp(version, saveStateVersion, sizeof(saveStateVersion) - 1))
	{
		free(version);
		saveStateDestroy();
		return EmuSystem::makeError("Incorrect state version");
	}
	free(version);

	machineLoadState(machine);

	// from this point on, errors are fatal and require the existing game to close
	if(!createBoardFromLoadGame(app))
	{
		saveStateDestroy();
		auto err = EmuSystem::makeError("Can't initialize machine:%s from save-state", machine->name);
		app.exitGame(false);
		return err;
	}

	clearAllMediaNames();
	SaveState* state = saveStateOpenForRead("board");
	currentRomType[0] = saveStateGet(state, "cartType00", 0);
	cartName[0] = saveStateGetFileString(state, "cartName00");
	currentRomType[1] = saveStateGet(state, "cartType01", 0);
	cartName[1] = saveStateGetFileString(state, "cartName01");
	diskName[0] = saveStateGetFileString(state, "diskName00");
	diskName[1] = saveStateGetFileString(state, "diskName01");
	hdName[0] = saveStateGetFileString(state, "diskName02");
	hdName[1] = saveStateGetFileString(state, "diskName03");
	hdName[2] = saveStateGetFileString(state, "diskName10");
	hdName[3] = saveStateGetFileString(state, "diskName11");
	saveStateClose(state);

	if(auto err = insertMedia(app);
		err)
	{
		app.exitGame(false);
		return err;
	}

	boardInfo.loadState();
	saveStateDestroy();
	logMsg("state loaded with machine:%s", machine->name);
	return {};
}

EmuSystem::Error EmuSystem::loadState(EmuApp &app, const char *path)
{
	return loadBlueMSXState(app, path);
}

void EmuSystem::saveBackupMem()
{
	if(gameIsRunning())
	{
		// TODO: add BlueMSX API to flush volatile data
	}
}

void EmuSystem::closeSystem()
{
	destroyMachine();
}

EmuSystem::Error EmuSystem::loadGame(Base::ApplicationContext ctx, IO &, EmuSystemCreateParams, OnLoadProgressDelegate)
{
	// configure media loading
	auto mediaPath = fullGamePath();
	auto mediaFilename = gameFileName();
	FS::FileString fileInZipName{};
	const char *fileInZipNamePtr{};
	const char *mediaNamePtr = mediaFilename.data();
	bool loadDiskAsHD = false;
	if(EmuApp::hasArchiveExtension(mediaFilename.data()))
	{
		fileInZipName = getFirstMediaFilenameInArchive(mediaPath);
		if(!strlen(fileInZipName.data()))
		{
			return EmuSystem::makeError("No media in archive");
		}
		logMsg("found:%s in archive:%s", fileInZipName.data(), mediaPath);
		fileInZipNamePtr = fileInZipName.data();
		mediaNamePtr = fileInZipNamePtr;
		if(hasMSXDiskExtension(fileInZipNamePtr))
		{
			auto fileInZip = FS::fileFromArchive(mediaPath, fileInZipName.data());
			loadDiskAsHD = fileInZip.size() >= 1024 * 1024;
		}
	}
	else if(hasMSXDiskExtension(mediaFilename.data()))
	{
		loadDiskAsHD = FS::file_size(mediaPath) >= 1024 * 1024;
		if(loadDiskAsHD)
			logMsg("loading disk image as HD");
	}

	// create machine
	auto &app = EmuApp::get(ctx);
	if(strlen(optionMachineName.val)) // try machine from session config first
	{
		if(auto err = setCurrentMachineName(app, optionMachineName.val, false);
		err)
		{
			destroyMachine();
		}
	}
	auto destroyMachineOnReturn = IG::scopeGuard([](){ destroyMachine(); });
	if(!strlen(currentMachineName()))
	{
		if(auto err = setCurrentMachineName(app, optionDefaultMachineName.val, false);
			err)
		{
			return err;
		}
	}

	// load media
	if(hasMSXROMExtension(mediaNamePtr))
	{
		cartName[0] = mediaFilename;
		if(!boardChangeCartridge(0, ROM_UNKNOWN, mediaPath, fileInZipNamePtr))
		{
			return EmuSystem::makeError("Error loading ROM");
		}
	}
	else if(hasMSXDiskExtension(mediaNamePtr))
	{
		if(loadDiskAsHD)
		{
			int hdId = diskGetHdDriveId(0, 0);
			string_copy(cartName[0], "Sunrise IDE");
			if(!boardChangeCartridge(0, ROM_SUNRISEIDE, "Sunrise IDE", 0))
			{
				return EmuSystem::makeError("Error loading Sunrise IDE device");
			}
			hdName[0] = mediaFilename;
			if(!diskChange(hdId, mediaPath, fileInZipNamePtr))
			{
				return EmuSystem::makeError("Error loading HD");
			}
		}
		else
		{
			diskName[0] = mediaFilename;
			if(!diskChange(0, mediaPath, fileInZipNamePtr))
			{
				return EmuSystem::makeError("Error loading Disk");
			}
		}
	}
	else
	{
		return EmuSystem::makeError("Unknown file type");
	}
	destroyMachineOnReturn.cancel();
	return {};
}

void EmuSystem::configAudioRate(IG::FloatSeconds frameTime, uint32_t rate)
{
	assumeExpr(rate == 44100);// TODO: not all sound chips handle non-44100Hz sample rate
	unsigned mixRate = std::round(rate * (59.924 * frameTime.count()));
	mixerSetSampleRate(mixer, mixRate);
	logMsg("set mixer rate %d", (int)mixerGetSampleRate(mixer));
}

static Int32 soundWrite(void *audio, Int16 *buffer, UInt32 samples)
{
	static_cast<EmuAudio*>(audio)->writeFrames(buffer, samples / 2);
	return 0;
}

static void commitUnchangedVideoFrame()
{
	if(emuVideo)
	{
		emuVideo->startUnchangedFrame(emuSysTask);
		emuVideo = {};
		emuSysTask = {};
	}
}

void EmuSystem::renderFramebuffer(EmuVideo &video)
{
	video.startFrameWithAltFormat({}, frameBufferPixmap());
}

void RefreshScreen(int screenMode)
{
	//logMsg("called RefreshScreen");
	if(emuVideo) [[likely]]
	{
		emuVideo->startFrameWithAltFormat(emuSysTask, frameBufferPixmap());
		emuVideo = {};
		emuSysTask = {};
	}
	boardInfo.stop(boardInfo.cpuRef);
}

void EmuSystem::runFrame(EmuSystemTask *task, EmuVideo *video, EmuAudio *audio)
{
	emuSysTask = task;
	emuVideo = video;
	mixerSetWriteCallback(mixer, audio ? soundWrite : nullptr, audio, 0);
	boardInfo.run(boardInfo.cpuRef);
	((R800*)boardInfo.cpuRef)->terminate = 0;
	commitUnchangedVideoFrame(); // runs if emuVideo wasn't unset in emulation of this frame
}

bool EmuSystem::shouldFastForward()
{
	// fast-forward during floppy access
	return fdcActive;
}

void EmuApp::onCustomizeNavView(EmuApp::NavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build((127./255.) * .4, (255./255.) * .4, (212./255.) * .4, 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build((127./255.) * .4, (255./255.) * .4, (212./255.) * .4, 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build((42./255.) * .4, (85./255.) * .4, (85./255.) * .4, 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};
	view.setBackgroundGradient(navViewGrad);
}

void EmuApp::onMainWindowCreated(ViewAttachParams attach, Input::Event e)
{
	if(canInstallCBIOS && checkForMachineFolderOnStart &&
		!strlen(machineCustomPath.data()) && !FS::exists(machineBasePath)) // prompt to install if using default machine path & it doesn't exist
	{
		pushAndShowNewYesNoAlertView(attach, e,
			installFirmwareFilesMessage,
			"Yes", "No",
			[](View &v)
			{
				installFirmwareFiles(v.appContext());
			}, nullptr);
	}
};

EmuSystem::Error EmuSystem::onInit(Base::ApplicationContext)
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

	// Init general emu
	langInit();
	videoManagerReset();
	tapeSetReadOnly(1);
	mediaDbSetDefaultRomType(ROM_UNKNOWN);

	// Init Mixer
	mixerSetMasterVolume(mixer, 100);
	mixerSetStereo(mixer, 1);
	mixerEnableMaster(mixer, 1);
	int logFrequency = 50;
	int frequency = (int)(3579545 * ::pow(2.0, (logFrequency - 50) / 15.0515));
	mixerSetBoardFrequencyFixed(frequency);

	return {};
}
