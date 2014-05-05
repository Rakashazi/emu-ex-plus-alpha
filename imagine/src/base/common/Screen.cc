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
#include "windowPrivate.hh"
#ifdef CONFIG_GFX
#include <imagine/gfx/Gfx.hh>
#endif

namespace Base
{

#ifdef CONFIG_BASE_MULTI_SCREEN
StaticArrayList<Screen*, 4> screen_;
#else
static Screen mainScreen_;
#endif

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

static void checkBufferSwapTime(Window &win)
{
	// check if buffer swap blocks even though triple-buffering is used
	auto beforeSwap = TimeSys::now();
	win.swapBuffers();
	auto afterSwap = TimeSys::now();
	long long diffSwap = (afterSwap - beforeSwap).toNs();
	if(diffSwap > 16000000)
	{
		logWarn("buffer swap took %lldns", diffSwap);
	}
}

bool Screen::frameUpdate(FrameTimeBase frameTime, bool forceDraw)
{
	assert(frameTime);
	assert(appIsRunning());
	framePosted = false;
	bool didDraw = false;
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
		if(forceDraw || w.needsDraw())
		{
			w.draw(frameTime);
			#if !defined NDEBUG && defined __ANDROID__
			checkBufferSwapTime(w);
			#else
			w.swapBuffers();
			#endif
			didDraw = true;
		}
	}
	if(didDraw)
	{
		swapsComplete();
	}
	inFrameHandler = false;
	//logMsg("%s", frameIsPosted() ? "drawing next frame" : "stopping at this frame");
	return didDraw;
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

}
