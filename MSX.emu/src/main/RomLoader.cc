/*  This file is part of MSX.emu.

	MSX.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MSX.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MSX.emu.  If not, see <http://www.gnu.org/licenses/> */


#include "ziphelper.h"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/fs/FS.hh>
#include <imagine/logger/logger.h>
#include <string.h>
#include "internal.hh"

extern "C"
{
	#include <blueMSX/Memory/RomLoader.h>
}

static UInt8 *fileToMallocBuffer(IO &file, int *size)
{
	int fileSize = file.size();
	auto buff = (UInt8*)malloc(fileSize);
	file.read(buff, fileSize);
	*size = fileSize;
	return buff;
}

UInt8 *romLoad(const char *filename, const char *filenameInArchive, int *size)
{
	if(!filename || !strlen(filename))
		return nullptr;
	logMsg("loading ROM file:%s:%s", filename, filenameInArchive);
	FS::PathString filePath{filename};
	auto filePathInFirmwarePath = FS::pathString(machineBasePath(appCtx), filename);
	FS::PathString *searchPath[]{&filePath, &filePathInFirmwarePath};
	if(filenameInArchive && strlen(filenameInArchive))
	{
		for(const auto &path : searchPath)
		{
			auto buff = (UInt8*)zipLoadFile(path->data(), filenameInArchive, size);
			if(buff)
				return buff;
		}
	}
	else
	{
		for(const auto &path : searchPath)
		{
			FileIO file{path->data(), IO::AccessHint::ALL, IO::OPEN_TEST};
			if(!file)
				continue;
			return fileToMallocBuffer(file, size);
		}
		// fallback to app assets
		auto file = appCtx.openAsset(filename, IO::AccessHint::ALL, IO::OPEN_TEST);
		if(file)
		{
			return fileToMallocBuffer(file, size);
		}
	}
	logErr("can't load ROM");
	return nullptr;
}

CLINK FILE *openMachineIni(const char *path, const char *mode)
{
	auto filePathInFirmwarePath = FS::pathString(machineBasePath(appCtx), path);
	FileIO file{filePathInFirmwarePath, IO::AccessHint::ALL, IO::OPEN_TEST};
	if(file)
	{
		return GenericIO{std::move(file)}.moveToFileStream(mode);
	}
	auto assetFile = appCtx.openAsset(path, IO::AccessHint::ALL, IO::OPEN_TEST);
	if(assetFile)
	{
		return GenericIO{std::move(assetFile)}.moveToFileStream(mode);
	}
	return {};
}
