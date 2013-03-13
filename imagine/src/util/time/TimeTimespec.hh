#pragma once

#include <util/time/timespec.hh>
#include <util/operators.hh>

class TimeTimespec : Arithmetics<TimeTimespec>, Compares<TimeTimespec>
{
private:
	struct timespec t {0,0};
public:
	constexpr TimeTimespec() { }
	constexpr TimeTimespec(struct timespec t): t(t) { }

	static constexpr long MSEC_PER_SEC = 1000;
	static constexpr long USEC_PER_SEC = 1000000;
	static constexpr long NSEC_PER_USEC = 1000;
	static constexpr long NSEC_PER_MSEC = 1000000;
	static constexpr long NSEC_PER_SEC = 1000000000;

	void setUSecs(long int usecs)
	{
		t = (struct timespec){ 0, (typeof(t.tv_nsec))(usecs*NSEC_PER_USEC) };
	}

	void setTimeNow()
	{
		*this = timeNow();
	}

	static TimeTimespec timeNow()
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
