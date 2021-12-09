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
#include <imagine/fs/FS.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/format.hh>
#include <imagine/util/string.h>
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
FS::FileString cartName[2]{};
extern RomType currentRomType[2];
FS::FileString diskName[2]{};
static FS::FileString tapeName{};
FS::FileString hdName[4]{};
static EmuSystemTaskContext emuSysTask{};
static EmuVideo *emuVideo{};
static const char saveStateVersion[] = "blueMSX - state  v 8";
extern int pendingInt;
Base::ApplicationContext appCtx{};

#if defined CONFIG_BASE_ANDROID || defined CONFIG_ENV_WEBOS || defined CONFIG_BASE_IOS || defined CONFIG_MACHINE_IS_PANDORA
static const bool checkForMachineFolderOnStart = true;
#else
static const bool checkForMachineFolderOnStart = false;
#endif

CLINK Int16 *mixerGetBuffer(Mixer* mixer, UInt32 *samplesOut);

FS::PathString machineBasePath(Base::ApplicationContext app)
{
	if(EmuSystem::firmwarePath().empty())
	{
		#if defined CONFIG_ENV_LINUX && !defined CONFIG_MACHINE_PANDORA
		return IG::format<FS::PathString>("{}/MSX.emu", EmuApp::assetPath(app));
		#else
		return IG::format<FS::PathString>("{}/MSX.emu", app.sharedStoragePath());
		#endif
	}
	else
	{
		return EmuSystem::firmwarePath();
	}
}

bool hasMSXTapeExtension(std::string_view name)
{
	return name.ends_with(".cas");
}

bool hasMSXDiskExtension(std::string_view name)
{
	return name.ends_with(".dsk");
}

bool hasMSXROMExtension(std::string_view name)
{
	return IG::stringEndsWithAny(name, ".rom", ".mx1", ".mx2", ".col");
}

static bool hasMSXExtension(std::string_view name)
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

static void insertMedia(EmuApp &app)
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
					throw std::runtime_error("Error loading Sunrise IDE device");
				}
			bdefault:
			{
				if(cartName[i].empty())
					continue;
				logMsg("loading ROM %s", cartName[i].data());
				if(!insertROM(app, cartName[i].data(), i))
				{
					throw std::runtime_error(fmt::format("Error loading ROM{}:\n{}", i, cartName[i]));
				}
			}
		}
	}

	iterateTimes(2, i)
	{
		if(diskName[i].empty())
			continue;
		logMsg("loading Disk %s", diskName[i].data());
		if(!insertDisk(app, diskName[i].data(), i))
		{
			throw std::runtime_error(fmt::format("Error loading Disk{}:\n{}", i, diskName[i]));
		}
	}

	iterateTimes(4, i)
	{
		if(hdName[i].empty())
			continue;
		logMsg("loading HD %s", hdName[i].data());
		if(!insertDisk(app, hdName[i].data(), diskGetHdDriveId(i / 2, i % 2)))
		{
			throw std::runtime_error(fmt::format("Error loading Disk{}:\n{}", i, hdName[i]));
		}
	}
}

static bool msxIsInit()
{
	return boardInfo.run != 0;
}

