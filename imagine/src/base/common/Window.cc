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
#include <imagine/input/Input.hh>

namespace Base
{

OnGLDrawableChangedDelegate onGLDrawableChanged;

#ifdef CONFIG_BASE_MULTI_WINDOW
StaticArrayList<Window*, 4> window_;
#else
Window *mainWin = nullptr;
#endif

void BaseWindow::setOnSurfaceChange(SurfaceChangeDelegate del)
{
	onSurfaceChange = del ? del : [](Window &, SurfaceChange){};
}

void BaseWindow::setOnDraw(DrawDelegate del)
{
	onDraw = del ? del : [](Window &, DrawParams){};
}

void BaseWindow::setOnFocusChange(FocusChangeDelegate del)
{
	onFocusChange = del ? del : [](Window &, bool){};
}

void BaseWindow::setOnDragDrop(DragDropDelegate del)
{
	onDragDrop = del ? del : [](Window &, const char *){};
}

void BaseWindow::setOnInputEvent(InputEventDelegate del)
{
	onInputEvent = del ? del : [](Window &, const Input::Event &){};
}

void BaseWindow::setOnDismissRequest(DismissRequestDelegate del)
{
	onDismissRequest = del ? del : [](Window &win){ Base::exit(); };
}

void BaseWindow::setOnDismiss(DismissDelegate del)
{
	onDismiss = del ? del : [](Window &win){};
}

void BaseWindow::initDelegates(const WindowConfig &config)
{
	setOnSurfaceChange(config.onSurfaceChange());
	setOnDraw(config.onDraw());
	setOnFocusChange(config.onFocusChange());
	setOnDragDrop(config.onDragDrop());
	setOnInputEvent(config.onInputEvent());
	setOnDismissRequest(config.onDismissRequest());
	setOnDismiss(config.onDismiss());
}

void BaseWindow::initDefaultValidSoftOrientations()
{
	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	validSoftOrientations_ = defaultSystemOrientations();
	#endif
}

void BaseWindow::init(const WindowConfig &config)
{
	initDelegates(config);
	initDefaultValidSoftOrientations();
}

void Window::setOnSurfaceChange(SurfaceChangeDelegate del)
{
	BaseWindow::setOnSurfaceChange(del);
}

void Window::setOnDraw(DrawDelegate del)
{
	BaseWindow::setOnDraw(del);
}

void Window::setOnFocusChange(FocusChangeDelegate del)
{
	BaseWindow::setOnFocusChange(del);
}

void Window::setOnDragDrop(DragDropDelegate del)
{
	BaseWindow::setOnDragDrop(del);
}

void Window::setOnInputEvent(InputEventDelegate del)
{
	BaseWindow::setOnInputEvent(del);
}

void Window::setOnDismissRequest(DismissRequestDelegate del)
{
	BaseWindow::setOnDismissRequest(del);
}

void Window::setOnDismiss(DismissDelegate del)
{
	BaseWindow::setOnDismiss(del);
}

Window &mainWindow()
{
	assert(Window::windows());
	return *Window::window(0);
}

Screen *Window::screen()
{
	#ifdef CONFIG_BASE_MULTI_SCREEN
	return screen_;
	#else
	return &mainScreen();
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

void Window::dispatchInputEvent(const Input::Event &event)
{
	onInputEvent.callCopy(*this, event);
}

void Window::dispatchFocusChange(bool in)
{
	onFocusChange.callCopy(*this, in);
}

void Window::dispatchDragDrop(const char *filename)
{
	onDragDrop.callCopy(*this, filename);
}

void Window::dispatchDismissRequest()
{
	onDismissRequest.callCopy(*this);
}

void Window::dispatchSurfaceChange()
{
	onSurfaceChange.callCopy(*this, moveAndClear(surfaceChange));
}

void Window::dispatchOnDraw()
{
	if(!needsDraw())
		return;
	setNeedsDraw(false);
	DrawParams params;
	if(unlikely(surfaceChange.flags))
	{
		dispatchSurfaceChange();
		params.wasResized_ = true;
	}
	onDraw.callCopy(*this, params);
}

bool Window::updateSize(IG::Point2D<int> surfaceSize)
{
	auto oldW = w, oldH = h;
	w = surfaceSize.x;
	h = surfaceSize.y;
	if(orientationIsSideways(softOrientation_))
		std::swap(w, h);
	if(oldW == w && oldH == h) // is the new size the same as the old?
	{
		logMsg("same window size %d,%d", realWidth(), realHeight());
		return false;
	}
	if(softOrientation_ == VIEW_ROTATE_0)
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
	if(orientationIsSideways(softOrientation_))
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
	if(orientationIsSideways(softOrientation_))
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
bool Window::setValidOrientations(uint oMask)
{
	oMask = validateOrientationMask(oMask);
	validSoftOrientations_ = oMask;
	if(validSoftOrientations_ & setSoftOrientation)
		return requestOrientationChange(setSoftOrientation);
	if(!(validSoftOrientations_ & softOrientation_))
	{
		if(validSoftOrientations_ & VIEW_ROTATE_0)
			return requestOrientationChange(VIEW_ROTATE_0);
		else if(validSoftOrientations_ & VIEW_ROTATE_90)
			return requestOrientationChange(VIEW_ROTATE_90);
		else if(validSoftOrientations_ & VIEW_ROTATE_180)
			return requestOrientationChange(VIEW_ROTATE_180);
		else if(validSoftOrientations_ & VIEW_ROTATE_270)
			return requestOrientationChange(VIEW_ROTATE_270);
		else
		{
			bug_exit("bad orientation mask: 0x%X", oMask);
		}
	}
	return false;
}

bool Window::requestOrientationChange(uint o)
{
	assert(o == VIEW_ROTATE_0 || o == VIEW_ROTATE_90 || o == VIEW_ROTATE_180 || o == VIEW_ROTATE_270);
	setSoftOrientation = o;
	if((validSoftOrientations_ & o) && softOrientation_ != o)
	{
		logMsg("setting orientation %s", orientationToStr(o));
		int savedRealWidth = realWidth();
		int savedRealHeight = realHeight();
		softOrientation_ = o;
		updateSize({savedRealWidth, savedRealHeight});
		postDraw();
		if(*this == mainWindow())
			setSystemOrientation(o);
		Input::configureInputForOrientation(*this);
		return true;
	}
	return false;
}
#endif

uint Window::softOrientation() const
{
	return softOrientation_;
}

uint Window::validSoftOrientations() const
{
	return validSoftOrientations_;
}

void Window::postNeededScreens()
{
	iterateTimes(windows(), i)
	{
		auto &w = *window(i);
		if(w.needsDraw())
		{
			w.screen()->postFrame();
		}
	}
	iterateTimes(Screen::screens(), i)
	{
		if(Screen::screen(i)->onFrameDelegates())
		{
			Screen::screen(i)->postFrame();
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

void setOnGLDrawableChanged(OnGLDrawableChangedDelegate del)
{
	onGLDrawableChanged = del;
}

}
