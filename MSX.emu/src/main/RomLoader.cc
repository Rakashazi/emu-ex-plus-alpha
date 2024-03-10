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
#include <imagine/io/IO.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>
#include <string.h>
#include "MainSystem.hh"

extern "C"
{
	#include <blueMSX/Memory/RomLoader.h>
}

namespace EmuEx
{

IG::ApplicationContext gAppContext();

constexpr SystemLogger log{"MSXemu"};

ArchiveIO &MsxSystem::firmwareArchive(CStringView path) const
{
	if(!firmwareArch)
	{
		log.info("{} not cached, opening archive", path);
		firmwareArch = {appContext().openFileUri(path)};
	}
	else
	{
		firmwareArch.rewind();
	}
	return firmwareArch;
}

}

using namespace IG;
using namespace EmuEx;

static UInt8 *fileToMallocBuffer(Readable auto &file, int *size)
{
	int fileSize = file.size();
	auto buff = (UInt8*)malloc(fileSize);
	file.read(buff, fileSize);
	*size = fileSize;
	return buff;
}

static IO fileFromFirmwarePath(CStringView path)
{
	auto &sys = static_cast<MsxSystem&>(gSystem());
	auto appCtx = sys.appContext();
	auto firmwarePath = sys.firmwarePath();
	if(firmwarePath.size())
	{
		try
		{
			if(FS::hasArchiveExtension(firmwarePath))
			{
				auto &arch = sys.firmwareArchive(firmwarePath);
				if(FS::seekFileInArchive(arch, [&](auto &entry){ return entry.name().ends_with(path.data()); }))
				{
					return MapIO{arch};
				}
			}
			else
			{
				return appCtx.openFileUri(FS::uriString(firmwarePath, path), {.accessHint = IOAccessHint::All});
			}
		}
		catch(...)
		{
			EmuEx::log.error("error opening path:{}", path);
		}
	}
	// fall back to asset path
	auto assetFile = appCtx.openAsset(path, {.test = true, .accessHint = IOAccessHint::All});
	if(assetFile)
	{
		return assetFile;
	}
	EmuEx::log.error("{} not found in firmware path", path);
	return {};
}

UInt8 *romLoad(const char *filename, const char *filenameInArchive, int *size)
{
	if(!filename || !strlen(filename))
		return nullptr;
	EmuEx::log.info("loading ROM file:{}:{}", filename, filenameInArchive);
	if(filenameInArchive && strlen(filenameInArchive))
	{
		auto buff = (UInt8*)zipLoadFile(filename, filenameInArchive, size);
		if(buff)
			return buff;
		EmuEx::log.error("can't load ROM from zip");
		return nullptr;
	}
	else
	{
		auto &sys = static_cast<MsxSystem&>(gSystem());
		auto appCtx = sys.appContext();
		if(filename[0] == '/' || IG::isUri(filename)) // try to load absolute path directly
		{
			auto file = appCtx.openFileUri(filename, {.test = true, .accessHint = IOAccessHint::All});
			if(file)
			{
				return fileToMallocBuffer(file, size);
			}
			EmuEx::log.error("can't load ROM from absolute path");
			return nullptr;
		}
		// relative path, try firmware directory
		{
			auto file = fileFromFirmwarePath(filename);
			if(file)
			{
				return fileToMallocBuffer(file, size);
			}
		}
		EmuEx::log.error("can't load ROM from relative path");
		return nullptr;
	}
}

CLINK FILE *openMachineIni(const char *filename, const char *mode)
{
	EmuEx::log.info("loading machine ini:{}", filename);
	auto file = fileFromFirmwarePath(filename);
	if(file)
	{
		return MapIO{std::move(file)}.toFileStream(mode);
	}
	return {};
}
