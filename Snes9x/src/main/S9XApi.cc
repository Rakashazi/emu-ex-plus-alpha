#define thisModuleName "main"
#include <logger/interface.h>
#include <fs/sys.hh>

#include <sys/stat.h>
#include <snes9x.h>
#ifdef USE_SNES9X_15X
	#include <apu/apu.h>
	#include <controls.h>
#else
	#include <apu.h>
	#include <soundux.h>
#endif
#include <display.h>
#include <memmap.h>

void S9xMessage(int, int, const char *msg)
{
	if(msg)
		logMsg("%s", msg);
}

/*void S9xSetPalette (void)
{
	return;
}*/

#ifdef USE_SNES9X_15X

bool8 S9xContinueUpdate (int width, int height)
{
	return (TRUE);
}

void S9xHandlePortCommand (s9xcommand_t cmd, int16 data1, int16 data2)
{

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

#endif

bool S9xPollButton (uint32 id, bool *pressed)
{
	return 0;
}

bool S9xPollAxis (uint32 id, int16 *value)
{
	return 0;
}

bool S9xPollPointer (uint32 id, int16 *x, int16 *y)
{
	return 0;
}

void S9xExit (void)
{
	assert(0);
}

//bool8 S9xOpenSoundDevice () { return 1; }

void S9xToggleSoundChannel (int c)
{
	static uint8	sound_switch = 255;

	if (c == 8)
		sound_switch = 255;
	else
		sound_switch ^= 1 << c;

	S9xSetSoundControl(sound_switch);
}

const char * S9xStringInput (const char *message)
{
	assert(0);
	return 0;
}

void _splitpath (const char *path, char *drive, char *dir, char *fname, char *ext)
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

void _makepath (char *path, const char *, const char *dir, const char *fname, const char *ext)
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

extern FsSys::cPath gamePath;
#include <EmuSystem.hh>

#ifdef USE_SNES9X_15X

const char * S9xGetDirectory (enum s9x_getdirtype dirtype)
{
	return gamePath;
}

const char * S9xGetFilenameInc (const char *ex, enum s9x_getdirtype dirtype)
{
	assert(0); // not used yet
	return 0;
}

const char * S9xGetFilename (const char *ex, enum s9x_getdirtype dirtype)
{
	static char	s[PATH_MAX + 1];
	snprintf(s, PATH_MAX + 1, "%s/%s.%s", gamePath, EmuSystem::gameName, ex);
	return s;
}

#else

const char *S9xGetFilename (const char *ex)
{
	static char	s[PATH_MAX + 1];
	snprintf(s, PATH_MAX + 1, "%s/%s.%s", EmuSystem::savePath(), EmuSystem::gameName, ex);
	return s;
}

const char *S9xGetFilenameInc (const char *e)
{
	assert(0); // not used yet
	return 0;
}

const char *S9xGetSnapshotDirectory()
{
	return EmuSystem::savePath();
}

extern "C" char* osd_GetPackDir()
{
	static char	filename[PATH_MAX + 1];
	strcpy(filename, EmuSystem::savePath());

	if(!strncmp((char*)&Memory.ROM [0xffc0], "SUPER POWER LEAG 4   ", 21))
	{
		strcat(filename, "/SPL4-SP7");
	}
	else if(!strncmp((char*)&Memory.ROM [0xffc0], "MOMOTETSU HAPPY      ",21))
	{
		strcat(filename, "/SMHT-SP7");
	}
	else if(!strncmp((char*)&Memory.ROM [0xffc0], "HU TENGAI MAKYO ZERO ", 21))
	{
		strcat(filename, "/FEOEZSP7");
	}
	else if(!strncmp((char*)&Memory.ROM [0xffc0], "JUMP TENGAIMAKYO ZERO",21))
	{
		strcat(filename, "/SJUMPSP7");
	}
	else
	{
		strcat(filename, "/MISC-SP7");
	}
	return filename;
}

#endif

const char * S9xChooseFilename (bool8 read_only)
{
	return 0;
}

const char * S9xChooseMovieFilename (bool8 read_only)
{
	return 0;
}

const char * S9xBasename (const char *f)
{
	const char	*p;

	if ((p = strrchr(f, '/')) != NULL || (p = strrchr(f, '\\')) != NULL)
		return (p + 1);

	return (f);
}

bool8 S9xOpenSnapshotFile (const char *filename, bool8 read_only, STREAM *file)
{
	if ((*file = OPEN_STREAM(filename, read_only ? "rb" : "wb")))
		return (TRUE);

	return (FALSE);
}

void S9xCloseSnapshotFile (STREAM file)
{
	CLOSE_STREAM(file);
}

// from logger.h
void S9xResetLogger (void) { }

// from screenshot.h
bool8 S9xDoScreenshot (int, int) { return 1; }

// from gfx.h
void S9xDisplayMessages (uint16 *, int, int, int, int) { }

#undef thisModuleName
