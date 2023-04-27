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
#include <cstdint>

namespace IG
{

enum class IOAdvice
{
	Normal, Sequential, Random, WillNeed
};

enum class IOAccessHint
{
	Normal, Sequential, Random, All
};

enum class IOBufferMode
{
	Direct, // may point directly to mapped memory, not valid after IO is destroyed
	Release // may take IO's underlying memory and is always valid, invalidates IO object
};

enum class IOSeekMode
{
	Set = SEEK_SET,
	Cur = SEEK_CUR,
	End = SEEK_END,
};

enum class OpenFlagsMask: uint8_t
{
	// allow reading file
	Read = bit(0),
	// allow modifying file
	Write = bit(1),
	// create a new file if it doesn't already exist
	Create = bit(2),
	// if using WRITE, truncate any existing file to 0 bytes
	Truncate = bit(3),
	// return from constructor without throwing exception if opening fails,
	// used to avoid redundant FS::exists() tests when searching for a file to open
	Test = bit(4),

	// common flag combinations
	New = Write | Create | Truncate,
	CreateRW = Read | Write | Create
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
