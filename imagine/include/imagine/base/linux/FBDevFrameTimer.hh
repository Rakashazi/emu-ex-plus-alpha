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

#include <imagine/base/FrameTimer.hh>
#include <imagine/base/EventLoop.hh>
#include <semaphore.h>

namespace Base
{

class Screen;

class FBDevFrameTimer : public FrameTimerI
{
public:
	constexpr FBDevFrameTimer() {}
	FBDevFrameTimer(Screen &screen, EventLoop loop = {});
	~FBDevFrameTimer() final;
	void scheduleVSync() final;
	void cancel() final;
	static bool testSupport();

	explicit operator bool() const
	{
		return fdSrc.fd() >= 0;
	}

private:
	Base::FDEventSource fdSrc{};
	sem_t sem{};
	bool requested{};
	bool cancelled{};
	bool quiting{};
};

}
