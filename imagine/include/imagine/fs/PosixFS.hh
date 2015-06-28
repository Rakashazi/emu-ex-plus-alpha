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
#include <memory>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <imagine/engine-globals.h>
#include <imagine/util/bits.h>

namespace FS
{

static constexpr uint FILE_STRING_SIZE = IG::maxConst(512, NAME_MAX + 1);
using FileStringImpl = std::array<char, FILE_STRING_SIZE>;

static constexpr uint PATH_STRING_SIZE = IG::maxConst(1024, PATH_MAX);
using PathStringImpl = std::array<char, PATH_STRING_SIZE>;

using FileTimeTypeImpl = std::time_t;

enum class file_type;

static constexpr int accExistsImpl = F_OK;
static constexpr int accReadBitImpl = R_OK;
static constexpr int accWriteBitImpl = W_OK;
static constexpr int accExecBitImpl = X_OK;

union FullDirent
{
	struct dirent d;
	char b[offsetof (struct dirent, d_name) + FILE_STRING_SIZE]{};
};

class DirectoryEntryImpl
{
protected:
	FullDirent dirent_{};

public:
	file_type type_{};
	file_type linkType_{};
	PathStringImpl basePath{};

	FullDirent &dirent() { return dirent_; };
	const char *name() const;
	file_type type();
	file_type symlink_type();
	PathStringImpl path() const;
};

class DirectoryIteratorImpl
{
protected:
	std::shared_ptr<DIR> dir{};
	DirectoryEntryImpl entry{};

	void init(const char *path, CallResult &result);
};

};
