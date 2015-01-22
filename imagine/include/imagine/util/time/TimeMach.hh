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

#include <mach/mach_time.h>
#include <imagine/util/operators.hh>

class TimeMach : Arithmetics<TimeMach>, Compares<TimeMach>
{
private:
	uint64_t t = 0;
	static double timebaseNSec, timebaseUSec, timebaseMSec, timebaseSec;
public:
	constexpr TimeMach() {}
	constexpr TimeMach(uint64_t t): t(t) {}

	static void setTimebase()
	{
		mach_timebase_info_data_t info;
		mach_timebase_info(&info);
		timebaseNSec = (double)info.numer / (double)info.denom;
		timebaseUSec = 1e-3 * (double)info.numer / (double)info.denom;
		timebaseMSec = 1e-6 * (double)info.numer / (double)info.denom;
		timebaseSec = 1e-9 * (double)info.numer / (double)info.denom;
	}

	static TimeMach makeWithNSecs(long int nsecs)
	{
		return {(uint64_t)(nsecs / timebaseNSec)};
	}

	static TimeMach makeWithUSecs(long int usecs)
	{
		return {(uint64_t)(usecs / timebaseUSec)};
	}

	static TimeMach makeWithMSecs(long int msecs)
	{
		return {(uint64_t)(msecs / timebaseMSec)};
	}

	static TimeMach makeWithSecs(long int secs)
	{
		return {(uint64_t)(secs / timebaseSec)};
	}

	static TimeMach now()
	{
		return {mach_absolute_time()};
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

	explicit operator bool() const
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
