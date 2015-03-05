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

#include <time.h>
#include <assert.h>
#include <imagine/time/Time.hh>

namespace IG
{

static constexpr uint64_t NSEC_PER_USEC = 1000;
static constexpr uint64_t NSEC_PER_MSEC = 1000000;
static constexpr uint64_t NSEC_PER_SEC = 1000000000;

Time Time::makeWithNSecs(uint64_t nsecs)
{
	Time time;
	time.t = nsecs;
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
	struct timespec tspec;
	clock_gettime(CLOCK_MONOTONIC, &tspec);
	Time time;
	time.t = ((uint64_t)tspec.tv_sec * NSEC_PER_SEC) + (uint64_t)tspec.tv_nsec;
	return time;
}

uint64_t Time::nSecs() const
{
	return t;
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

Time & Time::operator -=(Time const& rhs)
{
	assert(t >= rhs.t);
	t = t - rhs.t;
	return *this;
}

Time & Time::operator +=(Time const& rhs)
{
	t = t + rhs.t;
	return *this;
}

Time::operator float() const
{
	return float(t)/1.0e9f;
}

Time::operator double() const
{
	return double(t)/double(1.0e9);
}

Time::operator bool() const
{
	return t;
}

bool Time::operator <(Time const& rhs) const
{
	return t < rhs.t;
}

bool Time::operator >(Time const& rhs) const
{
	return t > rhs.t;
}

bool Time::operator ==(Time const& rhs) const
{
	return t == rhs.t;
}

}
