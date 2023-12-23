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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/fs/FSUtils.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>
#include <vector>

namespace EmuEx
{

using namespace IG;

constexpr SystemLogger log{"PathUtils"};

std::vector<FS::PathString> subDirectoryStrings(ApplicationContext ctx, CStringView path)
{
	std::vector<FS::PathString> subDirs{};
	ctx.forEachInDirectoryUri(path,
		[&](auto &entry)
		{
			//log.debug("entry:{}", entry.path());
			if(entry.type() != FS::file_type::directory)
				return true;
			subDirs.emplace_back(entry.path());
			return true;
		});
	return subDirs;
}

void flattenSubDirectories(ApplicationContext ctx, const std::vector<FS::PathString> &subDirs, CStringView outPath)
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
					log.error("error while moving {} from legacy save path", entry.path());
					return false;
				}
				return true;
			});
		ctx.removeDirectoryUri(subDir);
	}
}

void updateLegacySavePathOnStoragePath(ApplicationContext ctx, EmuSystem &sys)
{
	try
	{
		auto storagePath = ctx.storagePath();
		auto oldSavePath = FS::uriString(storagePath, "Game Data", sys.shortSystemName());
		auto oldSaveSubDirs = subDirectoryStrings(ctx, oldSavePath);
		if(oldSaveSubDirs.empty())
		{
			log.info("no legacy save folders in:{}", oldSavePath);
			return;
		}
		auto newSavePath = FS::createDirectoryUriSegments(ctx, storagePath, "EmuEx", sys.shortSystemName(), "saves");
		flattenSubDirectories(ctx, oldSaveSubDirs, newSavePath);
		ctx.removeDirectoryUri(oldSavePath);
	}
	catch(...)
	{
		return;
	}
}

bool hasWriteAccessToDir(CStringView path)
{
	// on Android test file creation since
	// access() can still claim emulated storage is writable
	// even though parts are locked-down by the OS (like on 4.4+)
	if constexpr(Config::envIsAndroid)
	{
		if(IG::isUri(path))
			return true;
		auto testFilePath = FS::pathString(path, ".safe-to-delete-me");
		PosixIO testFile{testFilePath, OpenFlags::testNewFile()};
		auto removeTestFile = IG::scopeGuard([&]() { if(testFile) FS::remove(testFilePath); });
		return (bool)testFile;
	}
	else
	{
		assert(!IG::isUri(path));
		return FS::access(path, FS::acc::w);
	}
}

}
