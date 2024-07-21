#define LOGTAG "main"
#include <imagine/logger/logger.h>
#include <imagine/fs/FS.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/io/IO.hh>
#include <imagine/util/format.hh>
#include <imagine/util/string.h>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuApp.hh>
#include "MainSystem.hh"
#include <sys/stat.h>
#include <zlib.h>
#ifndef SNES9X_VERSION_1_4
#include <apu/apu.h>
#include <controls.h>
#else
#include <apu.h>
#include <soundux.h>
#endif
#include <display.h>
#include <memmap.h>

using namespace EmuEx;

#ifndef SNES9X_VERSION_1_4
uint16 SSettings::DisplayColor{};
uint32 SSettings::SkipFrames{};
uint32 SSettings::TurboSkipFrames{};
bool8 SSettings::StopEmulation{};
std::string SGFX::InfoString;
uint32 SGFX::InfoStringTimeout{};
char SGFX::FrameDisplayString[256]{};
#else
static std::string globalPath;
#endif

void S9xMessage(int, int, const char *msg)
{
	if(msg)
		logMsg("%s", msg);
}

void S9xPrintf(const char* msg, ...)
{
	if(!logger_isEnabled())
		return;
	va_list args;
	va_start(args, msg);
	logger_vprintf(LOG_M, msg, args);
	va_end(args);
}

void S9xPrintfError(const char* msg, ...)
{
	if(!logger_isEnabled())
		return;
	va_list args;
	va_start(args, msg);
	logger_vprintf(LOG_E, msg, args);
	va_end(args);
}

#ifdef SNES9X_VERSION_1_4

enum s9x_getdirtype
{
	DEFAULT_DIR = 0,
	HOME_DIR,
	ROMFILENAME_DIR,
	ROM_DIR,
	SRAM_DIR,
	SNAPSHOT_DIR,
	SCREENSHOT_DIR,
	SPC_DIR,
	CHEAT_DIR,
	PATCH_DIR,
	BIOS_DIR,
	LOG_DIR,
	SAT_DIR,
	LAST_DIR
};

/*bool8 S9xOpenSoundDevice(int mode, bool8 stereo, int buffer_size)
{
	return TRUE;
}*/

extern "C" void S9xLoadSDD1Data()
{
    Memory.FreeSDD1Data();
	Settings.SDD1Pack = TRUE;
}

const char *S9xGetFilenameInc(const char *e)
{
	assert(0); // not used yet
	return 0;
}

const char *S9xGetSnapshotDirectory(const char *name)
{
	globalPath = EmuEx::gSystem().contentSaveFilePath(name);
	return globalPath.c_str();
}

extern "C" char* osd_GetPackDir()
{
	auto &sys = EmuEx::gSystem();
	if(!strncmp((char*)&Memory.ROM [0xffc0], "SUPER POWER LEAG 4   ", 21))
	{
		globalPath = sys.contentSaveFilePath("SPL4-SP7");
	}
	else if(!strncmp((char*)&Memory.ROM [0xffc0], "MOMOTETSU HAPPY      ",21))
	{
		globalPath = sys.contentSaveFilePath("SMHT-SP7");
	}
	else if(!strncmp((char*)&Memory.ROM [0xffc0], "HU TENGAI MAKYO ZERO ", 21))
	{
		globalPath = sys.contentSaveFilePath("FEOEZSP7");
	}
	else if(!strncmp((char*)&Memory.ROM [0xffc0], "JUMP TENGAIMAKYO ZERO",21))
	{
		globalPath = sys.contentSaveFilePath("SJUMPSP7");
	}
	else
	{
		globalPath = sys.contentSaveFilePath("MISC-SP7");
	}
	return globalPath.data();
}

static s9x_getdirtype toDirType(std::string_view ext)
{
	if(ext == ".cht")
		return CHEAT_DIR;
	else if(ext == ".ips")
		return PATCH_DIR;
	else
		return SRAM_DIR;
}

