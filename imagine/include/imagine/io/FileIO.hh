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

#include <type_traits>
#include <imagine/io/IO.hh>
#include <imagine/fs/sys.hh> // for FsSys::PathString
#include <imagine/base/Base.hh>

#if defined CONFIG_IO_WIN32
#include <imagine/io/Win32IO.hh>
using FileIO = Win32IO;
#else
#include <imagine/io/PosixFileIO.hh>
using FileIO = PosixFileIO;
#endif

#ifdef __ANDROID__
#include <imagine/io/AAssetIO.hh>
using AssetIO = AAssetIO;
#else
using AssetIO = FileIO;
#endif

AssetIO openAppAssetIO(const char *name);

template <size_t S>
static AssetIO openAppAssetIO(std::array<char, S> name)
{
	return openAppAssetIO(name.data());
}

CallResult writeToNewFile(const char *path, void *data, size_t size);
ssize_t readFromFile(const char *path, void *data, size_t size);
CallResult writeIOToNewFile(IO &io, const char *path);
