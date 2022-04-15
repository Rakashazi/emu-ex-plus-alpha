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

static void applyAccessHint(PosixFileIO &io, IO::AccessHint access, bool isMapped)
{
	switch(access)
	{
		case IO::AccessHint::NORMAL:
		case IO::AccessHint::UNMAPPED: return;
		case IO::AccessHint::SEQUENTIAL: return io.advise(0, 0, IO::Advice::SEQUENTIAL);
		case IO::AccessHint::RANDOM: return io.advise(0, 0, IO::Advice::RANDOM);
		case IO::AccessHint::ALL:
			if(!isMapped || (isMapped && !hasMmapPopulateFlag))
				return io.advise(0, 0, IO::Advice::WILLNEED);
	}
}

PosixFileIO::PosixFileIO(UniqueFileDescriptor fd_, IO::AccessHint access, IO::OpenFlags openFlags):
	ioImpl{std::in_place_type<PosixIO>, std::move(fd_)}
{
	tryMmap(access, openFlags);
}

PosixFileIO::PosixFileIO(UniqueFileDescriptor fd, IO::OpenFlags openFlags):
	PosixFileIO{std::move(fd), IO::AccessHint::NORMAL, openFlags} {}

PosixFileIO::PosixFileIO(IG::CStringView path, IO::AccessHint access, IO::OpenFlags openFlags):
	ioImpl{std::in_place_type<PosixIO>, path, openFlags}
{
	tryMmap(access, openFlags);
}

PosixFileIO::PosixFileIO(IG::CStringView path, IO::OpenFlags openFlags):
	PosixFileIO{path, IO::AccessHint::NORMAL, openFlags} {}

void PosixFileIO::tryMmap(IO::AccessHint access, IO::OpenFlags openFlags)
{
	assumeExpr(std::holds_alternative<PosixIO>(ioImpl));
	auto &io = *std::get_if<PosixIO>(&ioImpl);
	// try to open as memory map only if read-only
	if(openFlags & IO::WRITE_BIT || access == IO::AccessHint::UNMAPPED || !io)
		return;
	size_t size = io.size();
	if(!size) [[unlikely]]
		return;
	PosixIO::MapFlags flags = access == IO::AccessHint::ALL ? PosixIO::MAP_POPULATE_PAGES : 0;
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

static IO& getIO(std::variant<PosixIO, MapIO> &ioImpl)
{
	return visit([](auto &io) -> IO& { return io; }, ioImpl);
}

PosixFileIO::operator IO*() { return &getIO(ioImpl); }

PosixFileIO::operator IO&() { return getIO(ioImpl); }

PosixFileIO::operator GenericIO()
{
	return visit([](auto &&io) { return GenericIO{std::move(io)}; }, ioImpl);
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
	return visit([&](auto &io){ return io.map(); }, ioImpl);
}

ssize_t PosixFileIO::write(const void *buff, size_t bytes)
{
	return visit([&](auto &io){ return io.write(buff, bytes); }, ioImpl);
}

bool PosixFileIO::truncate(off_t offset)
{
	return visit([&](auto &io){ return io.truncate(offset); }, ioImpl);
}

off_t PosixFileIO::seek(off_t offset, IO::SeekMode mode)
{
	return visit([&](auto &io){ return io.seek(offset, mode); }, ioImpl);
}

void PosixFileIO::sync()
{
	visit([&](auto &io){ io.sync(); }, ioImpl);
}

size_t PosixFileIO::size()
{
	return visit([&](auto &io){ return io.size(); }, ioImpl);
}

bool PosixFileIO::eof()
{
	return visit([&](auto &io){ return io.eof(); }, ioImpl);
}

void PosixFileIO::advise(off_t offset, size_t bytes, IO::Advice advice)
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

}
