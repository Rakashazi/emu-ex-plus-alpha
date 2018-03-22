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

#define LOGTAG "PosixIO"
#include <sys/stat.h>
#include <sys/mman.h>
#include <imagine/io/PosixIO.hh>
#include <imagine/util/fd-utils.h>
#include <imagine/util/string.h>
#include <imagine/util/utility.h>
#include <imagine/logger/logger.h>
#include "utils.hh"

using namespace IG;

PosixIO::~PosixIO()
{
	close();
}

PosixIO::PosixIO(PosixIO &&o)
{
	*this = o;
	o.fd_ = -1;
}

PosixIO &PosixIO::operator=(PosixIO &&o)
{
	close();
	*this = o;
	o.fd_ = -1;
	return *this;
}

GenericIO PosixIO::makeGeneric()
{
	return GenericIO{*this};
}

std::error_code PosixIO::open(const char *path, uint mode)
{
	close();

	// validate flags
	assert(mode < bit(OPEN_FLAGS_BITS+1));

	constexpr mode_t defaultOpenMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int flags = 0;
	mode_t openMode{};
	std::array<char, 5> logFlagsStr{};
	
	// setup flags
	if(mode & OPEN_WRITE)
	{
		if(mode & OPEN_READ)
		{
			flags |= O_RDWR;
			string_cat(logFlagsStr, "rw");
		}
		else
		{
			flags |= O_WRONLY;
			string_cat(logFlagsStr, "w");
		}
	}
	else
	{
		flags |= O_RDONLY;
		string_cat(logFlagsStr, "r");
	}
	if(mode & OPEN_CREATE)
	{
		flags |= O_CREAT;
		openMode = defaultOpenMode;
		string_cat(logFlagsStr, "c");
		if(!(mode & OPEN_KEEP_EXISTING))
		{
			flags |= O_TRUNC;
			string_cat(logFlagsStr, "t");
		}
	}

	if((fd_ = ::open(path, flags, openMode)) == -1)
	{
		logMsg("error opening file (%s) @ %s", logFlagsStr.data(), path);
		return {errno, std::system_category()};
	}
	logMsg("opened file (%s) fd %d @ %s", logFlagsStr.data(), fd_, path);
	return {};
}

ssize_t PosixIO::read(void *buff, size_t bytes, std::error_code *ecOut)
{
	auto bytesRead = ::read(fd_, buff, bytes);
	if(bytesRead == -1)
	{
		logErr("error reading %zu bytes", bytes);
		if(ecOut)
			*ecOut = {errno, std::system_category()};
	}
	else
	{
		//logMsg("read %zd bytes out of %zu requested from fd_: %d", bytesRead, bytes, fd_);
	}
	return bytesRead;
}

ssize_t PosixIO::readAtPos(void *buff, size_t bytes, off_t offset, std::error_code *ecOut)
{
	auto bytesRead = ::pread(fd_, buff, bytes, offset);
	if(bytesRead == -1)
	{
		logErr("error reading %zu bytes at offset %lld", bytes, (long long)offset);
		if(ecOut)
			*ecOut = {errno, std::system_category()};
	}
	return bytesRead;
}

ssize_t PosixIO::write(const void *buff, size_t bytes, std::error_code *ecOut)
{
	auto bytesWritten = ::write(fd_, buff, bytes);
	if(bytesWritten == -1)
	{
		logErr("error writing %zu bytes", bytes);
		if(ecOut)
			*ecOut = {errno, std::system_category()};
	}
	return bytesWritten;
}

std::error_code PosixIO::truncate(off_t offset)
{
	logMsg("truncating at offset %lld", (long long)offset);
	if(ftruncate(fd_, offset) == -1)
	{
		logErr("truncate failed");
		return {errno, std::system_category()};
	}
	return {};
}

off_t PosixIO::seek(off_t offset, IO::SeekMode mode, std::error_code *ecOut)
{
	assumeExpr(isSeekModeValid(mode));
	auto newPos = lseek(fd_, offset, mode);
	if(newPos == 1)
	{
		logErr("seek to offset %lld failed", (long long)offset);
		if(ecOut)
			*ecOut = {errno, std::system_category()};
		return -1;
	}
	return newPos;
}

void PosixIO::close()
{
	if(fd_ >= 0)
	{
		::close(fd_);
		logMsg("closed fd: %d", fd_);
		fd_ = -1;
	}
}

void PosixIO::sync()
{
	fsync(fd_);
}

size_t PosixIO::size()
{
	return fd_size(fd_);
}

bool PosixIO::eof()
{
	return tell() >= (off_t)size();
}

#if _XOPEN_SOURCE >= 600 || _POSIX_C_SOURCE >= 200112L
static int adviceToFAdv(IO::Advice advice)
{
	switch(advice)
	{
		default: return POSIX_FADV_NORMAL;
		case IO::Advice::SEQUENTIAL: return POSIX_FADV_SEQUENTIAL;
		case IO::Advice::RANDOM: return POSIX_FADV_RANDOM;
		case IO::Advice::WILLNEED: return POSIX_FADV_WILLNEED;
	}
}
#endif

void PosixIO::advise(off_t offset, size_t bytes, Advice advice)
{
	#ifdef __APPLE__
	if(advice == Advice::SEQUENTIAL || advice == Advice::WILLNEED)
		fcntl(fd_, F_RDAHEAD, 1);
	#else
		#if _XOPEN_SOURCE >= 600 || _POSIX_C_SOURCE >= 200112L
		int fAdv =  adviceToFAdv(advice);
		if(posix_fadvise(fd_, offset, bytes, fAdv) != 0)
		{
			logMsg("fadvise for offset 0x%llX with size %zu failed", (unsigned long long)offset, bytes);
		}
		#endif
	#endif
}

PosixIO::operator bool()
{
	return fd_ != -1;
}

int PosixIO::fd() const
{
	return fd_;
}

std::error_code openPosixMapIO(BufferMapIO &io, IO::AccessHint access, int fd)
{
	io.close();
	off_t size = fd_size(fd);
	int flags = MAP_SHARED;
	#if defined __linux__
	if(access == IO::AccessHint::ALL)
		flags |= MAP_POPULATE;
	#endif
	void *data = mmap(nullptr, size, PROT_READ, flags, fd, 0);
	if(data == MAP_FAILED)
		return {errno, std::system_category()};
	io.open((const char*)data, size,
		[data](BufferMapIO &io)
		{
			logMsg("unmapping %p", data);
			munmap(data, io.size());
		});
	return {};
}
