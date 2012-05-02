#pragma once

#include <engine-globals.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fs/Fs.hh>
#include <sys/fs_external.h>

class FsPs3
{
public:
	constexpr FsPs3(): entry(0), entries(0) { }
	uint numEntries() const;
	const char *entryFilename(uint index);
	void closeDir();

	CallResult openDir(const char* path, uint flags = 0, FsDirFilterFunc f = 0, FsDirSortFunc s = 0);

	static int chdir(const char *dir);
	static int fileType(const char *path);
	static char *workDir();
	static void remove(const char *file);
	static int chown(const char *path, int owner, int group) { return 0; } // TODO
	static int hasWriteAccess(const char *path) { return 1; } // TODO

	static void initWorkDir();

	// definitions for common file path sizes
	static const uint cPathSize = 1024;
	typedef char cPath[cPathSize];
private:
	CellFsDirent *entry;
	int entries;
	static char workPath[CELL_FS_MAX_MP_LENGTH + CELL_FS_MAX_FS_PATH_LENGTH];
};
