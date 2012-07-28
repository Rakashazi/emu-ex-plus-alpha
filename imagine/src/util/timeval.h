#pragma once
#include <sys/time.h>
#include <assert.h>
#include <math.h>

static const long int uSecPerSec = 1000000;
static const long int mSecPerSec = 1000;

static bool timeval_isValid(struct timeval x)
{
	return x.tv_usec >= 0 && x.tv_usec < uSecPerSec;
}

static int timeval_compare(struct timeval x, struct timeval y)
{
	if(x.tv_sec < y.tv_sec)
		return -1;
	else if(x.tv_sec > y.tv_sec)
		return 1;
	else if(x.tv_usec < y.tv_usec)
		return -1;
	else if(x.tv_usec > y.tv_usec)
		return 1;
	return 0;
}

static struct timeval timeval_subtract(struct timeval x, struct timeval rhs)
{
	assert(timeval_isValid(x) && timeval_isValid(rhs));
	assert(timeval_compare(x, rhs) >= 0);
	struct timeval result = x;
	result.tv_sec -= rhs.tv_sec;
	result.tv_usec -= rhs.tv_usec;
	if(result.tv_usec < 0)
	{
		result.tv_usec += uSecPerSec;
		result.tv_sec--;
	}
	return result;
}

static struct timeval timeval_add(struct timeval x, struct timeval rhs)
{
	assert(timeval_isValid(x) && timeval_isValid(rhs));
	struct timeval result = x;
	result.tv_sec += rhs.tv_sec;
	result.tv_usec += rhs.tv_usec;
	if(result.tv_usec >= uSecPerSec)
	{
		result.tv_usec -= uSecPerSec;
		result.tv_sec++;
	}
	return result;
}

static long int timeval_divUsecs(struct timeval x, long int usecs)
{
	return (((int64)x.tv_sec * uSecPerSec) / usecs) + (x.tv_usec / usecs);
}

static double timeval_toDouble(struct timeval t)
{
	return (double)t.tv_sec + (double)t.tv_usec/(double)1.0e6;
}
