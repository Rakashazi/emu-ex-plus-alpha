#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/engine-globals.h>
#include <cstdio>
#include <array>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <imagine/fs/Fs.hh>

class FsPosix : public Fs
{
public:
	static constexpr uint timeStrSize = 26; // 26 bytes needed for asctime_r (from glibc & Apple man pages)
	using timeStr = char[timeStrSize];
	// definitions for common file path sizes
	static constexpr uint PATH_STRING_SIZE = IG::maxConst(1024, PATH_MAX);
	using PathString = std::array<char, PATH_STRING_SIZE>;

	constexpr FsPosix() {}
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
	static CallResult mTimeAsStr(const char *path, timeStr time);

	static bool fileExists(const char *path)
	{
		return fileType(path) != TYPE_NONE;
	}

private:
	char **entry = nullptr;
	int numEntries_ = 0;
	static int workDirChanged;
};
