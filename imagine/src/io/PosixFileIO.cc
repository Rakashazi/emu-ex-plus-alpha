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
#include "IOUtils.hh"

template class IOUtils<PosixFileIO>;

PosixFileIO::PosixFileIO(PosixFileIO &&o)
{
	*this = std::move(o);
}

PosixFileIO &PosixFileIO::operator=(PosixFileIO &&o)
{
	close();
	ioImpl = std::exchange(o.ioImpl, {});
	return *this;
}

PosixFileIO::~PosixFileIO()
{
	close();
}

PosixFileIO::operator IO*() { return &io(); }

PosixFileIO::operator IO&() { return io(); }

GenericIO PosixFileIO::makeGeneric()
{
	if(std::holds_alternative<BufferMapIO>(ioImpl))
		return GenericIO{std::get<BufferMapIO>(ioImpl)};
	else
		return GenericIO{std::get<PosixIO>(ioImpl)};
}

std::error_code PosixFileIO::open(const char *path, IO::AccessHint access, uint32_t mode)
{
	close();
	{
		PosixIO file;
		auto ec = file.open(path, mode);
		if(ec)
		{
			return ec;
		}
		ioImpl = std::move(file);
	}

	// try to open as memory map if read-only
	if(!(mode & IO::OPEN_WRITE))
	{
		BufferMapIO mappedFile;
		auto ec = openPosixMapIO(mappedFile, access, std::get<PosixIO>(ioImpl).fd());
		if(!ec)
		{
			//logMsg("switched to mmap mode");
			ioImpl = std::move(mappedFile);
		}
	}

	// setup advice if using read access
	if((mode & IO::OPEN_READ))
	{
		switch(access)
		{
			bdefault:
			bcase IO::AccessHint::SEQUENTIAL:	advise(0, 0, IO::Advice::SEQUENTIAL);
			bcase IO::AccessHint::RANDOM:	advise(0, 0, IO::Advice::RANDOM);
			bcase IO::AccessHint::ALL:	advise(0, 0, IO::Advice::WILLNEED);
		}
	}

	return {};
}

ssize_t PosixFileIO::read(void *buff, size_t bytes, std::error_code *ecOut)
{
	return io().read(buff, bytes, ecOut);
}

ssize_t PosixFileIO::readAtPos(void *buff, size_t bytes, off_t offset, std::error_code *ecOut)
{
	return io().readAtPos(buff, bytes, offset, ecOut);
}

const char *PosixFileIO::mmapConst()
{
	return io().mmapConst();
}

ssize_t PosixFileIO::write(const void *buff, size_t bytes, std::error_code *ecOut)
{
	return io().write(buff, bytes, ecOut);
}

std::error_code PosixFileIO::truncate(off_t offset)
{
	return io().truncate(offset);
}

off_t PosixFileIO::seek(off_t offset, IO::SeekMode mode, std::error_code *ecOut)
{
	return io().seek(offset, mode, ecOut);
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

PosixFileIO::operator bool() const
{
	return (bool)io();
}

IO &PosixFileIO::io()
{
	return std::visit([](auto &&obj) -> IO& { return obj; }, ioImpl);
}

const IO &PosixFileIO::io() const
{
	return std::visit([](auto &&obj) -> const IO& { return obj; }, ioImpl);
}
