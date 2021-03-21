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
#include <ctime>
#include <unistd.h>
#include <limits.h>

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
	file_time_type lastWriteTime_{};

public:
	constexpr file_status() {}
	constexpr file_status(file_type type_, std::uintmax_t size_, file_time_type lastWriteTime_):
		type_{type_}, size_{size_}, lastWriteTime_{lastWriteTime_} {}

	file_type type() const
	{
		return type_;
	}

	std::uintmax_t size() const
	{
		return size_;
	}

	file_time_type lastWriteTime() const
	{
		return lastWriteTime_;
	}

	std::tm lastWriteTimeLocal() const;
};

using directory_entry = DirectoryEntryImpl;

struct RootPathInfo
{
	size_t length = 0;
	FS::FileString name{};

	constexpr RootPathInfo() {}
	constexpr RootPathInfo(FS::FileString name, size_t length):
		length{length}, name{name}
	{}
};

struct PathLocation
{
	RootPathInfo root{};
	PathString path{};
	FileString description{};

	constexpr PathLocation() {}
	constexpr PathLocation(PathString path, FileString description, RootPathInfo root):
		root{root}, path{path}, description{description}
	{}

	explicit operator bool()
	{
		return path[0];
	}
};

}
