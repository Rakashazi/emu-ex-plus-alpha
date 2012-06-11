#pragma once

#include <util/timeval.h>
#include <util/operators.hh>

class TimeTimeval : Arithmetics< TimeTimeval >, Compares< TimeTimeval >
{
public:
	struct timeval t;

	constexpr TimeTimeval(): t() { }
	constexpr TimeTimeval(struct timeval t): t(t) { }

	void setUSecs(int usecs)
	{
		t = (struct timeval){ 0, usecs };
	}

	void setTimeNow()
	{
		gettimeofday(&t, 0);
	}

	uint toMs()
	{
		uint ms1 = t.tv_sec * mSecPerSec;
		uint ms2 = t.tv_usec / 1000;
		return ms1 + ms2;
	}

	void addUSec(int us)
	{
		int secsAdd = us / uSecPerSec;
		int usecsAdd = us % uSecPerSec;
		timeval add;
		add.tv_sec = secsAdd;
		add.tv_usec = usecsAdd;
		t = timeval_add(t, add);
	}

	uint divByUSecs(int usecs)
	{
		return timeval_divUsecs(t, usecs);
	}

	uint modByUSecs(int usecs)
	{
		return ((t.tv_sec * 1000000) + t.tv_usec) % usecs;
	}

	TimeTimeval & operator -=(TimeTimeval const& diminuend)
	{
		struct timeval diff = timeval_subtract(t, diminuend.t);
		t = diff;
		return *this;
	}

	TimeTimeval & operator +=(TimeTimeval const& x)
	{
		struct timeval sum = timeval_add(t, x.t);
		t = sum;
		return *this;
	}

	operator float() const
	{
		return float(t.tv_sec) + float(t.tv_usec)/1.0e6f;
	}

	operator double() const
	{
		return double(t.tv_sec) + double(t.tv_usec)/double(1.0e6);
	}

	bool operator <(TimeTimeval const& rhs) const
	{
		return timeval_compare(t, rhs.t) == -1;
	}

	bool operator >(TimeTimeval const& rhs) const
	{
		return timeval_compare(t, rhs.t) == 1;
	}

	bool operator ==(TimeTimeval const& rhs) const
	{
		return timeval_compare(t, rhs.t) == 0;
	}
};
