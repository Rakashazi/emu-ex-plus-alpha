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

#define thisModuleName "fs"
#include <logger/interface.h>
#include <util/strings.h>
#include <base/Base.hh>

#include "sys.hh"

#if defined (CONFIG_BASE_WIN32)
	#include <direct.h>
#else
	#include <sys/stat.h>
	#include <unistd.h>
#endif

bool Fs::fileExists(const char *path)
{
	return FsSys::fileType(path) != TYPE_NONE;
}

CallResult Fs::changeToAppDir(const char *launchCmd)
{
	char path[strlen(launchCmd)+1];
	logMsg("app called with cmd %s", launchCmd);
	strcpy(path, launchCmd);
	dirNameInPlace(path);
	if(FsSys::chdir(path) != 0)
	{
		logErr("error changing working directory to %s", path);
		return INVALID_PARAMETER;
	}
	//logMsg("changed working directory to %s", path);
	if(!Base::appPath)
	{
		Base::appPath = string_dup(FsSys::workDir());
		logMsg("set app dir to %s", Base::appPath);
	}
	return OK;
}

void  Fs::makePathAbs(const char *path, char *outPath, size_t size)
{
	// TODO: implement more complex cases and error checking
	assert(FsSys::workDir()[0] == '/');
	logMsg("work dir %s", FsSys::workDir());
	if(path[0] == '/')
	{
		strcpy(outPath, path);
	}
	else if(string_equal(path, "."))
	{
		strcpy(outPath, FsSys::workDir());
	}
	else if(string_equal(path, ".."))
	{
		char *cutoff = strrchr(FsSys::workDir(), '/');
		size_t copySize = cutoff - FsSys::workDir();
		if(cutoff == FsSys::workDir())
			copySize = 1; // at root
		memcpy(outPath, FsSys::workDir(), copySize);
		outPath[copySize] = 0;
	}
	else //assume all other paths are relative and append workDir
	{
		sprintf(outPath, "%s/%s", strlen(FsSys::workDir()) > 1 ? FsSys::workDir() : "", path);
	}
}

#undef thisModuleName
