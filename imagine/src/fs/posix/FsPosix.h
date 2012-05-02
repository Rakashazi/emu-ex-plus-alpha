#pragma once

#include <engine-globals.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fs/Fs.hh>


class FsPosix
{
public:
	constexpr FsPosix(): entry(0), numEntries_(0) { }
	uint numEntries() const;
	const char *entryFilename(uint index) const;
	void closeDir();

	CallResult openDir(const char* path, uint flags = 0, FsDirFilterFunc f = 0, FsDirSortFunc s = 0);

	static int chdir(const char *dir);
	static int fileType(const char *path);
	static uint fileSize(const char *path);
	static char *workDir();
	static int chown(const char *path, uid_t owner, gid_t group);
	static int hasWriteAccess(const char *path);
	static void remove(const char *file);
	static CallResult mkdir(const char *dir);
	static CallResult rename(const char *oldname, const char *newname);

	// definitions for common file path sizes
	static const uint cPathSize = 1024;
	typedef char cPath[cPathSize];

private:
	struct dirent **entry;
	int numEntries_;
	static int workDirChanged;
};
