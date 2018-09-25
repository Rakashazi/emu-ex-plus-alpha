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

#include <imagine/base/EventLoop.hh>
#include <semaphore.h>

namespace Base
{

class FBDevFrameTimer
{
private:
	Base::FDEventSource fdSrc;
	int fd = -1;
	sem_t sem{};
	bool requested = false;
	bool cancelled = false;

public:
	constexpr FBDevFrameTimer() {}
	bool init(EventLoop loop);
	void deinit();
	void scheduleVSync();
	void cancel();

	explicit operator bool() const
	{
		return fd >= 0;
	}
};

}