const char *S9xGetFilename(const char *ex)
{
	auto &sys = static_cast<Snes9xSystem&>(EmuEx::gSystem());
	s9x_getdirtype dirtype = toDirType(ex);
	if(dirtype == ROMFILENAME_DIR)
		globalPath = sys.contentFilePath(ex);
	else if(dirtype == CHEAT_DIR)
		globalPath = sys.userFilePath(sys.cheatsDir, ex);
	else if(dirtype == PATCH_DIR)
		globalPath = sys.userFilePath(sys.patchesDir, ex);
	else if(dirtype == SAT_DIR)
		globalPath = sys.userFilePath(sys.satDir, ex);
	else
		globalPath = sys.contentSaveFilePath(ex);
	//logMsg("built s9x path:%s", globalPath.c_str());
	return globalPath.c_str();
}

const char *S9xBasename(const char *f)
{
	const char	*p;

	if ((p = strrchr(f, '/')) != NULL || (p = strrchr(f, '\\')) != NULL)
		return (p + 1);

	return (f);
}

#else

void S9xHandlePortCommand(s9xcommand_t, int16, int16) {}
bool8 S9xOpenSoundDevice() { return TRUE; }
const char *S9xGetCrosshair(int) { return nullptr; }

std::string S9xGetFilenameInc(std::string_view, enum s9x_getdirtype)
{
	logErr("S9xGetFilenameInc not used yet");
	return {};
}

std::string S9xGetFilename(std::string_view ex, enum s9x_getdirtype dirtype)
{
	auto &sys = static_cast<Snes9xSystem&>(EmuEx::gSystem());
	if(dirtype == ROMFILENAME_DIR)
		return std::string{sys.contentFilePath(ex)};
	else if(dirtype == CHEAT_DIR)
		return std::string{sys.userFilePath(sys.cheatsDir, ex)};
	else if(dirtype == PATCH_DIR)
		return std::string{sys.userFilePath(sys.patchesDir, ex)};
	else if(dirtype == SAT_DIR)
		return std::string{sys.userFilePath(sys.satDir, ex)};
	else
		return std::string{sys.contentSaveFilePath(ex)};
}

std::string S9xGetFilename(std::string_view, std::string_view ex, enum s9x_getdirtype dirtype)
{
	return S9xGetFilename(ex, dirtype);
}

std::string S9xGetFullFilename(std::string_view name, enum s9x_getdirtype dirtype)
{
	auto &sys = static_cast<Snes9xSystem&>(EmuEx::gSystem());
	if(dirtype == ROMFILENAME_DIR)
		return std::string{sys.contentDirectory(name)};
	else if(dirtype == CHEAT_DIR)
		return std::string{sys.userPath(sys.cheatsDir, name)};
	else if(dirtype == PATCH_DIR)
		return std::string{sys.userPath(sys.patchesDir, name)};
	else if(dirtype == SAT_DIR)
		return std::string{sys.userPath(sys.satDir, name)};
	else
		return std::string{sys.contentSavePath(name)};
}

constexpr size_t BsxBiosSize = 0x100000;

static bool isBsxBios(uint8 *data, ssize_t size)
{
	return size == BsxBiosSize && std::string_view{(char*)data + 0x7FC0, 21} == "Satellaview BS-X     ";
}

