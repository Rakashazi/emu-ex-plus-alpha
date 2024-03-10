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

#include <unistd.h> // for SEEK_*
#include <sys/uio.h>
#include <cstdint>
#include <span>

namespace IG
{

enum class IOAdvice
{
	Normal, Sequential, Random, WillNeed
};

enum class IOAccessHint : uint8_t
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

struct OpenFlags
{
	using BitSetClassInt = uint16_t;

	BitSetClassInt
	// allow reading file
	read:1{},
	// allow modifying file
	write:1{},
	// create a new file if it doesn't already exist
	create:1{},
	// if using write flag, truncate any existing file to 0 bytes
	truncate:1{},
	// return from constructor without throwing exception if opening fails,
	// used to avoid redundant FS::exists() tests when searching for a file to open
	test:1{};

	IOAccessHint accessHint{};

	constexpr bool operator==(OpenFlags const&) const = default;

	// common flag combinations:
	// create a file for writing, clobbering an existing one
	static constexpr OpenFlags newFile() { return {.write = true, .create = true, .truncate = true}; }
	static constexpr OpenFlags testNewFile() { return {.write = true, .create = true, .truncate = true, .test = true}; }
	// create a file for reading/writing, keeping an existing one
	static constexpr OpenFlags createFile() { return {.read = true, .write = true, .create = true}; }
	static constexpr OpenFlags testCreateFile() { return {.read = true, .write = true, .create = true, .test = true}; }
};

template <class T>
concept Readable =
	requires(T &i, void *buff, size_t bytes)
	{
		i.read(buff, bytes);
	};

template <class T>
concept Writable =
	requires(T &i, const void *buff, size_t bytes)
	{
		i.write(buff, bytes);
	};

template <class T>
concept Seekable =
	requires(T &i, off_t offset, IOSeekMode mode)
	{
		i.seek(offset, mode);
	};

struct OutVector
{
	// should map exactly to iovec in <sys/uio.h>
	const void *dataPtr{};
	size_t bytes{};

	constexpr OutVector() = default;
	template<class T>
	constexpr OutVector(std::span<const T> buff):
		dataPtr{static_cast<const void*>(buff.data())},
		bytes{buff.size_bytes()} {}
	constexpr auto *data() const { return static_cast<const uint8_t*>(dataPtr); }
	constexpr size_t size() const { return bytes; }
	auto iovecPtr() const { return reinterpret_cast<const iovec*>(this); }
};

class AssetIO;
class FileIO;
class MapIO;
class IO;

}
