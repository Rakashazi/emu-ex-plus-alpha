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

#pragma once

#include <imagine/engine-globals.h>
#include <imagine/io/IO.hh>
#include <imagine/io/BufferMapIO.hh>

class PosixIO : public IO
{
public:
	using IOUtils::read;
	using IOUtils::readAtPos;
	using IOUtils::write;
	using IOUtils::tell;

	constexpr PosixIO() {}
	~PosixIO() override;
	PosixIO(PosixIO &&o);
	PosixIO &operator=(PosixIO &&o);
	operator GenericIO();
	CallResult open(const char *path, uint mode = 0);
	CallResult create(const char *path, uint mode = 0)
	{
		mode |= OPEN_WRITE | OPEN_CREATE;
		return open(path, mode);
	}
	int fd() const;

	ssize_t read(void *buff, size_t bytes, CallResult *resultOut) override;
	ssize_t readAtPos(void *buff, size_t bytes, off_t offset, CallResult *resultOut) override;
	ssize_t write(const void *buff, size_t bytes, CallResult *resultOut) override;
	CallResult truncate(off_t offset) override;
	off_t tell(CallResult *resultOut) override;
	CallResult seek(off_t offset, SeekMode mode) override;
	void close() override;
	void sync() override;
	size_t size() override;
	bool eof() override;
	void advise(off_t offset, size_t bytes, Advice advice) override;
	explicit operator bool() override;

protected:
	int fd_ = -1;

	// no copying outside of class
	PosixIO(const PosixIO &) = default;
	PosixIO &operator=(const PosixIO &) = default;
};

CallResult openPosixMapIO(BufferMapIO &io, int fd);
