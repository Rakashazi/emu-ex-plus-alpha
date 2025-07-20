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

#include <imagine/config/defs.hh>
#include <imagine/logger/logger.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <sys/ioctl.h>
#include <sys/stat.h>

namespace IG
{
constexpr SystemLogger log{"fdUtils"};
}

CLINK ssize_t fd_writeAll(int filedes, const void *buffer, size_t size)
{
	size_t written = 0;
	while(size != written)
	{
		ssize_t ret = write(filedes, ((char*)buffer)+written, size-written);
		if(ret == -1)
			return -1;
		written += ret;
	}
	return (ssize_t)size;
}

CLINK off_t fd_size(int fd)
{
	struct stat stats;
	if(fstat(fd, &stats) == -1)
	{
		if(Config::DEBUG_BUILD)
			IG::log.error("fstat({}) failed:{}", fd, strerror(errno));
		return 0;
	}
	return stats.st_size;
}

CLINK const char* fd_seekModeToStr(int mode)
{
	switch(mode)
	{
		case SEEK_SET : return "SET";
		case SEEK_END : return "END";
		case SEEK_CUR : return "CUR";
	}
	return {};
}

static int getFDFlags(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if(flags == -1)
	{
		IG::log.error("fcntl({}) failed getting flags", fd);
	}
	return flags;
}

CLINK void fd_setNonblock(int fd, bool on)
{
	int flags = getFDFlags(fd);
	flags = on ? flags | O_NONBLOCK : flags & ~O_NONBLOCK;
	if(int err = fcntl(fd, F_SETFL, flags); err)
	{
		IG::log.error("fcntl({}) failed {} O_NONBLOCK", fd, on ? "setting" : "clearing");
	}
}

CLINK bool fd_getNonblock(int fd)
{
	return getFDFlags(fd) & O_NONBLOCK;
}

CLINK int fd_bytesReadable(int fd)
{
	int bytes = 0;
	if(ioctl(fd, FIONREAD, (char*)&bytes) < 0)
	{
		IG::log.error("failed ioctl FIONREAD");
		return 0;
	}
	assert(bytes >= 0);
	return bytes;
}

CLINK int fd_isValid(int fd)
{
	return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}
