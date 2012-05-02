#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <util/number.h>

#ifndef CONFIG_FS_PS3
#include <sys/ioctl.h>
#endif

static ssize_t fd_writeAll(int filedes, const void *buffer, size_t size)
{
	size_t written = 0;
	while(size != written)
	{
		ssize_t ret = write(filedes, ((uchar*)buffer)+written, size-written);
		if(ret == -1)
			return -1;
		written += ret;
	}
	return size;
}

static off_t fd_size(int fd)
{
	off_t savedPos = lseek(fd, 0, SEEK_CUR);
	off_t size = lseek(fd, 0, SEEK_END);
	lseek(fd, savedPos, SEEK_SET);
	return size;
}

static const char* fd_seekModeToStr(int mode)
{
	switch(mode)
	{
		case SEEK_SET : return "SET";
		case SEEK_END : return "END";
		case SEEK_CUR : return "CUR";
	}
	return NULL;
}

#ifndef CONFIG_FS_PS3
static void fd_setNonblock(int fd, bool on)
{
	int flags;
	flags = fcntl(fd,F_GETFL,0);
	assert(flags != -1);
	if(on)
	{
		if(!(flags & O_NONBLOCK))
			logMsg("set O_NONBLOCK on fd %d", fd);
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	}
	else
	{
		if(flags & O_NONBLOCK)
			logMsg("unset O_NONBLOCK on fd %d", fd);
		fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
	}
}

static size_t fd_bytesReadable(int fd)
{
	size_t bytes;
	if(ioctl(fd, FIONREAD, (char*)&bytes) < 0)
	{
		logErr("failed ioctl FIONREAD");
		return 0;
	}
	return bytes;
}

static void fd_skipAvailableData(int fd)
{
	size_t bytesToSkip = 0;
	if(ioctl(fd, FIONREAD, (char*)&bytesToSkip) < 0)
	{
		logErr("failed ioctl FIONREAD");
		return;
	}
	logMsg("skipping %d bytes", (int)bytesToSkip);
	char dummy[8];
	while(bytesToSkip)
	{
		int ret = read(fd, dummy, IG::min(bytesToSkip, sizeof dummy));
		if(ret < 0)
		{
			logMsg("error in read()");
			return;
		}
		bytesToSkip -= ret;
	}
}
#endif
