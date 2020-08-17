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

#define LOGTAG "Pipe"
#include <imagine/base/Pipe.hh>
#include <imagine/util/fd-utils.h>
#include <imagine/logger/logger.h>
#include <cstring>
#include <fcntl.h>
#include <errno.h>

namespace Base
{

static std::array<PosixIO, 2> makePipe()
{
	std::array<int, 2> fd{-1, -1};
	#ifdef __linux__
	int res = pipe2(fd.data(), O_CLOEXEC);
	#else
	int res = pipe(fd.data());
	#endif
	if(res == -1)
	{
		logErr("error creating pipe");
	}
	return {fd[0], fd[1]};
}

Pipe::Pipe(const char *debugLabel, uint32_t preferredSize):
	debugLabel{debugLabel ? debugLabel : "unnamed"},
	io{makePipe()},
	fdSrc{label(), io[0].fd()}
{
	logMsg("opened fds:%d,%d (%s)", io[0].fd(), io[1].fd(), label());
	if(preferredSize)
	{
		setPreferredSize(preferredSize);
	}
}

Pipe::~Pipe()
{
	deinit();
}

Pipe::Pipe(Pipe &&o)
{
	*this = std::move(o);
}

Pipe &Pipe::operator=(Pipe &&o)
{
	deinit();
	io = std::exchange(o.io, {-1, -1});
	fdSrc = std::move(o.fdSrc);
	debugLabel = o.debugLabel;
	return *this;
}

PosixIO &Pipe::source()
{
	return io[0];
}

PosixIO &Pipe::sink()
{
	return io[1];
}

void Pipe::attach(EventLoop loop, PollEventDelegate callback)
{
	if(io[0].fd() == -1)
	{
		logMsg("can't add null pipe to event loop");
		return;
	}
	fdSrc.attach(loop, callback);
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
	fdSrc.dispatchEvents(POLLEV_IN);
}

void Pipe::setPreferredSize(int size)
{
	#ifdef __linux__
	fcntl(io[1].fd(), F_SETPIPE_SZ, size);
	logDMsg("set size:%d (%s)", size, label());
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

void Pipe::deinit()
{
	logMsg("closing %s", label());
	fdSrc.detach();
}

const char *Pipe::label() const
{
	return debugLabel;
}

}