void S9xReadBSXBios(uint8 *data)
{
	auto &sys = static_cast<Snes9xSystem&>(EmuEx::gSystem());
	auto appCtx = sys.appContext();
	auto &bsxBiosPath = sys.bsxBiosPath;
	if(bsxBiosPath.empty())
		throw std::runtime_error{"No BS-X BIOS set"};
	logMsg("loading BS-X BIOS:%s", bsxBiosPath.data());
	if(EmuApp::hasArchiveExtension(appCtx.fileUriDisplayName(bsxBiosPath)))
	{
		for(auto &entry : FS::ArchiveIterator{appCtx.openFileUri(bsxBiosPath)})
		{
			if(entry.type() == FS::file_type::directory || !Snes9xSystem::hasBiosExtension(entry.name()))
				continue;
			auto size = entry.read(data, BsxBiosSize);
			if(!isBsxBios(data, size))
				throw std::runtime_error{"Incompatible BS-X BIOS"};
			return;
		}
		throw std::runtime_error{"BS-X BIOS not in archive, must end in .bin or .bios"};
	}
	else
	{
		auto io = appCtx.openFileUri(bsxBiosPath, {.accessHint = IOAccessHint::All});
		auto size = io.read(data, BsxBiosSize);
		if(!isBsxBios(data, size))
			throw std::runtime_error{"Incompatible BS-X BIOS"};
	}
}

std::string S9xBasename(std::string_view f)
{
	const char	*p;

	if ((p = strrchr(f.data(), '/')) != NULL || (p = strrchr(f.data(), '\\')) != NULL)
		return (p + 1);

	return std::string{f};
}

#endif

bool S9xPollAxis(uint32, int16*)
{
	return 0;
}

bool S9xPollPointer(uint32, int16*, int16*)
{
	return 0;
}

void S9xExit(void)
{
	bug_unreachable("should not be called");
}

void S9xToggleSoundChannel(int c)
{
	static uint8	sound_switch = 255;

	if (c == 8)
		sound_switch = 255;
	else
		sound_switch ^= 1 << c;

	S9xSetSoundControl(sound_switch);
}

const char * S9xStringInput(const char*)
{
	bug_unreachable("should not be called");
	return 0;
}

void _splitpath(const char *path, char *drive, char *dir, char *fname, char *ext)
{
	*drive = 0;

	const char	*slash = strrchr(path, SLASH_CHAR),
				*dot   = strrchr(path, '.');

	if (dot && slash && dot < slash)
		dot = NULL;

	if (!slash)
	{
		*dir = 0;

		strcpy(fname, path);

		if (dot)
		{
			fname[dot - path] = 0;
			strcpy(ext, dot + 1);
		}
		else
			*ext = 0;
	}
	else
	{
		strcpy(dir, path);
		dir[slash - path] = 0;

		strcpy(fname, slash + 1);

		if (dot)
		{
			fname[dot - slash - 1] = 0;
			strcpy(ext, dot + 1);
		}
		else
			*ext = 0;
	}
}

void _makepath(char *path, const char *, const char *dir, const char *fname, const char *ext)
{
	if (dir && *dir)
	{
		strcpy(path, dir);
		strcat(path, SLASH_STR);
	}
	else
		*path = 0;

	strcat(path, fname);

	if (ext && *ext)
	{
		strcat(path, ".");
		strcat(path, ext);
	}
}

bool8 S9xOpenSnapshotFile(const char *filename, bool8 read_only, STREAM *file)
{
	if ((*file = OPEN_STREAM(filename, read_only ? "rb" : "wb")))
		return (TRUE);

	return (FALSE);
}

void S9xCloseSnapshotFile(STREAM file)
{
	CLOSE_STREAM(file);
}

FILE *fopenHelper(const char* filename, const char* mode)
{
	return IG::FileUtils::fopenUri(EmuEx::gAppContext(), filename, mode);
}

void removeFileHelper(const char* filename)
{
	EmuEx::gAppContext().removeFileUri(filename);
}

gzFile gzopenHelper(const char *filename, const char *mode)
{
	auto openFlags = std::string_view{mode}.contains('w') ? OpenFlags::newFile() : OpenFlags{};
	return gzdopen(gAppContext().openFileUriFd(filename, openFlags | OpenFlags{.test = true}).release(), mode);
}

// from screenshot.h
bool8 S9xDoScreenshot(int, int) { return 1; }

// from gfx.h
void S9xSyncSpeed() {}
bool8 S9xInitUpdate() { return 1; }

void notifyBackupMemoryWritten()
{
	EmuEx::gSystem().onBackupMemoryWritten();
}
