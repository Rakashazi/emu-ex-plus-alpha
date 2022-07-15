#define LOGTAG "main"
#include <imagine/logger/logger.h>
#include <imagine/fs/FS.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/format.hh>
#include <imagine/util/string.h>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuApp.hh>
#include "MainSystem.hh"
#include <sys/stat.h>
#include <snes9x.h>
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
uint16 SSettings::DisplayColor = 0;
uint32 SSettings::SkipFrames = 0;
uint32 SSettings::TurboSkipFrames = 0;
const char *SGFX::InfoString{};
uint32 SGFX::InfoStringTimeout = 0;
char SGFX::FrameDisplayString[256]{};
#endif
static std::string globalPath{};

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

#ifndef SNES9X_VERSION_1_4

void S9xHandlePortCommand(s9xcommand_t cmd, int16 data1, int16 data2) {}

bool8 S9xOpenSoundDevice()
{
	return TRUE;
}

const char *S9xGetCrosshair(int idx)
{
	return nullptr;
}

const char * S9xGetFilenameInc(const char *ex, enum s9x_getdirtype dirtype)
{
	logErr("S9xGetFilenameInc not used yet");
	return nullptr;
}

#else

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

#endif

#ifndef SNES9X_VERSION_1_4
const char *S9xGetFilename(const char *ex, enum s9x_getdirtype dirtype)
#else
const char *S9xGetFilename(const char *ex)
#endif
{
	bool isRomDir{};
	auto &sys = EmuEx::gSystem();
	#ifndef SNES9X_VERSION_1_4
	if(dirtype == ROMFILENAME_DIR)
	{
		isRomDir = true;
	}
	#endif
	if(isRomDir)
		globalPath = sys.contentSaveFilePath(ex);
	else
		globalPath = sys.contentSaveFilePath(ex);
	//logMsg("built s9x path:%s", globalPath.c_str());
	return globalPath.c_str();
}

#ifndef SNES9X_VERSION_1_4
const char *S9xGetFullFilename(const char *name, enum s9x_getdirtype dirtype)
#else
const char *S9xGetFullFilename(const char *name)
#endif
{
	bool isRomDir{};
	auto &sys = EmuEx::gSystem();
	#ifndef SNES9X_VERSION_1_4
	if(dirtype == ROMFILENAME_DIR)
	{
		isRomDir = true;
	}
	#endif
	if(isRomDir)
		globalPath = sys.contentDirectory(name);
	else
		globalPath = sys.contentSavePath(name);
	//logMsg("built s9x path:%s", globalPath.c_str());
	return globalPath.c_str();
}

bool S9xPollAxis(uint32 id, int16 *value)
{
	return 0;
}

bool S9xPollPointer(uint32 id, int16 *x, int16 *y)
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

const char * S9xStringInput(const char *message)
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

const char *S9xBasename(const char *f)
{
	const char	*p;

	if ((p = strrchr(f, '/')) != NULL || (p = strrchr(f, '\\')) != NULL)
		return (p + 1);

	return (f);
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
	unsigned openFlags = std::string_view{mode}.contains('w') ? IG::IO::OPEN_NEW : 0;
	return gzdopen(gAppContext().openFileUriFd(filename, openFlags | IG::IO::TEST_BIT).release(), mode);
}

// from logger.h
void S9xResetLogger() {}

// from screenshot.h
bool8 S9xDoScreenshot(int, int) { return 1; }

// from gfx.h
void S9xSyncSpeed() {}
bool8 S9xInitUpdate() { return 1; }
