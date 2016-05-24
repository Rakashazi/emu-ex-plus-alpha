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

#include <imagine/fs/ArchiveFS.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/string.h>
#include <emuframework/FilePicker.hh>

extern "C"
{
	#include "zfile.h"
}

CLINK FILE *zfile_fopen(const char *path, const char *mode)
{
	if(hasArchiveExtension(path))
	{
		if(strchr(mode, 'w'))
		{
			logErr("opening archive %s with write mode not supported", path);
			return nullptr;
		}
		CallResult res = OK;
		for(auto &entry : FS::ArchiveIterator{path, res})
		{
			if(entry.type() == FS::file_type::directory)
			{
				continue;
			}
			auto name = entry.name();
			logMsg("archive file entry:%s", name);
			if(EmuSystem::defaultFsFilter(name))
			{
				return GenericIO{entry.moveIO().moveToMapIO()}.moveToFileStream(mode);
			}
		}
		if(res != OK)
		{
			logErr("error opening archive:%s", path);
			return nullptr;
		}
		logErr("no recognized file extensions in archive:%s", path);
		return nullptr;
	}
	else
	{
		return fopen(path, mode);
	}
}
