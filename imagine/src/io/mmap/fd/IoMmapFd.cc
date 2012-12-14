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

#define thisModuleName "io:mmap:fd"
#include <logger/interface.h>
#include <util/fd-utils.h>
#include <sys/mman.h>

#include "IoMmapFd.hh"

Io * IoMmapFd::open(int fd)
{
	off_t size = fd_size(fd);
	void *data = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
	if(data == MAP_FAILED)
		return 0;

	if(madvise(data, size, MADV_SEQUENTIAL) != 0)
		logWarn("warning, madvise failed");

	IoMmapFd *inst = new IoMmapFd;
	if(inst == 0)
	{
		logErr("out of memory");
		return 0;
	}

	inst->init((const uchar*)data, size);
	return inst;
}

void IoMmapFd::close()
{
	if(data)
	{
		logMsg("unmapping %p", data);
		munmap((void*)data, iSize);
		data = nullptr;
	}
}
