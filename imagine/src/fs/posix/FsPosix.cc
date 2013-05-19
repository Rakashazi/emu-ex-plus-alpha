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

#define thisModuleName "fsPosix"
#include <cstdlib>
#include <logger/interface.h>
#include <base/Base.hh>
#include <util/strings.h>
#include <util/string/generic.h>
#include <unistd.h>
#include <errno.h>
#include "FsPosix.hh"

#ifdef __APPLE__
	#include <util/string/apple.h>
#endif

#if defined CONFIG_BASE_IOS && __IPHONE_OS_VERSION_MAX_ALLOWED <= 50100
	// Changed in iOS SDK 6.0, but needed to compile on 5.1 for ARMv6
	#define SELECTOR_CONST
#else
	#define SELECTOR_CONST const
#endif

#if defined(CONFIG_BASE_ANDROID) || defined(CONFIG_BASE_MACOSX) || (defined(CONFIG_BASE_IOS) && __IPHONE_OS_VERSION_MAX_ALLOWED >= 60000)
	#define CMP_CAST (int (*)(const dirent**, const dirent**))
#elif defined(CONFIG_BASE_IOS)
	// Changed in iOS SDK 6.0, but needed to compile on 5.1 for ARMv6
	#define CMP_CAST (int (*)(const void*, const void*))
#else
	#define CMP_CAST
#endif

// scandir selectors

static int noDotRefs(SELECTOR_CONST struct dirent *entry)
{
	const char *name = entry->d_name;
	if(string_equal(name,".") || string_equal(name,".."))
		return 0;
	else return 1;
}

static FsDirFilterFunc currentFilter = nullptr;

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

static int customSelector(SELECTOR_CONST struct dirent *entry)
{
	assert(currentFilter);

	if(!noDotRefs(entry))
		return 0;

	int type = entry->d_type == DT_DIR ? Fs::TYPE_DIR : Fs::TYPE_FILE;
	//logMsg("%s : %s", entry->d_name, dTypeToString(entry->d_type));
	if(entry->d_type == DT_UNKNOWN)
	{
		//logMsg("type from dir entry unknown, using stat on %s", entry->d_name);
		struct stat s;
		if(stat(entry->d_name, &s) == 0)
		{
			if(S_ISLNK(s.st_mode))
			{
				type = fileTypeFromSymlink(entry->d_name);
			}
			else
			{
				type = S_ISDIR(s.st_mode) ? Fs::TYPE_DIR : Fs::TYPE_FILE;
				//logMsg("stat type of %s is %s", entry->d_name, type == Fs::TYPE_DIR ? "Dir" : "File");
			}
		}
		else
			logMsg("error in stat");
	}
	else if(entry->d_type == DT_LNK)
	{
		type = fileTypeFromSymlink(entry->d_name);
	}
	return currentFilter(entry->d_name, type);
}
// scandir selectors end

static FsDirSortFunc currentSorter = 0;

static int customSorter(const struct dirent **e1, const struct dirent **e2)
{
	assert(currentSorter);
	struct stat s1, s2;
	stat((*e1)->d_name, &s1);
	stat((*e2)->d_name, &s2);
	//logMsg("comparing %s,%ld to %s,%ld", (*e1)->d_name, (long int)s1.st_mtime, (*e2)->d_name, (long int)s2.st_mtime);
	return currentSorter((*e1)->d_name, s1.st_mtime, (*e2)->d_name, s2.st_mtime);
}

CallResult FsPosix::openDir(const char* path, uint flags, FsDirFilterFunc f, FsDirSortFunc s)
{
	currentFilter = f;
	currentSorter = s;
	if(entry)
		closeDir();
	logMsg("opening directory %s", path);	

	char prevWorkDir[strlen(workDir()) + 1];
	if(!string_equal(path, ".")) // save working dir and switch into the new one
	{
		strcpy(prevWorkDir, workDir());
		if(chdir(path) != 0)
		{
			logErr("unable to enter %s", path);
			return INVALID_PARAMETER;
		}
	}

	numEntries_ = scandir(".", &entry,
		currentFilter ? customSelector : noDotRefs,
		(flags & Fs::OPEN_UNSORT) ? 0 : (currentSorter ? CMP_CAST customSorter : CMP_CAST alphasort));

	#ifdef __APPLE__
		// Precompose all strings for text renderer
		// TODO: make optional when renderer supports decomposed unicode
		iterateTimes(numEntries_, i)
		{
			precomposeUnicodeString(entry[i]->d_name, entry[i]->d_name, sizeof(entry[i]->d_name));
		}
	#endif

	if(!string_equal(path, ".")) // switch back to original working dir
	{
		if(chdir(prevWorkDir) != 0)
		{
			// TODO: handle this better
			logErr("unable to return to %s", prevWorkDir);
		}
	}

	if (numEntries_ == -1)
	{
		numEntries_ = 0;
		logErr("unable to open directory");
		return INVALID_PARAMETER;
	}

	return OK;
}

void FsPosix::closeDir()
{
	// TODO: test this and make sure it's correct
	iterateTimes(numEntries_, i)
	{
		free(entry[i]);
	}
	free(entry);
	entry = 0;
}

/*
CallResult fs_posix_nextEntryInDir(FsHnd hnd, DirEntryHnd* entryAddr, DirHnd dir)
{
	struct dirent *ep = readdir(dir);

	if(!ep)
	{
		logMsg("no more entries left in directory");
	}

	*entryAddr = (DirEntryHnd)ep;
	return OK;
}*/

const char *FsPosix::entryFilename(uint index) const
{
	//logDMsg("entry name is %s", thisEntry[index]->d_name);
	return entry[index]->d_name;
}

uint FsPosix::numEntries() const
{
	return numEntries_;
}

/*void fs_posix_closeDir(FsHnd hnd, DirHnd dir)
{
	closedir(dir);
}*/

int FsPosix::workDirChanged = 0;

char *FsPosix::workDir()
{
	static cPath wDir = { 0 };
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
		if(!getcwd(wDir, sizeof(wDir)))
		{
			logWarn("unable to get working dir from getcwd");
			return wDir;
		}
		#ifdef __APPLE__
			// Precompose all strings for text renderer
			// TODO: make optional when renderer supports decomposed unicode
			precomposeUnicodeString(wDir, wDir, sizeof(wDir));
		#endif
		workDirChanged = 0;
	}
	return wDir;
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

CallResult FsPosix::changeToAppDir(const char *launchCmd)
{
	logMsg("app called with cmd %s", launchCmd);
	cPath dirnameTemp;
	auto dir = string_dirname(launchCmd, dirnameTemp);
	if(chdir(dir) != 0)
	{
		logErr("error changing working directory to %s", dir);
		return INVALID_PARAMETER;
	}
	//logMsg("changed working directory to %s", path);
	if(!Base::appPath)
	{
		Base::appPath = string_dup(workDir());
		logMsg("set app dir to %s", Base::appPath);
	}
	return OK;
}
