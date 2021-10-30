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
#include <imagine/fs/FSDefs.hh>
#include <imagine/util/string/CStringView.hh>
#include <variant>

class PosixFileIO : public IOUtils<PosixFileIO>
{
public:
	using IOUtilsBase = IOUtils<PosixFileIO>;
	using IOUtilsBase::read;
	using IOUtilsBase::readAtPos;
	using IOUtilsBase::write;
	using IOUtilsBase::seek;
	using IOUtilsBase::seekS;
	using IOUtilsBase::seekE;
	using IOUtilsBase::seekC;
	using IOUtilsBase::tell;
	using IOUtilsBase::send;
	using IOUtilsBase::buffer;
	using IOUtilsBase::get;

	constexpr PosixFileIO() {}
	PosixFileIO(IG::CStringView path, IO::AccessHint access, unsigned openFlags = 0);
	[[nodiscard]]
	static PosixFileIO create(IG::CStringView path, unsigned openFlags = 0);
	explicit operator IO*();
	operator IO&();
	operator GenericIO();
	static MapIO makePosixMapIO(IO::AccessHint access, int fd);
	ssize_t read(void *buff, size_t bytes, std::error_code *ecOut);
	ssize_t readAtPos(void *buff, size_t bytes, off_t offset, std::error_code *ecOut);
	std::span<uint8_t> map();
	ssize_t write(const void *buff, size_t bytes, std::error_code *ecOut);
	std::error_code truncate(off_t offset);
	off_t seek(off_t offset, IO::SeekMode mode, std::error_code *ecOut);
	void sync();
	size_t size();
	bool eof();
	void advise(off_t offset, size_t bytes, IO::Advice advice);
	explicit operator bool() const;
	IG::ByteBuffer releaseBuffer();

protected:
	std::variant<PosixIO, MapIO> ioImpl{};
};
