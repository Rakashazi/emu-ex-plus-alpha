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

static IG::StaticString<5> flagsString(uint32_t openFlags)
{
	IG::StaticString<5> logFlagsStr{};
	if(openFlags & IO::OPEN_READ || !(openFlags & IO::OPEN_WRITE)) logFlagsStr += 'r';
	if(openFlags & IO::OPEN_WRITE) logFlagsStr += 'w';
	if(openFlags & IO::OPEN_CREATE_NEW) logFlagsStr += 'c';
	if(openFlags & IO::OPEN_KEEP_EXISTING) logFlagsStr += 't';
	return logFlagsStr;
}

PosixIO::PosixIO(IG::CStringView path, uint32_t openFlags)
{
	// validate flags
	assert(openFlags < IG::bit(OPEN_FLAGS_BITS+1));

	constexpr mode_t defaultOpenMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int flags = 0;
	mode_t openMode{};

	// setup flags
	if(openFlags & OPEN_WRITE)
	{
		if(openFlags & OPEN_READ)
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
	if(openFlags & OPEN_CREATE_NEW)
	{
		flags |= O_CREAT;
		openMode = defaultOpenMode;
		if(!(openFlags & OPEN_KEEP_EXISTING))
		{
			flags |= O_TRUNC;
		}
	}

	if((fd_ = ::open(path, flags, openMode)) == -1) [[unlikely]]
	{
		if constexpr(Config::DEBUG_BUILD)
			logErr("error opening file (%s) @ %s:%s", flagsString(openFlags).data(), path.data(), strerror(errno));
		if(openFlags & IO::OPEN_TEST)
			return;
		else
			throw std::system_error{errno, std::system_category(), path};
	}
	if constexpr(Config::DEBUG_BUILD)
		logMsg("opened file (%s) fd %d @ %s", flagsString(openFlags).data(), (int)fd_, path.data());
}

PosixIO PosixIO::create(IG::CStringView path, uint32_t mode)
{
	mode |= OPEN_CREATE;
	return {path, mode};
}

ssize_t PosixIO::read(void *buff, size_t bytes, std::error_code *ecOut)
{
	auto bytesRead = ::read(fd_, buff, bytes);
	if(bytesRead == -1)
	{
		if(Config::DEBUG_BUILD && errno != EAGAIN)
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
	auto newPos = lseek(fd_, offset, (int)mode);
	if(newPos == -1) [[unlikely]]
	{
		logErr("seek to offset %lld failed", (long long)offset);
		if(ecOut)
			*ecOut = {errno, std::system_category()};
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

int PosixIO::releaseFd()
{
	return fd_.release();
}

int PosixIO::fd() const
{
	return fd_;
}
