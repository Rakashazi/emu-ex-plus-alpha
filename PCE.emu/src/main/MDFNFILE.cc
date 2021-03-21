/*  This file is part of PCE.emu.

	PCE.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	PCE.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with PCE.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "file"
#include <imagine/io/FileIO.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/util/string.h>
#include <imagine/logger/logger.h>
#include <emuframework/EmuApp.hh>
#include <emuframework/FilePicker.hh>
#include <mednafen/types.h>
#include <mednafen/git.h>
#include <mednafen/file.h>
#include <mednafen/memory.h>
#include <mednafen/MemoryStream.h>

using namespace Mednafen;

static bool hasKnownExtension(const char *name, const std::vector<FileExtensionSpecStruct>& extSpec)
{
	for(const auto &e : extSpec)
	{
		if(string_hasDotExtension(name, e.extension + 1)) // skip "."
			return true;
	}
	return false;
}

MDFNFILE::MDFNFILE(VirtualFS* vfs, const char* path, const std::vector<FileExtensionSpecStruct>& known_ext, const char* purpose):
	ext(f_ext), fbase(f_fbase), f_vfs{vfs}
{
	if(EmuApp::hasArchiveExtension(path))
	{
		std::error_code ec{};
		for(auto &entry : FS::ArchiveIterator{path, ec})
		{
			if(entry.type() == FS::file_type::directory)
			{
				continue;
			}
			auto name = entry.name();
			logMsg("archive file entry:%s", name);
			if(hasKnownExtension(name, known_ext))
			{
				auto io = entry.moveIO();
				str = std::make_unique<MemoryStream>(io.size(), true);
				if(io.read(str->map(), str->map_size()) != (int)str->map_size())
				{
					throw MDFN_Error(0, "Error reading archive");
				}
				auto extStr = strrchr(path, '.');
				f_ext = extStr ? extStr + 1 : "";
				return; // success
			}
		}
		if(ec)
		{
			throw MDFN_Error(0, "Error opening archive");
		}
		throw MDFN_Error(0, "No recognized file extensions in archive");
	}
	else
	{
		str.reset(vfs->open(path, VirtualFS::MODE_READ));
		auto extStr = strrchr(path, '.');
		f_ext = extStr ? extStr + 1 : "";
	}
}

MDFNFILE::MDFNFILE(VirtualFS* vfs, std::unique_ptr<Stream> str, const char *path, const char *purpose):
	ext(f_ext), fbase(f_fbase),
	str{std::move(str)},
	f_vfs{vfs}
{
	auto extStr = strrchr(path, '.');
	f_ext = extStr ? extStr + 1 : "";
}
