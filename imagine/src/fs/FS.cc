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
#include <imagine/fs/FS.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/utility.h>
#include <imagine/util/string.h>
#include "libgen.hh"
#include <stdlib.h>

namespace FS
{

PathString makeAppPathFromLaunchCommand(IG::CStringView launchCmd)
{
	logMsg("app path from launch command:%s", launchCmd.data());
	PathStringArray realPath;
	if(!realpath(FS::dirname(launchCmd).data(), realPath.data()))
	{
		logErr("error in realpath()");
		return {};
	}
	return realPath.data();
}

FileString basename(IG::CStringView path)
{
	return basenameImpl(path);
}

PathString dirname(IG::CStringView path)
{
	return dirnameImpl(path);
}

bool isUri(std::string_view str)
{
	if(str[0] == '/')
		return false;
	else return true;
}

PathString decodeUri(std::string_view uri)
{
	PathStringArray output;
	assert(uri.size() < sizeof(output) - 1);
	IG::decodeUri(uri, output.data());
	return output.data();
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
		s1.begin(), s1.end(),
		s2.begin(), s2.end(),
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
