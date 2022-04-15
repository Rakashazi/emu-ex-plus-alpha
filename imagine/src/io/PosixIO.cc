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
#include <imagine/io/PosixIO.hh>
#include <imagine/util/fd-utils.h>
#include <imagine/util/utility.h>
#include <imagine/util/string/StaticString.hh>
#include <imagine/logger/logger.h>
#include "utils.hh"
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <system_error>

namespace IG
{

#if !defined __linux__
constexpr int MAP_POPULATE = 0;
#endif

static IG::StaticString<5> flagsString(IO::OpenFlags openFlags)
{
	IG::StaticString<5> logFlagsStr{};
	if(openFlags & IO::READ_BIT) logFlagsStr += 'r';
	if(openFlags & IO::WRITE_BIT) logFlagsStr += 'w';
	if(openFlags & IO::CREATE_BIT) logFlagsStr += 'c';
	if(openFlags & IO::TRUNCATE_BIT) logFlagsStr += 't';
	return logFlagsStr;
}

PosixIO::PosixIO(IG::CStringView path, OpenFlags openFlags)
{
	// validate flags
	assert(openFlags < IG::bit(OPEN_FLAGS_BITS+1));

	constexpr mode_t defaultOpenMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int flags = 0;
	mode_t openMode{};

	// setup flags
	if(openFlags & WRITE_BIT)
	{
		if(openFlags & READ_BIT)
		{
			flags |= O_RDWR;
		}
		else
		{
			flags |= O_WRONLY;
		}
	}
	else
	{
		flags |= O_RDONLY;
	}
	if(openFlags & CREATE_BIT)
	{
		flags |= O_CREAT;
		openMode = defaultOpenMode;
		if(openFlags & TRUNCATE_BIT)
		{
			flags |= O_TRUNC;
		}
	}

	if((fd_ = ::open(path, flags, openMode)) == -1) [[unlikely]]
	{
		if constexpr(Config::DEBUG_BUILD)
			logErr("error opening file (%s) @ %s:%s", flagsString(openFlags).data(), path.data(), strerror(errno));
		if(openFlags & IO::TEST_BIT)
			return;
		else
			throw std::system_error{errno, std::system_category(), path};
	}
	if constexpr(Config::DEBUG_BUILD)
		logMsg("opened file (%s) fd %d @ %s", flagsString(openFlags).data(), (int)fd_, path.data());
}

ssize_t PosixIO::read(void *buff, size_t bytes)
{
	auto bytesRead = ::read(fd_, buff, bytes);
	if(bytesRead == -1) [[unlikely]]
	{
		if(Config::DEBUG_BUILD && errno != EAGAIN)
			logErr("error reading %zu bytes", bytes);
	}
	else
	{
		//logMsg("read %zd bytes out of %zu requested from fd_: %d", bytesRead, bytes, fd_);
	}
	return bytesRead;
}

ssize_t PosixIO::readAtPos(void *buff, size_t bytes, off_t offset)
{
	auto bytesRead = ::pread(fd_, buff, bytes, offset);
	if(bytesRead == -1) [[unlikely]]
	{
		logErr("error reading %zu bytes at offset %lld", bytes, (long long)offset);
	}
	return bytesRead;
}

ssize_t PosixIO::write(const void *buff, size_t bytes)
{
	auto bytesWritten = ::write(fd_, buff, bytes);
	if(bytesWritten == -1)
	{
		logErr("error writing %zu bytes", bytes);
	}
	return bytesWritten;
}

bool PosixIO::truncate(off_t offset)
{
	logMsg("truncating at offset %lld", (long long)offset);
	if(ftruncate(fd_, offset) == -1) [[unlikely]]
	{
		logErr("truncate failed");
		return false;
	}
	return true;
}

off_t PosixIO::seek(off_t offset, IO::SeekMode mode)
{
	auto newPos = lseek(fd_, offset, (int)mode);
	if(newPos == -1) [[unlikely]]
	{
		logErr("seek to offset %lld failed", (long long)offset);
		return -1;
	}
	return newPos;
}

void PosixIO::sync()
{
	fsync(fd_);
}

size_t PosixIO::size()
{
	auto s = fd_size(fd_);
	if(s == 0)
	{
		logMsg("fd:%d is empty or a stream", fd_.get());
	}
	return s;
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

PosixIO::operator bool() const
{
	return fd_ != -1;
}

IOBuffer PosixIO::releaseBuffer()
{
	return mapRange(0, size(), MAP_WRITE);
}

IOBuffer PosixIO::mapRange(off_t start, size_t size, MapFlags mapFlags)
{
	int flags = MAP_SHARED;
	if(mapFlags & MAP_POPULATE_PAGES)
		flags |= MAP_POPULATE;
	int prot = PROT_READ;
	if(mapFlags & MAP_WRITE)
		prot |= PROT_WRITE;
	void *data = mmap(nullptr, size, prot, flags, fd(), start);
	if(data == MAP_FAILED)
	{
		logErr("mmap fd:%d @ %zu (%zu bytes) failed", fd(), (size_t)start, size);
		return {};
	}
	logMsg("mapped fd:%d @ %zu to %p (%zu bytes)", fd(), (size_t)start, data, size);
	return byteBufferFromMmap(data, size);
}

IOBuffer PosixIO::byteBufferFromMmap(void *data, size_t size)
{
	return
	{
		{(uint8_t*)data, size}, IOBuffer::MAPPED_FILE_BIT,
		[](const uint8_t *ptr, size_t size)
		{
			logMsg("unmapping:%p (%zu bytes)", ptr, size);
			if(munmap((void*)ptr, size) == -1)
			{
				if(Config::DEBUG_BUILD)
					logErr("munmap(%p, %zu) error:%s", ptr, size, strerror(errno));
			}
		}
	};
}

UniqueFileDescriptor PosixIO::releaseFd()
{
	return std::move(fd_);
}

int PosixIO::fd() const
{
	return fd_;
}

}
