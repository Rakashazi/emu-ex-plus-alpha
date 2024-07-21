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
#include <imagine/util/fd-utils.h>
#include <imagine/util/utility.h>
#include <imagine/util/string/StaticString.hh>
#include <imagine/util/format.hh>
#include <imagine/config/defs.hh>
#include <imagine/logger/logger.h>
#include "utils.hh"
#include <imagine/io/IOUtils-impl.hh>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <system_error>

namespace IG
{

template class IOUtils<PosixIO>;

constexpr SystemLogger log{"PosixIO"};

#if !defined __linux__
constexpr int MAP_POPULATE = 0;
#endif

constexpr auto flagsString(OpenFlags openFlags)
{
	StaticString<5> logFlagsStr;
	if(openFlags.read) logFlagsStr += 'r';
	if(openFlags.write) logFlagsStr += 'w';
	if(openFlags.create) logFlagsStr += 'c';
	if(openFlags.truncate) logFlagsStr += 't';
	return logFlagsStr;
}

constexpr auto protectionFlagsString(int flags)
{
	StaticString<3> logFlagsStr;
	if(flags & PROT_READ) logFlagsStr += 'r';
	if(flags & PROT_WRITE) logFlagsStr += 'w';
	return logFlagsStr;
}

PosixIO::PosixIO(CStringView path, OpenFlags openFlags)
{
	constexpr mode_t defaultOpenMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int flags = 0;
	mode_t openMode{};

	// setup flags
	if(openFlags.write)
	{
		if(openFlags.read)
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
	if(openFlags.create)
	{
		flags |= O_CREAT;
		openMode = defaultOpenMode;
		if(openFlags.truncate)
		{
			flags |= O_TRUNC;
		}
	}

	if((fd_ = ::open(path, flags, openMode)) == -1) [[unlikely]]
	{
		if constexpr(Config::DEBUG_BUILD)
			log.error("error opening file ({}) @ {}:{}", flagsString(openFlags), path, strerror(errno));
		if(openFlags.test)
			return;
		else
			throw std::system_error{errno, std::generic_category(), path};
	}
	if constexpr(Config::DEBUG_BUILD)
		log.info("opened ({}) fd:{} @ {}", flagsString(openFlags), fd(), path);
}

ssize_t PosixIO::read(void *buff, size_t bytes, std::optional<off_t> offset)
{
	if(offset)
	{
		auto bytesRead = ::pread(fd(), buff, bytes, *offset);
		if(bytesRead == -1) [[unlikely]]
		{
			log.error("error reading {} bytes at offset:{}", bytes, *offset);
		}
		return bytesRead;
	}
	else
	{
		auto bytesRead = ::read(fd(), buff, bytes);
		if(bytesRead == -1) [[unlikely]]
		{
			if(Config::DEBUG_BUILD && errno != EAGAIN)
				log.error("error reading %zu bytes", bytes);
		}
		return bytesRead;
	}
}

ssize_t PosixIO::write(const void *buff, size_t bytes, std::optional<off_t> offset)
{
	if(offset)
	{
		auto bytesWritten = ::pwrite(fd(), buff, bytes, *offset);
		if(bytesWritten == -1)
		{
			log.error("error writing {} bytes at offset {}", bytes, *offset);
		}
		return bytesWritten;
	}
	else
	{
		auto bytesWritten = ::write(fd(), buff, bytes);
		if(bytesWritten == -1)
		{
			log.error("error writing {} bytes", bytes);
		}
		return bytesWritten;
	}
}

#if defined __ANDROID__ && ANDROID_MIN_API < 24
#include <sys/syscall.h>

static ssize_t pwritevWrapper(PosixIO &io, std::span<const OutVector> buffs, off_t offset)
{
	return syscall(__NR_pwritev, io.fd(), buffs.data()->iovecPtr(), buffs.size(), offset);
}
#else
static ssize_t pwritevWrapper(PosixIO &io, std::span<const OutVector> buffs, off_t offset)
{
	#ifdef CONFIG_OS_IOS
	if(__builtin_available(iOS 14, *))
		return ::pwritev(io.fd(), buffs.data()->iovecPtr(), buffs.size(), offset);
	else
		return io.genericWriteVector(buffs, offset);
	#else
	return ::pwritev(io.fd(), buffs.data()->iovecPtr(), buffs.size(), offset);
	#endif
}
#endif

ssize_t PosixIO::writeVector(std::span<const OutVector> buffs, std::optional<off_t> offset)
{
	if(!buffs.size())
		return 0;
	if(offset)
	{
		auto bytesWritten = pwritevWrapper(*this, buffs, *offset);
		if(bytesWritten == -1)
		{
			log.error("error writing {} buffers at offset {}", buffs.size(), *offset);
		}
		return bytesWritten;
	}
	else
	{
		auto bytesWritten = ::writev(fd(), buffs.data()->iovecPtr(), buffs.size());
		if(bytesWritten == -1)
		{
			log.error("error writing {} buffers", buffs.size());
		}
		return bytesWritten;
	}
}

bool PosixIO::truncate(off_t offset)
{
	log.info("truncating at offset {}", offset);
	if(ftruncate(fd(), offset) == -1) [[unlikely]]
	{
		log.error("truncate failed");
		return false;
	}
	return true;
}

off_t PosixIO::seek(off_t offset, SeekMode mode)
{
	auto newPos = lseek(fd(), offset, (int)mode);
	if(newPos == -1) [[unlikely]]
	{
		log.error("seek to offset {} failed", offset);
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
		log.info("fd:{} is empty or a stream", fd());
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
		case IOAdvice::Sequential: return POSIX_FADV_SEQUENTIAL;
		case IOAdvice::Random: return POSIX_FADV_RANDOM;
		case IOAdvice::WillNeed: return POSIX_FADV_WILLNEED;
	}
}
#endif

void PosixIO::advise([[maybe_unused]] off_t offset, [[maybe_unused]] size_t bytes, [[maybe_unused]] Advice advice)
{
	#ifdef __APPLE__
	if(advice == Advice::Sequential || advice == Advice::WillNeed)
		fcntl(fd(), F_RDAHEAD, 1);
	#else
		#if _XOPEN_SOURCE >= 600 || _POSIX_C_SOURCE >= 200112L
		int fAdv =  adviceToFAdv(advice);
		if(posix_fadvise(fd(), offset, bytes, fAdv) != 0)
		{
			log.error("fadvise for offset:{:X} with size:{} failed", offset, bytes);
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
			log.error("fcntl({}) failed:{}", fd(), strerror(errno));
		flags = 0;
	}
	bool isWritable = (flags & O_WRONLY) || (flags & O_RDWR);
	return mapRange(0, size(), isWritable ? IOMapFlags{.write = true} : IOMapFlags{});
}

IOBuffer PosixIO::mapRange(off_t start, size_t size, IOMapFlags mapFlags)
{
	int flags = MAP_SHARED;
	if(mapFlags.populatePages)
		flags |= MAP_POPULATE;
	int prot = PROT_READ;
	if(mapFlags.write)
		prot |= PROT_WRITE;
	void *data = mmap(nullptr, size, prot, flags, fd(), start);
	if(data == MAP_FAILED) [[unlikely]]
	{
		log.error("mmap ({}) fd:{} @ {} ({} bytes) failed", protectionFlagsString(prot), fd(), start, size);
		return {};
	}
	log.info("mapped ({}) fd:{} @ {} to {} ({} bytes)", protectionFlagsString(prot), fd(), start, data, size);
	return byteBufferFromMmap(data, size);
}

IOBuffer PosixIO::byteBufferFromMmap(void *data, size_t size)
{
	return
	{
		{(uint8_t*)data, size}, {.mappedFile = true},
		[](const uint8_t *ptr, size_t size)
		{
			log.info("unmapping:{} ({} bytes)", (void*)ptr, size);
			if(munmap((void*)ptr, size) == -1)
			{
				if constexpr(Config::DEBUG_BUILD)
					log.error("munmap({}, {}) error:{}", (void*)ptr, size, strerror(errno));
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
