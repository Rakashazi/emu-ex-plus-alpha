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

#define LOGTAG "IOFD"
#include <engine-globals.h>
#include <logger/interface.h>
#include <util/bits.h>
#include <util/fd-utils.h>

#ifdef CONFIG_IO_MMAP_FD
#include <io/mmap/fd/IoMmapFd.hh>
#endif

#include <io/fd/IoFd.hh>
#include <io/utils.hh>

#ifdef CONFIG_FS_PS3 // need FS module for working directory support
#include <fs/ps3/FsPs3.hh>
#endif

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

using namespace IG;

static const bool preferMmapIO = 1;
static const mode_t defaultOpenMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

static int openFile(const char *location, int flags, mode_t mode)
{
	int fd = open(location, flags, mode);

	if(fd == -1)
	{
		//logErr("error in open()");
		return fd;
	}

	return fd;
}

Io* IoFd::open(const char *location, uint mode, CallResult *errorOut)
{
	// try to verify that mode isn't a corrupt value
	assert(mode < bit(IO_OPEN_USED_BITS+1));
	
	int flags = 0;
	
	if(mode & IO_OPEN_WRITE)
	{
		flags |= O_RDWR;
	}
	else
	{
		flags |= O_RDONLY;
	}

	#ifdef CONFIG_FS_PS3
	char aPath[1024];
	FsPs3::makePathAbs(location, aPath, sizeof(aPath));
	location = aPath;
	logMsg("converted path to %s", aPath);
	#endif

	int fd;
	if((fd = openFile(location, flags, 0)) == -1)
	{
		logMsg("error opening file (%s) @ %s", mode & IO_OPEN_WRITE ? "rw" : "r", location);
		if(errorOut)
		{
			switch(errno)
			{
				bcase EACCES: *errorOut = PERMISSION_DENIED;
				bcase ENOENT: *errorOut = NOT_FOUND;
				bdefault: *errorOut = IO_ERROR;
			}
		}
		return nullptr;
	}
	
	logMsg("opened file (%s) @ %s", mode & IO_OPEN_WRITE ? "rw" : "r", location);

	// if the file is read-only try to use IoMmapFd instead
	#ifdef CONFIG_IO_MMAP_FD
	if(preferMmapIO && !(mode & IO_OPEN_WRITE))
	{
		Io *mmapFile = IoMmapFd::open(fd);
		if(mmapFile)
		{
			//logMsg("switched to mmap mode");
			::close(fd);
			return mmapFile;
		}
	}
	#endif

	#ifdef CONFIG_BASE_IOS
	fcntl(fd, F_RDAHEAD, 1);
	#else
		#if _XOPEN_SOURCE >= 600 || _POSIX_C_SOURCE >= 200112L
		posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
		#endif
	#endif

	IoFd *inst = new IoFd;
	if(!inst)
	{
		logErr("out of memory");
		::close(fd);
		if(errorOut)
			*errorOut = OUT_OF_MEMORY;
		return nullptr;
	}

	inst->fd = fd;

	return inst;
}

Io* IoFd::create(const char *location, uint mode, CallResult *errorOut)
{
	// try to verify that mode isn't a corrupt value
	assert(mode < bit(IO_CREATE_USED_BITS+1));
	
	#ifdef CONFIG_FS_PS3
	char aPath[1024];
	FsPs3::makePathAbs(location, aPath, sizeof(aPath));
	location = aPath;
	logMsg("converted path to %s", aPath);
	#endif

	IoFd *inst = new IoFd;
	if(!inst)
	{
		logErr("out of memory");
		if(errorOut)
			*errorOut = OUT_OF_MEMORY;
		return nullptr;
	}

	int fd;
	if(mode & IO_CREATE_KEEP)
	{
		if((fd = openFile(location, O_RDWR, defaultOpenMode)) != -1)
		{
			logMsg("opened existing file @ %s, fd %d", location, fd);
			inst->fd = fd;
			return inst;
		}
	}

	logMsg("creating file @ %s",  location);
	if((fd = openFile(location, O_RDWR | O_CREAT | O_TRUNC, defaultOpenMode)) == -1)
	{
		logMsg("failed creating file");
		delete inst;
		if(errorOut)
		{
			switch(errno)
			{
				bcase EACCES: *errorOut = PERMISSION_DENIED;
				bcase ENOSPC: *errorOut = NO_FREE_ENTRIES;
				bdefault: *errorOut = IO_ERROR;
			}
		}
		return 0;
	}

	//logMsg("fd %d", fd);
	inst->fd = fd;
	return inst;
}

