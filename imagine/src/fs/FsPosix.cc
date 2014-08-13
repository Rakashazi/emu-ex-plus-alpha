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

#define LOGTAG "FSPosix"
#include <cstdlib>
#include <imagine/fs/FsPosix.hh>
#include <imagine/logger/logger.h>
#include <imagine/base/Base.hh>
#include <imagine/util/strings.h>
#include <imagine/util/string/generic.h>
#include <unistd.h>
#include <errno.h>

#ifdef __APPLE__
#include <imagine/util/string/apple.h>
#endif

union FullDirent
{
	struct dirent d;
	char b[offsetof (struct dirent, d_name) + NAME_MAX + 1];
};

static int noDotRefs(const struct dirent &entry)
{
	const char *name = entry.d_name;
	if(string_equal(name,".") || string_equal(name,".."))
		return 0;
	else return 1;
}

static const char* dTypeToString(unsigned char d_type)
{
	switch(d_type)
	{
		case DT_UNKNOWN: return "Unknown";
		case DT_REG: return "File";
		case DT_DIR: return "Directory";
		case DT_FIFO: return "FIFO";
		case DT_SOCK: return "Socket";
		case DT_CHR: return "Char Device";
		case DT_BLK: return "Block Device";
		case DT_LNK: return "Symbolic Link";
		default: return "Unhandled Type";
	}
}

static int fileTypeFromStat(const char *path)
{
	struct stat s;
	if(stat(path, &s) == 0)
	{
		int type = S_ISDIR(s.st_mode) ? Fs::TYPE_DIR : Fs::TYPE_FILE;
		//logMsg("stat type of %s is %s", path, type == Fs::TYPE_DIR ? "Dir" : "File");
		return type;
	}
	else
	{
		logMsg("error in stat");
		return Fs::TYPE_FILE;
	}
}

static int fileTypeFromSymlink(const char *path)
{
	char deRefPath[512]; int deRefPathSize;
	if((deRefPathSize = readlink(path, deRefPath, sizeof(deRefPath))) == -1)
	{
		logWarn("error in readlink() %s", path);
		return Fs::TYPE_FILE;
	}
	else if(deRefPathSize == sizeof(deRefPath))
	{
		logWarn("readlink() buffer too small");
		return Fs::TYPE_FILE;
	}
	else
	{
		deRefPath[deRefPathSize] = '\0';
		//logMsg("using stat on deref symlink %s", deRefPath);
		return fileTypeFromStat(deRefPath);
	}
}

// NOTE: assumes the working directory is the same as the directory entry being inspected
static int fileTypeFromDirent(const struct dirent &entry)
{
	if(entry.d_type == DT_UNKNOWN)
	{
		//logMsg("type from dir entry unknown, using stat on %s", entry->d_name);
		struct stat s;
		if(stat(entry.d_name, &s) == 0)
		{
			if(S_ISLNK(s.st_mode))
			{
				return fileTypeFromSymlink(entry.d_name);
			}
			else
			{
				return S_ISDIR(s.st_mode) ? Fs::TYPE_DIR : Fs::TYPE_FILE;
				//logMsg("stat type of %s is %s", entry->d_name, type == Fs::TYPE_DIR ? "Dir" : "File");
			}
		}
		else
		{
			logMsg("error in stat");
			return Fs::TYPE_FILE;
		}
	}
	else if(entry.d_type == DT_LNK)
	{
		return fileTypeFromSymlink(entry.d_name);
	}
	return entry.d_type == DT_DIR ? Fs::TYPE_DIR : Fs::TYPE_FILE;
}

/*static int customSorter(const struct dirent **e1, const struct dirent **e2)
{
	assert(currentSorter);
	struct stat s1, s2;
	stat((*e1)->d_name, &s1);
	stat((*e2)->d_name, &s2);
	//logMsg("comparing %s,%ld to %s,%ld", (*e1)->d_name, (long int)s1.st_mtime, (*e2)->d_name, (long int)s2.st_mtime);
	return currentSorter((*e1)->d_name, s1.st_mtime, (*e2)->d_name, s2.st_mtime);
}*/

CallResult FsPosix::openDir(const char* path, uint flags, FsDirFilterFunc f, FsDirSortFunc s)
{
	closeDir();
	logMsg("opening directory %s", path);

	bool switchDirs = !string_equal(path, "."); // save working dir and switch into the new one
	char prevWorkDir[strlen(workDir()) + 1];
	if(switchDirs)
	{
		strcpy(prevWorkDir, workDir());
		if(chdir(path) != 0)
		{
			logErr("unable to enter %s", path);
			return INVALID_PARAMETER;
		}
	}

	auto dir = opendir(".");
	if(!dir)
	{
		logErr("unable to open directory");
		return INVALID_PARAMETER;
	}

	{
		FullDirent currEntry;
		struct dirent *resultEntry;
		while(readdir_r(dir, &currEntry.d, &resultEntry) == 0 && resultEntry)
		{
			auto &d = currEntry.d;
			logMsg("reading entry: %s", d.d_name);
			if(!noDotRefs(d))
				continue;
			if(f)
			{
				auto type = fileTypeFromDirent(d);
				if(!f(d.d_name, type))
					continue;
			}

			uint nameLen = strlen(d.d_name);
			entry = (char**)mem_realloc(entry, sizeof(char**) * (numEntries_+1));
			auto &currEntry = entry[numEntries_];
			currEntry = (char*)mem_alloc(nameLen + 1);
			strcpy(currEntry, d.d_name);
			#ifdef __APPLE__
			// Precompose all strings for text renderer
			// TODO: make optional when renderer supports decomposed unicode
			precomposeUnicodeString(currEntry, currEntry, nameLen+1);
			#endif
			numEntries_++;
		}
	}

	if(s)
	{
		//std::sort();
	}
	else if(!(flags & Fs::OPEN_UNSORT))
	{
		// sort by string value
		std::sort(entry, &entry[numEntries_],
			[](const char *s1, const char *s2)
			{
				return strcasecmp(s1, s2) < 0;
			}
		);
	}

	if(switchDirs) // switch back to original working dir
	{
		if(chdir(prevWorkDir) != 0)
		{
			// TODO: handle this better
			logErr("unable to return to %s", prevWorkDir);
		}
	}

	closedir(dir);
	return OK;
}

