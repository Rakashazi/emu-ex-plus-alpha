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
#include <emuframework/EmuSystem.hh>
#include <emuframework/CommonFrameworkIncludes.hh>
#include <imagine/io/api/stdio.hh>

extern "C"
{
	#include "sysfile.h"
}

extern FsSys::PathString sysFilePath[Config::envIsLinux ? 4 : 2][3];

CLINK FILE *sysfile_open(const char *name, char **complete_path_return, const char *open_mode)
{
	for(const auto &pathGroup : sysFilePath)
	{
		for(const auto &p : pathGroup)
		{
			if(!strlen(p.data()))
				continue;
			auto fullPath = makeFSPathStringPrintf("%s/%s", p.data(), name);
			auto file = fopen(fullPath.data(), open_mode);
			if(file)
			{
				if(complete_path_return)
				{
					*complete_path_return = strdup(fullPath.data());
					if(!*complete_path_return)
					{
						logErr("out of memory trying to allocate string in sysfile_open");
						fclose(file);
						return nullptr;
					}
				}
				return file;
			}
		}
	}
	logErr("can't open %s in system paths", name);
	return nullptr;
}

CLINK int sysfile_locate(const char *name, char **complete_path_return)
{
	for(const auto &pathGroup : sysFilePath)
	{
		for(const auto &p : pathGroup)
		{
			if(!strlen(p.data()))
				continue;
			auto fullPath = makeFSPathStringPrintf("%s/%s", p.data(), name);
			if(FsSys::fileExists(fullPath.data()))
			{
				if(complete_path_return)
				{
					*complete_path_return = strdup(fullPath.data());
					if(!*complete_path_return)
					{
						logErr("out of memory trying to allocate string in sysfile_locate");
						return -1;
					}
				}
				return 0;
			}
		}
	}
	logErr("%s not found in system paths", name);
	return -1;
}

CLINK int sysfile_load(const char *name, BYTE *dest, int minsize, int maxsize)
{
	for(const auto &pathGroup : sysFilePath)
	{
		for(const auto &p : pathGroup)
		{
			if(!strlen(p.data()))
				continue;
			auto complete_path = makeFSPathStringPrintf("%s/%s", p.data(), name);
			FileIO file;
			file.open(complete_path.data());
			if(file)
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
					logErr("ROM %s: short file", complete_path.data());
					goto fail;
				}
				if(rsize == (maxsize + 2))
				{
					logWarn("ROM `%s': two bytes too large - removing assumed start address", complete_path.data());
					if(file.read((char*)dest, 2) < 2)
					{
						goto fail;
					}
					rsize -= 2;
				}
				if(load_at_end && rsize < (maxsize))
				{
					dest += maxsize - rsize;
				}
				else if(rsize > (maxsize))
				{
					logWarn("ROM `%s': long file, discarding end.", complete_path.data());
					rsize = maxsize;
				}
				if((rsize = file.read((char *)dest, rsize)) < minsize)
					goto fail;

				return (int)rsize;

				fail:
					logErr("failed loading system file: %s", name);
					return -1;
			}
		}
	}
	logErr("can't load %s in system paths", name);
	return -1;
}
