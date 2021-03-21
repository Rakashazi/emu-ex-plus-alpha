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
#include <algorithm>
#include <ctime>
#include <array>
#include <memory>
#include <system_error>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>

namespace FS
{

static constexpr uint32_t FILE_STRING_SIZE = std::max(512, NAME_MAX + 1);
using FileStringImpl = std::array<char, FILE_STRING_SIZE>;

static constexpr uint32_t PATH_STRING_SIZE = std::max(1024, PATH_MAX);
using PathStringImpl = std::array<char, PATH_STRING_SIZE>;

using FileTimeTypeImpl = std::time_t;

enum class file_type;

static constexpr int accExistsImpl = F_OK;
static constexpr int accReadBitImpl = R_OK;
static constexpr int accWriteBitImpl = W_OK;
static constexpr int accExecBitImpl = X_OK;

class DirectoryEntryImpl
{
public:
	DirectoryEntryImpl(const char *path, std::error_code &ec);
	DirectoryEntryImpl(const char *path);
	bool readNextDir();
	bool hasEntry() const;
	const char *name() const;
	file_type type() const;
	file_type symlink_type() const;
	PathStringImpl path() const;
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
	PathStringImpl basePath{};

	DirectoryEntryImpl(const char *path, std::error_code *ec);
	static void closeDirectoryStream(DIR *);
};

};
