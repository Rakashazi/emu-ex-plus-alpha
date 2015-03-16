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
#include <assert.h>
#include <imagine/time/Time.hh>

namespace IG
{

static mach_timebase_info_data_t timebase{};

[[gnu::constructor]] static void initTimebase()
{
	mach_timebase_info(&timebase);
}

Time Time::makeWithNSecs(uint64_t nsecs)
{
	Time time;
	time.t = nsecs * timebase.denom / timebase.numer;
	return time;
}

Time Time::makeWithUSecs(uint64_t usecs)
{
	return makeWithNSecs(usecs * NSEC_PER_USEC);
}

Time Time::makeWithMSecs(uint64_t msecs)
{
	return makeWithNSecs(msecs * NSEC_PER_MSEC);
}

Time Time::makeWithSecs(uint64_t secs)
{
	return makeWithNSecs(secs * NSEC_PER_SEC);
}

Time Time::now()
{
	Time time;
	time.t = mach_absolute_time();
	return time;
}

uint64_t Time::nSecs() const
{
	return t * timebase.numer / timebase.denom;
}

uint64_t Time::uSecs() const
{
	return nSecs() / NSEC_PER_USEC;
}

uint64_t Time::mSecs() const
{
	return nSecs() / NSEC_PER_MSEC;
}

uint64_t Time::secs() const
{
	return nSecs() / NSEC_PER_SEC;
}

Time::operator float() const
{
	return (float)t * (float)timebase.numer / (float)timebase.denom / 1.0e9f;
}

Time::operator double() const
{
	return (double)t * (double)timebase.numer / (double)timebase.denom / 1.0e9f;
}

}
