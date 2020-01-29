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

#ifdef NDEBUG
Pipe::Pipe(uint32_t preferredSize)
#else
Pipe::Pipe(const char *debugLabel, uint32_t preferredSize): debugLabel{debugLabel ? debugLabel : "unnamed"}
#endif
{
	int res = pipe(msgPipe.data());
	assert(res == 0);
	logMsg("opened fds:%d,%d (%s)", msgPipe[0], msgPipe[1], label());
	if(preferredSize)
	{
		setPreferredSize(preferredSize);
	}
}

Pipe::~Pipe()
{
	deinit();
}

void Pipe::deinit()
{
	if(msgPipe[0] != -1)
	{
		if(fdSrc.hasEventLoop())
		{
			removeFromEventLoop();
		}
		close(msgPipe[0]);
		close(msgPipe[1]);
		logMsg("closed fds:%d,%d (%s)", msgPipe[0], msgPipe[1], label());
	}
}

Pipe::Pipe(Pipe &&o)
{
	*this = std::move(o);
}

Pipe &Pipe::operator=(Pipe &&o)
{
	deinit();
	msgPipe = std::exchange(o.msgPipe, {-1, -1});
	del = o.del;
	fdSrc = std::move(o.fdSrc);
	fdSrc.setCallback([this](int fd, int events){ return this->del(*this); });
	#ifndef NDEBUG
	debugLabel = o.debugLabel;
	#endif
	return *this;
}

void Pipe::addToEventLoop(EventLoop loop, Delegate del)
{
	if(msgPipe[0] == -1)
	{
		logMsg("can't add null pipe to event loop");
		return;
	}
	this->del = del;
	if(!loop)
		loop = EventLoop::forThread();
	fdSrc = {label(), msgPipe[0], loop,
		[this](int fd, int events)
		{
			return this->del(*this);
		}};
}

void Pipe::removeFromEventLoop()
{
	if(msgPipe[0] != -1)
	{
		fdSrc.removeFromEventLoop();
	}
}

bool Pipe::write(const void *data, size_t size)
{
	if(::write(msgPipe[1], data, size) != (int)size)
	{
		logErr("unable to write message to pipe: %s (%s)", strerror(errno), label());
		return false;
	}
	return true;
}

bool Pipe::read(void *data, size_t size)
{
	if(::read(msgPipe[0], data, size) == -1)
	{
		if(Config::DEBUG_BUILD && errno != EAGAIN)
		{
			logErr("error reading from pipe (%s)", label());
		}
		return false;
	}
	return true;
}

bool Pipe::hasData()
{
	return fd_bytesReadable(msgPipe[0]);
}

void Pipe::setPreferredSize(int size)
{
	#ifdef __linux__
	fcntl(msgPipe[1], F_SETPIPE_SZ, size);
	logDMsg("set fds:%d,%d size to:%d", msgPipe[0], msgPipe[1], size);
	#endif
}

void Pipe::setReadNonBlocking(bool on)
{
	fd_setNonblock(msgPipe[0], on);
}

bool Pipe::isReadNonBlocking() const
{
	return fd_getNonblock(msgPipe[0]);
}

const char *Pipe::label() const
{
	#ifdef NDEBUG
	return nullptr;
	#else
	return debugLabel;
	#endif
}

Pipe::operator bool() const
{
	return msgPipe[0] != -1;
}

}
