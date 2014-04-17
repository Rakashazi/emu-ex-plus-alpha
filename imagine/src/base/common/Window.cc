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

#define LOGTAG "Window"
#include <imagine/base/Base.hh>
#include "windowPrivate.hh"
#ifdef CONFIG_GFX
#include <imagine/gfx/Gfx.hh>
#endif
#ifdef CONFIG_INPUT
#include <imagine/input/Input.hh>
#endif

namespace Base
{

// whether to handle animations if orientations are handled in software
bool animateOrientationChange = !Config::envIsWebOS3;

StaticArrayList<Window*, 4> window;
static Screen mainScreen_;

Screen &mainScreen()
{
	return mainScreen_;
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

void Screen::clearOnFrameDelegates()
{
	onFrameDelegate.clear();
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

#ifdef CONFIG_BASE_MULTI_WINDOW

Window &mainWindow()
{
	assert(!window.empty());
	return *window[0];
}

bool frameUpdate(FrameTimeBase frameTime, bool forceDraw)
{
	mainScreen().framePosted = false;
	bool didDraw = false;
	if(unlikely(!appIsRunning()))
	{
		// unpost all windows if inactive
		for(auto w : window)
		{
			w->setNeedsDraw(false);
		}
		return false;
	}
	mainScreen().inFrameHandler = true;
	mainScreen().runOnFrameDelegates(frameTime);
	for(auto w : window)
	{
		if(forceDraw || w->needsDraw())
		{
			w->setAsDrawTarget();
			Gfx::setClipRect(false);
			Gfx::clear();
			w->draw(frameTime);
			w->needsSwap = true;
			didDraw = true;
		}
	}
	for(auto w : window)
	{
		if(w->needsSwap)
		{
			w->swapBuffers();
			w->needsSwap = false;
		}
	}
	mainScreen().inFrameHandler = false;
	//logMsg("%s", mainScreen().frameIsPosted() ? "drawing next frame" : "stopping at this frame");
	return didDraw;
}

#else

Window *mainWin = nullptr;

Window &mainWindow()
{
	assert(mainWin);
	return *mainWin;
}

static void checkTripleBufferSwap()
{
	// check if buffer swap blocks even though triple-buffering is used
	auto beforeSwap = TimeSys::now();
	mainWin->swapBuffers();
	auto afterSwap = TimeSys::now();
	long long diffSwap = (afterSwap - beforeSwap).toNs();
	if(diffSwap > 16000000)
	{
		logWarn("buffer swap took %lldns", diffSwap);
	}
}

bool frameUpdate(FrameTimeBase frameTime, bool forceDraw)
{
	mainScreen().framePosted = false;
	if(unlikely(!appIsRunning()))
	{
		mainWin->setNeedsDraw(false);
		return false;
	}
	mainScreen().inFrameHandler = true;
	bool didDraw = false;
	mainScreen().runOnFrameDelegates(frameTime);
	if(forceDraw || mainWin->needsDraw())
	{
		mainWin->draw(frameTime);
		#if !defined NDEBUG && defined __ANDROID__
		checkTripleBufferSwap();
		#else
		mainWin->swapBuffers();
		#endif
		Gfx::setClipRect(false);
		Gfx::clear();
		didDraw =  true;
	}
	else
	{
		didDraw = false;
	}
	mainScreen().inFrameHandler = false;
	//logMsg("%s", mainScreen().frameIsPosted() ? "drawing next frame" : "stopping at this frame");
	return didDraw;
}

#endif

void Window::setNeedsDraw(bool needsDraw)
{
	drawPosted = needsDraw;
}

bool Window::needsDraw()
{
	return drawPosted;
}

void Window::postResize(bool redraw)
{
	logMsg("posted resize");
	resizePosted = true;
	if(redraw)
		postDraw();
}

void Window::dispatchResize()
{
	resizePosted = false;
	Base::onViewChange(*this);
}

void Window::draw(FrameTimeBase frameTime)
{
	setNeedsDraw(false);
	if(unlikely(resizePosted))
	{
		dispatchResize();
	}
	#ifdef CONFIG_GFX
	Gfx::renderFrame(*this, frameTime);
	#endif
}

bool Window::updateSize(IG::Point2D<int> surfaceSize)
{
	auto oldW = w, oldH = h;
	w = surfaceSize.x;
	h = surfaceSize.y;
	if(orientationIsSideways(rotateView))
		std::swap(w, h);
	if(oldW == w && oldH == h) // is the new size the same as the old?
	{
		logMsg("same window size %d,%d", realWidth(), realHeight());
		return false;
	}
	if(rotateView == VIEW_ROTATE_0)
		logMsg("updated window size %d,%d", w, h);
	else
		logMsg("updated window size %d,%d with rotation, real size %d,%d", w, h, realWidth(), realHeight());
	#ifdef __ANDROID__
	updatePhysicalSize(pixelSizeAsMM({realWidth(), realHeight()}), pixelSizeAsSMM({realWidth(), realHeight()}));
	#else
	updatePhysicalSize(pixelSizeAsMM({realWidth(), realHeight()}));
	#endif
	return true;
}

bool Window::updatePhysicalSize(IG::Point2D<float> surfaceSizeMM, IG::Point2D<float> surfaceSizeSMM)
{
	bool changed = false;
	auto oldW = wMM, oldH = hMM;
	wMM = surfaceSizeMM.x;
	hMM = surfaceSizeMM.y;
	if(orientationIsSideways(rotateView))
		std::swap(wMM, hMM);
	mmToPixelXScaler = w / wMM;
	mmToPixelYScaler = h / hMM;
	if(oldW != wMM || oldH != hMM)
		changed = true;
	#ifdef __ANDROID__
	assert(surfaceSizeSMM.x && surfaceSizeSMM.y);
	auto oldSW = wSMM, oldSH = hSMM;
	wSMM = surfaceSizeSMM.x;
	hSMM = surfaceSizeSMM.y;
	if(orientationIsSideways(rotateView))
		std::swap(wSMM, hSMM);
	smmToPixelXScaler = w / wSMM;
	smmToPixelYScaler = h / hSMM;
	if(oldSW != wSMM || oldSH != hSMM)
		changed = true;
	logMsg("size in MM %fx%f, scaled %fx%f", (double)wMM, (double)hMM, (double)wSMM, (double)hSMM);
	#else
	logMsg("size in MM %fx%f", (double)wMM, (double)hMM);
	#endif
	return changed;
}

bool Window::updatePhysicalSize(IG::Point2D<float> surfaceSizeMM)
{
	return updatePhysicalSize(surfaceSizeMM, {0., 0.});
}

bool Window::updatePhysicalSizeWithCurrentSize()
{
	#ifdef __ANDROID__
	return updatePhysicalSize(pixelSizeAsMM({realWidth(), realHeight()}), pixelSizeAsSMM({realWidth(), realHeight()}));
	#else
	return updatePhysicalSize(pixelSizeAsMM({realWidth(), realHeight()}));
	#endif
}

#ifdef CONFIG_GFX_SOFT_ORIENTATION
uint Window::setValidOrientations(uint oMask, bool preferAnimated)
{
	if(oMask == VIEW_ROTATE_AUTO)
	{
		oMask = VIEW_ROTATE_0 | VIEW_ROTATE_90 | VIEW_ROTATE_180 | VIEW_ROTATE_270;
	}
	else
	{
		assert(oMask >= bit(0) && oMask < bit(4));
	}

	if(bit_numSet(oMask) > 1)
		setAutoOrientation(true);
	else
		setAutoOrientation(false);

	validOrientations = oMask;
	if(validOrientations & preferedOrientation)
		return setOrientation(preferedOrientation, preferAnimated);
	if(!(validOrientations & rotateView))
	{
		if(validOrientations & VIEW_ROTATE_0)
			return setOrientation(VIEW_ROTATE_0, preferAnimated);
		else if(validOrientations & VIEW_ROTATE_90)
			return setOrientation(VIEW_ROTATE_90, preferAnimated);
		else if(validOrientations & VIEW_ROTATE_180)
			return setOrientation(VIEW_ROTATE_180, preferAnimated);
		else if(validOrientations & VIEW_ROTATE_270)
			return setOrientation(VIEW_ROTATE_270, preferAnimated);
		else
		{
			logWarn("warning: valid orientation mask contain no valid values");
			return 0;
		}
	}
	return 0;
}

uint Window::setOrientation(uint o, bool preferAnimated)
{
	assert(o == VIEW_ROTATE_0 || o == VIEW_ROTATE_90 || o == VIEW_ROTATE_180 || o == VIEW_ROTATE_270);

	if((validOrientations & o) && rotateView != o)
	{
		logMsg("setting orientation %d", o);
		int savedRealWidth = realWidth();
		int savedRealHeight = realHeight();
		auto oldRotateView = rotateView;
		rotateView = o;
		if(animateOrientationChange)
		{
			if(preferAnimated)
			{
				Gfx::animateProjectionMatrixRotation(orientationToGC(oldRotateView), orientationToGC(rotateView));
			}
			else
			{
				Gfx::setProjectionMatrixRotation(orientationToGC(rotateView));
			}
		}
		updateSize({savedRealWidth, savedRealHeight});
		postResize();
		setSystemOrientation(o);
		#ifdef CONFIG_INPUT
		Input::configureInputForOrientation(*this);
		#endif
		return 1;
	}
	else
		return 0;
}
#endif

}
