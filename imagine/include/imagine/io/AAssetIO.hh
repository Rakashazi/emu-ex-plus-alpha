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
#include <imagine/io/BufferMapIO.hh>
#include <android/asset_manager.h>

class AAssetIO : public IO
{
public:
	using IOUtils::read;
	using IOUtils::write;
	using IOUtils::seek;

	AAssetIO() {}
	~AAssetIO() final;
	AAssetIO(AAssetIO &&o);
	AAssetIO &operator=(AAssetIO &&o);
	GenericIO makeGeneric();
	std::error_code open(const char *name);

	ssize_t read(void *buff, size_t bytes, std::error_code *ecOut) final;
	const char *mmapConst() final;
	ssize_t write(const void *buff, size_t bytes, std::error_code *ecOut) final;
	off_t seek(off_t offset, SeekMode mode, std::error_code *ecOut) final;
	void close() final;
	size_t size() final;
	bool eof() final;
	explicit operator bool() final;

public:
	AAsset *asset{};
	BufferMapIO mapIO{};
};
