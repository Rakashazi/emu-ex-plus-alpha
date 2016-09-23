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

#include <imagine/base/EventLoopFileSource.hh>
#include <imagine/time/Time.hh>

namespace Base
{

class DRMFrameTimer
{
private:
	Base::EventLoopFileSource fdSrc;
	int fd = -1;
	bool requested = false;
	bool cancelled = false;
	IG::Time timestamp{};

public:
	constexpr DRMFrameTimer() {}
	bool init();
	void deinit();
	void scheduleVSync();
	void cancel();

	explicit operator bool() const
	{
		return fd >= 0;
	}
};

}
