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

#include <imagine/engine-globals.h>
#include <imagine/io/BufferMapIO.hh>
#include <android/asset_manager.h>

class AAssetIO : public IO
{
public:
	using IOUtils::read;
	using IOUtils::write;
	using IOUtils::tell;

	constexpr AAssetIO() {}
	~AAssetIO() override;
	AAssetIO(AAssetIO &&o);
	AAssetIO &operator=(AAssetIO &&o);
	operator GenericIO();
	CallResult open(const char *name);

	ssize_t read(void *buff, size_t bytes, CallResult *resultOut) override;
	const char *mmapConst() override;
	ssize_t write(const void *buff, size_t bytes, CallResult *resultOut) override;
	off_t tell(CallResult *resultOut) override;
	CallResult seek(off_t offset, SeekMode mode) override;
	void close() override;
	size_t size() override;
	bool eof() override;
	explicit operator bool() override;

public:
	AAsset *asset{};
	BufferMapIO mapIO;
};
