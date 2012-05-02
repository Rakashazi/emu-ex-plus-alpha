#pragma once
#include <time.h>
#include <math.h>

struct timespec timespec_subtract(struct timespec x, struct timespec y)
{
	struct timespec result;
	result.tv_sec = x.tv_sec - y.tv_sec;
	result.tv_nsec = x.tv_nsec - y.tv_nsec;
	
	while(result.tv_nsec < 0)
	{
		result.tv_sec--;
		result.tv_nsec = (1000000000) + result.tv_nsec;
	}

	return result;
}

struct timespec timespec_add(struct timespec x, struct timespec y)
{
	struct timespec result;
	result.tv_sec = x.tv_sec + y.tv_sec;
	result.tv_nsec = x.tv_nsec + y.tv_nsec;
	
	while(result.tv_nsec < 0)
	{
		result.tv_sec--;
		result.tv_nsec = (1000000000) + result.tv_nsec;
	}
	
	while(result.tv_nsec >= 1000000000)
	{
		result.tv_sec++;
		result.tv_nsec -= (1000000000);
	}

	return result;
}

static int timespec_compare(struct timespec x, struct timespec y)
{
	if(x.tv_sec < y.tv_sec)
	{
		return -1;
	}
	else if(x.tv_sec > y.tv_sec)
	{
		return 1;
	}
	else
	{
		if(x.tv_nsec < y.tv_nsec)
		{
			return -1;
		}
		else if(x.tv_nsec > y.tv_nsec)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
}

static struct timespec toTimespec(double time)
{
	double intPart;
	double fracPart = modf(time, &intPart);

	struct timespec t;
	t.tv_sec = intPart;
	t.tv_nsec = fracPart*1.0e9;
	
	return t;
}

static double timespec_toDouble(struct timespec t)
{
	return (double)t.tv_sec + (double)t.tv_nsec/1.0e9;
}
