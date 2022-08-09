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
#include <imagine/config/defs.hh>
#include <imagine/logger/logger.h>
#include "utils.hh"
#include "IOUtils.hh"
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <system_error>

namespace IG
{

template class IOUtils<PosixIO>;

#if !defined __linux__
constexpr int MAP_POPULATE = 0;
#endif

static auto flagsString(OpenFlagsMask openFlags)
{
	IG::StaticString<5> logFlagsStr{};
	if(to_underlying(openFlags & OpenFlagsMask::READ)) logFlagsStr += 'r';
	if(to_underlying(openFlags & OpenFlagsMask::WRITE)) logFlagsStr += 'w';
	if(to_underlying(openFlags & OpenFlagsMask::CREATE)) logFlagsStr += 'c';
	if(to_underlying(openFlags & OpenFlagsMask::TRUNCATE)) logFlagsStr += 't';
	return logFlagsStr;
}

static auto protectionFlagsString(int flags)
{
	IG::StaticString<3> logFlagsStr{};
	if(flags & PROT_READ) logFlagsStr += 'r';
	if(flags & PROT_WRITE) logFlagsStr += 'w';
	return logFlagsStr;
}

PosixIO::PosixIO(IG::CStringView path, OpenFlagsMask openFlags)
{
	constexpr mode_t defaultOpenMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int flags = 0;
	mode_t openMode{};

	// setup flags
	if(to_underlying(openFlags & OpenFlagsMask::WRITE))
	{
		if(to_underlying(openFlags & OpenFlagsMask::READ))
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
	if(to_underlying(openFlags & OpenFlagsMask::CREATE))
	{
		flags |= O_CREAT;
		openMode = defaultOpenMode;
		if(to_underlying(openFlags & OpenFlagsMask::TRUNCATE))
		{
			flags |= O_TRUNC;
		}
	}

	if((fd_ = ::open(path, flags, openMode)) == -1) [[unlikely]]
	{
		if constexpr(Config::DEBUG_BUILD)
			logErr("error opening file (%s) @ %s:%s", flagsString(openFlags).data(), path.data(), strerror(errno));
		if(to_underlying(openFlags & OpenFlagsMask::TEST))
			return;
		else
			throw std::system_error{errno, std::system_category(), path};
	}
	if constexpr(Config::DEBUG_BUILD)
		logMsg("opened (%s) fd:%d @ %s", flagsString(openFlags).data(), fd(), path.data());
}

ssize_t PosixIO::read(void *buff, size_t bytes)
{
	auto bytesRead = ::read(fd(), buff, bytes);
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
	auto bytesRead = ::pread(fd(), buff, bytes, offset);
	if(bytesRead == -1) [[unlikely]]
	{
		logErr("error reading %zu bytes at offset %lld", bytes, (long long)offset);
	}
	return bytesRead;
}

ssize_t PosixIO::write(const void *buff, size_t bytes)
{
	auto bytesWritten = ::write(fd(), buff, bytes);
	if(bytesWritten == -1)
	{
		logErr("error writing %zu bytes", bytes);
	}
	return bytesWritten;
}

bool PosixIO::truncate(off_t offset)
{
	logMsg("truncating at offset %lld", (long long)offset);
	if(ftruncate(fd(), offset) == -1) [[unlikely]]
	{
		logErr("truncate failed");
		return false;
	}
	return true;
}

off_t PosixIO::seek(off_t offset, SeekMode mode)
{
	auto newPos = lseek(fd(), offset, (int)mode);
	if(newPos == -1) [[unlikely]]
	{
		logErr("seek to offset %lld failed", (long long)offset);
		return -1;
	}
	return newPos;
}

void PosixIO::sync()
{
	fsync(fd());
}

size_t PosixIO::size()
{
	auto s = fd_size(fd());
	if(s == 0)
	{
		logMsg("fd:%d is empty or a stream", fd());
	}
	return s;
}

bool PosixIO::eof()
{
	return tell() >= (off_t)size();
}

#if _XOPEN_SOURCE >= 600 || _POSIX_C_SOURCE >= 200112L
static int adviceToFAdv(IOAdvice advice)
{
	switch(advice)
	{
		default: return POSIX_FADV_NORMAL;
		case IOAdvice::SEQUENTIAL: return POSIX_FADV_SEQUENTIAL;
		case IOAdvice::RANDOM: return POSIX_FADV_RANDOM;
		case IOAdvice::WILLNEED: return POSIX_FADV_WILLNEED;
	}
}
#endif

void PosixIO::advise(off_t offset, size_t bytes, Advice advice)
{
	#ifdef __APPLE__
	if(advice == Advice::SEQUENTIAL || advice == Advice::WILLNEED)
		fcntl(fd(), F_RDAHEAD, 1);
	#else
		#if _XOPEN_SOURCE >= 600 || _POSIX_C_SOURCE >= 200112L
		int fAdv =  adviceToFAdv(advice);
		if(posix_fadvise(fd(), offset, bytes, fAdv) != 0)
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
	auto flags = fcntl(fd(), F_GETFL);
	if(flags == -1) [[unlikely]]
	{
		if(Config::DEBUG_BUILD)
			logErr("fcntl(%d) failed:%s", fd(), strerror(errno));
		flags = 0;
	}
	bool isWritable = (flags & O_WRONLY) || (flags & O_RDWR);
	return mapRange(0, size(), isWritable ? MAP_WRITE : 0);
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
	if(data == MAP_FAILED) [[unlikely]]
	{
		logErr("mmap (%s) fd:%d @ %zu (%zu bytes) failed", protectionFlagsString(prot).data(), fd(), (size_t)start, size);
		return {};
	}
	logMsg("mapped (%s) fd:%d @ %zu to %p (%zu bytes)", protectionFlagsString(prot).data(), fd(), (size_t)start, data, size);
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
