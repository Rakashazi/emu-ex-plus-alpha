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

#define LOGTAG "FS"
#include <stdlib.h>
#include <imagine/fs/sys.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/strings.h>

FsSys::PathString makeFSPathStringPrintf(const char *format, ...)
{
	FsSys::PathString path{};
	va_list args;
	va_start(args, format);
	int ret = vsnprintf(path.data(), sizeof(path), format, args);
	assert(ret >= 0);
	va_end(args);
	return path;
}

FsSys::PathString makeAppPathFromLaunchCommand(const char *launchCmd)
{
	logMsg("getting app path from launch command: %s", launchCmd);
	FsSys::PathString realPath, dirnameTemp;
	if(!realpath(string_dirname(launchCmd, dirnameTemp), realPath.data()))
	{
		bug_exit("error in realpath()");
		return {};
	}
	return realPath;
}
