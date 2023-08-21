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
#include <imagine/logger/logger.h>
#include <imagine/util/string.h>
#include <imagine/util/ScopeGuard.hh>
#include "ziphelper.h"
#include "MainSystem.hh"
#include <cstdlib>

namespace EmuEx
{
IG::ApplicationContext gAppContext();
}

using namespace EmuEx;

static struct archive *writeArch{};
static FS::ArchiveIterator cachedZipIt{};
static FS::PathString cachedZipName{};

void zipCacheReadOnlyZip(const char* zipName)
{
	if(zipName && strlen(zipName))
	{
		logMsg("setting cached read zip archive:%s", zipName);
		cachedZipIt = {EmuEx::gAppContext().openFileUri(zipName)};
		cachedZipName = zipName;
	}
	else
	{
		cachedZipIt = {};
		cachedZipName = {};
		logMsg("unset cached read zip archive");
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
			auto io = entry.releaseIO();
			int fileSize = io.size();
			void *buff = malloc(fileSize);
			io.read(buff, fileSize);
			*size = fileSize;
			entry.reset(std::move(io));
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
	int fd = EmuEx::gAppContext().openFileUriFd(fileName, IG::OpenFlags::testNewFile()).release();
	if(archive_write_open_fd(writeArch, fd) != ARCHIVE_OK)
	{
		archive_write_free(writeArch);
		writeArch = {};
		if(fd != -1)
			::close(fd);
		return false;
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
