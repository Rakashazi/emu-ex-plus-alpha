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

namespace Base
{

void Pipe::init(Delegate del)
{
	if(msgPipe[0] != -1)
	{
		logMsg("pipe already init");
		return;
	}
	int res = pipe(msgPipe);
	assert(res == 0);
	var_selfs(del);
	fdSrc.init(msgPipe[0],
		[this](int fd, int events)
		{
			assert(this->del);
			this->del(*this);
			return 1;
		});
	logMsg("init pipe with fd: %d %d", msgPipe[0], msgPipe[1]);
}

void Pipe::deinit()
{
	if(msgPipe[0] != -1)
	{
		fdSrc.deinit();
		close(msgPipe[0]);
		close(msgPipe[1]);
		logMsg("deinit pipe with fd: %d %d", msgPipe[0], msgPipe[1]);
		msgPipe[0] = -1;
		msgPipe[1] = -1;
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
	if(fd_bytesReadable(msgPipe[0]))
	{
		if(::read(msgPipe[0], data, size) == -1)
		{
			logErr("error reading from pipe");
			return false;
		}
		return true;
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

}
