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
#include <imagine/fs/PosixFS.hh>
#include <imagine/util/string/CStringView.hh>
#include <imagine/util/string/uri.hh>
#include <concepts>
#include <cstddef>
#include <memory>
#include <iterator>
#include <string_view>

// Tries to mirror API of C++ filesystem TS library in most cases

namespace IG::FS
{

class directory_iterator
{
public:
	using iterator_category = std::input_iterator_tag;
	using value_type = directory_entry;
	using difference_type = ptrdiff_t;
	using pointer = value_type*;
	using reference = value_type&;

	constexpr directory_iterator() = default;
	directory_iterator(CStringView path);
	directory_iterator(const directory_iterator&) = default;
	directory_iterator(directory_iterator&&) = default;
	directory_entry& operator*();
	directory_entry* operator->();
	void operator++();
	bool operator==(directory_iterator const &rhs) const;

protected:
	std::shared_ptr<DirectoryStream> impl;
};

inline const directory_iterator &begin(const directory_iterator& iter)
{
	return iter;
}

inline directory_iterator end(const directory_iterator&)
{
	return {};
}

PathString current_path();
void current_path(CStringView path);
bool exists(CStringView path);
std::uintmax_t file_size(CStringView path);
file_status status(CStringView path);
file_status symlink_status(CStringView path);
void chown(CStringView path, uid_t owner, gid_t group);
bool access(CStringView path, acc type);
bool remove(CStringView path);
bool create_directory(CStringView path);
bool rename(CStringView oldPath, CStringView newPath);

PathString makeAppPathFromLaunchCommand(CStringView launchPath);
FileString basename(CStringView path);
PathString dirname(CStringView path);
FileString displayName(CStringView path);

// URI path functions
static constexpr std::string_view uriPathSegmentTreeName{"/tree/"};
static constexpr std::string_view uriPathSegmentDocumentName{"/document/"};
PathString dirnameUri(CStringView pathOrUri);
std::pair<std::string_view, size_t> uriPathSegment(std::string_view uri, std::string_view segmentName);

template <class T>
concept ConvertibleToPathString = std::convertible_to<T, PathStringImpl> || std::convertible_to<T, std::string_view>;

static constexpr PathString pathString(ConvertibleToPathString auto &&base, auto &&...components)
{
	PathString path{IG_forward(base)};
	([&]()
	{
		path += '/';
		path += IG_forward(components);
	}(), ...);
	return path;
}

static constexpr PathString uriString(ConvertibleToPathString auto &&base, auto &&...components)
{
	if(!isUri(base))
		return pathString(IG_forward(base), IG_forward(components)...);
	// assumes base is already encoded and encodes the components
	PathString uri{IG_forward(base)};
	([&]()
	{
		uri += "%2F";
		uri += encodeUri<PathString>(IG_forward(components));
	}(), ...);
	return uri;
}

static PathString createDirectorySegments(ConvertibleToPathString auto &&base, auto &&...components)
{
	PathString path{IG_forward(base)};
	([&]()
	{
		path += '/';
		path += IG_forward(components);
		create_directory(path);
	}(), ...);
	return path;
}

}

namespace IG
{
	// convenience aliases for path strings
	using FS::PathString;
	using FS::FileString;
	using FS::pathString;
	using FS::uriString;
}
