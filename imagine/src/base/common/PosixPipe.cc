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
#include <cstring>

namespace Base
{

Pipe::Pipe()
{
	int res = pipe(msgPipe.data());
	assert(res == 0);
	logMsg("opened pipe fds:%d %d", msgPipe[0], msgPipe[1]);
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
		logMsg("closed pipe fds:%d %d", msgPipe[0], msgPipe[1]);
	}
}

Pipe::Pipe(Pipe &&o)
{
	moveObject(o);
}

Pipe &Pipe::operator=(Pipe &&o)
{
	deinit();
	moveObject(o);
	return *this;
}

void Pipe::moveObject(Pipe &o)
{
	msgPipe = o.msgPipe;
	o.fdSrc.setCallback([this](int fd, int events){ return this->del(*this); });
	fdSrc = std::move(o.fdSrc);
	del = o.del;
	o.msgPipe[0] = -1;
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
	fdSrc = {msgPipe[0], loop,
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

bool Pipe::write(const void *data, uint size)
{
	if(::write(msgPipe[1], data, size) != (int)size)
	{
		logErr("unable to write message to pipe: %s", strerror(errno));
		return false;
	}
	return true;
}

bool Pipe::read(void *data, uint size)
{
	if(::read(msgPipe[0], data, size) == -1)
	{
		logErr("error reading from pipe");
		return false;
	}
	return true;
}

bool Pipe::tryRead(void *data, uint size)
{
	if(hasData())
	{
		return read(data, size);
	}
	else
	{
		logWarn("trying to read with no bytes in pipe");
		return false;
	}
}

bool Pipe::hasData()
{
	return fd_bytesReadable(msgPipe[0]);
}

Pipe::operator bool() const
{
	return msgPipe[0] != -1;
}

}