void IoFd::close()
{
	if(fd >= 0)
	{
		::close(fd);
		logMsg("closed fd %d", fd);
		fd = -1;
	}
}

void IoFd::truncate(ulong offset)
{
	logMsg("truncating at offset %u", (uint)offset);
	if(ftruncate(fd, offset) != 0)
	{
		logErr("truncate failed");
	}
}

void IoFd::sync()
{
	fsync(fd);
}

//TODO - add more error checks

ssize_t IoFd::readUpTo(void* buffer, size_t numBytes)
{
	size_t bytesRead = ::read(fd, buffer, numBytes);
	//logMsg("read %d bytes out of %d requested from file @ %p", (int)bytesRead, (int)numBytes, this);

	return(bytesRead);
}

size_t IoFd::fwrite(const void* ptr, size_t size, size_t nmemb)
{
	size_t elemWritten = 0;
	auto cPtr = (const char*)ptr;
	iterateTimes(nmemb, i)
	{
		size_t toWrite = size;
		while(toWrite > 0)
		{
			size_t written = write(fd, cPtr, toWrite);
			if(written <= 0) // error writing, return all elements written to this point
			{
				logErr("error writing %d bytes, in elem %d of %d", (int)toWrite, (int)elemWritten, (int)nmemb);
				return elemWritten;
			}
			toWrite -= written;
			cPtr += written;
			//logDMsg("wrote %d bytes", (int)written);
		}
		elemWritten++;
	}
	
	assert(elemWritten == nmemb);
	return elemWritten;
}

CallResult IoFd::tell(ulong &offset)
{
	off_t pos = lseek(fd, 0, SEEK_CUR);
	if(pos >= 0)
	{
		offset = pos;
		return OK;
	}
	else
		return IO_ERROR;
}

static int fdSeekMode(int mode)
{
	switch(mode)
	{
		case IO_SEEK_ABS: return SEEK_SET;
		case IO_SEEK_ABS_END: return SEEK_END;
		case IO_SEEK_REL: return SEEK_CUR;
		default:
			bug_branch("%d", mode);
			return 0;
	}
}

CallResult IoFd::seek(long offset, uint mode)
{
	if(!isSeekModeValid(mode))
	{
		bug_exit("invalid seek mode: %u", mode);
		return INVALID_PARAMETER;
	}
	if(lseek(fd, offset, fdSeekMode(mode)) >= 0)
	{
		return OK;
	}
	else
		return IO_ERROR;
}

ulong IoFd::size()
{
	return fd_size(fd);
}

int IoFd::eof()
{
	//logMsg("called eof");
	return IoFd::size() == (uint)ftell() ? 1 : 0;
}

void IoFd::advise(long offset, size_t len, Advice advice)
{
	#ifdef CONFIG_BASE_IOS
	if(advice == ADVICE_SEQUENTIAL)
		fcntl(fd, F_RDAHEAD, 1);
	#else
		#if _XOPEN_SOURCE >= 600 || _POSIX_C_SOURCE >= 200112L
		if(advice == ADVICE_SEQUENTIAL)
		{
			posix_fadvise(fd, offset, len, POSIX_FADV_SEQUENTIAL);
		}
		else if(advice == ADVICE_WILLNEED)
		{
			if(posix_fadvise(fd, offset, len, POSIX_FADV_WILLNEED) != 0)
			{
				logMsg("advise will need offset 0x%X with size %u failed", (uint)offset, (uint)len);
			}
		}
		#endif
	#endif
}
