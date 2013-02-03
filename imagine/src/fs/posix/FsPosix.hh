#pragma once

#include <engine-globals.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fs/Fs.hh>

class FsPosix : public Fs
{
public:
	constexpr FsPosix() { }
	uint numEntries() const override;
	const char *entryFilename(uint index) const override;
	void closeDir() override;

	CallResult openDir(const char* path, uint flags = 0, FsDirFilterFunc f = nullptr, FsDirSortFunc s = nullptr);

	static int chdir(const char *dir);
	static int fileType(const char *path);
	static uint fileSize(const char *path);
	static char *workDir();
	static int chown(const char *path, uid_t owner, gid_t group);
	static int hasWriteAccess(const char *path);
	static void remove(const char *file);
	static CallResult mkdir(const char *dir);
	static CallResult rename(const char *oldname, const char *newname);

	static const uint timeStrSize = 26; // 26 bytes needed for asctime_r (from glibc & Apple man pages)
	typedef char timeStr[timeStrSize];

	static CallResult mTimeAsStr(const char *path, timeStr time);

	static bool fileExists(const char *path)
	{
		return fileType(path) != TYPE_NONE;
	}
	static CallResult changeToAppDir(const char *launchCmd);

	// definitions for common file path sizes
	static constexpr uint cPathSize = IG::max(1024, PATH_MAX);
	typedef char cPath[cPathSize];

private:
	struct dirent **entry = nullptr;
	int numEntries_ = 0;
	static int workDirChanged;
};
