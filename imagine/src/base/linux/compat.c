#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/uio.h>

char __libc_single_threaded = 0;

unsigned long __isoc23_strtoul(const char* str, char** str_end, int base)
{
	return strtol(str, str_end, base);
}

char *secure_getenv(const char *name)
{
	return getenv(name);
}

void *reallocarray(void *optr, size_t nmemb, size_t elem_size)
{
	size_t bytes;
	if(__builtin_mul_overflow (nmemb, elem_size, &bytes))
	{
		errno = ENOMEM;
		return 0;
	}
	return realloc (optr, bytes);
}

ssize_t pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
	return syscall(__NR_pwritev, fd, iov, iovcnt, offset);
}

pid_t gettid()
{
	return syscall(__NR_gettid);
}