void FsPosix::closeDir()
{
	if(entry)
	{
		iterateTimes(numEntries_, i)
		{
			mem_free(entry[i]);
		}
		mem_free(entry);
		entry = nullptr;
		numEntries_ = 0;
	}
}

const char *FsPosix::entryFilename(uint index) const
{
	return entry[index];
}

uint FsPosix::numEntries() const
{
	return numEntries_;
}

int FsPosix::workDirChanged = 0;

char *FsPosix::workDir()
{
	static PathString wDir{};
	if(workDirChanged)
	{
		//logMsg("getting working dir");
		// TODO: look into getenv("PWD") instead of getcwd
		/*{
			auto currentDirName = getenv("PWD");
			if(!currentDirName)
			{
				logWarn("unable to get working dir from getenv");
				return wDir;
			}
			string_copy(wDir, currentDirName);
		}*/
		if(!getcwd(wDir.data(), sizeof(wDir)))
		{
			logWarn("unable to get working dir from getcwd: %s", strerror(errno));
			return wDir.data();
		}
		#ifdef __APPLE__
		// Precompose all strings for text renderer
		// TODO: make optional when renderer supports decomposed unicode
		precomposeUnicodeString(wDir.data(), wDir.data(), sizeof(wDir));
		#endif
		workDirChanged = 0;
	}
	return wDir.data();
}

int FsPosix::fileType(const char *path)
{
	struct stat s;
	if(stat(path, &s) == 0)
	{
		//logMsg("%s exists", path);
		return S_ISDIR(s.st_mode) ? Fs::TYPE_DIR : Fs::TYPE_FILE;
	}
	else
		return Fs::TYPE_NONE;
}

CallResult FsPosix::mTimeAsStr(const char *path, timeStr time)
{
	logMsg("checking m-time for %s", path);
	struct stat s;
	if(stat(path, &s) == 0)
	{
		struct tm bTime;
		time_t mTime = s.st_mtime; // st_mtime isn't time_t on Android so convert if needed
		if(!localtime_r(&mTime, &bTime))
		{
			logErr("localtime_r failed");
			return INVALID_PARAMETER;
		}
		if(!asctime_r(&bTime, time))
		{
			logErr("asctime_r failed");
			return INVALID_PARAMETER;
		}
		uint len = strlen(time);
		if(len && time[len-1] == '\n')
			time[len-1] = 0;
		return OK;
	}
	else
		return IO_ERROR;
}

uint FsPosix::fileSize(const char *path)
{
	struct stat s;
	if(stat(path, &s) == 0)
	{
		return s.st_size;
	}
	return 0;
}

int FsPosix::chdir(const char *dir)
{
	logMsg("changing dir to: %s", dir);
	workDirChanged = 1;
	return ::chdir(dir);
}

int FsPosix::hasWriteAccess(const char *path)
{
	if(access(path, W_OK) == 0)
	{
		logMsg("%s already has write access", path);
		return 1;
	}
	if(errno != EACCES)
	{
		logMsg("couldn't read access bits for %s", path);
		return -1;
	}
	logMsg("no write access to: %s", path);
	return 0;
}

int FsPosix::chown(const char *path, uid_t owner, gid_t group)
{
	return ::chown(path, owner, group);
}

void FsPosix::remove(const char *file)
{
	logErr("removing file: %s", file);
	if(::unlink(file) == -1)
	{
		logErr("errno %d", errno);
		switch(errno)
		{
			bcase EACCES: logMsg("EACCES");
			bcase EROFS: logMsg("EROFS");
			bcase EPERM: logMsg("EPERM");
			bcase EBUSY: logMsg("EBUSY");
			bcase ENOENT: logMsg("ENOENT");
		}
	}
}

CallResult FsPosix::mkdir(const char *dir)
{
	const mode_t defaultOpenMode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
	if(::mkdir(dir, defaultOpenMode) == -1)
	{
		if(errno == EEXIST)
			logErr("dir already exists: %s", dir);
		else
			logErr("error %d creating dir: %s", errno, dir);
		switch(errno)
		{
			case EACCES: return PERMISSION_DENIED;
			case ENOSPC: return NO_FREE_ENTRIES;
			case EEXIST: return ALREADY_EXISTS;
			default: return IO_ERROR;
		}
	}
	logMsg("created dir: %s", dir);
	return OK;
}

CallResult FsPosix::rename(const char *oldname, const char *newname)
{
	if(::rename(oldname, newname) == -1)
	{
		logErr("error renaming %s to %s", oldname, newname);
		switch(errno)
		{
			case EACCES: return PERMISSION_DENIED;
			case ENOSPC: return NO_FREE_ENTRIES;
			case EEXIST: return ALREADY_EXISTS;
			default: return IO_ERROR;
		}
	}
	logMsg("renamed %s to %s", oldname, newname);
	return OK;
}
