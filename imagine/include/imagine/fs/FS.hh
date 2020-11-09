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
#include <cstddef>
#include <system_error>
#include <compare>

// Tries to mirror API of C++ filesystem TS library in most cases

namespace FS
{

class directory_iterator : public DirectoryIteratorImpl,
	public std::iterator<std::input_iterator_tag, directory_entry>
{
public:
	constexpr directory_iterator() {}
	directory_iterator(PathString path): directory_iterator{path.data()} {}
	directory_iterator(const char *path);
	directory_iterator(PathString path, std::error_code &result): directory_iterator{path.data(), result} {}
	directory_iterator(const char *path, std::error_code &result);
	directory_iterator(const directory_iterator&) = default;
	directory_iterator(directory_iterator&&) = default;
	~directory_iterator();
	directory_entry& operator*();
	directory_entry* operator->();
	void operator++();
	bool operator==(directory_iterator const &rhs) const;
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
PathString current_path(std::error_code &result);

void current_path(const char *path);
void current_path(const char *path, std::error_code &result);

static void current_path(PathString path)
{
	current_path(path.data());
}

static void current_path(PathString path, std::error_code &result)
{
	current_path(path.data(), result);
}

bool exists(const char *path);
bool exists(const char *path, std::error_code &result);

static bool exists(PathString path)
{
	return exists(path.data());
}

static bool exists(PathString path, std::error_code &result)
{
	return exists(path.data(), result);
}

std::uintmax_t file_size(const char *path);
std::uintmax_t file_size(const char *path, std::error_code &result);

static std::uintmax_t file_size(PathString path)
{
	return file_size(path.data());
}

static std::uintmax_t file_size(PathString path, std::error_code &result)
{
	return file_size(path.data(), result);
}

file_status status(const char *path);
file_status status(const char *path, std::error_code &result);

static file_status status(PathString path)
{
	return status(path.data());
}

static file_status status(PathString path, std::error_code &result)
{
	return status(path.data(), result);
}

file_status symlink_status(const char *path);
file_status symlink_status(const char *path, std::error_code &result);

static file_status symlink_status(PathString path)
{
	return symlink_status(path.data());
}

static file_status symlink_status(PathString path, std::error_code &result)
{
	return symlink_status(path.data(), result);
}

void chown(const char *path, uid_t owner, gid_t group);
void chown(const char *path, uid_t owner, gid_t group, std::error_code &result);

static void chown(PathString path, uid_t owner, gid_t group)
{
	chown(path.data(), owner, group);
}

static void chown(PathString path, uid_t owner, gid_t group, std::error_code &result)
{
	chown(path.data(), owner, group, result);
}

bool access(const char *path, acc type);
bool access(const char *path, acc type, std::error_code &result);

static bool access(PathString path, acc type)
{
	return access(path.data(), type);
}

static bool access(PathString path, acc type, std::error_code &result)
{
	return access(path.data(), type, result);
}

bool remove(const char *path);
bool remove(const char *path, std::error_code &result);

static bool remove(PathString path)
{
	return remove(path.data());
}

static bool remove(PathString path, std::error_code &result)
{
	return remove(path.data(), result);
}

bool create_directory(const char *path);
bool create_directory(const char *path, std::error_code &result);

static bool create_directory(PathString path)
{
	return create_directory(path.data());
}

static bool create_directory(PathString path, std::error_code &result)
{
	return create_directory(path.data(), result);
}

void rename(const char *oldPath, const char *newPath);
void rename(const char *oldPath, const char *newPath, std::error_code &result);

static void rename(PathString oldPath, PathString newPath)
{
	rename(oldPath.data(), newPath.data());
}

static void rename(PathString oldPath, PathString newPath, std::error_code &result)
{
	rename(oldPath.data(), newPath.data(), result);
}

[[gnu::format(printf, 1, 2)]]
FileString makeFileStringPrintf(const char *format, ...);
[[gnu::format(printf, 1, 2)]]
PathString makePathStringPrintf(const char *format, ...);
FileString makeFileString(const char *str);
FileString makeFileStringWithoutDotExtension(const char *str);

template <size_t S>
FileString makeFileStringWithoutDotExtension(std::array<char, S> &str)
{
	return makeFileStringWithoutDotExtension(str.data());
}

PathString makePathString(const char *str);
PathString makePathString(const char *dir, const char *base);
PathString makeAppPathFromLaunchCommand(const char *launchPath);

FileString basename(const char *path);

template <size_t S>
static FileString basename(std::array<char, S> &path)
{
	return basename(path.data());
}

PathString dirname(const char *path);

template <size_t S>
static PathString dirname(std::array<char, S> &path)
{
	return dirname(path.data());
}

using FileStringCompareFunc = bool (*)(const FS::FileString &s1, const FS::FileString &s2);

bool fileStringNoCaseLexCompare(FS::FileString s1, FS::FileString s2);

int directoryItems(const char *path);
static int directoryItems(PathString path) { return directoryItems(path.data()); }

};
