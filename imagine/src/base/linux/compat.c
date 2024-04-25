#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <sys/random.h>

char __libc_single_threaded = 0;

extern int vsscanf(const char *__restrict __s, const char *__restrict __format, __gnuc_va_list __arg) __attribute__ ((__nothrow__ , __leaf__));

int __isoc23_sscanf(const char *__restrict s, const char *__restrict format, ...)
{
	va_list args;
	va_start(args, format);
	int ret = vsscanf(s, format, args);
	va_end(args);
	return ret;
}

struct _IO_FILE;
typedef struct _IO_FILE FILE;
extern int vfscanf(FILE *__restrict __s, const char *__restrict __format, __gnuc_va_list __arg);

int __isoc23_fscanf(FILE *__restrict stream, const char *__restrict format, ...)
{
	va_list args;
	va_start(args, format);
	int ret = vfscanf(stream, format, args);
	va_end(args);
	return ret;
}

extern long int strtol(const char *__restrict __nptr, char **__restrict __endptr, int __base) __attribute__ ((__nothrow__ , __leaf__));

unsigned long __isoc23_strtoul(const char*__restrict str, char**__restrict str_end, int base)
{
	return strtol(str, str_end, base);
}

unsigned long long int __isoc23_strtoull(const char*__restrict str, char**__restrict str_end, int base)
{
	return strtol(str, str_end, base);
}

long int __isoc23_strtol(const char*__restrict str, char**__restrict str_end, int base)
{
	return strtol(str, str_end, base);
}

long long int __isoc23_strtoll(const char*__restrict str, char**__restrict str_end, int base)
{
	return strtol(str, str_end, base);
}

#include <stdlib.h> // include later to avoid strtol renames

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

int getentropy(void *buffer_, size_t length)
{
	uint8_t *buffer = (uint8_t*)buffer_;
	for(size_t i = 0; i < length; i++)
	{
		buffer[i] = rand();
	}
	return length;
}

uint32_t arc4random() { return rand(); }
