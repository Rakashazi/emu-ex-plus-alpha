/*  This file is part of EmuFramework.

	EmuFramework is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	EmuFramework is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/io/FileIO.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/util/string.h>
#include <imagine/util/bit.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/logger/logger.h>
#include <mednafen/types.h>
#include <mednafen/git.h>
#include <mednafen/file.h>
#include <mednafen/memory.h>
#include <mednafen/MemoryStream.h>
#include <fcntl.h>

namespace EmuEx
{
IG::ApplicationContext gAppContext();
}

namespace Mednafen
{

constexpr IG::SystemLogger log{"MDFNFILE"};

static bool hasKnownExtension(std::string_view name, const std::vector<FileExtensionSpecStruct>& extSpec)
{
	for(const auto &e : extSpec)
	{
		if(name.ends_with(e.extension))
			return true;
	}
	return false;
}

MDFNFILE::MDFNFILE(VirtualFS* vfs, const std::string& path, const std::vector<FileExtensionSpecStruct>& known_ext,
	[[maybe_unused]] const char* purpose, [[maybe_unused]] int* monocomp_double_ext)
{
	if(IG::FS::hasArchiveExtension(path))
	{
		try
		{
			for(auto &entry : IG::FS::ArchiveIterator{EmuEx::gAppContext().openFileUri(path)})
			{
				if(entry.type() == IG::FS::file_type::directory)
				{
					continue;
				}
				if(hasKnownExtension(entry.name(), known_ext))
				{
					log.info("archive file entry:{}", entry.name());
					str = std::make_unique<MemoryStream>(entry.size(), true);
					if(entry.read(str->map(), str->map_size()) != (int)str->map_size())
					{
						throw MDFN_Error(0, "Error reading archive");
					}
					return; // success
				}
			}
			throw MDFN_Error(0, "No recognized file extensions in archive");
		}
		catch(...)
		{
			throw MDFN_Error(0, "Error opening archive");
		}
	}
	else
	{
		str.reset(vfs->open(path, VirtualFS::MODE_READ));
	}
}

MDFNFILE::MDFNFILE(VirtualFS*, std::unique_ptr<Stream> str):
	str{std::move(str)} {}

extern int openFdHelper(const char *file, int oflag, mode_t)
{
	auto openFlags = (oflag & O_CREAT) ? IG::OpenFlags::newFile() : IG::OpenFlags{};
	return EmuEx::gAppContext().openFileUriFd(file, openFlags | IG::OpenFlags{.test = true}).release();
}

extern FILE *fopenHelper(const char* filename, const char* mode)
{
	return IG::FileUtils::fopenUri(EmuEx::gAppContext(), filename, mode);
}

}
