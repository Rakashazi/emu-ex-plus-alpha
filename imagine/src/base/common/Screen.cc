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
#include <imagine/util/time/sys.hh>
#include "windowPrivate.hh"

namespace Base
{

#ifdef CONFIG_BASE_MULTI_SCREEN
StaticArrayList<Screen*, 4> screen_;
#else
static Screen mainScreen_;
#endif

Screen::ChangeDelegate Screen::onChange;

void Screen::setOnChange(ChangeDelegate del)
{
	onChange = del;
}

Screen &mainScreen()
{
	return *Screen::screen(0);
}

void Screen::addOnFrameDelegate(OnFrameDelegate del)
{
	assert(onFrameDelegate.freeSpace());
	onFrameDelegate.push_back(del);
}

bool Screen::removeOnFrameDelegate(OnFrameDelegate del)
{
	return onFrameDelegate.remove(del);
}

bool Screen::containsOnFrameDelegate(OnFrameDelegate del)
{
	return contains(onFrameDelegate, del);
}

void Screen::runOnFrameDelegates(FrameTimeBase frameTime)
{
	if(onFrameDelegate.empty())
		return;
	logMsg("running %d onFrame delegates", onFrameDelegate.size());
	auto thisFrameDelegate = onFrameDelegate;
	onFrameDelegate.clear();
	for(auto &delegate : thisFrameDelegate)
	{
		delegate(*this, frameTime);
	}
}

bool Screen::frameIsPosted()
{
	return framePosted;
}

void Screen::frameUpdate(FrameTimeBase frameTime)
{
	assert(frameTime);
	assert(appIsRunning());
	framePosted = false;
	inFrameHandler = true;
	runOnFrameDelegates(frameTime);
	iterateTimes(Window::windows(), i)
	{
		auto &w = *Window::window(i);
		#ifdef CONFIG_BASE_MULT_SCREEN
		if(w.screen() != *this)
		{
			continue;
		}
		#endif
		w.dispatchOnDraw(frameTime);
	}
	inFrameHandler = false;
	//logMsg("%s", frameIsPosted() ? "drawing next frame" : "stopping at this frame");
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

void Screen::unpostAll()
{
	iterateTimes(screens(), i)
	{
		screen(i)->unpostFrame();
	}
}

bool Screen::screensArePosted()
{
	iterateTimes(screens(), i)
	{
		if(screen(i)->frameIsPosted())
			return true;
	}
	return false;
}

uint Screen::elaspedFrames(FrameTimeBase frameTime)
{
	if(!prevFrameTime || frameTime < prevFrameTime)
		return 0;
	if(unlikely(!timePerFrame))
	{
		timePerFrame = frameTimeBaseFromS((double)1./(double)refreshRate());
		assert(timePerFrame);
	}
	FrameTimeBase diff = frameTime - prevFrameTime;
	uint elapsed = divRoundClosest(diff, timePerFrame);
	return elapsed;
}

void Screen::startDebugFrameStats(FrameTimeBase frameTime)
{
	#ifndef NDEBUG
	FrameTimeBase timeSinceCurrentFrame = frameTimeBaseFromNS(TimeSys::now().toNs()) - frameTime;
	FrameTimeBase diffFromLastFrame = frameTime - prevFrameTime;
	/*logMsg("frame at %f, %f since then, %f since last frame",
		frameTimeBaseToSDec(frameTime),
		frameTimeBaseToSDec(timeSinceCurrentFrame),
		frameTimeBaseToSDec(diffFromLastFrame));*/
	auto elapsedFrames = elaspedFrames(frameTime);
	if(elapsedFrames > 1)
	{
		logWarn("Lost %u frame(s) after %u continuous, at time %f (%f since last frame)",
			elapsedFrames - 1, continuousFrames,
			frameTimeBaseToSDec(frameTime), frameTimeBaseToSDec(diffFromLastFrame));
		continuousFrames = 0;
	}
	#endif
}

void Screen::endDebugFrameStats()
{
	#ifndef NDEBUG
	continuousFrames = frameIsPosted() ? continuousFrames + 1 : 0;
	#endif
}

}
