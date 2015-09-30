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
	size(f_size), data((const uint8* const &)f_data),
	ext((const char * const &)f_ext), fbase((const char * const &)f_fbase)
{
	if(hasArchiveExtension(path))
	{
		CallResult res = OK;
		for(auto &entry : FS::ArchiveIterator{path, res})
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
				f_size = io.size();
				f_data = (uint8*)MDFN_malloc_T(f_size, "file read buffer");
				if(io.read(f_data, f_size) != f_size)
				{
					throw MDFN_Error(0, "Error reading archive");
				}
				auto extStr = strrchr(path, '.');
				f_ext = strdup(extStr ? extStr + 1 : "");
				return; // success
			}
		}
		if(res != OK)
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
		f_size = file.size();
		f_data = (uint8*)MDFN_malloc_T(f_size, "file read buffer");
		if(file.read(f_data, f_size) != f_size)
		{
			throw MDFN_Error(0, "Error reading file");
		}
		auto extStr = strrchr(path, '.');
		f_ext = strdup(extStr ? extStr + 1 : "");
	}
}

MDFNFILE::MDFNFILE(IO &io, const char *path, const char *purpose):
	size(f_size), data((const uint8* const &)f_data),
	ext((const char * const &)f_ext), fbase((const char * const &)f_fbase)
{
	f_size = io.size();
	f_data = (uint8*)MDFN_malloc_T(f_size, "file read buffer");
	if(io.read(f_data, f_size) != f_size)
	{
		throw MDFN_Error(0, "Error reading file");
	}
	auto extStr = strrchr(path, '.');
	f_ext = strdup(extStr ? extStr + 1 : "");
}

MDFNFILE::~MDFNFILE()
{
	Close();
}
