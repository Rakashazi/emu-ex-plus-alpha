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

#include <imagine/io/PosixIO.hh>
#include <imagine/io/MapIO.hh>
#include <imagine/io/ArchiveIO.hh>
#ifdef __ANDROID__
#include <imagine/io/AAssetIO.hh>
#endif
#include <variant>

namespace IG
{

using IOVariant = std::variant<
PosixIO,
MapIO,
#ifdef __ANDROID__
AAssetIO,
#endif
ArchiveIO>;

class IO : public IOVariant, public IOUtils<IO>
{
public:
	using IOVariant::IOVariant;
	using IOVariant::operator=;
	using IOUtilsBase = IOUtils<IO>;
	using IOUtilsBase::write;
	using IOUtilsBase::seek;
	using IOUtilsBase::tell;
	using IOUtilsBase::send;
	using IOUtilsBase::buffer;
	using IOUtilsBase::get;
	using IOUtilsBase::toFileStream;
	using AccessHint = IOAccessHint;
	using Advice = IOAdvice;
	using BufferMode = IOBufferMode;
	using SeekMode = IOSeekMode;

	// reading
	ssize_t read(void *buff, size_t bytes);
	ssize_t readAtPos(void *buff, size_t bytes, off_t offset);

	// writing
	ssize_t write(const void *buff, size_t bytes);
	bool truncate(off_t offset);

	// seeking
	off_t seek(off_t offset, SeekMode mode);

	// other functions
	std::span<uint8_t> map();
	void sync();
	size_t size();
	bool eof();
	void advise(off_t offset, size_t bytes, Advice advice);
	explicit operator bool() const;
};

}
