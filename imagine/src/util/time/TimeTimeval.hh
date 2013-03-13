#pragma once

#include <util/time/timeval.hh>
#include <util/operators.hh>

class TimeTimeval : Arithmetics< TimeTimeval >, Compares< TimeTimeval >
{
private:
	struct timeval t {0,0};
public:
	constexpr TimeTimeval() { }
	constexpr TimeTimeval(struct timeval t): t(t) { }

	static constexpr long MSEC_PER_SEC = 1000;
	static constexpr long USEC_PER_SEC = 1000000;

	void setUSecs(long int usecs)
	{
		t = (struct timeval){ 0, (typeof(t.tv_usec))usecs };
	}

	void setTimeNow()
	{
		*this = timeNow();
	}

	static TimeTimeval timeNow()
	{
		TimeTimeval time;
		gettimeofday(&time.t, 0);
		return time;
	}

	long int toMs()
	{
		auto ms1 = t.tv_sec * MSEC_PER_SEC;
		auto ms2 = t.tv_usec / 1000;
		return ms1 + ms2;
	}

	void addUSec(long int us)
	{
		timeval add {us / USEC_PER_SEC, (typeof(t.tv_usec))(us % USEC_PER_SEC)};
		t = timeval_add(t, add);
	}

	uint divByUSecs(long int usecs)
	{
		return timeval_divUsecs(t, usecs);
	}

	uint divByNSecs(long int nsecs)
	{
		return timeval_divNsecs(t, nsecs);
	}

	uint modByUSecs(long int usecs)
	{
		return (((int64)t.tv_sec * 1000000) + t.tv_usec) % usecs;
	}

	TimeTimeval & operator -=(TimeTimeval const& diminuend)
	{
		t = timeval_subtract(t, diminuend.t);
		return *this;
	}

	TimeTimeval & operator +=(TimeTimeval const& x)
	{
		t = timeval_add(t, x.t);
		return *this;
	}

	operator float() const
	{
		return float(t.tv_sec) + float(t.tv_usec)/1.0e6f;
	}

	operator double() const
	{
		return timeval_toDouble(t);
	}

	operator bool() const
	{
		return t.tv_sec != 0 || t.tv_usec != 0;
	}

	bool operator <(TimeTimeval const& rhs) const
	{
		return timeval_compare(t, rhs.t) < 0;
	}

	bool operator >(TimeTimeval const& rhs) const
	{
		return timeval_compare(t, rhs.t) > 0;
	}

	bool operator ==(TimeTimeval const& rhs) const
	{
		return timeval_compare(t, rhs.t) == 0;
	}
};
