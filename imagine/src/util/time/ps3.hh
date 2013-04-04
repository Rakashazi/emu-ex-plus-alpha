#pragma once

#include <sys/sys_time.h>
#include <util/operators.hh>

class TimePs3 : Arithmetics< TimePs3 >
{
public:
	system_time_t t;

	constexpr TimePs3(): t() { }
	constexpr TimePs3(system_time_t t): t(t) { }

	void setUSecs(int usecs)
	{
		t = usecs;
	}

	void setTimeNow()
	{
		*this = timeNow();
	}

	static TimePs3 timeNow()
	{
		TimePs3 time;
		time.t = sys_time_get_system_time();
		return time;
	}

	uint divByUSecs(int usecs)
	{
		return t / usecs;
	}

	uint divByNSecs(long int nsecs)
	{
		return t / (uint64_t)(nsecs / 1000);
	}

	TimePs3 & operator -=(TimePs3 const& diminuend)
	{
		t -= diminuend.t;
		return *this;
	}

	operator float() const
	{
		return float(t)/1.0e6f;
	}

	bool operator <(TimePs3 const& rhs) const
	{
		return t < rhs.t;
	}

	bool operator >(TimePs3 const& rhs) const
	{
		return t > rhs.t;
	}

	bool operator ==(TimePs3 const& rhs) const
	{
		return t == rhs.t;
	}
};
