#pragma once

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

#include <mednafen/VirtualFS.h>
#include <imagine/io/ArchiveIO.hh>

namespace Mednafen
{

class ArchiveVFS final : public VirtualFS
{
public:
	ArchiveVFS(IG::ArchiveIO);
	Stream* open(const std::string& path, const uint32 mode, const int do_lock = false, const bool throw_on_noent = true, const CanaryType canary = CanaryType::open) final;
	FILE* openAsStdio(const std::string& path, const uint32 mode) final;
	int mkdir(const std::string& path, const bool throw_on_exist = false, const bool throw_on_noent = true) final;
	bool unlink(const std::string& path, const bool throw_on_noent = false, const CanaryType canary = CanaryType::unlink) final;
	void rename(const std::string& oldpath, const std::string& newpath, const CanaryType canary = CanaryType::rename) final;
	bool is_absolute_path(const std::string& path) final;
	void check_firop_safe(const std::string& path) final;
	bool finfo(const std::string& path, FileInfo*, const bool throw_on_noent = true) final;
	void readdirentries(const std::string& path, std::function<bool(const std::string&)> callb) final;
	std::string get_human_path(const std::string& path) final;

private:
	IG::ArchiveIO arch;

	void seekFile(const std::string& path);
};

}
