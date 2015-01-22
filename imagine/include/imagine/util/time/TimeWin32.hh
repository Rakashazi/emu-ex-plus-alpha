#pragma once

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

#include <imagine/util/windows/windows.h>
#include <imagine/util/operators.hh>

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

	static TimeWin32 makeWithNSecs(long int nsecs)
	{
		return {nsecs / timebaseNSec};
	}

	static TimeWin32 makeWithUSecs(long int usecs)
	{
		return {usecs / timebaseUSec};
	}

	static TimeWin32 makeWithMSecs(long int msecs)
	{
		return {msecs / timebaseMSec};
	}

	static TimeWin32 makeWithSecs(long int secs)
	{
		return {secs / timebaseSec};
	}

	static TimeWin32 now()
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

	explicit operator bool() const
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
