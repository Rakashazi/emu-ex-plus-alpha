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

#define LOGTAG "MapIO"
#include <imagine/io/MapIO.hh>
#include <imagine/config/defs.hh>
#include <imagine/logger/logger.h>
#include "utils.hh"
#include "IOUtils.hh"
#include <cerrno>
#include <cstring>
#if defined __linux__ || defined __APPLE__
#include <sys/mman.h>
#include <imagine/vmem/pageSize.hh>
#endif

namespace IG
{

template class IOUtils<MapIO>;

MapIO::MapIO(IOBuffer buff):
	currPos{buff.data()},
	buff{std::move(buff)} {}

ssize_t MapIO::read(void *buff, size_t bytes)
{
	assert(currPos >= data());
	auto bytesRead = readAtAddr(buff, bytes, currPos);
	if(bytesRead > 0)
	{
		currPos += bytesRead;
	}
	return bytesRead;
}

ssize_t MapIO::readAtPos(void *buff, size_t bytes, off_t offset)
{
	return readAtAddr(buff, bytes, data() + offset);
}

std::span<uint8_t> MapIO::map()
{
	return {data(), size()};
}

ssize_t MapIO::write(const void *buff, size_t bytes)
{
	// TODO
	return -1;
}

off_t MapIO::seek(off_t offset, IOSeekMode mode)
{
	auto newPos = transformOffsetToAbsolute(mode, offset, data(), dataEnd(), currPos);
	if(newPos < data() || newPos > dataEnd())
	{
		logErr("illegal seek position");
		return -1;
	}
	currPos = newPos;
	return currPos - data();
}

size_t MapIO::size()
{
	return buff.size();
}

bool MapIO::eof()
{
	return currPos >= dataEnd();
}

MapIO::operator bool() const
{
	return data();
}

static int adviceToMAdv(IOAdvice advice)
{
	switch(advice)
	{
		default: return MADV_NORMAL;
		case IOAdvice::Sequential: return MADV_SEQUENTIAL;
		case IOAdvice::Random: return MADV_RANDOM;
		case IOAdvice::WillNeed: return MADV_WILLNEED;
	}
}

#if defined __linux__ || defined __APPLE__
void MapIO::advise(off_t offset, size_t bytes, Advice advice)
{
	assert(offset >= 0);
	if(!bytes)
		bytes = size();
	if(bytes > size() - offset) // clip to end of data
	{
		bytes = size() - offset;
	}
	auto srcAddr = data() + offset;
	void *pageSrcAddr = roundDownToPageSize(srcAddr);
	bytes += (uintptr_t)srcAddr - (uintptr_t)pageSrcAddr; // add extra bytes from rounding down to page size
	int mAdv = adviceToMAdv(advice);
	if(madvise(pageSrcAddr, bytes, mAdv) != 0 && Config::DEBUG_BUILD)
	{
		logWarn("madvise(%p, %zu, %s) failed:%s", pageSrcAddr, bytes, asString(advice), strerror(errno));
	}
	else
	{
		logDMsg("madvise(%p, %zu, %s)", pageSrcAddr, bytes, asString(advice));
	}
}
#endif

IOBuffer MapIO::releaseBuffer()
{
	logMsg("releasing buffer:%p (%zu bytes)", buff.data(), buff.size());
	return std::move(buff);
}

uint8_t *MapIO::data() const
{
	return buff.data();
}

uint8_t *MapIO::dataEnd() const
{
	return data() + buff.size();
}

ssize_t MapIO::readAtAddr(void* buff, size_t bytes, const uint8_t *addr)
{
	if(addr >= dataEnd())
	{
		if(!data()) [[unlikely]]
			return -1;
		else
			return 0;
	}

	size_t bytesToRead;
	const uint8_t *posToReadTo = addr + bytes;
	if(posToReadTo > dataEnd())
	{
		bytesToRead = dataEnd() - addr;
	}
	else bytesToRead = bytes;

	//logDMsg("reading %llu bytes at offset %llu", (unsigned long long)bytesToRead, (unsigned long long)(addr - data));
	memcpy(buff, addr, bytesToRead);

	return bytesToRead;
}

}
