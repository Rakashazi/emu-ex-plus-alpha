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

extern "C"
{
	#include "sysfile.h"
	#include "lib.h"
	#include "archdep.h"
}

namespace EmuEx
{

static int loadSysFile(Readable auto &file, const char *name, uint8_t *dest, int minsize, int maxsize)
{
	//logMsg("loading system file: %s", complete_path);
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
		logErr("ROM %s: short file", name);
		return -1;
	}
	if(rsize == (maxsize + 2))
	{
		logWarn("ROM `%s': two bytes too large - removing assumed start address", name);
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
		logWarn("ROM `%s': long file, discarding end.", name);
		rsize = maxsize;
	}
	if((rsize = file.read((char *)dest, rsize)) < minsize)
		return -1;

	return (int)rsize;
}

static ArchiveIO archiveIOForSysFile(IG::CStringView archivePath, std::string_view sysFileName, std::string_view subPath, char **complete_path_return)
{
	auto sysFilePath = FS::pathString(subPath, sysFileName);
	try
	{
		for(auto &entry : FS::ArchiveIterator{gAppContext().openFileUri(archivePath)})
		{
			if(entry.type() == FS::file_type::directory)
			{
				continue;
			}
			auto name = entry.name();
			if(!name.ends_with(sysFilePath))
				continue;
			logMsg("archive file entry:%s", name.data());
			if(complete_path_return)
			{
				*complete_path_return = strdup(name.data());
				assert(*complete_path_return);
			}
			return entry.releaseIO();
		}
		logErr("not found in archive:%s", archivePath.data());
	}
	catch(...)
	{
		logErr("error opening archive:%s", archivePath.data());
	}
	return {};
}

static AssetIO assetIOForSysFile(IG::ApplicationContext ctx, std::string_view sysFileName, std::string_view subPath, char **complete_path_return)
{
	auto fullPath = FS::pathString(subPath, sysFileName);
	auto file = ctx.openAsset(fullPath, IOAccessHint::All, {.test = true});
	if(!file)
		return {};
	if(complete_path_return)
	{
		*complete_path_return = strdup(fullPath.c_str());
		assert(*complete_path_return);
	}
	return file;
}

std::vector<std::string> C64System::systemFilesWithExtension(const char *ext) const
{
	logMsg("looking for system files with extension:%s", ext);
	auto appContext = gAppContext();
	std::vector<std::string> filenames{};
	try
	{
		for(const auto &basePath : sysFilePath)
		{
			if(basePath.empty())
				continue;
			auto displayName = appContext.fileUriDisplayName(basePath);
			if(displayName.empty())
				continue;
			if(EmuApp::hasArchiveExtension(displayName))
			{
				for(auto &entry : FS::ArchiveIterator{appContext.openFileUri(basePath)})
				{
					if(entry.type() == FS::file_type::directory)
					{
						continue;
					}
					auto name = entry.name();
					if(FS::basename(FS::dirname(name)) != sysFileDir)
						continue;
					if(name.ends_with(ext))
					{
						logMsg("archive file entry:%s", name.data());
						filenames.emplace_back(FS::basename(name));
					}
				}
			}
			else
			{
				appContext.forEachInDirectoryUri(FS::uriString(basePath, sysFileDir),
					[&filenames, ext](auto &entry)
					{
						auto name = entry.name();
						if(name.ends_with(ext))
						{
							logMsg("file entry:%s", name.data());
							filenames.emplace_back(name);
						}
						return true;
					});
			}
		}
	}
	catch(...)
	{
		logErr("error while getting system files");
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
	logMsg("sysfile open:%s subPath:%s", name, subPath);
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
			auto io = archiveIOForSysFile(basePath, name, subPath, complete_path_return);
			if(!io)
				continue;
			// Uncompress file into memory and wrap in FILE
			return MapIO{std::move(io)}.toFileStream(open_mode);
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
	logErr("can't open %s in system paths", name);
	system.lastMissingSysFile = name;
	return nullptr;
}

CLINK int sysfile_locate(const char *name, const char *subPath, char **complete_path_return)
{
	logMsg("sysfile locate:%s subPath:%s", name, subPath);
	auto appContext = gAppContext();
	auto &sysFilePath = static_cast<C64System&>(gSystem()).sysFilePath;
	for(const auto &basePath : sysFilePath)
	{
		if(basePath.empty())
			continue;
		auto displayName = appContext.fileUriDisplayName(basePath);
		if(displayName.empty())
			continue;
		if(EmuApp::hasArchiveExtension(displayName))
		{
			auto io = archiveIOForSysFile(basePath, name, subPath, complete_path_return);
			if(!io)
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
	logErr("%s not found in system paths", name);
	if(complete_path_return)
	{
		*complete_path_return = nullptr;
	}
	return -1;
}

CLINK int sysfile_load(const char *name, const char *subPath, uint8_t *dest, int minsize, int maxsize)
{
	logMsg("sysfile load:%s subPath:%s", name, subPath);
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
			auto io = archiveIOForSysFile(basePath, name, subPath, nullptr);
			if(!io)
				continue;
			auto size = loadSysFile(io, name, dest, minsize, maxsize);
			if(size == -1)
			{
				logErr("failed loading system file:%s from:%s", name, basePath.data());
				return -1;
			}
			return size;
		}
		else
		{
			auto file = appContext.openFileUri(FS::uriString(basePath, subPath, name), IOAccessHint::All, {.test = true});
			if(!file)
				continue;
			//logMsg("loading system file: %s", complete_path);
			auto size = loadSysFile(file, name, dest, minsize, maxsize);
			if(size == -1)
			{
				logErr("failed loading system file:%s from:%s", name, basePath.data());
				continue;
			}
			return size;
		}
	}
	logErr("can't load %s in system paths", name);
	system.lastMissingSysFile = name;
	return -1;
}

CLINK char *archdep_default_rtc_file_name(void)
{
	return strdup(FS::pathString(gAppContext().supportPath(), "vice.rtc").data());
}
