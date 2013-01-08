#pragma once
#include <sys/time.h>
#include <assert.h>

#define NSEC_PER_SEC 1000000000
#define USEC_PER_SEC 1000000

static bool timeval_isValid(struct timeval t)
{
	return t.tv_usec >= 0 && t.tv_usec < USEC_PER_SEC;
}

static int timeval_compare(struct timeval t1, struct timeval t2)
{
	if(t1.tv_sec != t2.tv_sec)
		return t1.tv_sec - t2.tv_sec;
	return t1.tv_usec - t2.tv_usec;
}

static struct timeval timeval_subtract(struct timeval t1, struct timeval t2)
{
	assert(timeval_isValid(t1) && timeval_isValid(t2));
	assert(timeval_compare(t1, t2) >= 0);
	auto result = t1;
	result.tv_sec -= t2.tv_sec;
	result.tv_usec -= t2.tv_usec;
	if(result.tv_usec < 0)
	{
		result.tv_usec += USEC_PER_SEC;
		result.tv_sec--;
	}
	return result;
}

static struct timeval timeval_add(struct timeval t1, struct timeval t2)
{
	assert(timeval_isValid(t1) && timeval_isValid(t2));
	auto result = t1;
	result.tv_sec += t2.tv_sec;
	result.tv_usec += t2.tv_usec;
	if(result.tv_usec >= USEC_PER_SEC)
	{
		result.tv_usec -= USEC_PER_SEC;
		result.tv_sec++;
	}
	return result;
}

static long int timeval_divUsecs(struct timeval t, long int usecs)
{
	return (((int64)t.tv_sec * USEC_PER_SEC) / usecs) + (t.tv_usec / usecs);
}

static long int timeval_divNsecs(struct timeval t, long int nsecs)
{
	return (((int64)t.tv_sec * NSEC_PER_SEC) / nsecs) + ((t.tv_usec * 1000) / nsecs);
}

static double timeval_toDouble(struct timeval t)
{
	return (double)t.tv_sec + (double)t.tv_usec/(double)1.0e6;
}

#undef NSEC_PER_SEC
#undef USEC_PER_SEC
