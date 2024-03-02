/*  This file is part of MSX.emu.

	MSX.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MSX.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MSX.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "main"
#include <archive.h>
#include <archive_entry.h>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/io/IO.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/string.h>
#include <imagine/util/ScopeGuard.hh>
#include "ziphelper.h"
#include "MainSystem.hh"
#include <cstdlib>

namespace EmuEx
{
IG::ApplicationContext gAppContext();

constexpr SystemLogger log{"MSX.emu"};
}

using namespace EmuEx;

static struct archive *writeArch{};
static FS::ArchiveIterator cachedZipIt{};
static FS::PathString cachedZipName{};
static uint8_t *buffData{};
static size_t buffSize{};

void setZipMemBuffer(std::span<uint8_t> buff)
{
	buffData = buff.data();
	buffSize = buff.size();
}

size_t zipMemBufferSize() { return buffSize; }

static void unsetCachedReadZip()
{
	cachedZipIt = {};
	cachedZipName = {};
	EmuEx::log.info("unset cached read zip archive");
}

void zipCacheReadOnlyZip(const char* zipName_)
{
	if(!zipName_ || !strlen(zipName_))
	{
		unsetCachedReadZip();
		return;
	}
	else
	{
		std::string_view zipName{zipName_};
		cachedZipName = zipName;
		if(zipName == ":::B")
		{
			EmuEx::log.info("using memory buffer as cached read zip archive");
			std::span buff{buffData, buffSize};
			cachedZipIt = IO{buff};
		}
		else
		{
			EmuEx::log.info("setting cached read zip archive:{}", zipName);
			cachedZipIt = {EmuEx::gAppContext().openFileUri(zipName)};
		}
	}
}

static void *loadFromArchiveIt(FS::ArchiveIterator &it, const char* zipName, const char* fileName, int* size)
{
	for(auto &entry : it)
	{
		if(entry.type() == FS::file_type::directory)
		{
			continue;
		}
		//logMsg("archive file entry:%s", entry.name());
		if(entry.name() == fileName)
		{
			int fileSize = entry.size();
			void *buff = malloc(fileSize);
			entry.read(buff, fileSize);
			*size = fileSize;
			return buff;
		}
	}
	logErr("file %s not in %sarchive:%s", fileName,
		cachedZipIt.hasEntry() && cachedZipName == zipName ? "cached " : "", zipName);
	return nullptr;
}

void* zipLoadFile(const char* zipName, const char* fileName, int* size)
{
	try
	{
		if(cachedZipIt.hasEntry() && cachedZipName == zipName)
		{
			cachedZipIt.rewind();
			return loadFromArchiveIt(cachedZipIt, zipName, fileName, size);
		}
		else
		{
			auto it = FS::ArchiveIterator{EmuEx::gAppContext().openFileUri(zipName)};
			return loadFromArchiveIt(it, zipName, fileName, size);
		}
	}
	catch(...)
	{
		logErr("error opening archive:%s", zipName);
		return nullptr;
	}
}

bool zipStartWrite(const char *fileName)
{
	assert(!writeArch);
	writeArch = archive_write_new();
	archive_write_set_format_zip(writeArch);
	if(std::string_view{fileName} == ":::B")
	{
		EmuEx::log.info("using memory buffer for zip write");
		if(archive_write_open_memory(writeArch, buffData, buffSize, &buffSize) != ARCHIVE_OK)
		{
			archive_write_free(writeArch);
			writeArch = {};
			return false;
		}
	}
	else
	{
		int fd = EmuEx::gAppContext().openFileUriFd(fileName, IG::OpenFlags::testNewFile()).release();
		if(archive_write_open_fd(writeArch, fd) != ARCHIVE_OK)
		{
			archive_write_free(writeArch);
			writeArch = {};
			if(fd != -1)
				::close(fd);
			return false;
		}
	}
	return true;
}

int zipSaveFile(const char* zipName, const char* fileName, int append, const void* buffer, int size)
{
	assert(writeArch);
	auto entry = archive_entry_new();
	auto freeEntry = IG::scopeGuard([&](){ archive_entry_free(entry); });
	archive_entry_set_pathname(entry, fileName);
	archive_entry_set_size(entry, size);
	archive_entry_set_filetype(entry, AE_IFREG);
	archive_entry_set_perm(entry, 0644);
	if(archive_write_header(writeArch, entry) < ARCHIVE_OK)
	{
		logErr("error writing archive header: %s", archive_error_string(writeArch));
		return 0;
	}
	if(archive_write_data(writeArch, buffer, size) < ARCHIVE_OK)
	{
		logErr("error writing archive data: %s", archive_error_string(writeArch));
		return 0;
	}
	return 1;
}

void zipEndWrite()
{
	assert(writeArch);
	archive_write_close(writeArch);
	archive_write_free(writeArch);
	writeArch = {};
}

FILE *fopenHelper(const char* filename, const char* mode)
{
	return FileUtils::fopenUri(EmuEx::gAppContext(), filename, mode);
}
