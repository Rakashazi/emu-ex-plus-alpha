#pragma once

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/fs_external.h>
#include <engine-globals.h>
#include <fs/Fs.hh>

class FsPs3 : public Fs
{
public:
	constexpr FsPs3() { }
	uint numEntries() const;
	const char *entryFilename(uint index) const;
	void closeDir();

	CallResult openDir(const char* path, uint flags = 0, FsDirFilterFunc f = 0, FsDirSortFunc s = 0);

	static int chdir(const char *dir);
	static int fileType(const char *path);
	static char *workDir();
	static void remove(const char *file);
	static int chown(const char *path, int owner, int group) { return 0; } // TODO
	static int hasWriteAccess(const char *path) { return 1; } // TODO

	typedef char timeStr[1]; // TODO
	static CallResult mTimeAsStr(const char *path, timeStr time);

	static bool fileExists(const char *path)
	{
		return fileType(path) != TYPE_NONE;
	}
	static void makePathAbs(const char *path, char *outPath, size_t size);

	static void initWorkDir();

	// definitions for common file path sizes
	static const uint cPathSize = 1024;
	typedef char cPath[cPathSize];
private:
	CellFsDirent *entry = nullptr;
	int entries = 0;
	static char workPath[CELL_FS_MAX_MP_LENGTH + CELL_FS_MAX_FS_PATH_LENGTH];
};

static const uint PATH_MAX = FsPs3::cPathSize;

static char *realpath(const char *in, char *out)
{
	FsPs3::makePathAbs(in, out, PATH_MAX);
	return out;
}
