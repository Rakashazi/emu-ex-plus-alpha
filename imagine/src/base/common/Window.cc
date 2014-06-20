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
#include <imagine/gfx/Gfx.hh>
#ifdef CONFIG_INPUT
#include <imagine/input/Input.hh>
#endif

namespace Base
{

// whether to handle animations if orientations are handled in software
bool animateOrientationChange = !Config::envIsWebOS3;

#ifdef CONFIG_BASE_MULTI_WINDOW
StaticArrayList<Window*, 4> window_;
#else
Window *mainWin = nullptr;
#endif

void Window::setOnSurfaceChange(SurfaceChangeDelegate del)
{
	onSurfaceChange = del ? del : [](Window &, SurfaceChange){};
}

void Window::setOnDraw(DrawDelegate del)
{
	onDraw = del ? del : [](Window &, DrawParams){};
}

void Window::setOnFocusChange(FocusChangeDelegate del)
{
	onFocusChange = del ? del : [](Window &, bool){};
}

void Window::setOnDragDrop(DragDropDelegate del)
{
	onDragDrop = del ? del : [](Window &, const char *){};
}

void Window::setOnInputEvent(InputEventDelegate del)
{
	onInputEvent = del ? del : [](Window &, const Input::Event &){};
}

void Window::setOnDismissRequest(DismissRequestDelegate del)
{
	onDismissRequest = del ? del : [](Window &win){ Base::exit(); };
}

void Window::setOnDismiss(DismissDelegate del)
{
	onDismiss = del ? del : [](Window &win){};
}

void Window::initDelegates()
{
	setOnSurfaceChange({});
	setOnDraw({});
	setOnFocusChange({});
	setOnDragDrop({});
	setOnInputEvent({});
	setOnDismissRequest({});
	setOnDismiss({});
}

Window &mainWindow()
{
	assert(Window::windows());
	return *Window::window(0);
}

Screen &Window::screen()
{
	#ifdef CONFIG_BASE_MULTI_SCREEN
	return *screen_;
	#else
	return mainScreen();
	#endif
}

void Window::setNeedsDraw(bool needsDraw)
{
	if(needsDraw && hasSurface())
		drawPosted = true;
	else
		drawPosted = false;
}

bool Window::needsDraw()
{
	return drawPosted;
}

void Window::setNeedsCustomViewportResize(bool needsResize)
{
	if(needsResize)
	{
		surfaceChange.addCustomViewportResized();
		setNeedsDraw(true);
	}
	else
		surfaceChange.removeCustomViewportResized();

}

void Window::dispatchSurfaceChange()
{
	onSurfaceChange(*this, moveAndClear(surfaceChange));
}

void Window::dispatchOnDraw(FrameTimeBase frameTime)
{
	if(!needsDraw())
		return;
	setNeedsDraw(false);
	DrawParams params;
	params.frameTime_ = frameTime;
	if(unlikely(surfaceChange.flags))
	{
		dispatchSurfaceChange();
		params.wasResized_ = true;
	}
	onDraw(*this, params);
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
	surfaceChange.addSurfaceResized();
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
			logWarn("warning: valid orientation mask contains no valid values");
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
		postDraw();
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

void Window::postNeededScreens()
{
	iterateTimes(windows(), i)
	{
		auto &w = *window(i);
		if(w.needsDraw())
		{
			w.screen().postFrame();
		}
	}
}

uint Window::windows()
{
	#ifdef CONFIG_BASE_MULTI_WINDOW
	return window_.size();
	#else
	return mainWin ? 1 : 0;
	#endif
}

Window *Window::window(uint idx)
{
	#ifdef CONFIG_BASE_MULTI_WINDOW
	if(idx >= window_.size())
		return nullptr;
	return window_[idx];
	#else
	return mainWin;
	#endif
}

void Window::dismiss()
{
	onDismiss(*this);
	deinit();
	*this = {};
	#ifdef CONFIG_BASE_MULTI_WINDOW
	window_.remove(this);
	#else
	mainWin = nullptr;
	#endif
}

}
