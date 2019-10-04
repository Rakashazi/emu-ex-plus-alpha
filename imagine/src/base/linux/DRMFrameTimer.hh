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

#include "../common/SimpleFrameTimer.hh"
#include <imagine/time/Time.hh>

namespace Base
{

class DRMFrameTimer : public FrameTimer
{
public:
	DRMFrameTimer(EventLoop loop);
	~DRMFrameTimer() final;
	void scheduleVSync() final;
	void cancel() final;

	explicit operator bool() const
	{
		return fd >= 0;
	}

protected:
	Base::FDEventSource fdSrc;
	IG::Time timestamp{};
	int fd = -1;
	bool requested = false;
	bool cancelled = false;
};

}
