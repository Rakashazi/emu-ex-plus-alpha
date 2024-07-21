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
#include <imagine/fs/FSUtils.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/utility.h>
#include <imagine/util/string.h>
#include "libgen.hh"
#include <stdlib.h>

namespace IG::FS
{

PathString makeAppPathFromLaunchCommand(CStringView launchCmd)
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

FileString basename(CStringView path)
{
	return basenameImpl(path);
}

PathString dirname(CStringView path)
{
	return dirnameImpl(path);
}

FileString displayName(CStringView path)
{
	if(path.empty() || !FS::exists(path))
		return {};
	else
		return basename(path);
}

PathString dirnameUri(CStringView pathOrUri)
{
	if(pathOrUri.empty())
		return {};
	if(!isUri(pathOrUri))
		return dirname(pathOrUri);
	if(auto [treePath, treePos] = FS::uriPathSegment(pathOrUri, FS::uriPathSegmentTreeName);
		Config::envIsAndroid && treePos != std::string_view::npos)
	{
		auto [docPath, docPos] = FS::uriPathSegment(pathOrUri, FS::uriPathSegmentDocumentName);
		if(docPos == std::string_view::npos)
		{
			logErr("invalid document path in tree URI:%s", pathOrUri.data());
			return {};
		}
		if(auto lastSlashPos = docPath.rfind("%2F");
			lastSlashPos != std::string_view::npos) // return everything before the last /
		{
			return {pathOrUri, docPos + lastSlashPos};
		}
		if(auto colonPos = docPath.find("%3A");
			colonPos != std::string_view::npos) // at root, return everything before and including the :
		{
			colonPos += 3;
			return {pathOrUri, docPos + colonPos};
		}
	}
	logErr("can't get directory name on unsupported URI:%s", pathOrUri.data());
	return {};
}

std::pair<std::string_view, size_t> uriPathSegment(std::string_view uri, std::string_view name)
{
	assert(name.starts_with('/') && name.ends_with('/'));
	auto pathPos = uri.find(name);
	if(pathPos == std::string_view::npos)
		return {{}, std::string_view::npos};
	pathPos += name.size();
	auto pathStart = uri.substr(pathPos);
	// return the substring of the segment and the absolute offset into the original string view
	return {pathStart.substr(0, pathStart.find('/')), pathPos};
}

size_t directoryItems(CStringView path)
{
	size_t items = 0;
	forEachInDirectory(path, [&](auto&){ items++; return true; });
	return items;
}

bool forEachInDirectory(CStringView path, DirectoryEntryDelegate del, DirOpenFlags flags)
{
	bool entriesRead{};
	for(FS::DirectoryStream dirStream{path, flags}; dirStream.hasEntry(); dirStream.readNextDir())
	{
		entriesRead = true;
		if(!del(dirStream.entry()))
			break;
	}
	return entriesRead;
}

std::string formatLastWriteTimeLocal(ApplicationContext ctx, CStringView path)
{
	return ctx.formatDateAndTime(status(path).lastWriteTime());
}

}
