#pragma once

#include <mach/mach_time.h>
#include <util/operators.hh>

class TimeMach : Arithmetics<TimeMach>, Compares<TimeMach>
{
private:
	uint64_t t = 0;
	static double timebaseNSec, timebaseUSec, timebaseMSec, timebaseSec;
public:
	constexpr TimeMach() { }
	constexpr TimeMach(uint64_t t): t(t) { }

	static void setTimebase()
	{
		mach_timebase_info_data_t info;
		mach_timebase_info(&info);
		timebaseNSec = (double)info.numer / (double)info.denom;
		timebaseUSec = 1e-3 * (double)info.numer / (double)info.denom;
		timebaseMSec = 1e-6 * (double)info.numer / (double)info.denom;
		timebaseSec = 1e-9 * (double)info.numer / (double)info.denom;
	}

	void setUSecs(long int usecs)
	{
		t = usecs / timebaseUSec;
	}

	void setTimeNow()
	{
		*this = timeNow();
	}

	static TimeMach timeNow()
	{
		TimeMach time;
		time.t = mach_absolute_time();
		return time;
	}

	long int toMs()
	{
		return t * timebaseMSec;
	}

	int64 toNs()
	{
		return t * timebaseNSec;
	}

	void addUSec(long int us)
	{
		t += us / timebaseUSec;
	}

	uint divByUSecs(long int usecs)
	{
		return t / (uint64_t)(usecs / timebaseUSec);
	}

	uint divByNSecs(long int nsecs)
	{
		return t / (uint64_t)(nsecs / timebaseNSec);
	}

	uint modByUSecs(long int usecs)
	{
		return t % (uint64_t)(usecs / timebaseUSec);
	}

	TimeMach & operator -=(TimeMach const& rhs)
	{
		t -= rhs.t;
		return *this;
	}

	TimeMach & operator +=(TimeMach const& rhs)
	{
		t += rhs.t;
		return *this;
	}

	operator float() const
	{
		return t * timebaseSec;
	}

	operator double() const
	{
		return t * timebaseSec;
	}

	operator bool() const
	{
		return t;
	}

	bool operator <(TimeMach const& rhs) const
	{
		return t < rhs.t;
	}

	bool operator >(TimeMach const& rhs) const
	{
		return t > rhs.t;
	}

	bool operator ==(TimeMach const& rhs) const
	{
		return t == rhs.t;
	}
};
