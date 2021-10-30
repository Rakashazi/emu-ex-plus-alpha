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
#include <cstddef>
#include <compare>
#include <memory>

// Tries to mirror API of C++ filesystem TS library in most cases

namespace FS
{

class directory_iterator : public std::iterator<std::input_iterator_tag, directory_entry>
{
public:
	constexpr directory_iterator() {}
	directory_iterator(IG::CStringView path);
	directory_iterator(const directory_iterator&) = default;
	directory_iterator(directory_iterator&&) = default;
	~directory_iterator();
	directory_entry& operator*();
	directory_entry* operator->();
	void operator++();
	bool operator==(directory_iterator const &rhs) const;

protected:
	std::shared_ptr<DirectoryEntryImpl> impl{};
};

static const directory_iterator &begin(const directory_iterator &iter)
{
	return iter;
}

static directory_iterator end(const directory_iterator &)
{
	return {};
}

PathString current_path();
void current_path(IG::CStringView path);
bool exists(IG::CStringView path);
std::uintmax_t file_size(IG::CStringView path);
file_status status(IG::CStringView path);
file_status symlink_status(IG::CStringView path);
void chown(IG::CStringView path, uid_t owner, gid_t group);
bool access(IG::CStringView path, acc type);
bool remove(IG::CStringView path);
bool create_directory(IG::CStringView path);
void rename(IG::CStringView oldPath, IG::CStringView newPath);

FileString makeFileString(IG::CStringView str);
FileString makeFileStringWithoutDotExtension(IG::CStringView str);

PathString makePathString(IG::CStringView str);
PathString makePathString(IG::CStringView dir, IG::CStringView base);
PathString makeAppPathFromLaunchCommand(IG::CStringView launchPath);

FileString basename(IG::CStringView path);
PathString dirname(IG::CStringView path);

using FileStringCompareFunc = bool (*)(const FS::FileString &s1, const FS::FileString &s2);

bool fileStringNoCaseLexCompare(FS::FileString s1, FS::FileString s2);

int directoryItems(IG::CStringView path);

};
