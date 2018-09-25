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
#if !defined __APPLE__ && !defined __ANDROID__ && !defined __PPU__ && defined _GNU_SOURCE
#define CONFIG_USE_GNU_BASENAME
#endif
#include <stdlib.h>
#include <imagine/fs/FS.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/string.h>
#include <imagine/util/utility.h>
#ifdef CONFIG_USE_GNU_BASENAME
#include <imagine/util/string/glibc.h>
#endif
#include "libgen.hh"

namespace FS
{

FileString makeFileStringPrintf(const char *format, ...)
{
	FileString path{};
	va_list args;
	va_start(args, format);
	vsnprintf(path.data(), path.size(), format, args);
	va_end(args);
	return path;
}

PathString makePathStringPrintf(const char *format, ...)
{
	PathString path{};
	va_list args;
	va_start(args, format);
	vsnprintf(path.data(), path.size(), format, args);
	va_end(args);
	return path;
}

FileString makeFileString(const char *str)
{
	FileString path{};
	string_copy(path, str);
	return path;
}

PathString makePathString(const char *str)
{
	PathString path{};
	string_copy(path, str);
	return path;
}

PathString makePathString(const char *dir, const char *file)
{
	return makePathStringPrintf("%s/%s", dir, file);
}

PathString makeAppPathFromLaunchCommand(const char *launchCmd)
{
	logMsg("app path from launch command:%s", launchCmd);
	FS::PathString realPath;
	if(!realpath(FS::dirname(launchCmd).data(), realPath.data()))
	{
		logErr("error in realpath()");
		return {};
	}
	return realPath;
}

FileString basename(const char *path)
{
	FileString name;
	#ifdef CONFIG_USE_GNU_BASENAME
	string_copy(name, gnu_basename(path));
	#elif defined __ANDROID__
	string_copy(name, ::posixBasenameImpl(path));
	#elif defined _WIN32
	char namePart[_MAX_FNAME], extPart[_MAX_EXT];
	_splitpath(path, nullptr, nullptr, namePart, extPart);
	string_printf(name, "%s%s", namePart, extPart);
	#else
	// standard version can modify input, and returns a pointer within it
	// BSD version can modify input, but always returns its own allocated storage
	PathString tempPath = makePathString(path);
	string_copy(name, ::posixBasenameImpl(tempPath.data()));
	#endif
	return name;
}

PathString dirname(const char *path)
{
	PathString dir;
	#if defined __ANDROID__
	string_copy(dir, ::posixDirnameImpl(path));
	#elif defined _WIN32
	char drivePart[_MAX_DRIVE], dirPart[_MAX_DIR];
	_splitpath(path, drivePart, dirPart, nullptr, nullptr);
	string_printf(dir, "%s%s", drivePart, dirPart);
	#else
	// standard version can modify input, and returns a pointer within it
	// BSD version can modify input, but always returns its own allocated storage
	PathString tempPath = makePathString(path);
	string_copy(dir, ::posixDirnameImpl(tempPath.data()));
	#endif
	return dir;
}

FileStringCompareFunc fileStringNoCaseLexCompare()
{
	return [](const FS::FileString &s1, const FS::FileString &s2)
		{
			return std::lexicographical_compare(
				s1.data(), s1.data() + strlen(s1.data()),
				s2.data(), s2.data() + strlen(s2.data()),
				[](char c1, char c2)
				{
					return std::tolower(c1) < std::tolower(c2);
				});
		};
}

int directoryItems(const char *path)
{
		uint items = 0;
		for(auto &d : FS::directory_iterator(path))
		{
			items++;
		}
		return items;
}

}
