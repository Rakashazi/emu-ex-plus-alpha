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
#include <imagine/util/string/CStringView.hh>
#include <imagine/util/string/StaticString.hh>
#include <algorithm>
#include <ctime>
#include <array>
#include <memory>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>

namespace FS
{

static constexpr uint32_t FILE_STRING_SIZE = std::max(512, NAME_MAX + 1);
using FileStringImpl = IG::StaticString<FILE_STRING_SIZE>;
class FileString : public FileStringImpl
{
public:
	using FileStringImpl::FileStringImpl;
	using FileStringImpl::operator=;
};

static constexpr uint32_t PATH_STRING_SIZE = std::max(1024, PATH_MAX);
using PathStringImpl = IG::StaticString<PATH_STRING_SIZE>;
class PathString : public PathStringImpl
{
public:
	using PathStringImpl::PathStringImpl;
	using PathStringImpl::operator=;
};

using FileTimeTypeImpl = std::time_t;

enum class file_type;

static constexpr int accExistsImpl = F_OK;
static constexpr int accReadBitImpl = R_OK;
static constexpr int accWriteBitImpl = W_OK;
static constexpr int accExecBitImpl = X_OK;

class DirectoryEntryImpl
{
public:
	DirectoryEntryImpl(IG::CStringView path);
	bool readNextDir();
	bool hasEntry() const;
	std::string_view name() const;
	file_type type() const;
	file_type symlink_type() const;
	PathString path() const;
	void close();

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
	struct dirent *dirent_{};
	mutable file_type type_{};
	mutable file_type linkType_{};
	PathString basePath{};

	static void closeDirectoryStream(DIR *);
};

};
