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

#define LOGTAG "PosixFileIO"
#include <imagine/io/FileIO.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/fd-utils.h>
#include <imagine/util/variant.hh>
#include "IOUtils.hh"
#include <cstring>

namespace IG
{

#if defined __linux__
constexpr bool hasMmapPopulateFlag = true;
#else
constexpr bool hasMmapPopulateFlag = false;
#endif

template class IOUtils<PosixFileIO>;

static void applyAccessHint(PosixFileIO &io, IOAccessHint access, bool isMapped)
{
	switch(access)
	{
		case IOAccessHint::NORMAL: return;
		case IOAccessHint::SEQUENTIAL: return io.advise(0, 0, IOAdvice::SEQUENTIAL);
		case IOAccessHint::RANDOM: return io.advise(0, 0, IOAdvice::RANDOM);
		case IOAccessHint::ALL:
			if(!isMapped || (isMapped && !hasMmapPopulateFlag))
				return io.advise(0, 0, IOAdvice::WILLNEED);
	}
}

PosixFileIO::PosixFileIO(UniqueFileDescriptor fd_, IOAccessHint access, FileOpenFlags openFlags):
	ioImpl{std::in_place_type<PosixIO>, std::move(fd_)}
{
	tryMmap(access, openFlags);
}

PosixFileIO::PosixFileIO(UniqueFileDescriptor fd, FileOpenFlags openFlags):
	PosixFileIO{std::move(fd), IOAccessHint::NORMAL, openFlags} {}

PosixFileIO::PosixFileIO(IG::CStringView path, IOAccessHint access, FileOpenFlags openFlags):
	ioImpl{std::in_place_type<PosixIO>, path, openFlags}
{
	tryMmap(access, openFlags);
}

PosixFileIO::PosixFileIO(IG::CStringView path, FileOpenFlags openFlags):
	PosixFileIO{path, IOAccessHint::NORMAL, openFlags} {}

void PosixFileIO::tryMmap(IOAccessHint access, FileOpenFlags openFlags)
{
	assumeExpr(std::holds_alternative<PosixIO>(ioImpl));
	auto &io = *std::get_if<PosixIO>(&ioImpl);
	// try to open as memory map only if read-only
	if(openFlags & FILE_WRITE_BIT || !io)
		return;
	size_t size = io.size();
	if(!size) [[unlikely]]
		return;
	PosixIO::MapFlags flags = access == IOAccessHint::ALL ? PosixIO::MAP_POPULATE_PAGES : 0;
	MapIO mappedFile{io.mapRange(0, size, flags)};
	if(mappedFile)
	{
		ioImpl = std::move(mappedFile);
		applyAccessHint(*this, access, true);
	}
	else
	{
		applyAccessHint(*this, access, false);
	}
}

ssize_t PosixFileIO::read(void *buff, size_t bytes)
{
	return visit([&](auto &io){ return io.read(buff, bytes); }, ioImpl);
}

ssize_t PosixFileIO::readAtPos(void *buff, size_t bytes, off_t offset)
{
	return visit([&](auto &io){ return io.readAtPos(buff, bytes, offset); }, ioImpl);
}

std::span<uint8_t> PosixFileIO::map()
{
	return visit([&](auto &io)
	{
		if constexpr(requires {io.map();})
			return io.map();
		else
			return std::span<uint8_t>{};
	}, ioImpl);
}

ssize_t PosixFileIO::write(const void *buff, size_t bytes)
{
	return visit([&](auto &io){ return io.write(buff, bytes); }, ioImpl);
}

bool PosixFileIO::truncate(off_t offset)
{
	return visit([&](auto &io)
	{
		if constexpr(requires {io.truncate(offset);})
			return io.truncate(offset);
		else
			return false;
	}, ioImpl);
}

off_t PosixFileIO::seek(off_t offset, IOSeekMode mode)
{
	return visit([&](auto &io){ return io.seek(offset, mode); }, ioImpl);
}

void PosixFileIO::sync()
{
	visit([&](auto &io)
	{
		if constexpr(requires {io.sync();})
			io.sync();
	}, ioImpl);
}

size_t PosixFileIO::size()
{
	return visit([&](auto &io){ return io.size(); }, ioImpl);
}

bool PosixFileIO::eof()
{
	return visit([&](auto &io){ return io.eof(); }, ioImpl);
}

void PosixFileIO::advise(off_t offset, size_t bytes, IOAdvice advice)
{
	visit([&](auto &io){ io.advise(offset, bytes, advice); }, ioImpl);
}

PosixFileIO::operator bool() const
{
	return visit([&](auto &io){ return (bool)io; }, ioImpl);
}

IOBuffer PosixFileIO::releaseBuffer()
{
	return visit([&](auto &io){ return io.releaseBuffer(); }, ioImpl);
}

UniqueFileDescriptor PosixFileIO::releaseFd()
{
	if(auto ioPtr = std::get_if<PosixIO>(&ioImpl);
		ioPtr)
	{
		return ioPtr->releaseFd();
	}
	logWarn("trying to release fd of mapped IO");
	return {};
}

PosixFileIO::operator IO()
{
	return visit([](auto &&io) { return IO{std::move(io)}; }, ioImpl);
}

}
