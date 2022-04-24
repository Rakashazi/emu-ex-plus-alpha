#pragma once

#include <imagine/util/utility.h>
#include <unistd.h>

CLINK ssize_t fd_writeAll(int filedes, const void *buffer, size_t size);
CLINK off_t fd_size(int fd);
CLINK const char* fd_seekModeToStr(int mode);
CLINK void fd_setNonblock(int fd, bool on);
CLINK bool fd_getNonblock(int fd);
CLINK int fd_bytesReadable(int fd);
CLINK int fd_isValid(int fd);
