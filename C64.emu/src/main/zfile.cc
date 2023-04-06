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
#include <imagine/io/IO.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/string.h>
#include <emuframework/EmuApp.hh>
#include <emuframework/FilePicker.hh>
#include "MainSystem.hh"

extern "C"
{
	#include "zfile.h"
}

using namespace EmuEx;

CLINK FILE *zfile_fopen(const char *path, const char *mode_)
{
	std::string_view mode{mode_};
	auto appContext = gAppContext();
	if(EmuApp::hasArchiveExtension(appContext.fileUriDisplayName(path)))
	{
		if(mode.contains('w') || mode.contains('+'))
		{
			logErr("opening archive %s with write mode not supported", path);
			return nullptr;
		}
		try
		{
			for(auto &entry : FS::ArchiveIterator{appContext.openFileUri(path)})
			{
				if(entry.type() == FS::file_type::directory)
				{
					continue;
				}
				if(EmuSystem::defaultFsFilter(entry.name()))
				{
					logMsg("archive file entry:%s", entry.name().data());
					return MapIO{entry.releaseIO()}.toFileStream(mode_);
				}
			}
			logErr("no recognized file extensions in archive:%s", path);
		}
		catch(...)
		{
			logErr("error opening archive:%s", path);
		}
		return nullptr;
	}
	else
	{
		return FileUtils::fopenUri(appContext, path, mode_);
	}
}

CLINK off_t archdep_file_size(FILE *stream)
{
	off_t pos = ftello(stream);
	if(pos == -1)
		return -1;
	fseeko(stream, 0, SEEK_END);
	off_t end = ftello(stream);
	fseeko(stream, pos, SEEK_SET);
	return end;
}