static void clearAllMediaNames()
{
	cartName[0].clear();
	cartName[1].clear();
	diskName[0].clear();
	diskName[1].clear();
	hdName[0].clear();
	hdName[1].clear();
	hdName[2].clear();
	hdName[3].clear();
	tapeName.clear();
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

static void throwMachineInitError(std::string_view machineName)
{
	throw std::runtime_error(fmt::format("Error loading machine files for\n\"{}\",\nmake sure they are in:\n{}",
		machineName, EmuSystem::firmwarePath()));
}

static bool initMachine(std::string_view machineName)
{
	if(machine && machine->name == machineName)
	{
		return true;
	}
	logMsg("loading machine %s", machineName.data());
	if(machine)
		machineDestroy(machine);
	FS::current_path(EmuSystem::firmwarePath());
	machine = machineCreate(machineName.data());
	FS::current_path(EmuSystem::contentSavePath());
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

void setCurrentMachineName(EmuApp &app, std::string_view machineName, bool insertMediaFiles)
{
	if(machine && machine->name == machineName)
	{
		logMsg("keeping current machine:%s", machine->name);
		return;
	}
	if(!initMachine(machineName))
	{
		throwMachineInitError(machineName);
	}
	if(!createBoardFromLoadGame(app))
	{
		throw std::runtime_error(fmt::format("Error initializing {}", machine->name));
	}
	if(insertMediaFiles)
		insertMedia(app);
}

static FS::FileString getFirstFilenameInArchive(Base::ApplicationContext ctx, IG::CStringView zipPath, auto nameMatch)
{
	try
	{
		for(auto &entry : FS::ArchiveIterator{ctx.openFileUri(zipPath)})
		{
			if(entry.type() == FS::file_type::directory)
			{
				continue;
			}
			auto name = entry.name();
			logMsg("archive file entry:%s", entry.name().data());
			if(nameMatch(name))
			{
				return FS::FileString{name};
			}
		}
	}
	catch(...)
	{
		logErr("error opening archive:%s", zipPath.data());
	}
	return {};
}

static FS::FileString getFirstROMFilenameInArchive(Base::ApplicationContext ctx, IG::CStringView zipPath)
{
	return getFirstFilenameInArchive(ctx, zipPath, hasMSXROMExtension);
}

static FS::FileString getFirstDiskFilenameInArchive(Base::ApplicationContext ctx, IG::CStringView zipPath)
{
	return getFirstFilenameInArchive(ctx, zipPath, hasMSXDiskExtension);
}

static FS::FileString getFirstTapeFilenameInArchive(Base::ApplicationContext ctx, IG::CStringView zipPath)
{
	return getFirstFilenameInArchive(ctx, zipPath, hasMSXTapeExtension);
}

static FS::FileString getFirstMediaFilenameInArchive(Base::ApplicationContext ctx, IG::CStringView zipPath)
{
	return getFirstFilenameInArchive(ctx, zipPath, hasMSXExtension);
}

bool insertROM(EmuApp &app, const char *name, unsigned slot)
{
	assert(EmuSystem::contentDirectory().size());
	auto path = FS::pathString(EmuSystem::contentDirectory(), name);
	FS::FileString fileInZipName{};
	if(EmuApp::hasArchiveExtension(path))
	{
		fileInZipName = getFirstROMFilenameInArchive(app.appContext(), path);
		if(fileInZipName.empty())
		{
			app.postMessage(true, "No ROM found in archive:%s", path);
			return false;
		}
		logMsg("found:%s in archive:%s", fileInZipName.data(), path.data());
	}
	if(!boardChangeCartridge(slot, ROM_UNKNOWN, path.data(), fileInZipName.size() ? fileInZipName.data() : nullptr))
	{
		app.postMessage(true, "Error loading ROM");
		return false;
	}
	return true;
}

bool insertDisk(EmuApp &app, const char *name, unsigned slot)
{
	assert(EmuSystem::contentDirectory().size());
	auto path = FS::pathString(EmuSystem::contentDirectory(), name);
	FS::FileString fileInZipName{};
	if(EmuApp::hasArchiveExtension(path))
	{
		fileInZipName = getFirstDiskFilenameInArchive(app.appContext(), path);
		if(fileInZipName.empty())
		{
			app.postMessage(true, "No disk found in archive:%s", path);
			return false;
		}
		logMsg("found:%s in archive:%s", fileInZipName.data(), path.data());
	}
	if(!diskChange(slot, path.data(), fileInZipName.size() ? fileInZipName.data() : nullptr))
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
		try
		{
			insertMedia(app);
		}
		catch(std::exception &err)
		{
			app.postErrorMessage(3, err.what());
		}
	}
	else
	{
		boardInfo.softReset();
	}
}

FS::FileString EmuSystem::stateFilename(int slot, std::string_view name)
{
	return IG::format<FS::FileString>("{}.0{}.sta", name, saveSlotCharUpper(slot));
}

static void saveBlueMSXState(const char *filename)
{
	if(!zipStartWrite(filename))
	{
		logErr("error creating zip:%s", filename);
		EmuSystem::throwFileWriteError();
	}
	saveStateCreateForWrite(filename);
	int rv = zipSaveFile(filename, "version", 0, saveStateVersion, sizeof(saveStateVersion));
	if (!rv)
	{
		saveStateDestroy();
		zipEndWrite();
		logErr("error writing to zip:%s", filename);
		EmuSystem::throwFileWriteError();
	}

	SaveState* state = saveStateOpenForWrite("board");

	saveStateSet(state, "pendingInt", pendingInt);
	saveStateSet(state, "cartType00", currentRomType[0]);
	if(cartName[0].size())
		saveStateSetBuffer(state, "cartName00",  cartName[0].data(), cartName[0].size() + 1);
	saveStateSet(state, "cartType01", currentRomType[1]);
	if(cartName[1].size())
		saveStateSetBuffer(state, "cartName01",  cartName[1].data(), cartName[1].size() + 1);
	if(diskName[0].size())
		saveStateSetBuffer(state, "diskName00",  diskName[0].data(), diskName[0].size() + 1);
	if(diskName[1].size())
		saveStateSetBuffer(state, "diskName01",  diskName[1].data(), diskName[1].size() + 1);
	if(hdName[0].size())
		saveStateSetBuffer(state, "diskName02",  hdName[0].data(), hdName[0].size() + 1);
	if(hdName[1].size())
		saveStateSetBuffer(state, "diskName03",  hdName[1].data(), hdName[1].size() + 1);
	if(hdName[2].size())
		saveStateSetBuffer(state, "diskName10",  hdName[2].data(), hdName[2].size() + 1);
	if(hdName[3].size())
		saveStateSetBuffer(state, "diskName11",  hdName[3].data(), hdName[3].size() + 1);
	saveStateClose(state);

	machineSaveState(machine);
	boardInfo.saveState();
	saveStateDestroy();
	zipEndWrite();
}

void EmuSystem::saveState(Base::ApplicationContext ctx, IG::CStringView path)
{
	return saveBlueMSXState(path);
}

static FS::FileString saveStateGetFileString(SaveState* state, const char* tagName)
{
	FS::FileStringArray name{};
	saveStateGetBuffer(state, tagName,  name.data(), name.size());
	if(std::string_view nameView{name.data()};
		nameView.size())
	{
		// strip any file path
		return FS::basename(nameView);
	}
	return {};
}

static void loadBlueMSXState(EmuApp &app, const char *filename)
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
		EmuSystem::throwFileReadError();
	}
	if(0 != strncmp(version, saveStateVersion, sizeof(saveStateVersion) - 1))
	{
		free(version);
		saveStateDestroy();
		throw std::runtime_error("Incorrect state version");
	}
	free(version);

	machineLoadState(machine);

	// from this point on, errors are fatal and require the existing game to close
	if(!createBoardFromLoadGame(app))
	{
		saveStateDestroy();
		auto err = fmt::format("Can't initialize machine:{} from save-state", machine->name);
		app.exitGame(false);
		throw std::runtime_error{err};
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

	try
	{
		insertMedia(app);
	}
	catch(...)
	{
		app.exitGame(false);
		throw;
	}

	boardInfo.loadState();
	saveStateDestroy();
	logMsg("state loaded with machine:%s", machine->name);
}

