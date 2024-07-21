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

#include <imagine/base/Pipe.hh>
#include <imagine/util/fd-utils.h>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>
#include <cstring>
#include <fcntl.h>
#include <cerrno>

namespace IG
{

constexpr SystemLogger log{"Pipe"};

static auto makePipe()
{
	std::array<int, 2> fd{-1, -1};
	#ifdef __linux__
	int res = pipe2(fd.data(), O_CLOEXEC);
	#else
	int res = pipe(fd.data());
	#endif
	if(res == -1)
	{
		log.error("error creating pipe");
	}
	return std::array<PosixIO, 2>{UniqueFileDescriptor{fd[0]}, UniqueFileDescriptor{fd[1]}};
}

Pipe::Pipe(const char *debugLabel, int preferredSize):
	io{makePipe()},
	fdSrc{io[0].fd(), {.debugLabel = debugLabel}, {}}
{
	log.info("opened fds:{},{} ({})", io[0].fd(), io[1].fd(), debugLabel);
	if(preferredSize)
	{
		setPreferredSize(preferredSize);
	}
}

PosixIO &Pipe::source()
{
	return io[0];
}

PosixIO &Pipe::sink()
{
	return io[1];
}

void Pipe::attach(EventLoop loop)
{
	if(io[0].fd() == -1)
	{
		log.info("can't add null pipe to event loop");
		return;
	}
	fdSrc.attach(loop);
}

void Pipe::detach()
{
	fdSrc.detach();
}

bool Pipe::hasData()
{
	return fd_bytesReadable(io[0].fd());
}

void Pipe::dispatchSourceEvents()
{
	fdSrc.dispatchEvents(pollEventInput);
}

void Pipe::setPreferredSize([[maybe_unused]] int size)
{
	#ifdef __linux__
	fcntl(io[1].fd(), F_SETPIPE_SZ, size);
	log.debug("set size:{} ({})", size, fdSrc.debugLabel());
	#endif
}

void Pipe::setReadNonBlocking(bool on)
{
	fd_setNonblock(io[0].fd(), on);
}

bool Pipe::isReadNonBlocking() const
{
	return fd_getNonblock(io[0].fd());
}

Pipe::operator bool() const
{
	return io[0].fd() != -1;
}

}
