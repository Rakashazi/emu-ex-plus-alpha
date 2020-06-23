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
#include <imagine/base/Base.hh>
#include <imagine/base/Screen.hh>
#include <imagine/logger/logger.h>
#include <imagine/time/Time.hh>
#include <imagine/util/utility.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/math/int.hh>
#include "windowPrivate.hh"
#include "basePrivate.hh"

namespace Base
{

#ifdef CONFIG_BASE_MULTI_SCREEN
std::vector<Screen*> screen_;
static auto &screenArr() { return screen_; }
#else
static Screen mainScreen_;
static auto screenArr() { return std::array<Screen*, 1>{&mainScreen_}; }
#endif

Screen::ChangeDelegate Screen::onChange;

[[gnu::weak]] bool logDroppedFrames = false;

void Screen::setOnChange(ChangeDelegate del)
{
	onChange = del;
}

Screen &mainScreen()
{
	return *Screen::screen(0);
}

bool Screen::addOnFrame(OnFrameDelegate del, int priority)
{
	postFrame();
	return onFrameDelegate.add(del, priority);
}

bool Screen::removeOnFrame(OnFrameDelegate del)
{
	if(unlikely(appIsExiting()))
	{
		// Screen destructor may have run already
		return false;
	}
	bool removed = onFrameDelegate.remove(del);
	if(!onFrameDelegate.size())
	{
		unpostFrame();
	}
	return removed;
}

bool Screen::containsOnFrame(OnFrameDelegate del)
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

uint32_t Screen::onFrameDelegates()
{
	return onFrameDelegate.size();
}

bool Screen::runningOnFrameDelegates()
{
	return inFrameHandler;
}

bool Screen::isPosted()
{
	return framePosted;
}

void Screen::frameUpdate(FrameTime timestamp)
{
	assert(timestamp.count());
	assert(isActive);
	framePosted = false;
	inFrameHandler = true;
	runOnFrameDelegates(timestamp);
	inFrameHandler = false;
	//logMsg("%s", isPosted() ? "drawing next frame" : "stopping at this frame");
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

uint32_t Screen::screens()
{
	return screenArr().size();
}

Screen *Screen::screen(uint32_t idx)
{
	if(idx >= screenArr().size())
		return nullptr;
	return screenArr()[idx];
}

void Screen::addScreen(Screen *s)
{
	#ifdef CONFIG_BASE_MULTI_SCREEN
	screen_.push_back(s);
	#else
	assert(!mainScreen_);
	mainScreen_ = *s;
	#endif
}

void Screen::setActiveAll(bool active)
{
	for(auto screen : screenArr())
	{
		screen->setActive(active);
	}
}

bool Screen::screensArePosted()
{
	for(auto screen : screenArr())
	{
		if(screen->isPosted())
			return true;
	}
	return false;
}

FrameParams Screen::makeFrameParams(FrameTime timestamp) const
{
	return {timestamp, lastFrameTimestamp(), frameTime()};
}

void Screen::startDebugFrameStats(FrameTime timestamp)
{
	#ifndef NDEBUG
	FrameTime timeSinceCurrentFrame = IG::steadyClockTimestamp() - timestamp;
	FrameTime diffFromLastFrame = timestamp - prevFrameTimestamp;
	/*logMsg("frame at %f, %f since then, %f since last frame",
		frameTimeBaseToSDec(frameTime),
		frameTimeBaseToSDec(timeSinceCurrentFrame),
		frameTimeBaseToSDec(diffFromLastFrame));*/
	auto elapsed = makeFrameParams(timestamp).elapsedFrames();
	if(elapsed > 1)
	{
		if(logDroppedFrames)
			logDMsg("Lost %u frame(s) after %u continuous, at time %f (%f since last frame)",
				elapsed - 1, continuousFrames,
				IG::FloatSeconds(timestamp).count(),
				IG::FloatSeconds(diffFromLastFrame).count());
		continuousFrames = 0;
	}
	#endif
}

void Screen::endDebugFrameStats()
{
	#ifndef NDEBUG
	continuousFrames = isPosted() ? continuousFrames + 1 : 0;
	#endif
}

}
