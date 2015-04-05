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
#include <imagine/time/Time.hh>
#include "windowPrivate.hh"

namespace Base
{

#ifdef CONFIG_BASE_MULTI_SCREEN
StaticArrayList<Screen*, 4> screen_;
#else
static Screen mainScreen_;
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

void Screen::addOnFrame(OnFrameDelegate del)
{
	assert(onFrameDelegate.freeSpace());
	onFrameDelegate.push_back(del);
	postFrame();
}

bool Screen::addOnFrameOnce(OnFrameDelegate del)
{
	if(!containsOnFrame(del))
	{
		addOnFrame(del);
		return true;
	}
	return false;
}

bool Screen::removeOnFrame(OnFrameDelegate del)
{
	bool removed = onFrameDelegate.remove(del);
	if(onFrameDelegate.empty())
	{
		unpostFrame();
	}
	return removed;
}

bool Screen::containsOnFrame(OnFrameDelegate del)
{
	return contains(onFrameDelegate, del);
}

void Screen::runOnFrameDelegates(FrameTimeBase frameTime)
{
	if(onFrameDelegate.empty())
		return;
	//logMsg("running %d onFrame delegates", onFrameDelegate.size());
	auto thisFrameDelegate = onFrameDelegate;
	onFrameDelegate.clear();
	for(auto &delegate : thisFrameDelegate)
	{
		delegate({*this, frameTime, delegate});
	}
}

uint Screen::onFrameDelegates()
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

void Screen::frameUpdate(FrameTimeBase frameTime)
{
	assert(frameTime);
	assert(isActive);
	framePosted = false;
	inFrameHandler = true;
	runOnFrameDelegates(frameTime);
	inFrameHandler = false;
	iterateTimes(Window::windows(), i)
	{
		auto &w = *Window::window(i);
		if(Config::BASE_MULTI_SCREEN && w.screen() != this)
		{
			continue;
		}
		w.dispatchOnDraw();
	}
	//logMsg("%s", isPosted() ? "drawing next frame" : "stopping at this frame");
}

void Screen::setActive(bool active)
{
	if(active && !isActive)
	{
		logMsg("screen:%p activated", this);
		isActive = true;
		if(!onFrameDelegate.empty())
			postFrame();
	}
	else if(!active && isActive)
	{
		logMsg("screen:%p deactivated", this);
		isActive = false;
		unpostFrame();
	}
}

uint Screen::screens()
{
	#ifdef CONFIG_BASE_MULTI_SCREEN
	return screen_.size();
	#else
	return 1;
	#endif
}

Screen *Screen::screen(uint idx)
{
	#ifdef CONFIG_BASE_MULTI_SCREEN
	if(idx >= screen_.size())
		return nullptr;
	return screen_[idx];
	#else
	return &mainScreen_;
	#endif
}

void Screen::addScreen(Screen *s)
{
	#ifdef CONFIG_BASE_MULTI_SCREEN
	assert(!screen_.isFull());
	screen_.push_back(s);
	#else
	assert(!mainScreen_);
	mainScreen_ = *s;
	#endif
}

void Screen::setActiveAll(bool active)
{
	iterateTimes(screens(), i)
	{
		screen(i)->setActive(active);
	}
}

bool Screen::screensArePosted()
{
	iterateTimes(screens(), i)
	{
		if(screen(i)->isPosted())
			return true;
	}
	return false;
}

uint Screen::elapsedFrames(FrameTimeBase frameTime)
{
	if(!prevFrameTime || frameTime < prevFrameTime)
		return 0;
	if(unlikely(!timePerFrame))
	{
		timePerFrame = frameTimeBaseFromSecs((double)1./(double)refreshRate());
		assert(timePerFrame);
	}
	FrameTimeBase diff = frameTime - prevFrameTime;
	uint elapsed = divRoundClosest(diff, timePerFrame);
	return elapsed;
}

void Screen::startDebugFrameStats(FrameTimeBase frameTime)
{
	#ifndef NDEBUG
	FrameTimeBase timeSinceCurrentFrame = frameTimeBaseFromNSecs(IG::Time::now().nSecs()) - frameTime;
	FrameTimeBase diffFromLastFrame = frameTime - prevFrameTime;
	/*logMsg("frame at %f, %f since then, %f since last frame",
		frameTimeBaseToSDec(frameTime),
		frameTimeBaseToSDec(timeSinceCurrentFrame),
		frameTimeBaseToSDec(diffFromLastFrame));*/
	auto elapsed = elapsedFrames(frameTime);
	if(elapsed > 1)
	{
		if(logDroppedFrames)
			logDMsg("Lost %u frame(s) after %u continuous, at time %f (%f since last frame)",
				elapsed - 1, continuousFrames,
				frameTimeBaseToSecsDec(frameTime), frameTimeBaseToSecsDec(diffFromLastFrame));
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
