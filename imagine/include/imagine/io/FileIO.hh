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
#include <imagine/io/PosixFileIO.hh>
using FileIO = PosixFileIO;

#ifdef __ANDROID__
#include <imagine/io/AAssetIO.hh>
using AssetIOImpl = AAssetIO;
#else
using AssetIOImpl = FileIO;
#endif

class AssetIO final: public AssetIOImpl
{
public:
	using AssetIOImpl::AssetIOImpl;
};

namespace FileUtils
{

ssize_t writeToPath(const char *path, void *data, size_t size, std::error_code *ecOut = nullptr);
ssize_t writeToPath(const char *path, IO &io, std::error_code *ecOut = nullptr);
ssize_t readFromPath(const char *path, void *data, size_t size);

}
