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

#include <imagine/io/FileIO.hh>
#include <imagine/logger/logger.h>

PosixFileIO::PosixFileIO()
{
	new(&posixIO()) PosixIO{};
}

PosixFileIO::~PosixFileIO()
{
	io().close();
}

PosixFileIO::PosixFileIO(PosixFileIO &&o)
{
	usingMapIO = o.usingMapIO;
	if(o.usingMapIO)
		new(&bufferMapIO()) BufferMapIO{std::move(o.bufferMapIO())};
	else
		new(&posixIO()) PosixIO{std::move(o.posixIO())};
}

PosixFileIO &PosixFileIO::operator=(PosixFileIO &&o)
{
	close();
	usingMapIO = o.usingMapIO;
	if(o.usingMapIO)
		new(&bufferMapIO()) BufferMapIO{std::move(o.bufferMapIO())};
	else
		new(&posixIO()) PosixIO{std::move(o.posixIO())};
	return *this;
}

PosixFileIO::operator GenericIO()
{
	if(usingMapIO)
		return GenericIO{bufferMapIO()};
	else
		return GenericIO{posixIO()};
}

CallResult PosixFileIO::open(const char *path, uint mode)
{
	close();
	{
		PosixIO file;
		auto r = file.open(path, mode);
		if(r != OK)
		{
			return r;
		}
		new(&posixIO()) PosixIO{std::move(file)};
		usingMapIO = false;
	}

	// try to open as memory map if read-only
	if(!(mode & IO::OPEN_WRITE))
	{
		BufferMapIO mappedFile;
		if(openPosixMapIO(mappedFile, posixIO().fd()) == OK)
		{
			//logMsg("switched to mmap mode");
			posixIO().close();
			new(&bufferMapIO()) BufferMapIO{std::move(mappedFile)};
			usingMapIO = true;
		}
	}

	// setup advice if using read access
	if((mode & IO::OPEN_READ) && size() > 4096)
	{
		logMsg("set sequential advice for file");
		advise(0, 0, IO::ADVICE_SEQUENTIAL);
	}

	return OK;
}

ssize_t PosixFileIO::read(void *buff, size_t bytes, CallResult *resultOut)
{
	return io().read(buff, bytes, resultOut);
}

ssize_t PosixFileIO::readAtPos(void *buff, size_t bytes, off_t offset, CallResult *resultOut)
{
	return io().readAtPos(buff, bytes, offset, resultOut);
}

const char *PosixFileIO::mmapConst()
{
	return io().mmapConst();
}

ssize_t PosixFileIO::write(const void *buff, size_t bytes, CallResult *resultOut)
{
	return io().write(buff, bytes, resultOut);
}

CallResult PosixFileIO::truncate(off_t offset)
{
	return io().truncate(offset);
}

off_t PosixFileIO::tell(CallResult *resultOut)
{
	return io().tell(resultOut);
}

CallResult PosixFileIO::seek(off_t offset, IO::SeekMode mode)
{
	return io().seek(offset, mode);
}

void PosixFileIO::close()
{
	io().close();
}

void PosixFileIO::sync()
{
	io().sync();
}

size_t PosixFileIO::size()
{
	return io().size();
}

bool PosixFileIO::eof()
{
	return io().eof();
}

void PosixFileIO::advise(off_t offset, size_t bytes, IO::Advice advice)
{
	io().advise(offset, bytes, advice);
}

PosixFileIO::operator bool()
{
	return (bool)io();
}

IO &PosixFileIO::io()
{
	return usingMapIO ? (IO&)bufferMapIO() : (IO&)posixIO();
}
