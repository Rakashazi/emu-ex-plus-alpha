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

void Screen::runOnFrameDelegates(FrameTimeBase timestamp)
{
	onFrameDelegate.runAll([&](OnFrameDelegate del){ return del({*this, timestamp}); });
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

void Screen::frameUpdate(FrameTimeBase timestamp)
{
	assert(timestamp);
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

uint32_t Screen::elapsedFrames(FrameTimeBase timestamp)
{
	if(!prevFrameTimestamp)
		return 1;
	assumeExpr(timestamp >= prevFrameTimestamp);
	if(unlikely(!timePerFrame))
	{
		timePerFrame = frameTimeBaseFromSecs(1./frameRate());
		assert(timePerFrame);
	}
	assumeExpr(timePerFrame > 0);
	FrameTimeBase diff = timestamp - prevFrameTimestamp;
	uint32_t elapsed = divRoundClosest(diff, timePerFrame);
	return std::max(elapsed, 1u);
}

void Screen::startDebugFrameStats(FrameTimeBase timestamp)
{
	#ifndef NDEBUG
	FrameTimeBase timeSinceCurrentFrame = frameTimeBaseFromNSecs(IG::Time::now().nSecs()) - timestamp;
	FrameTimeBase diffFromLastFrame = timestamp - prevFrameTimestamp;
	/*logMsg("frame at %f, %f since then, %f since last frame",
		frameTimeBaseToSDec(frameTime),
		frameTimeBaseToSDec(timeSinceCurrentFrame),
		frameTimeBaseToSDec(diffFromLastFrame));*/
	auto elapsed = elapsedFrames(timestamp);
	if(elapsed > 1)
	{
		if(logDroppedFrames)
			logDMsg("Lost %u frame(s) after %u continuous, at time %f (%f since last frame)",
				elapsed - 1, continuousFrames,
				frameTimeBaseToSecsDec(timestamp), frameTimeBaseToSecsDec(diffFromLastFrame));
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
