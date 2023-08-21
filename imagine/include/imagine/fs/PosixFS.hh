#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/config/defs.hh>
#include <imagine/fs/FSDefs.hh>
#include <imagine/util/string/CStringView.hh>
#include <imagine/util/utility.h>
#include <ctime>
#include <memory>
#include <string_view>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>

namespace IG::FS
{

class directory_entry
{
public:
	constexpr directory_entry() = default;
	constexpr directory_entry(auto &&path, struct dirent *dirent):
		dirent_{dirent}, path_{IG_forward(path)} {}
	constexpr directory_entry(auto &&path, auto &&name, file_type type, file_type linkType = file_type::unknown):
		path_{IG_forward(path)}, name_{IG_forward(name)}, type_{type}, linkType_{linkType} {}
	std::string_view name() const;
	file_type type() const;
	file_type symlink_type() const;
	constexpr const PathString &path() const { return path_; };
	constexpr explicit operator bool() const { return path_.size(); }

protected:
	struct dirent *dirent_{};
	PathString path_{};
	FileString name_{};
	mutable file_type type_{};
	mutable file_type linkType_{};
};

class DirectoryStream
{
public:
	DirectoryStream(CStringView path, DirOpenFlags flags = {});
	bool readNextDir();
	bool hasEntry() const;
	directory_entry &entry() { return entry_; }

protected:
	struct DirectoryStreamDeleter
	{
		void operator()(DIR *ptr) const
		{
			closeDirectoryStream(ptr);
		}
	};
	using UniqueDirectoryStream = std::unique_ptr<DIR, DirectoryStreamDeleter>;

	UniqueDirectoryStream dir{};
	directory_entry entry_;
	PathString basePath{};

	static void closeDirectoryStream(DIR *);
};

};
