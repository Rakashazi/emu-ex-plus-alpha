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

namespace EmuEx
{
IG::ApplicationContext gAppContext();
}

using namespace EmuEx;

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
	if(filenameInArchive && strlen(filenameInArchive))
	{
		auto buff = (UInt8*)zipLoadFile(filename, filenameInArchive, size);
		if(buff)
			return buff;
		logErr("can't load ROM from zip");
		return nullptr;
	}
	else
	{
		auto &sys = gSystem();
		auto appCtx = sys.appContext();
		if(filename[0] == '/' || IG::isUri(filename)) // try to load absolute path directly
		{
			auto file = appCtx.openFileUri(filename, IO::AccessHint::ALL, IO::TEST_BIT);
			if(file)
			{
				return fileToMallocBuffer(file, size);
			}
			logErr("can't load ROM from absolute path");
			return nullptr;
		}
		// relative path, try firmware directory
		{
			auto file = appCtx.openFileUri(FS::uriString(machineBasePath(sys), filename), IO::AccessHint::ALL, IO::TEST_BIT);
			if(file)
			{
				return fileToMallocBuffer(file, size);
			}
		}
		// fallback to app assets
		{
			auto file = appCtx.openAsset(filename, IO::AccessHint::ALL, IO::TEST_BIT);
			if(file)
			{
				return fileToMallocBuffer(file, size);
			}
		}
		logErr("can't load ROM from relative path");
		return nullptr;
	}
}

CLINK FILE *openMachineIni(const char *path, const char *mode)
{
	auto &sys = gSystem();
	auto appCtx = sys.appContext();
	auto filePathInFirmwarePath = FS::uriString(machineBasePath(sys), path);
	auto file = appCtx.openFileUri(filePathInFirmwarePath, IO::AccessHint::ALL, IO::TEST_BIT);
	if(file)
	{
		return GenericIO{std::move(file)}.moveToFileStream(mode);
	}
	auto assetFile = appCtx.openAsset(path, IO::AccessHint::ALL, IO::TEST_BIT);
	if(assetFile)
	{
		return GenericIO{std::move(assetFile)}.moveToFileStream(mode);
	}
	return {};
}
