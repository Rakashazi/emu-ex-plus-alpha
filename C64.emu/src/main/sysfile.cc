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
#include "internal.hh"
#include <imagine/io/api/stdio.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>

extern "C"
{
	#include "sysfile.h"
	#include "lib.h"
	#include "archdep.h"
}

static std::array<const char*, 3> sysFileDirs =
{
	"",
	"DRIVES",
	"PRINTER",
};

static bool containsSysFileDirName(FS::FileString path)
{
	for(const auto &subDir : sysFileDirs)
	{
		if(path == subDir)
			return true;
	}
	return false;
}

static bool containsEmuSysFileDirName(FS::FileString path)
{
	return path == sysFileDirs[0];
}

static int loadSysFile(IO &file, const char *name, uint8_t *dest, int minsize, int maxsize)
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

static ArchiveIO archiveIOForSysFile(IG::CStringView archivePath, std::string_view sysFileName, char **complete_path_return)
{
	try
	{
		for(auto &entry : FS::ArchiveIterator{appContext.openFileUri(archivePath)})
		{
			if(entry.type() == FS::file_type::directory)
			{
				continue;
			}
			auto name = entry.name();
			if(FS::basename(name) != sysFileName)
				continue;
			if(!containsSysFileDirName(FS::basename(FS::dirname(name))))
				continue;
			logMsg("archive file entry:%s", name.data());
			if(complete_path_return)
			{
				*complete_path_return = strdup(name.data());
				assert(*complete_path_return);
			}
			return entry.moveIO();
		}
		logErr("not found in archive:%s", archivePath.data());
	}
	catch(...)
	{
		logErr("error opening archive:%s", archivePath.data());
	}
	return {};
}

static AssetIO assetIOForSysFile(Base::ApplicationContext ctx, std::string_view sysFileName, char **complete_path_return)
{
	for(const auto &subDir : sysFileDirs)
	{
		auto fullPath = FS::pathString(subDir, sysFileName);
		auto file = ctx.openAsset(fullPath, IO::AccessHint::ALL, IO::OPEN_TEST);
		if(!file)
			continue;
		if(complete_path_return)
		{
			*complete_path_return = strdup(fullPath.data());
			assert(*complete_path_return);
		}
		return file;
	}
	return {};
}

std::vector<std::string> systemFilesWithExtension(const char *ext)
{
	logMsg("looking for system files with extension:%s", ext);
	std::vector<std::string> filenames{};
	for(const auto &basePath : sysFilePath)
	{
		if(basePath.empty() || !appContext.fileUriExists(basePath))
			continue;
		if(EmuApp::hasArchiveExtension(appContext.fileUriDisplayName(basePath)))
		{
			for(auto &entry : FS::ArchiveIterator{appContext.openFileUri(basePath)})
			{
				if(entry.type() == FS::file_type::directory)
				{
					continue;
				}
				auto name = entry.name();
				if(!containsEmuSysFileDirName(FS::basename(FS::dirname(name))))
					continue;
				if(FS::basename(name).ends_with(ext))
				{
					logMsg("archive file entry:%s", name.data());
					filenames.emplace_back(FS::basename(name));
				}
			}
		}
		else
		{
			auto fullPath = FS::pathString(basePath, sysFileDirs[0]);
			for(auto &entry : FS::directory_iterator{fullPath})
			{
				auto name = entry.name();
				if(FS::basename(name).ends_with(ext))
				{
					logMsg("archive file entry:%s", name.data());
					filenames.emplace_back(name);
				}
			}
		}
	}
	std::sort(filenames.begin(), filenames.end());
	return filenames;
}

CLINK int sysfile_init(const char *emu_id)
{
	sysFileDirs[0] = emu_id;
	return 0;
}

CLINK FILE *sysfile_open(const char *name, char **complete_path_return, const char *open_mode)
{
	logMsg("sysfile open:%s", name);
	for(const auto &basePath : sysFilePath)
	{
		if(basePath.empty())
			continue;
		if(EmuApp::hasArchiveExtension(appContext.fileUriDisplayName(basePath)))
		{
			auto io = archiveIOForSysFile(basePath, name, complete_path_return);
			if(!io)
				continue;
			// Uncompress file into memory and wrap in FILE
			return GenericIO{MapIO{std::move(io)}}.moveToFileStream(open_mode);
		}
		else
		{
			if(!appContext.fileUriExists(basePath))
				continue;
			for(const auto &subDir : sysFileDirs)
			{
				auto fullPath = FS::pathString(basePath, subDir, name);
				auto file = FileUtils::fopenUri(appContext, fullPath, open_mode);
				if(!file)
					continue;
				if(complete_path_return)
				{
					*complete_path_return = strdup(fullPath.data());
					assert(*complete_path_return);
				}
				return file;
			}
		}
	}
	// fallback to asset path
	{
		auto io = assetIOForSysFile(appContext, name, complete_path_return);
		if(io)
		{
			return GenericIO{std::move(io)}.moveToFileStream(open_mode);
		}
	}
	logErr("can't open %s in system paths", name);
	return nullptr;
}

CLINK int sysfile_locate(const char *name, char **complete_path_return)
{
	logMsg("sysfile locate:%s", name);
	for(const auto &basePath : sysFilePath)
	{
		if(basePath.empty())
			continue;
		if(EmuApp::hasArchiveExtension(appContext.fileUriDisplayName(basePath)))
		{
			auto io = archiveIOForSysFile(basePath, name, complete_path_return);
			if(!io)
				continue;
			return 0;
		}
		else
		{
			if(!appContext.fileUriExists(basePath))
				continue;
			for(const auto &subDir : sysFileDirs)
			{
				auto fullPath = FS::pathString(basePath, subDir, name);
				if(appContext.fileUriExists(fullPath))
				{
					if(complete_path_return)
					{
						*complete_path_return = strdup(fullPath.data());
					}
					return 0;
				}
			}
		}
	}
	// fallback to asset path
	{
		auto io = assetIOForSysFile(appContext, name, complete_path_return);
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

CLINK int sysfile_load(const char *name, uint8_t *dest, int minsize, int maxsize)
{
	logMsg("sysfile load:%s", name);
	for(const auto &basePath : sysFilePath)
	{
		if(basePath.empty())
			continue;
		if(EmuApp::hasArchiveExtension(appContext.fileUriDisplayName(basePath)))
		{
			auto io = archiveIOForSysFile(basePath, name, nullptr);
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
			if(!appContext.fileUriExists(basePath))
				continue;
			for(const auto &subDir : sysFileDirs)
			{
				auto fullPath = FS::pathString(basePath, subDir, name);
				auto file = appContext.openFileUri(fullPath, IO::AccessHint::ALL, IO::OPEN_TEST);
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
	}
	logErr("can't load %s in system paths", name);
	return -1;
}

CLINK char *archdep_default_rtc_file_name(void)
{
	return strdup(FS::pathString(appContext.supportPath(), "vice.rtc").data());
}
