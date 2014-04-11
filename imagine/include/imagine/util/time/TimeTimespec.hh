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

#include <imagine/util/time/timespec.hh>
#include <imagine/util/operators.hh>

class TimeTimespec : Arithmetics<TimeTimespec>, Compares<TimeTimespec>
{
private:
	struct timespec t {0,0};
public:
	constexpr TimeTimespec() {}
	constexpr TimeTimespec(struct timespec t): t(t) {}

	static constexpr long MSEC_PER_SEC = 1000;
	static constexpr long USEC_PER_SEC = 1000000;
	static constexpr long NSEC_PER_USEC = 1000;
	static constexpr long NSEC_PER_MSEC = 1000000;
	static constexpr long NSEC_PER_SEC = 1000000000;

	static TimeTimespec makeWithNSecs(long int nsecs)
	{
		return {(struct timespec){nsecs / NSEC_PER_SEC, nsecs}};
	}

	static TimeTimespec makeWithUSecs(long int usecs)
	{
		return {(struct timespec){usecs / USEC_PER_SEC, (usecs % USEC_PER_SEC) * NSEC_PER_USEC}};
	}

	static TimeTimespec makeWithMSecs(long int msecs)
	{
		return {(struct timespec){msecs / MSEC_PER_SEC, (msecs % MSEC_PER_SEC) * NSEC_PER_MSEC}};
	}

	static TimeTimespec makeWithSecs(long int secs)
	{
		return {(struct timespec){secs, 0}};
	}

	static TimeTimespec now()
	{
		TimeTimespec time;
		clock_gettime(CLOCK_MONOTONIC, &time.t);
		return time;
	}

	long int toMs()
	{
		auto ms1 = t.tv_sec * MSEC_PER_SEC;
		auto ms2 = t.tv_nsec / 1000000;
		return ms1 + ms2;
	}

	int64 toNs()
	{
		int64 ns1 = (int64)t.tv_sec * NSEC_PER_SEC;
		return ns1 + (int64)t.tv_nsec;
	}

	void addUSec(long int us)
	{
		timespec add {us / USEC_PER_SEC, (us % USEC_PER_SEC) * NSEC_PER_USEC};
		t = timespec_add(t, add);
	}

	uint divByUSecs(long int usecs)
	{
		return timespec_divUsecs(t, usecs);
	}

	uint divByNSecs(long int nsecs)
	{
		return timespec_divNsecs(t, nsecs);
	}

	uint modByUSecs(long int usecs)
	{
		return (((int64)t.tv_sec * USEC_PER_SEC) + (t.tv_nsec / NSEC_PER_USEC)) % usecs;
	}

	TimeTimespec & operator -=(TimeTimespec const& diminuend)
	{
		t = timespec_subtract(t, diminuend.t);
		return *this;
	}

	TimeTimespec & operator +=(TimeTimespec const& x)
	{
		t = timespec_add(t, x.t);
		return *this;
	}

	operator float() const
	{
		return float(t.tv_sec) + float(t.tv_nsec)/1.0e9f;
	}

	operator double() const
	{
		return timespec_toDouble(t);
	}

	operator bool() const
	{
		return t.tv_sec != 0 || t.tv_nsec != 0;
	}

	bool operator <(TimeTimespec const& rhs) const
	{
		return timespec_compare(t, rhs.t) < 0;
	}

	bool operator >(TimeTimespec const& rhs) const
	{
		return timespec_compare(t, rhs.t) > 0;
	}

	bool operator ==(TimeTimespec const& rhs) const
	{
		return timespec_compare(t, rhs.t) == 0;
	}
};
