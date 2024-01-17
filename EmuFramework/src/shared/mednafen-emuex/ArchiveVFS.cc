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

#include "ArchiveVFS.hh"
#include <mednafen/MemoryStream.h>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/fs/FS.hh>
#include <imagine/io/MapIO.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>

namespace Mednafen
{

constexpr IG::SystemLogger log{"ArchiveVFS"};

ArchiveVFS::ArchiveVFS(IG::ArchiveIO arch):
	VirtualFS('/', "/"),
	arch{std::move(arch)} {}

Stream* ArchiveVFS::open(const std::string &path, const uint32 mode, const int do_lock, const bool throw_on_noent, const CanaryType canary)
{
	assert(mode == MODE_READ);
	assert(do_lock == 0);
	seekFile(path);
	auto stream = std::make_unique<MemoryStream>(arch.size(), true);
	if(arch.read(stream->map(), arch.size()) != ssize_t(arch.size()))
	{
		throw MDFN_Error(0, "Error reading archive file:\n%s", arch.name().data());
	}
	return stream.release();
}

FILE* ArchiveVFS::openAsStdio(const std::string& path, const uint32 mode)
{
	assert(mode == MODE_READ);
	seekFile(path);
	return IG::MapIO{arch}.toFileStream("rb");
}

void ArchiveVFS::seekFile(const std::string& path)
{
	arch.rewind();
	auto filename = IG::FS::basename(path);
	log.info("looking for file:{}", filename);
	if(!IG::FS::seekFileInArchive(arch, [&](auto &entry)
		{
			auto name = IG::FS::basename(entry.name());
			//log.debug("checking:{}", name);
			return name == filename;
		}))
	{
		throw MDFN_Error(ENOENT, "Not found");
	}
}

int ArchiveVFS::mkdir(const std::string& path, const bool throw_on_exist, const bool throw_on_noent) { return -1; }
bool ArchiveVFS::unlink(const std::string &path, const bool throw_on_noent, const CanaryType canary) { return false; }
void ArchiveVFS::rename(const std::string &oldpath, const std::string &newpath, const CanaryType canary) {}

bool ArchiveVFS::is_absolute_path(const std::string &path)
{
	if(!path.size())
		return false;
	if(is_path_separator(path[0]))
		return true;
	return false;
}

void ArchiveVFS::check_firop_safe(const std::string &path) {}
bool ArchiveVFS::finfo(const std::string& path, FileInfo*, const bool throw_on_noent) { return false; }
void ArchiveVFS::readdirentries(const std::string& path, std::function<bool(const std::string&)> callb) {}
std::string ArchiveVFS::get_human_path(const std::string& path) { return path; }

}
