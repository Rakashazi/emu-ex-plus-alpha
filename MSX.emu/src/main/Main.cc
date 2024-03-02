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
#include <emuframework/EmuAppInlines.hh>
#include <emuframework/EmuSystemInlines.hh>
#include <imagine/fs/FS.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/format.hh>
#include <imagine/util/string.h>
#include <imagine/logger/logger.h>

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

extern int pendingInt;
extern RomType currentRomType[2];
Machine *machine{};
IG::FS::FileString hdName[4]{};

namespace EmuEx
{

constexpr SystemLogger log{"MSX.emu"};
const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2022\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nBlueMSX Team\nbluemsx.com";
bool EmuSystem::handlesGenericIO = false; // TODO: need to re-factor BlueMSX file loading code
bool EmuSystem::hasResetModes = true;
bool EmuSystem::canRenderRGBA8888 = false;
bool EmuSystem::hasRectangularPixels = true;
bool EmuApp::needsGlobalInstance = true;
BoardInfo boardInfo{};
Mixer *mixer{};
static FS::FileString tapeName{};
static EmuSystemTaskContext emuSysTask{};
static EmuVideo *emuVideo{};
static const char saveStateVersion[] = "blueMSX - state  v 8";

CLINK Int16 *mixerGetBuffer(Mixer* mixer, UInt32 *samplesOut);

MsxApp::MsxApp(ApplicationInitParams initParams, ApplicationContext &ctx):
	EmuApp{initParams, ctx}, msxSystem{ctx} {}

bool hasColecoROMExtension(std::string_view name)
{
	return IG::endsWithAnyCaseless(name, ".col");
}

bool hasMSXTapeExtension(std::string_view name)
{
	return IG::endsWithAnyCaseless(name, ".cas");
}

bool hasMSXDiskExtension(std::string_view name)
{
	return IG::endsWithAnyCaseless(name, ".dsk");
}

bool hasMSXROMExtension(std::string_view name)
{
	return IG::endsWithAnyCaseless(name, ".rom", ".mx1", ".mx2", ".col");
}

static bool hasMSXExtension(std::string_view name)
{
	return hasMSXROMExtension(name) || hasMSXDiskExtension(name);
}

const char *EmuSystem::shortSystemName() const
{
	return "MSX";
}

const char *EmuSystem::systemName() const
{
	return "MSX";
}

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter = hasMSXExtension;

void MsxSystem::insertMedia(EmuApp &app)
{
	for(auto i : iotaCount(2))
	{
		switch(currentRomType[i])
		{
			case ROM_SCC:
				log.info("loading SCC");
				boardChangeCartridge(i, ROM_SCC, "", 0);
				break;
			case ROM_SCCPLUS:
				log.info("loading SCC+");
				boardChangeCartridge(i, ROM_SCCPLUS, "", 0);
				break;
			case ROM_SUNRISEIDE:
				log.info("loading Sunrise IDE");
				if(!boardChangeCartridge(i, ROM_SUNRISEIDE, "Sunrise IDE", 0))
				{
					throw std::runtime_error("Error loading Sunrise IDE device");
				}
				break;
			default:
			{
				if(cartName[i].empty())
					continue;
				log.info("loading ROM:{}", cartName[i]);
				if(!insertROM(app, cartName[i].data(), i))
				{
					throw std::runtime_error(std::format("Error loading ROM{}:\n{}", i, cartName[i]));
				}
			}
		}
	}

	for(auto i : iotaCount(2))
	{
		if(diskName[i].empty())
			continue;
		log.info("loading Disk:{}", diskName[i]);
		if(!insertDisk(app, diskName[i].data(), i))
		{
			throw std::runtime_error(std::format("Error loading Disk{}:\n{}", i, diskName[i]));
		}
	}

	for(auto i: iotaCount(4))
	{
		if(hdName[i].empty())
			continue;
		log.info("loading HD:{}", hdName[i]);
		if(!insertDisk(app, hdName[i].data(), diskGetHdDriveId(i / 2, i % 2)))
		{
			throw std::runtime_error(std::format("Error loading HD{}:\n{}", i, hdName[i]));
		}
	}
}

static bool msxIsInit()
{
	return boardInfo.run != 0;
}

void MsxSystem::clearAllMediaNames()
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

void MsxSystem::destroyBoard(bool clearMediaNames)
{
	if(!machine)
		return;
	log.info("destroying board");
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

bool MsxSystem::createBoard(EmuApp &app)
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
			log.info("creating MSX");
			joystickPortSetType(0, JOYSTICK_PORT_JOYSTICK);
			joystickPortSetType(1, JOYSTICK_PORT_JOYSTICK);
			activeBoardType = BOARD_MSX;
			setupVKeyboardMap(app, BOARD_MSX);
			return msxCreate(machine, VDP_SYNC_60HZ, &boardInfo);
		case BOARD_COLECO:
			log.info("creating Coleco");
			joystickPortSetType(0, JOYSTICK_PORT_COLECOJOYSTICK);
			joystickPortSetType(1, JOYSTICK_PORT_COLECOJOYSTICK);
			activeBoardType = BOARD_COLECO;
			setupVKeyboardMap(app, BOARD_COLECO);
			return colecoCreate(machine, VDP_SYNC_60HZ, &boardInfo);
		default:
			log.error("error: unknown board type:{}", (int)machine->board.type);
			return 0;
	}
}

