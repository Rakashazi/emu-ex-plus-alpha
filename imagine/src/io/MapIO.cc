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

ssize_t MapIO::read(void *buff, size_t bytesToRead, std::optional<off_t> offset)
{
	return copyBuffer(buff, bytesToRead, offset);
}

ssize_t MapIO::write(const void *buff, size_t bytesToWrite, std::optional<off_t> offset)
{
	return copyBuffer(buff, bytesToWrite, offset);
}

off_t MapIO::seek(off_t offset, IOSeekMode mode)
{
	size_t newPos = transformOffsetToAbsolute(mode, offset, 0, off_t(size()), off_t(currPos));
	if(newPos > size())
	{
		logErr("illegal seek position");
		return -1;
	}
	currPos = newPos;
	return newPos;
}

void MapIO::sync()
{
	msync(data(), size(), MS_SYNC);
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
	if(!bytes)
		bytes = size();
	auto span = subSpan(offset, bytes);
	if(!span.data())
		return;
	void *pageSrcAddr = roundDownToPageSize(span.data());
	bytes = span.size_bytes() + (uintptr_t(span.data()) - uintptr_t(pageSrcAddr)); // add extra bytes from rounding down to page size
	if(madvise(pageSrcAddr, bytes, adviceToMAdv(advice)) != 0)
	{
		logWarn("madvise(%p, %zu, %s) failed:%s",
			pageSrcAddr, bytes, asString(advice), Config::DEBUG_BUILD ? strerror(errno) : "");
	}
}
#endif

std::span<uint8_t> MapIO::subSpan(off_t offset, size_t maxBytes) const
{
	if(offset > off_t(size())) [[unlikely]]
	{
		logErr("offset%zd is larger than size:%zu", ssize_t(offset), size());
		return {};
	}
	auto bytes = std::min(maxBytes, size_t(size() - offset));
	//if(bytes != maxBytes) logDMsg("reduced size of span:%zu to %zu", maxBytes, bytes);
	return {data() + offset, bytes};
}

ssize_t MapIO::copyBuffer(auto *buff, size_t bytes, std::optional<off_t> offset)
{
	if(!data()) [[unlikely]]
		return -1;
	auto span = subSpan(offset ? *offset : currPos, bytes);
	if(!span.data())
		return 0;
	if constexpr(std::is_const_v<std::remove_pointer_t<decltype(buff)>>)
		memcpy(span.data(), buff, span.size_bytes()); // write from provided buffer
	else
		memcpy(buff, span.data(), span.size_bytes()); // read to provided buffer
	if(!offset)
	{
		currPos += span.size_bytes();
		assert(currPos <= size());
	}
	return span.size_bytes();
}

}
