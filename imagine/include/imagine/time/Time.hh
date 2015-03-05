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

// TODO: look into std::chrono in C++14 to replace this class

#include <imagine/util/operators.hh>

#if defined __APPLE__
#include <imagine/time/TimeMach.hh>
#else
#include <imagine/time/Timespec.hh>
#endif

namespace IG
{

class Time : public TimeImpl, public Arithmetics<Time>, public Compares<Time>
{
public:
	constexpr Time() {}
	static Time makeWithNSecs(uint64_t nsecs);
	static Time makeWithUSecs(uint64_t usecs);
	static Time makeWithMSecs(uint64_t msecs);
	static Time makeWithSecs(uint64_t secs);
	static Time now();
	uint64_t nSecs() const;
	uint64_t uSecs() const;
	uint64_t mSecs() const;
	uint64_t secs() const;
	Time & operator -=(Time const& rhs);
	Time & operator +=(Time const& rhs);
	operator float() const;
	operator double() const;
	explicit operator bool() const;
	bool operator <(Time const& rhs) const;
	bool operator >(Time const& rhs) const;
	bool operator ==(Time const& rhs) const;
};

template <class FUNC>
static Time timeFunc(FUNC func)
{
	auto before = Time::now();
	func();
	auto after = Time::now();
	return after - before;
}

template <class FUNC>
static Time timeFuncDebug(FUNC func)
{
	#ifdef NDEBUG
	// execute directly without timing
	func();
	return {};
	#else
	return timeFunc(func);
	#endif
}

}
