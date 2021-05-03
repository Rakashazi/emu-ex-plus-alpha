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
#include <imagine/util/fd-utils.h>
#include "IOUtils.hh"
#include <sys/mman.h>

template class IOUtils<PosixFileIO>;

static IO& getIO(std::variant<PosixIO, BufferMapIO> &ioImpl)
{
	return std::visit([](auto &&io) -> IO& { return io; }, ioImpl);
}

PosixFileIO::operator IO*() { return &getIO(ioImpl); }

PosixFileIO::operator IO&() { return getIO(ioImpl); }

GenericIO PosixFileIO::makeGeneric()
{
	return std::visit([](auto &&io) { return GenericIO{io}; }, ioImpl);
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
		BufferMapIO mappedFile = makePosixMapIO(access, std::get<PosixIO>(ioImpl).fd());
		if(mappedFile)
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

BufferMapIO PosixFileIO::makePosixMapIO(IO::AccessHint access, int fd)
{
	off_t size = fd_size(fd);
	int flags = MAP_SHARED;
	#if defined __linux__
	if(access == IO::AccessHint::ALL)
		flags |= MAP_POPULATE;
	#endif
	void *data = mmap(nullptr, size, PROT_READ, flags, fd, 0);
	if(data == MAP_FAILED)
		return {};
	BufferMapIO io;
	io.open((const char*)data, size,
		[data](BufferMapIO &io)
		{
			logMsg("unmapping %p", data);
			munmap(data, io.size());
		});
	return io;
}

ssize_t PosixFileIO::read(void *buff, size_t bytes, std::error_code *ecOut)
{
	return std::visit([&](auto &&io){ return io.read(buff, bytes, ecOut); }, ioImpl);
}

ssize_t PosixFileIO::readAtPos(void *buff, size_t bytes, off_t offset, std::error_code *ecOut)
{
	return std::visit([&](auto &&io){ return io.readAtPos(buff, bytes, offset, ecOut); }, ioImpl);
}

const uint8_t *PosixFileIO::mmapConst()
{
	return std::visit([&](auto &&io){ return io.mmapConst(); }, ioImpl);
}

ssize_t PosixFileIO::write(const void *buff, size_t bytes, std::error_code *ecOut)
{
	return std::visit([&](auto &&io){ return io.write(buff, bytes, ecOut); }, ioImpl);
}

std::error_code PosixFileIO::truncate(off_t offset)
{
	return std::visit([&](auto &&io){ return io.truncate(offset); }, ioImpl);
}

off_t PosixFileIO::seek(off_t offset, IO::SeekMode mode, std::error_code *ecOut)
{
	return std::visit([&](auto &&io){ return io.seek(offset, mode, ecOut); }, ioImpl);
}

void PosixFileIO::close()
{
	std::visit([&](auto &&io){ io.close(); }, ioImpl);
}

void PosixFileIO::sync()
{
	std::visit([&](auto &&io){ io.sync(); }, ioImpl);
}

size_t PosixFileIO::size()
{
	return std::visit([&](auto &&io){ return io.size(); }, ioImpl);
}

bool PosixFileIO::eof()
{
	return std::visit([&](auto &&io){ return io.eof(); }, ioImpl);
}

void PosixFileIO::advise(off_t offset, size_t bytes, IO::Advice advice)
{
	std::visit([&](auto &&io){ io.advise(offset, bytes, advice); }, ioImpl);
}

PosixFileIO::operator bool() const
{
	return std::visit([&](auto &&io){ return (bool)io; }, ioImpl);
}
