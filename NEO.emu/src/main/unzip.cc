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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/io/IO.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/bit.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/string.h>
#include <cstdlib>

extern "C"
{
	#include <gngeo/unzip.h>
	#include <gngeo/state.h>
}

using namespace IG;

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
		auto crc = entry.crc32();
		//logMsg("archive file entry:%s crc32:0x%X", name, crc);
		int loadByName = fileCRC == (uint32_t)-1 || !gn_strictROMChecking();
		if((loadByName && entry.name() == filename) || crc == fileCRC)
		{
			//logMsg("opened archive entry file:%s crc32:0x%X", name, crc);
			return new ZFILE{std::move(entry), archPtr};
		}
	}
	logMsg("file:%s crc32:0x%X not found in archive", filename, fileCRC);
	return nullptr;
}

void gn_unzip_fclose(ZFILE *z)
{
	//logMsg("done with archive entry");
	*z->arch = std::move(z->io);
	delete z;
}

int gn_unzip_fread(ZFILE *z, uint8_t *data, unsigned int size)
{
	//logMsg("reading %u bytes to %p", size, data);
	return z->io.read(data, size);
}

PKZIP *gn_open_zip(void *contextPtr, const char *path)
{
	auto &ctx = *((IG::ApplicationContext*)contextPtr);
	try
	{
		auto arch = std::make_unique<FS::ArchiveIterator>(ctx.openFileUri(path));
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

struct PKZIP *open_rom_zip(void *contextPtr, char *romPath, char *name)
{
	auto baseUri = FS::uriString(romPath, name);
	// Try to open each possible archive type
	if(auto gz = gn_open_zip(contextPtr, FS::PathString{baseUri + ".zip"}.data());
		gz)
	{
		return gz;
	}
	if(auto gz = gn_open_zip(contextPtr, FS::PathString{baseUri + ".7z"}.data());
		gz)
	{
		return gz;
	}
	if(auto gz = gn_open_zip(contextPtr, FS::PathString{baseUri + ".rar"}.data());
		gz)
	{
		return gz;
	}
	return nullptr;
}

gzFile gzopenHelper(void *contextPtr, const char *filename, const char *mode)
{
	auto &ctx = *((IG::ApplicationContext*)contextPtr);
	auto openFlags = std::string_view{mode}.contains('w') ? OpenFlags::newFile() : OpenFlags{};
	return gzdopen(ctx.openFileUriFd(filename, openFlags | OpenFlags{.test = true}).release(), mode);
}
