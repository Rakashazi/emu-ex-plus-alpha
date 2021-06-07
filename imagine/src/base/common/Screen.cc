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

#define LOGTAG "Screen"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Screen.hh>
#include <imagine/logger/logger.h>
#include <imagine/time/Time.hh>
#include <imagine/util/utility.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/math/int.hh>

namespace Base
{

Screen::Screen(ApplicationContext ctx, InitParams params):
	ScreenImpl{ctx, params},
	ctx{ctx}
{}

bool Screen::addOnFrame(OnFrameDelegate del, int priority)
{
	postFrame();
	return onFrameDelegate.add(del, priority);
}

bool Screen::removeOnFrame(OnFrameDelegate del)
{
	bool removed = onFrameDelegate.remove(del);
	if(!onFrameDelegate.size())
	{
		unpostFrame();
	}
	return removed;
}

bool Screen::containsOnFrame(OnFrameDelegate del) const
{
	return onFrameDelegate.contains(del);
}

void Screen::runOnFrameDelegates(FrameTime timestamp)
{
	auto params = makeFrameParams(timestamp);
	onFrameDelegate.runAll([&](OnFrameDelegate del)
		{
			return del(params);
		});
	if(onFrameDelegate.size())
	{
		//logDMsg("posting next frame");
		postFrame();
	}
}

uint32_t Screen::onFrameDelegates() const
{
	return onFrameDelegate.size();
}

bool Screen::isPosted() const
{
	return framePosted;
}

bool Screen::frameUpdate(FrameTime timestamp)
{
	assert(timestamp.count());
	assert(isActive);
	framePosted = false;
	if(!onFrameDelegate.size())
		return false;
	runOnFrameDelegates(timestamp);
	//logMsg("%s", isPosted() ? "drawing next frame" : "stopping at this frame");
	return true;
}

void Screen::setActive(bool active)
{
	if(active && !isActive)
	{
		logMsg("screen:%p activated", this);
		isActive = true;
		if(onFrameDelegate.size())
			postFrame();
	}
	else if(!active && isActive)
	{
		logMsg("screen:%p deactivated", this);
		isActive = false;
		unpostFrame();
	}
}

FrameParams Screen::makeFrameParams(FrameTime timestamp) const
{
	return {timestamp, frameTime()};
}

void Screen::postFrame()
{
	if(!isActive) [[unlikely]]
	{
		logMsg("skipped posting inactive screen:%p", this);
		return;
	}
	if(framePosted)
		return;
	//logMsg("posting frame");
	framePosted = true;
	postFrameTimer();
}

void Screen::unpostFrame()
{
	if(!framePosted)
		return;
	framePosted = false;
	unpostFrameTimer();
}


}
