/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuSystem.hh>
#include <imagine/fs/FSUtils.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/logger/logger.h>
#include <vector>

namespace EmuEx
{

using namespace IG;

std::vector<FS::PathString> subDirectoryStrings(ApplicationContext ctx, CStringView path)
{
	std::vector<FS::PathString> subDirs{};
	ctx.forEachInDirectoryUri(path,
		[&](auto &entry)
		{
			//logMsg("entry:%s", entry.path().data());
			if(entry.type() != FS::file_type::directory)
				return true;
			subDirs.emplace_back(entry.path());
			return true;
		});
	return subDirs;
}

void flattenSubDirectories(ApplicationContext ctx, std::vector<FS::PathString> subDirs, CStringView outPath)
{
	for(const auto &subDir : subDirs)
	{
		ctx.forEachInDirectoryUri(subDir,
			[&](auto &entry)
			{
				if(entry.type() == FS::file_type::directory)
					return true;
				if(!ctx.renameFileUri(entry.path(), FS::uriString(outPath, entry.name())))
				{
					logErr("error while moving %s from legacy save path", entry.path().data());
					return false;
				}
				return true;
			});
		ctx.removeDirectoryUri(subDir);
	}
}

void updateLegacySavePathOnStoragePath(ApplicationContext ctx)
{
	try
	{
		auto storagePath = ctx.storagePath();
		auto oldSavePath = FS::uriString(storagePath, "Game Data", EmuSystem::shortSystemName());
		auto oldSaveSubDirs = subDirectoryStrings(ctx, oldSavePath);
		if(oldSaveSubDirs.empty())
		{
			logMsg("no legacy save folders in:%s", oldSavePath.data());
			return;
		}
		auto newSavePath = FS::createDirectoryUriSegments(ctx, storagePath, "EmuEx", EmuSystem::shortSystemName(), "saves");
		flattenSubDirectories(ctx, std::move(oldSaveSubDirs), newSavePath);
		ctx.removeDirectoryUri(oldSavePath);
	}
	catch(...)
	{
		return;
	}
}

}
