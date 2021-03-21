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
#include <imagine/io/api/stdio.hh>
#include <imagine/fs/ArchiveFS.hh>
#include "internal.hh"

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
		if(string_equal(path.data(), subDir))
			return true;
	}
	return false;
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

static ArchiveIO archiveIOForSysFile(const char *archivePath, const char *sysFileName, char **complete_path_return)
{
	std::error_code ec{};
	for(auto &entry : FS::ArchiveIterator{archivePath, ec})
	{
		if(entry.type() == FS::file_type::directory)
		{
			continue;
		}
		auto name = entry.name();
		if(!string_equal(FS::basename(name).data(), sysFileName))
			continue;
		if(!containsSysFileDirName(FS::basename(FS::dirname(name).data())))
			continue;
		logMsg("archive file entry:%s", name);
		if(complete_path_return)
		{
			*complete_path_return = strdup(name);
			assert(*complete_path_return);
		}
		return entry.moveIO();
	}
	if(ec)
	{
		logErr("error opening archive:%s", archivePath);
	}
	else
	{
		logErr("not found in archive:%s", archivePath);
	}
	return {};
}

static AssetIO assetIOForSysFile(Base::ApplicationContext app, const char *sysFileName, char **complete_path_return)
{
	for(const auto &subDir : sysFileDirs)
	{
		auto fullPath = FS::makePathStringPrintf("%s/%s", subDir, sysFileName);
		auto file = EmuApp::openAppAssetIO(app, fullPath, IO::AccessHint::ALL);
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
		if(!strlen(basePath.data()) || !FS::exists(basePath))
			continue;
		if(EmuApp::hasArchiveExtension(basePath.data()))
		{
			auto io = archiveIOForSysFile(basePath.data(), name, complete_path_return);
			if(!io)
				continue;
			// Uncompress file into memory and wrap in FILE
			return io.moveToMapIO().makeGeneric().moveToFileStream(open_mode);
		}
		else
		{
			for(const auto &subDir : sysFileDirs)
			{
				auto fullPath = FS::makePathStringPrintf("%s/%s/%s", basePath.data(), subDir, name);
				auto file = fopen(fullPath.data(), open_mode);
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
			return io.makeGeneric().moveToFileStream(open_mode);
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
		if(!strlen(basePath.data()) || !FS::exists(basePath))
			continue;
		if(EmuApp::hasArchiveExtension(basePath.data()))
		{
			auto io = archiveIOForSysFile(basePath.data(), name, complete_path_return);
			if(!io)
				continue;
			return 0;
		}
		else
		{
			for(const auto &subDir : sysFileDirs)
			{
				auto fullPath = FS::makePathStringPrintf("%s/%s/%s", basePath.data(), subDir, name);
				if(FS::exists(fullPath))
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
		if(!strlen(basePath.data()) || !FS::exists(basePath))
			continue;
		if(EmuApp::hasArchiveExtension(basePath.data()))
		{
			auto io = archiveIOForSysFile(basePath.data(), name, nullptr);
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
			for(const auto &subDir : sysFileDirs)
			{
				auto fullPath = FS::makePathStringPrintf("%s/%s/%s", basePath.data(), subDir, name);
				FileIO file;
				file.open(fullPath.data(), IO::AccessHint::ALL);
				if(!file)
					continue;
				//logMsg("loading system file: %s", complete_path);
				auto size = loadSysFile(file, name, dest, minsize, maxsize);
				if(size == -1)
				{
					logErr("failed loading system file:%s from:%s", name, basePath.data());
					return -1;
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
	return strdup(FS::makePathString(EmuApp::supportPath(appContext).data(), "vice.rtc").data());
}