bool MsxSystem::createBoardFromLoadGame(EmuApp &app)
{
	destroyBoard(false);
	if(!createBoard(app))
	{
		boardInfo = {};
		return false;
	}
	//log.debug("z80 freq {}, r800 {}", ((R800*)boardInfo.cpuRef)->frequencyZ80, ((R800*)boardInfo.cpuRef)->frequencyR800);
	log.info("max carts:{}, disks:{}, tapes:{}", boardInfo.cartridgeCount, boardInfo.diskdriveCount, boardInfo.casetteCount);
	return true;
}

static void throwMachineInitError(std::string_view machineName)
{
	throw std::runtime_error(std::format("Error loading machine files for\n\"{}\",\nplease check Options➔File Paths➔BIOS",
		machineName));
}

static bool initMachine(std::string_view machineName)
{
	if(machine && machine->name == machineName)
	{
		return true;
	}
	log.info("loading machine:{}", machineName);
	if(machine)
		machineDestroy(machine);
	machine = machineCreate(machineName.data());
	if(!machine)
	{
		return false;
	}
	boardSetMachine(machine);
	return true;
}

void MsxSystem::destroyMachine(bool clearMediaNames)
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

void MsxSystem::setCurrentMachineName(EmuApp &app, std::string_view machineName, bool insertMediaFiles)
{
	if(machine && machine->name == machineName)
	{
		log.info("keeping current machine:{}", machine->name);
		return;
	}
	if(!initMachine(machineName))
	{
		throwMachineInitError(machineName);
	}
	if(!createBoardFromLoadGame(app))
	{
		throw std::runtime_error(std::format("Error initializing {}", machine->name));
	}
	if(insertMediaFiles)
		insertMedia(app);
}

static ArchiveIO getFirstFileInArchive(IG::ApplicationContext ctx, IG::CStringView zipPath, auto nameMatch)
{
	try
	{
		return FS::findFileInArchive(ctx.openFileUri(zipPath), [&](auto &entry){ return nameMatch(entry.name()); });
	}
	catch(...)
	{
		log.error("error opening archive:{}", zipPath);
	}
	return {};
}

static ArchiveIO getFirstROMFileInArchive(IG::ApplicationContext ctx, IG::CStringView zipPath)
{
	return getFirstFileInArchive(ctx, zipPath, hasMSXROMExtension);
}

static ArchiveIO getFirstDiskFileInArchive(IG::ApplicationContext ctx, IG::CStringView zipPath)
{
	return getFirstFileInArchive(ctx, zipPath, hasMSXDiskExtension);
}

static ArchiveIO getFirstTapeFileInArchive(IG::ApplicationContext ctx, IG::CStringView zipPath)
{
	return getFirstFileInArchive(ctx, zipPath, hasMSXTapeExtension);
}

