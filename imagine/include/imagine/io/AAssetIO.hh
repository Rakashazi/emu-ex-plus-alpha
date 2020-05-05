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

struct AAsset;

class AAssetIO final : public IO
{
public:
	using IO::read;
	using IO::readAtPos;
	using IO::write;
	using IO::seek;
	using IO::seekS;
	using IO::seekE;
	using IO::seekC;
	using IO::tell;
	using IO::send;
	using IO::constBufferView;
	using IO::get;

	constexpr AAssetIO() {}
	AAssetIO(AAssetIO &&o);
	AAssetIO &operator=(AAssetIO &&o);
	~AAssetIO() final;
	GenericIO makeGeneric();
	std::error_code open(const char *name, AccessHint access);

	ssize_t read(void *buff, size_t bytes, std::error_code *ecOut) final;
	const char *mmapConst() final;
	ssize_t write(const void *buff, size_t bytes, std::error_code *ecOut) final;
	off_t seek(off_t offset, SeekMode mode, std::error_code *ecOut) final;
	void close() final;
	size_t size() final;
	bool eof() final;
	explicit operator bool() const final;
	void advise(off_t offset, size_t bytes, Advice advice) final;

protected:
	AAsset *asset{};
	BufferMapIO mapIO{};

	bool makeMapIO();
};
