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

#include <imagine/util/bitset.hh>
#include <imagine/util/enum.hh>
#include <unistd.h> // for SEEK_*

namespace IG
{

enum class IOAdvice
{
	NORMAL, SEQUENTIAL, RANDOM, WILLNEED
};

enum class IOAccessHint
{
	NORMAL, SEQUENTIAL, RANDOM, ALL
};

enum class IOBufferMode
{
	DIRECT, // may point directly to mapped memory, not valid after IO is destroyed
	RELEASE // may take IO's underlying memory and is always valid, invalidates IO object
};

enum class IOSeekMode
{
	SET = SEEK_SET,
	CUR = SEEK_CUR,
	END = SEEK_END,
};

enum class OpenFlagsMask: uint8_t
{
	// allow reading file
	READ = bit(0),
	// allow modifying file
	WRITE = bit(1),
	// create a new file if it doesn't already exist
	CREATE = bit(2),
	// if using WRITE, truncate any existing file to 0 bytes
	TRUNCATE = bit(3),
	// return from constructor without throwing exception if opening fails,
	// used to avoid redundant FS::exists() tests when searching for a file to open
	TEST = bit(4),

	// common flag combinations
	NEW = WRITE | CREATE | TRUNCATE,
	CREATE_RW = READ | WRITE | CREATE
};

IG_DEFINE_ENUM_BIT_FLAG_FUNCTIONS(OpenFlagsMask);

template <class T>
concept Readable =
	requires(T &i, void *buff, size_t bytes)
	{
		i.read(buff, bytes);
	};

template <class T>
concept Writable =
	requires(T &i, void *buff, size_t bytes)
	{
		i.write(buff, bytes);
	};

template <class T>
concept Seekable =
	requires(T &i, off_t offset, IOSeekMode mode)
	{
		i.seek(offset, mode);
	};

class AssetIO;
class FileIO;

}
