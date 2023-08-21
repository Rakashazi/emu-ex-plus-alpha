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
#include <imagine/util/utility.h>
#include <imagine/util/string/StaticString.hh>
#include <chrono>
#include <array>
#include <algorithm>
#include <unistd.h>
#include <limits.h>

namespace IG::FS
{

using file_time_type = std::chrono::system_clock::time_point;

static constexpr size_t FILE_STRING_SIZE = std::max(512, NAME_MAX + 1);
using FileStringImpl = StaticString<FILE_STRING_SIZE - 1>;
class FileString : public FileStringImpl
{
public:
	using FileStringImpl::FileStringImpl;
	using FileStringImpl::operator=;
};

static constexpr size_t PATH_STRING_SIZE = std::max(1024, PATH_MAX);
using PathStringImpl = StaticString<PATH_STRING_SIZE - 1>;
class PathString : public PathStringImpl
{
public:
	using PathStringImpl::PathStringImpl;
	using PathStringImpl::operator=;
};

using FileStringArray = std::array<char, FILE_STRING_SIZE>;
using PathStringArray = std::array<char, PATH_STRING_SIZE>;

struct RootPathInfo
{
	size_t length{};
	FileString name;

	constexpr RootPathInfo() = default;
	constexpr RootPathInfo(auto &&name, size_t length):
		length{length}, name{IG_forward(name)} {}
};

struct RootedPath
{
	PathString path;
	RootPathInfo info;

	constexpr bool pathIsRoot() const
	{
		return path.size() == info.length;
	}
};

struct PathLocation
{
	RootedPath root;
	FileString description;

	constexpr PathLocation() = default;
	constexpr PathLocation(auto &&path, auto &&description, auto &&rootName):
		root{IG_forward(path), {IG_forward(rootName), path.size()}}, description{IG_forward(description)} {}
	constexpr PathLocation(auto &&path, auto &&description):
		PathLocation(IG_forward(path), IG_forward(description), IG_forward(description)) {}

	explicit constexpr operator bool() const
	{
		return root.path.size();
	}
};

enum class file_type : int8_t
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
	e = F_OK,
	r = R_OK,
	w = W_OK,
	x = X_OK,
	rw = r | w,
	rx = r | x,
	wx = w | x,
	rwx = r | w | x
};

class file_status
{
public:
	constexpr file_status() = default;
	constexpr file_status(file_type type, std::uintmax_t size, file_time_type lastWriteTime):
		size_{size}, lastWriteTime_{lastWriteTime}, type_{type} {}
	constexpr file_type type() const { return type_; }
	constexpr std::uintmax_t size() const { return size_; }
	constexpr file_time_type lastWriteTime() const { return lastWriteTime_; }

protected:
	std::uintmax_t size_{};
	file_time_type lastWriteTime_{};
	file_type type_ = file_type::none;
};

struct DirOpenFlags
{
	 uint8_t
	// return from constructor without throwing exception if opening fails
	test:1{};

	constexpr bool operator ==(DirOpenFlags const &) const = default;
};

class directory_entry;
class AssetDirectoryIterator;

}
