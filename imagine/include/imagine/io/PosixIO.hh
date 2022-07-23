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

#include <imagine/io/IOUtils.hh>
#include <imagine/util/memory/UniqueFileDescriptor.hh>
#include <imagine/util/string/CStringView.hh>

namespace IG
{

class PosixIO final : public IOUtils<PosixIO>
{
public:
	using IOUtilsBase = IOUtils<PosixIO>;
	using IOUtilsBase::write;
	using IOUtilsBase::seekS;
	using IOUtilsBase::seekE;
	using IOUtilsBase::seekC;
	using IOUtilsBase::tell;
	using IOUtilsBase::send;
	using IOUtilsBase::buffer;
	using IOUtilsBase::get;
	using IOUtilsBase::toFileStream;

	using MapFlags = uint8_t;
	static constexpr MapFlags MAP_WRITE = bit(0);
	static constexpr MapFlags MAP_POPULATE_PAGES = bit(1);

	constexpr PosixIO() = default;
	PosixIO(UniqueFileDescriptor fd):fd_{std::move(fd)} {}
	PosixIO(CStringView path, OpenFlags oFlags = {});
	UniqueFileDescriptor releaseFd();
	int fd() const;
	ssize_t read(void *buff, size_t bytes);
	ssize_t readAtPos(void *buff, size_t bytes, off_t offset);
	ssize_t write(const void *buff, size_t bytes);
	bool truncate(off_t offset);
	off_t seek(off_t offset, SeekMode mode);
	void sync();
	size_t size();
	bool eof();
	void advise(off_t offset, size_t bytes, Advice advice);
	explicit operator bool() const;
	IOBuffer releaseBuffer();
	IOBuffer mapRange(off_t start, size_t size, MapFlags);
	static IOBuffer byteBufferFromMmap(void *data, size_t size);

protected:
	UniqueFileDescriptor fd_{};
};

}
