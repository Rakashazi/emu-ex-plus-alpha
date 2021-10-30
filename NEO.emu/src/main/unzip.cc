/*  This file is part of NEO.emu.

	NEO.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NEO.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NEO.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "unzip"
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/string.h>
#include <cstdlib>

extern "C"
{
	#include <gngeo/unzip.h>
}

struct PKZIP : public FS::ArchiveIterator {};

struct ZFILE
{
	ArchiveIO io;
	FS::ArchiveIterator *arch;
};

ZFILE *gn_unzip_fopen(PKZIP *archPtr, const char *filename, uint32_t fileCRC)
{
	auto &arch = *archPtr;
	arch.rewind();
	for(auto &entry : arch)
	{
		if(entry.type() == FS::file_type::directory)
		{
			continue;
		}
		auto name = entry.name();
		auto crc = entry.crc32();
		//logMsg("archive file entry:%s crc32:0x%X", name, crc);
		int loadByName = fileCRC == (uint32_t)-1 || !gn_strictROMChecking();
		if((loadByName && (string_equal(name, filename))) || crc == fileCRC)
		{
			//logMsg("opened archive entry file:%s crc32:0x%X", name, crc);
			return new ZFILE{entry.moveIO(), archPtr};
		}
	}
	logMsg("file:%s crc32:0x%X not found in archive", filename, fileCRC);
	return nullptr;
}

void gn_unzip_fclose(ZFILE *z)
{
	//logMsg("done with archive entry");
	*z->arch = z->io.releaseArchive();
	delete z;
}

int gn_unzip_fread(ZFILE *z, uint8_t *data, unsigned int size)
{
	//logMsg("reading %u bytes to %p", size, data);
	return z->io.read(data, size);
}

PKZIP *gn_open_zip(const char *path)
{
	try
	{
		auto arch = std::make_unique<FS::ArchiveIterator>(path);
		return static_cast<PKZIP*>(arch.release());
	}
	catch(...)
	{
		logErr("error opening archive:%s", path);
		return nullptr;
	}
}

void gn_close_zip(PKZIP *archPtr)
{
	delete archPtr;
}

uint8_t *gn_unzip_file_malloc(PKZIP *archPtr, const char *filename, uint32_t fileCRC, unsigned int *outlen)
{
	auto z = gn_unzip_fopen(archPtr, filename, fileCRC);
	if(!z)
	{
		return nullptr;
	}
	auto closeZ = IG::scopeGuard([&](){ gn_unzip_fclose(z); });
	unsigned int size = z->io.size();
	auto buff = (uint8_t*)malloc(size);
	if(gn_unzip_fread(z, buff, size) != (int)size)
	{
		logErr("read error in gn_unzip_file_malloc");
		free(buff);
		return nullptr;
	}
	*outlen = size;
	return buff;
}
