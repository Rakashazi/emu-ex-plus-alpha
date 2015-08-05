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

#include <ctime>
#include <unistd.h>
#include <limits.h>
#include <imagine/engine-globals.h>
#include <imagine/util/operators.hh>

#include <imagine/fs/PosixFS.hh>

// Tries to mirror API of C++ filesystem TS library in most cases

namespace FS
{

using FileString = FileStringImpl;

using PathString = PathStringImpl;

using file_time_type = FileTimeTypeImpl;

enum class file_type
{
	none = 0,
	not_found = -1,
	regular = 1,
	directory = 2,
	symlink = 3,
	block = 4,
	character = 5,
	fifo = 6,
	socket = 7,
	unknown = 8
};

enum class acc
{
	e = accExistsImpl,
	r = accReadBitImpl,
	w = accWriteBitImpl,
	x = accExecBitImpl,
	rw = r | w,
	rx = r | x,
	wx = w | x,
	rwx = r | w | x
};

class file_status
{
protected:
	file_type type_ = file_type::none;
	std::uintmax_t size_ = 0;
	file_time_type lastWriteTime{};

public:
	constexpr file_status() {}
	constexpr file_status(file_type type_, std::uintmax_t size_, file_time_type lastWriteTime):
		type_{type_}, size_{size_}, lastWriteTime{lastWriteTime} {}

	file_type type() const
	{
		return type_;
	}

	std::uintmax_t size() const
	{
		return size_;
	}

	file_time_type last_write_time() const
	{
		return lastWriteTime;
	}

	std::tm last_write_time_local() const;
};

using directory_entry = DirectoryEntryImpl;

class directory_iterator : public DirectoryIteratorImpl,
	public std::iterator<std::input_iterator_tag, directory_entry>,
	public NotEquals<directory_iterator>
{
public:
	constexpr directory_iterator() {}
	directory_iterator(PathString path): directory_iterator{path.data()} {}
	directory_iterator(const char *path);
	directory_iterator(PathString path, CallResult &result): directory_iterator{path.data(), result} {}
	directory_iterator(const char *path, CallResult &result);
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
PathString current_path(CallResult &result);

void current_path(const char *path);
void current_path(const char *path, CallResult &result);

static void current_path(PathString path)
{
	current_path(path.data());
}

static void current_path(PathString path, CallResult &result)
{
	current_path(path.data(), result);
}

bool exists(const char *path);
bool exists(const char *path, CallResult &result);

static bool exists(PathString path)
{
	return exists(path.data());
}

static bool exists(PathString path, CallResult &result)
{
	return exists(path.data(), result);
}

std::uintmax_t file_size(const char *path);
std::uintmax_t file_size(const char *path, CallResult &result);

static std::uintmax_t file_size(PathString path)
{
	return file_size(path.data());
}

static std::uintmax_t file_size(PathString path, CallResult &result)
{
	return file_size(path.data(), result);
}

file_status status(const char *path);
file_status status(const char *path, CallResult &result);

static file_status status(PathString path)
{
	return status(path.data());
}

static file_status status(PathString path, CallResult &result)
{
	return status(path.data(), result);
}

file_status symlink_status(const char *path);
file_status symlink_status(const char *path, CallResult &result);

static file_status symlink_status(PathString path)
{
	return symlink_status(path.data());
}

static file_status symlink_status(PathString path, CallResult &result)
{
	return symlink_status(path.data(), result);
}

void chown(const char *path, uid_t owner, gid_t group);
void chown(const char *path, uid_t owner, gid_t group, CallResult &result);

static void chown(PathString path, uid_t owner, gid_t group)
{
	chown(path.data(), owner, group);
}

static void chown(PathString path, uid_t owner, gid_t group, CallResult &result)
{
	chown(path.data(), owner, group, result);
}

bool access(const char *path, acc type);
bool access(const char *path, acc type, CallResult &result);

static bool access(PathString path, acc type)
{
	return access(path.data(), type);
}

static bool access(PathString path, acc type, CallResult &result)
{
	return access(path.data(), type, result);
}

bool remove(const char *path);
bool remove(const char *path, CallResult &result);

static bool remove(PathString path)
{
	return remove(path.data());
}

static bool remove(PathString path, CallResult &result)
{
	return remove(path.data(), result);
}

bool create_directory(const char *path);
bool create_directory(const char *path, CallResult &result);

static bool create_directory(PathString path)
{
	return create_directory(path.data());
}

static bool create_directory(PathString path, CallResult &result)
{
	return create_directory(path.data(), result);
}

void rename(const char *oldPath, const char *newPath);
void rename(const char *oldPath, const char *newPath, CallResult &result);

static void rename(PathString oldPath, PathString newPath)
{
	rename(oldPath.data(), newPath.data());
}

static void rename(PathString oldPath, PathString newPath, CallResult &result)
{
	rename(oldPath.data(), newPath.data(), result);
}

template <typename... ARGS>
PathString makePathStringPrintf(ARGS&&... args)
{
	PathString path;
	int ret = string_printf(path, std::forward<ARGS>(args)...);
	assert(ret >= 0);
	return path;
}

FileString makeFileString(const char *str);
PathString makePathString(const char *str);

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

FileStringCompareFunc fileStringNoCaseLexCompare();

};
