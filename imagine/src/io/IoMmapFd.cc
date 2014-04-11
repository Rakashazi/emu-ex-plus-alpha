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

#define LOGTAG "IOMMapFD"
#include <imagine/logger/logger.h>
#include <imagine/util/fd-utils.h>
#include <sys/mman.h>
#include <imagine/util/system/pagesize.h>
#include <imagine/io/IoMmapFd.hh>

Io * IoMmapFd::open(int fd)
{
	off_t size = fd_size(fd);
	void *data = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
	if(data == MAP_FAILED)
		return 0;

	if(size > 4096 && madvise(data, size, MADV_SEQUENTIAL) != 0)
		logWarn("warning, madvise failed");

	IoMmapFd *inst = new IoMmapFd;
	if(inst == 0)
	{
		logErr("out of memory");
		return 0;
	}

	inst->init((const char*)data, size);
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

void IoMmapFd::advise(long offset, size_t len, Advice advice)
{
	void *srcAddr = (void*)((ptrsize)data + offset);
	void *pageSrcAddr = (void*)roundDownToPageSize((ptrsize)srcAddr);
	len += (ptrsize)srcAddr - (ptrsize)pageSrcAddr; // add extra length from rounding down to page size
	if(advice == ADVICE_SEQUENTIAL)
	{
		if(madvise(pageSrcAddr, len, MADV_SEQUENTIAL) != 0)
		{
			logMsg("advise sequential for offset 0x%X with size %u failed", (uint)offset, (uint)len);
		}
	}
	else if(advice == ADVICE_WILLNEED)
	{
		//logMsg("advising will need offset 0x%X @ page %p with size %u", (uint)offset, pageSrcAddr, (uint)len);
		if(madvise(pageSrcAddr, len, MADV_WILLNEED) != 0)
		{
			logMsg("advise will need for offset 0x%X with size %u failed", (uint)offset, (uint)len);
		}
	}
}
