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
#include <imagine/io/IO.hh>
#include <imagine/util/memory/UniqueFileDescriptor.hh>
#include <imagine/util/string/CStringView.hh>

namespace IG
{

class PosixIO final : public IO
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
	using IO::buffer;
	using IO::get;

	constexpr PosixIO() = default;
	constexpr PosixIO(int fd):fd_{fd} {}
	PosixIO(IG::CStringView path, OpenFlags oFlags = {});
	static PosixIO create(IG::CStringView path, OpenFlags oFlags = {});
	int releaseFd();
	int fd() const;
	ssize_t read(void *buff, size_t bytes) final;
	ssize_t readAtPos(void *buff, size_t bytes, off_t offset) final;
	ssize_t write(const void *buff, size_t bytes) final;
	bool truncate(off_t offset) final;
	off_t seek(off_t offset, IO::SeekMode mode) final;
	void sync() final;
	size_t size() final;
	bool eof() final;
	void advise(off_t offset, size_t bytes, Advice advice) final;
	explicit operator bool() const final;

protected:
	IG::UniqueFileDescriptor fd_{};
};

}
