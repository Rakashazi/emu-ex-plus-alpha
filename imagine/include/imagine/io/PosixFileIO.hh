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

class PosixFileIO : public IOUtils<PosixFileIO>
{
public:
	using IOUtils::read;
	using IOUtils::readAtPos;
	using IOUtils::write;
	using IOUtils::tell;

	PosixFileIO();
	~PosixFileIO();
	PosixFileIO(PosixFileIO &&o);
	PosixFileIO &operator=(PosixFileIO &&o);
	operator IO*(){ return &io(); }
	operator IO&(){ return io(); }
	operator GenericIO();
	CallResult open(const char *path, uint mode = 0);
	CallResult create(const char *path, uint mode = 0)
	{
		mode |= IO::OPEN_WRITE | IO::OPEN_CREATE;
		return open(path, mode);
	}

	ssize_t read(void *buff, size_t bytes, CallResult *resultOut);
	ssize_t readAtPos(void *buff, size_t bytes, off_t offset, CallResult *resultOut);
	const char *mmapConst();
	ssize_t write(const void *buff, size_t bytes, CallResult *resultOut);
	CallResult truncate(off_t offset);
	off_t tell(CallResult *resultOut);
	CallResult seek(off_t offset, IO::SeekMode mode);
	void close();
	void sync();
	size_t size();
	bool eof();
	void advise(off_t offset, size_t bytes, IO::Advice advice);
	explicit operator bool();

protected:
	union [[gnu::aligned]]
	{
		std::aligned_storage_t<sizeof(PosixIO), alignof(PosixIO)> posixIO_;
		std::aligned_storage_t<sizeof(BufferMapIO), alignof(BufferMapIO)> bufferMapIO_;
	};
	bool usingMapIO = false;

	IO &io();
	PosixIO &posixIO()
	{
		return *(PosixIO*)(void*)&posixIO_;
	}
	BufferMapIO &bufferMapIO()
	{
		return *(BufferMapIO*)(void*)&bufferMapIO_;
	}
};
