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

#include <imagine/time/Time.hh>
#include <imagine/util/operators.hh>

namespace Input
{

class Time : public TimeImpl, public PrimitiveOperators<Time>
{
public:
	constexpr Time() {}
	static Time makeWithNSecs(uint64_t nsecs);
	static Time makeWithUSecs(uint64_t usecs);
	static Time makeWithMSecs(uint64_t msecs);
	static Time makeWithSecs(uint64_t secs);
	uint64_t nSecs() const;
	uint64_t uSecs() const;
	uint64_t mSecs() const;
	uint64_t secs() const;
	operator IG::Time() const;
};


}
