#pragma once

#include <sys/sys_time.h>
#include <util/operators.hh>

class TimePs3 : Arithmetics< TimePs3 >
{
public:
	system_time_t t;

	TimePs3() { }
	TimePs3(system_time_t num) { t = num; }

	void setUSecs(int usecs)
	{
		t = usecs;
	}

	void setTimeNow()
	{
		t = sys_time_get_system_time();
	}

	uint divByUSecs(int usecs)
	{
		return t / usecs;
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
