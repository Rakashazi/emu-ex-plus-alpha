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
#include <emuframework/FilePicker.hh>
#include <mednafen/git.h>
#include <mednafen/file.h>
#include <mednafen/memory.h>
#include <mednafen/MemoryStream.h>

static bool hasKnownExtension(const char *name, const FileExtensionSpecStruct *extSpec)
{
	while(extSpec->extension)
	{
		if(string_hasDotExtension(name, extSpec->extension + 1)) // skip "." in extSpec->extension
			return true;
		extSpec++;
	}
	return false;
}

MDFNFILE::MDFNFILE(const char *path, const FileExtensionSpecStruct *known_ext, const char *purpose):
	ext(f_ext), fbase(f_fbase)
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
				f_ext = strdup(extStr ? extStr + 1 : "");
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
		FileIO file;
		file.open(path);
		if(!file)
		{
			throw MDFN_Error(0, "Error opening file");
		}
		str = std::make_unique<MemoryStream>(file.size(), true);
		if(file.read(str->map(), str->map_size()) != (int)str->map_size())
		{
			throw MDFN_Error(0, "Error reading file");
		}
		auto extStr = strrchr(path, '.');
		f_ext = strdup(extStr ? extStr + 1 : "");
	}
}

MDFNFILE::MDFNFILE(std::unique_ptr<Stream> str, const char *path, const char *purpose):
	ext(f_ext), fbase(f_fbase),
	str{std::move(str)}
{
	auto extStr = strrchr(path, '.');
	f_ext = strdup(extStr ? extStr + 1 : "");
}

MDFNFILE::~MDFNFILE()
{
	Close();
}
