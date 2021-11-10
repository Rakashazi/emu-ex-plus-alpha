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
#include <imagine/util/string/CStringView.hh>

class FileIO: public PosixFileIO
{
public:
	using PosixFileIO::PosixFileIO;
};

#ifdef __ANDROID__
#include <imagine/io/AAssetIO.hh>
using AssetIOImpl = AAssetIO;
#else
using AssetIOImpl = FileIO;
#endif

class AssetIO: public AssetIOImpl
{
public:
	using AssetIOImpl::AssetIOImpl;
};

namespace FileUtils
{

static constexpr size_t defaultBufferReadSizeLimit = 0x2000000; // 32 Megabytes

ssize_t writeToPath(IG::CStringView path, void *data, size_t size);
ssize_t writeToPath(IG::CStringView path, IO &io);
ssize_t readFromPath(IG::CStringView path, void *data, size_t size);
IG::ByteBuffer bufferFromPath(IG::CStringView path, unsigned openFlags = 0, size_t sizeLimit = defaultBufferReadSizeLimit);

}
