#pragma once

#include <util/windows/windows.h>
#include <util/operators.hh>

class TimeWin32 : Arithmetics<TimeWin32>, Compares<TimeWin32>
{
private:
	LONGLONG t = 0;
public:
	static double timebaseNSec, timebaseUSec, timebaseMSec, timebaseSec;
	constexpr TimeWin32() {}
	constexpr TimeWin32(uint64_t t): t(t) {}

	static void setTimebase()
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&timebaseSec);
		timebaseNSec = timebaseSec / 1000000000.;
		timebaseUSec = timebaseSec / 1000000.;
		timebaseMSec = timebaseSec / 1000.;
	}

	void setUSecs(long int usecs)
	{
		t = usecs / timebaseUSec;
	}

	void setTimeNow()
	{
		*this = timeNow();
	}

	static TimeWin32 timeNow()
	{
		TimeWin32 time;
		QueryPerformanceCounter((LARGE_INTEGER*)&time.t);
		return time;
	}

	long int toMs()
	{
		return t * timebaseMSec;
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

	TimeWin32 & operator -=(TimeWin32 const& rhs)
	{
		t -= rhs.t;
		return *this;
	}

	TimeWin32 & operator +=(TimeWin32 const& rhs)
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

	bool operator <(TimeWin32 const& rhs) const
	{
		return t < rhs.t;
	}

	bool operator >(TimeWin32 const& rhs) const
	{
		return t > rhs.t;
	}

	bool operator ==(TimeWin32 const& rhs) const
	{
		return t == rhs.t;
	}
};
