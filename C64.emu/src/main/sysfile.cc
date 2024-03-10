/*  This file is part of C64.emu.

	C64.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	C64.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with C64.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "sysfile"
#include <array>
#include <emuframework/EmuApp.hh>
#include <emuframework/FilePicker.hh>
#include "MainSystem.hh"
#include <imagine/io/IO.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>

extern "C"
{
	#include "sysfile.h"
	#include "lib.h"
	#include "archdep.h"
}

namespace EmuEx
{

constexpr SystemLogger log{"C64.emu"};

static int loadSysFile(Readable auto &file, const char *name, uint8_t *dest, int minsize, int maxsize)
{
	//log.debug("loading system file:{}", name);
	ssize_t rsize = file.size();
	bool load_at_end;
	if(minsize < 0)
	{
		minsize = -minsize;
		load_at_end = 0;
	}
	else
	{
		load_at_end = 1;
	}
	if(rsize < (minsize))
	{
		log.error("ROM {}: short file", name);
		return -1;
	}
	if(rsize == (maxsize + 2))
	{
		log.warn("ROM {}: two bytes too large - removing assumed start address", name);
		if(file.read((char*)dest, 2) < 2)
		{
			return -1;
		}
		rsize -= 2;
	}
	if(load_at_end && rsize < (maxsize))
	{
		dest += maxsize - rsize;
	}
	else if(rsize > (maxsize))
	{
		log.warn("ROM {}: long file, discarding end.", name);
		rsize = maxsize;
	}
	if((rsize = file.read((char *)dest, rsize)) < minsize)
		return -1;

	return (int)rsize;
}

static ArchiveIO *archiveIOForSysFile(C64System &system, IG::CStringView archivePath, std::string_view sysFileName, std::string_view subPath, char **complete_path_return)
{
	auto sysFilePath = FS::pathString(subPath, sysFileName);
	try
	{
		auto &arch = system.firmwareArchive(archivePath);
		if(FS::seekFileInArchive(arch, [&](auto &entry)
		{
			auto name = entry.name();
			if(!name.ends_with(sysFilePath))
				return false;
			log.info("found file in archive:{}", name);
			if(complete_path_return)
			{
				*complete_path_return = strdup(name.data());
				assert(*complete_path_return);
			}
			return true;
		}))
		{
			return &arch;
		}
		else
		{
			log.error("not found in archive:{}", archivePath);
		}
	}
	catch(...)
	{
		log.error("error opening archive:{}", archivePath);
	}
	return {};
}

static AssetIO assetIOForSysFile(IG::ApplicationContext ctx, std::string_view sysFileName, std::string_view subPath, char **complete_path_return)
{
	auto fullPath = FS::pathString(subPath, sysFileName);
	auto file = ctx.openAsset(fullPath, {.test = true, .accessHint = IOAccessHint::All});
	if(!file)
		return {};
	if(complete_path_return)
	{
		*complete_path_return = strdup(fullPath.c_str());
		assert(*complete_path_return);
	}
	return file;
}

ArchiveIO &C64System::firmwareArchive(CStringView path) const
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

static bool archiveHasDrivesDirectory(ApplicationContext ctx, CStringView path)
{
	return bool(FS::findDirectoryInArchive(ctx.openFileUri(path), [&](auto &entry){ return entry.name().ends_with("DRIVES/"); }));
}

void C64System::setSystemFilesPath(CStringView path, FS::file_type type)
{
	auto ctx = appContext();
	log.info("set firmware path:{}", path);
	if((type == FS::file_type::directory && !ctx.fileUriExists(FS::uriString(path, "DRIVES")))
		|| (FS::hasArchiveExtension(path) && !archiveHasDrivesDirectory(ctx, path)))
	{
		throw std::runtime_error{"Path is missing DRIVES folder"};
	}
	sysFilePath[0] = path;
	firmwareArch = {};
}

std::vector<std::string> C64System::systemFilesWithExtension(const char *ext) const
{
	log.info("looking for system files with extension:{}", ext);
	std::vector<std::string> filenames{};
	try
	{
		for(const auto &basePath : sysFilePath)
		{
			if(basePath.empty())
				continue;
			auto displayName = appContext().fileUriDisplayName(basePath);
			if(displayName.empty())
				continue;
			if(EmuApp::hasArchiveExtension(displayName))
			{
				firmwareArchive(basePath).forAllEntries([&](auto &entry)
				{
					auto name = entry.name();
					if(entry.type() == FS::file_type::directory
						|| !name.ends_with(ext)
						|| FS::basename(FS::dirname(name)) != sysFileDir)
					{
						return;
					}
					log.info("found file in archive:{}", name);
					filenames.emplace_back(FS::basename(name));
				});
			}
			else
			{
				appContext().forEachInDirectoryUri(FS::uriString(basePath, sysFileDir),
					[&filenames, ext](auto &entry)
					{
						auto name = entry.name();
						if(name.ends_with(ext))
						{
							log.info("found file:{}", name);
							filenames.emplace_back(name);
						}
						return true;
					});
			}
		}
	}
	catch(...)
	{
		log.error("error while getting system files");
	}
	std::sort(filenames.begin(), filenames.end());
	return filenames;
}

}

using namespace EmuEx;

CLINK int sysfile_init(const char *emu_id)
{
	static_cast<C64System&>(gSystem()).sysFileDir = emu_id;
	return 0;
}

CLINK FILE *sysfile_open(const char *name, const char *subPath, char **complete_path_return, const char *open_mode)
{
	EmuEx::log.info("sysfile open:{} subPath:{}", name, subPath);
	auto appContext = gAppContext();
	auto &system = static_cast<C64System&>(gSystem());
	for(const auto &basePath : system.sysFilePath)
	{
		if(basePath.empty())
			continue;
		auto displayName = appContext.fileUriDisplayName(basePath);
		if(displayName.empty())
			continue;
		if(EmuApp::hasArchiveExtension(displayName))
		{
			auto ioPtr = archiveIOForSysFile(system, basePath, name, subPath, complete_path_return);
			if(!ioPtr)
				continue;
			// Uncompress file into memory and wrap in FILE
			return MapIO{*ioPtr}.toFileStream(open_mode);
		}
		else
		{
			auto fullPath = FS::uriString(basePath, subPath, name);
			auto file = FileUtils::fopenUri(appContext, fullPath, open_mode);
			if(!file)
				continue;
			if(complete_path_return)
			{
				*complete_path_return = strdup(fullPath.c_str());
				assert(*complete_path_return);
			}
			return file;
		}
	}
	// fallback to asset path
	{
		auto io = assetIOForSysFile(appContext, name, subPath, complete_path_return);
		if(io)
		{
			return io.toFileStream(open_mode);
		}
	}
	EmuEx::log.error("can't open {} in system paths", name);
	system.lastMissingSysFile = name;
	return nullptr;
}

CLINK int sysfile_locate(const char *name, const char *subPath, char **complete_path_return)
{
	EmuEx::log.info("sysfile locate:{} subPath:{}", name, subPath);
	auto appContext = gAppContext();
	auto &system = static_cast<C64System&>(gSystem());
	for(const auto &basePath : system.sysFilePath)
	{
		if(basePath.empty())
			continue;
		auto displayName = appContext.fileUriDisplayName(basePath);
		if(displayName.empty())
			continue;
		if(EmuApp::hasArchiveExtension(displayName))
		{
			auto ioPtr = archiveIOForSysFile(system, basePath, name, subPath, complete_path_return);
			if(!ioPtr)
				continue;
			return 0;
		}
		else
		{
			auto fullPath = FS::uriString(basePath, subPath, name);
			if(appContext.fileUriExists(fullPath))
			{
				if(complete_path_return)
				{
					*complete_path_return = strdup(fullPath.c_str());
				}
				return 0;
			}
		}
	}
	// fallback to asset path
	{
		auto io = assetIOForSysFile(appContext, name, subPath, complete_path_return);
		if(io)
		{
			return 0;
		}
	}
	EmuEx::log.error("{} not found in system paths", name);
	if(complete_path_return)
	{
		*complete_path_return = nullptr;
	}
	return -1;
}

CLINK int sysfile_load(const char *name, const char *subPath, uint8_t *dest, int minsize, int maxsize)
{
	EmuEx::log.info("sysfile load:{} subPath:{}", name, subPath);
	auto appContext = gAppContext();
	auto &system = static_cast<C64System&>(gSystem());
	for(const auto &basePath : system.sysFilePath)
	{
		if(basePath.empty())
			continue;
		auto displayName = appContext.fileUriDisplayName(basePath);
		if(displayName.empty())
			continue;
		if(EmuApp::hasArchiveExtension(displayName))
		{
			auto ioPtr = archiveIOForSysFile(system, basePath, name, subPath, nullptr);
			if(!ioPtr)
				continue;
			auto size = loadSysFile(*ioPtr, name, dest, minsize, maxsize);
			if(size == -1)
			{
				EmuEx::log.error("failed loading system file:{} from:{}", name, basePath);
				return -1;
			}
			return size;
		}
		else
		{
			auto file = appContext.openFileUri(FS::uriString(basePath, subPath, name), {.test = true, .accessHint = IOAccessHint::All});
			if(!file)
				continue;
			auto size = loadSysFile(file, name, dest, minsize, maxsize);
			if(size == -1)
			{
				EmuEx::log.error("failed loading system file:{} from:{}", name, basePath);
				continue;
			}
			return size;
		}
	}
	EmuEx::log.error("can't load {} in system paths", name);
	system.lastMissingSysFile = name;
	return -1;
}

CLINK char *archdep_default_rtc_file_name(void)
{
	return strdup(FS::pathString(gAppContext().supportPath(), "vice.rtc").data());
}