static ArchiveIO getFirstMediaFileInArchive(IG::ApplicationContext ctx, IG::CStringView zipPath)
{
	return getFirstFileInArchive(ctx, zipPath, hasMSXExtension);
}

bool insertROM(EmuApp &app, const char *name, unsigned slot)
{
	assert(app.system().contentDirectory().size());
	auto path = app.system().contentDirectory(name);
	FS::FileString fileInZipName{};
	if(EmuApp::hasArchiveExtension(path))
	{
		fileInZipName = getFirstROMFileInArchive(app.appContext(), path).name();
		if(fileInZipName.empty())
		{
			app.postMessage(true, "No ROM found in archive:%s", path);
			return false;
		}
		log.info("found:{} in archive:{}", fileInZipName, path);
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
	assert(app.system().contentDirectory().size());
	auto path = app.system().contentDirectory(name);
	FS::FileString fileInZipName{};
	if(EmuApp::hasArchiveExtension(path))
	{
		fileInZipName = getFirstDiskFileInArchive(app.appContext(), path).name();
		if(fileInZipName.empty())
		{
			app.postMessage(true, "No disk found in archive:%s", path);
			return false;
		}
		log.info("found:{} in archive:{}", fileInZipName, path);
	}
	if(!diskChange(slot, path.data(), fileInZipName.size() ? fileInZipName.data() : nullptr))
	{
		app.postMessage(true, "Error loading Disk");
		return false;
	}
	return true;
}

void MsxSystem::reset(EmuApp &app, ResetMode mode)
{
	assert(hasContent());
	fdcActive = 0;
	if(mode == ResetMode::HARD)
	{
		boardInfo.destroy();
		if(!createBoard(app))
		{
			app.postMessage(true, "Error during MSX reset");
			app.closeSystemWithoutSave();
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

FS::FileString MsxSystem::stateFilename(int slot, std::string_view name) const
{
	return IG::format<FS::FileString>("{}.0{}.sta", name, saveSlotCharUpper(slot));
}

void MsxSystem::saveBlueMSXState(const char *filename)
{
	if(!zipStartWrite(filename))
	{
		log.error("error creating zip:{}", filename);
		EmuSystem::throwFileWriteError();
	}
	saveStateCreateForWrite(filename);
	int rv = zipSaveFile(filename, "version", 0, saveStateVersion, sizeof(saveStateVersion));
	if (!rv)
	{
		saveStateDestroy();
		zipEndWrite();
		log.error("error writing to zip:{}", filename);
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

void MsxSystem::loadBlueMSXState(EmuApp &app, const char *filename)
{
	log.info("loading state:{}", filename);
	assert(machine);
	saveStateCreateForRead(filename);
	auto destroySaveState = IG::scopeGuard([](){ saveStateDestroy(); });
	int size;
	char *version = (char*)zipLoadFile(filename, "version", &size);
	if(!version)
	{
		EmuSystem::throwFileReadError();
	}
	if(0 != strncmp(version, saveStateVersion, sizeof(saveStateVersion) - 1))
	{
		free(version);
		throw std::runtime_error("Incorrect state version");
	}
	free(version);

	ejectMedia();
	machineLoadState(machine);

	// from this point on, errors are fatal and require the existing game to close
	if(!createBoardFromLoadGame(app))
	{
		auto err = std::format("Can't initialize machine: \"{}\" from state", machine->name);
		app.closeSystemWithoutSave();
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
		app.closeSystemWithoutSave();
		throw;
	}

	boardInfo.loadState();
	log.info("state loaded with machine:{}", machine->name);
}

void MsxSystem::readState(EmuApp &app, std::span<uint8_t> buff)
{
	setZipMemBuffer(buff);
	loadBlueMSXState(app, ":::B");
}

size_t MsxSystem::writeState(std::span<uint8_t> buff, SaveStateFlags flags)
{
	assert(buff.size() == stateSize());
	setZipMemBuffer(buff);
	saveBlueMSXState(":::B");
	return zipMemBufferSize();
}

void MsxSystem::closeSystem()
{
	destroyMachine();
}

void MsxSystem::loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate)
{
	if(contentDirectory().empty())
	{
		throwMissingContentDirError();
	}
	// configure media loading
	auto mediaPath = contentLocation();
	auto mediaFilename = contentFileName();
	FS::FileString fileInZipName{};
	const char *fileInZipNamePtr{};
	const char *mediaNamePtr = mediaFilename.data();
	bool loadDiskAsHD{};
	bool isColecoRom = hasColecoROMExtension(mediaFilename);
	auto ctx = appContext();
	if(EmuApp::hasArchiveExtension(mediaFilename))
	{
		auto fileInZip = getFirstMediaFileInArchive(ctx, mediaPath);
		if(!fileInZip)
		{
			throw std::runtime_error("No media in archive");
		}
		fileInZipName = fileInZip.name();
		log.info("found:{} in archive:{}", fileInZipName, mediaPath);
		fileInZipNamePtr = fileInZipName.data();
		mediaNamePtr = fileInZipNamePtr;
		if(hasMSXDiskExtension(fileInZipName))
		{
			loadDiskAsHD = fileInZip.size() >= 1024 * 1024;
		}
		else
		{
			isColecoRom = hasColecoROMExtension(fileInZipName);
		}
	}
	else if(hasMSXDiskExtension(mediaFilename))
	{
		loadDiskAsHD = ctx.openFileUri(mediaPath).size() >= 1024 * 1024;
		if(loadDiskAsHD)
			log.info("loading disk image as HD");
	}

	// create machine
	auto &app = EmuApp::get(ctx);
	auto destroyMachineOnReturn = IG::scopeGuard([&](){ destroyMachine(); });
	if(optionSessionMachineNameStr.size()) // try machine from session config first
	{
		setCurrentMachineName(app, optionSessionMachineNameStr, false);
	}
	if(!strlen(currentMachineName()))
	{
		if(isColecoRom)
			setCurrentMachineName(app, optionDefaultColecoMachineNameStr, false);
		else
			setCurrentMachineName(app, optionDefaultMachineNameStr, false);
		log.info("using default machine:{}", currentMachineName());
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

void MsxSystem::configAudioRate(FrameTime outputFrameTime, int outputRate)
{
	assumeExpr(outputRate == 44100);// TODO: not all sound chips handle non-44100Hz sample rate
	UInt32 mixRate = std::round(audioMixRate(outputRate, outputFrameTime));
	if(mixerGetSampleRate(mixer) == mixRate)
		return;
	log.info("set sound mix rate:{}", mixRate);
	mixerSetSampleRate(mixer, mixRate);
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

void MsxSystem::renderFramebuffer(EmuVideo &video)
{
	video.startFrameWithFormat({}, frameBufferPixmap());
}

void MsxSystem::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
{
	emuSysTask = taskCtx;
	emuVideo = video;
	mixerSetWriteCallback(mixer, audio ? soundWrite : nullptr, audio, 0);
	boardInfo.run(boardInfo.cpuRef);
	((R800*)boardInfo.cpuRef)->terminate = 0;
	commitUnchangedVideoFrame(); // runs if emuVideo wasn't unset in emulation of this frame
}

bool MsxSystem::shouldFastForward() const
{
	// fast-forward during floppy access
	return fdcActive;
}

void EmuApp::onCustomizeNavView(EmuApp::NavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::PackedColor::format.build((127./255.) * .4, (255./255.) * .4, (212./255.) * .4, 1.) },
		{ .3, Gfx::PackedColor::format.build((127./255.) * .4, (255./255.) * .4, (212./255.) * .4, 1.) },
		{ .97, Gfx::PackedColor::format.build((42./255.) * .4, (85./255.) * .4, (85./255.) * .4, 1.) },
		{ 1., view.separatorColor() },
	};
	view.setBackgroundGradient(navViewGrad);
}

}

void RefreshScreen(int screenMode)
{
	using namespace EmuEx;
	//log.debug("called RefreshScreen");
	if(emuVideo) [[likely]]
	{
		emuVideo->startFrameWithFormat(emuSysTask, frameBufferPixmap());
		emuVideo = {};
		emuSysTask = {};
	}
	boardInfo.stop(boardInfo.cpuRef);
}
