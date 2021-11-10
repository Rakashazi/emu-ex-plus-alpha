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
#include <imagine/util/format.hh>
#include <imagine/util/utility.h>
#ifdef CONFIG_USE_GNU_BASENAME
#include <imagine/util/string/glibc.h>
#endif
#include "libgen.hh"

namespace FS
{

FileString makeFileString(IG::CStringView str)
{
	FileString path{};
	string_copy(path, str);
	return path;
}

FileString makeFileStringWithoutDotExtension(IG::CStringView str)
{
	auto fileStr = makeFileString(str);
	if(auto dotPos = string_dotExtension(str);
		dotPos)
	{
		fileStr[(uintptr_t)(dotPos - str)] = 0;
	}
	return fileStr;
}

PathString makePathString(IG::CStringView str)
{
	PathString path{};
	string_copy(path, str);
	return path;
}

PathString makePathString(IG::CStringView dir, IG::CStringView file)
{
	return IG::formatToPathString("{}/{}", dir, file);
}

PathString makeAppPathFromLaunchCommand(IG::CStringView launchCmd)
{
	logMsg("app path from launch command:%s", launchCmd.data());
	FS::PathString realPath{};
	if(!realpath(FS::dirname(launchCmd).data(), realPath.data()))
	{
		logErr("error in realpath()");
		return {};
	}
	return realPath;
}

FileString basename(IG::CStringView path)
{
	FileString name{};
	#ifdef CONFIG_USE_GNU_BASENAME
	string_copy(name, gnu_basename(path));
	#elif defined __ANDROID__
	string_copy(name, ::posixBasenameImpl(path));
	#else
	// standard version can modify input, and returns a pointer within it
	// BSD version can modify input, but always returns its own allocated storage
	PathString tempPath = makePathString(path);
	string_copy(name, ::posixBasenameImpl(tempPath.data()));
	#endif
	return name;
}

PathString dirname(IG::CStringView path)
{
	PathString dir{};
	#if defined __ANDROID__
	string_copy(dir, ::posixDirnameImpl(path));
	#else
	// standard version can modify input, and returns a pointer within it
	// BSD version can modify input, but always returns its own allocated storage
	PathString tempPath = makePathString(path);
	string_copy(dir, ::posixDirnameImpl(tempPath.data()));
	#endif
	return dir;
}

bool isUri(IG::CStringView str)
{
	if(str[0] == '/')
		return false;
	else return true;
}

PathString decodeUri(IG::CStringView uri)
{
	PathString output{};
	assert(strlen(uri) < output.size());
	::decodeUri(uri, output.data());
	return output;
}

FileString basenameUri(IG::CStringView pathOrUri, bool isEncodedUri)
{
	if(isEncodedUri)
		return basename(decodeUri(pathOrUri));
	else
		return basename(pathOrUri);
}

bool fileStringNoCaseLexCompare(FS::FileString s1, FS::FileString s2)
{
	return std::lexicographical_compare(
		s1.data(), s1.data() + strlen(s1.data()),
		s2.data(), s2.data() + strlen(s2.data()),
		[](char c1, char c2)
		{
			return std::tolower(c1) < std::tolower(c2);
		});
}

int directoryItems(IG::CStringView path)
{
		uint32_t items = 0;
		for(auto &d : FS::directory_iterator(path))
		{
			items++;
		}
		return items;
}

}
