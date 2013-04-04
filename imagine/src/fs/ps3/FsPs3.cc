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

#define thisModuleName "fsPs3"
#include "FsPs3.hh"

#include <base/Base.hh>
#include <logger/interface.h>
#include <mem/interface.h>
#include <assert.h>
#include <unistd.h>
#include <cell/cell_fs.h>
#include <meta.h>

static uint cellFsTypeToFsType(uint8_t type)
{
	switch(type)
	{
		case CELL_FS_TYPE_UNKNOWN:
		case CELL_FS_TYPE_SYMLINK:
		case CELL_FS_TYPE_REGULAR:
			return Fs::TYPE_FILE;
		case CELL_FS_TYPE_DIRECTORY:
			return Fs::TYPE_DIR;
		default:
			bug_branch("%d", type); return 0;
	}
}

CallResult FsPs3::openDir (const char* path, uint flags, FsDirFilterFunc keepFile, FsDirSortFunc s)
{
	cPath aPath;
	makePathAbs(path, aPath, sizeof(aPath));
	logMsg("opening directory %s, converted to %s", path, aPath);

	if(entry != 0)
	{
		closeDir();
	}

	int fd;
	CellFsErrno ret = cellFsOpendir(aPath, &fd);
	if(ret != CELL_FS_SUCCEEDED)
	{
		logErr("unable to open directory");
		return INVALID_PARAMETER;
	}

	while(1)
	{
		CellFsDirent readEntry;
		uint64_t read;
		CellFsErrno ret = cellFsReaddir(fd, &readEntry, &read);
		if(ret != CELL_FS_SUCCEEDED)
		{
			logErr("error reading directory");
			closeDir();
			cellFsClosedir(fd);
			return INVALID_PARAMETER;
		}
		if(read == 0)
			break; // no more entries
		if(string_equal(readEntry.d_name, ".") || string_equal(readEntry.d_name, ".."))
			continue;
		if(keepFile && !keepFile(readEntry.d_name, cellFsTypeToFsType(readEntry.d_type)))
			continue;
		logMsg("file entry %s", readEntry.d_name);
		entry = (CellFsDirent*)mem_realloc(entry, sizeof(CellFsDirent)*(entries+1));
		entry[entries] = readEntry;
		entries++;
	}

	cellFsClosedir(fd);

	return OK;
}

void FsPs3::closeDir ()
{
	if(entry != 0)
	{
		mem_free(entry);
		entry = 0;
	}
	entries = 0;
}

const char *FsPs3::entryFilename (uint index) const
{
	return entry[index].d_name;
}

uint FsPs3::numEntries () const
{
	return entries;
}

char FsPs3::workPath[CELL_FS_MAX_MP_LENGTH + CELL_FS_MAX_FS_PATH_LENGTH] = { 0 };
static char baseProductPath[CELL_FS_MAX_FS_PATH_LENGTH];

void FsPs3::initWorkDir()
{
	assert(strlen(CONFIG_PS3_PRODUCT_ID) == 9);
	strcpy(workPath, "/dev_hdd0/game/" CONFIG_PS3_PRODUCT_ID "/USRDIR");
	logMsg("init work path to %s", workPath);
	strcpy(baseProductPath, workPath);
	Base::appPath = baseProductPath;
}

char *FsPs3::workDir()
{
	return workPath;
}

int FsPs3::fileType(const char *path)
{
	cPath aPath;
	makePathAbs(path, aPath, sizeof(aPath));

	CellFsStat s;
	if(cellFsStat(aPath, &s) == CELL_FS_SUCCEEDED)
	{
		//logMsg("%s has mode %d", aPath, s.st_mode);
		return (s.st_mode & CELL_FS_S_IFDIR) ? Fs::TYPE_DIR : Fs::TYPE_FILE;
	}
	else
		return Fs::TYPE_NONE;
}

int FsPs3::chdir(const char *dir)
{
	cPath aDir;
	makePathAbs(dir, aDir, sizeof(aDir));
	strcpy(workPath, aDir);
	logMsg("set working dir: %s", workPath);
	return 0;
}

void FsPs3::remove(const char *file)
{
	cellFsUnlink(file);
}

void FsPs3::makePathAbs(const char *path, char *outPath, size_t size)
{
	// TODO: implement more complex cases and error checking
	assert(workDir()[0] == '/');
	if(path[0] == '/')
	{
		strcpy(outPath, path);
	}
	else if(string_equal(path, "."))
	{
		strcpy(outPath, workDir());
	}
	else if(string_equal(path, ".."))
	{
		char *cutoff = strrchr(workDir(), '/');
		size_t copySize = cutoff - workDir();
		if(cutoff == workDir())
			copySize = 1; // at root
		memcpy(outPath, workDir(), copySize);
		outPath[copySize] = 0;
	}
	else //assume all other paths are relative and append workDir
	{
		sprintf(outPath, "%s/%s", strlen(workDir()) > 1 ? workDir() : "", path);
	}
	logMsg("converted to absolute path: %s", outPath);
}

CallResult FsPs3::mTimeAsStr(const char *path, timeStr time)
{
	// TODO
	time[0] = 0;
	return OK;
}