void EmuSystem::loadState(EmuApp &app, IG::CStringView path)
{
	return loadBlueMSXState(app, path);
}

void EmuSystem::saveBackupMem(Base::ApplicationContext ctx)
{
	if(gameIsRunning())
	{
		// TODO: add BlueMSX API to flush volatile data
	}
}

void EmuSystem::closeSystem(Base::ApplicationContext)
{
	destroyMachine();
}

void EmuSystem::loadGame(Base::ApplicationContext ctx, IO &, EmuSystemCreateParams, OnLoadProgressDelegate)
{
	// configure media loading
	auto mediaPath = contentLocation();
	auto mediaFilename = contentFileName();
	FS::FileString fileInZipName{};
	const char *fileInZipNamePtr{};
	const char *mediaNamePtr = mediaFilename.data();
	bool loadDiskAsHD = false;
	if(EmuApp::hasArchiveExtension(mediaFilename))
	{
		fileInZipName = getFirstMediaFilenameInArchive(ctx, mediaPath);
		if(fileInZipName.empty())
		{
			throw std::runtime_error("No media in archive");
		}
		logMsg("found:%s in archive:%s", fileInZipName.data(), mediaPath.data());
		fileInZipNamePtr = fileInZipName.data();
		mediaNamePtr = fileInZipNamePtr;
		if(hasMSXDiskExtension(fileInZipName))
		{
			auto fileInZip = FS::fileFromArchive(ctx.openFileUri(mediaPath), fileInZipName);
			loadDiskAsHD = fileInZip.size() >= 1024 * 1024;
		}
	}
	else if(hasMSXDiskExtension(mediaFilename))
	{
		loadDiskAsHD = FS::file_size(mediaPath) >= 1024 * 1024;
		if(loadDiskAsHD)
			logMsg("loading disk image as HD");
	}

	// create machine
	auto &app = EmuApp::get(ctx);
	auto destroyMachineOnReturn = IG::scopeGuard([](){ destroyMachine(); });
	if(optionSessionMachineNameStr.size()) // try machine from session config first
	{
		setCurrentMachineName(app, optionSessionMachineNameStr, false);
	}
	if(!strlen(currentMachineName()))
	{
		setCurrentMachineName(app, optionDefaultMachineNameStr, false);
	}

	// load media
	if(hasMSXROMExtension(mediaNamePtr))
	{
		cartName[0] = mediaFilename;
		if(!boardChangeCartridge(0, ROM_UNKNOWN, mediaPath.data(), fileInZipNamePtr))
		{
			throw std::runtime_error("Error loading ROM");
		}
	}
	else if(hasMSXDiskExtension(mediaNamePtr))
	{
		if(loadDiskAsHD)
		{
			int hdId = diskGetHdDriveId(0, 0);
			cartName[0] = "Sunrise IDE";
			if(!boardChangeCartridge(0, ROM_SUNRISEIDE, "Sunrise IDE", 0))
			{
				throw std::runtime_error("Error loading Sunrise IDE device");
			}
			hdName[0] = mediaFilename;
			if(!diskChange(hdId, mediaPath.data(), fileInZipNamePtr))
			{
				throw std::runtime_error("Error loading HD");
			}
		}
		else
		{
			diskName[0] = mediaFilename;
			if(!diskChange(0, mediaPath.data(), fileInZipNamePtr))
			{
				throw std::runtime_error("Error loading Disk");
			}
		}
	}
	else
	{
		throw std::runtime_error("Unknown file type");
	}
	destroyMachineOnReturn.cancel();
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

void EmuSystem::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
{
	emuSysTask = taskCtx;
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

void EmuSystem::onInit(Base::ApplicationContext ctx)
{
	/*mediaDbCreateRomdb();
	mediaDbAddFromXmlFile("msxromdb.xml");
	mediaDbAddFromXmlFile("msxsysromdb.xml");*/

	appCtx = ctx;

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
}
